/*
author: Katelyn Mak
date created: 7/4/2024

This module writes and reads to the AXI MCDMA IP module a random payload.
The AXI MCDMA module should have the master mm2s axi stream connected to the slave mm2s axi stream in a loop.

*/
#include "misc.h"
#include "axi_mcdma.h"
#include "axi_mcdma_fifo_test.h"

#include <stdio.h> // todo: remove this once debugging is finished
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

int mcdma_test(int payload_length);
int config_mcdma(mcdma_device_t *device, int payload_length);
int config_mcdma_bd_chain(mcdma_device_t *device, int payload_length);
int config_mcdma_mm2s(mcdma_device_t *device);
int config_mcdma_s2mm(mcdma_device_t *device);
int start_mcdma(mcdma_device_t *device);
int verify_payload(mcdma_device_t *device, int payload_length);
void free_device(mcdma_device_t *device);

int mcdma_test (int payload_length) {
	fprintf(stderr, "============================= MCDMA FIFO TEST =============================\n");

	// Create device struct
	mcdma_device_t *device = malloc(sizeof( mcdma_device_t ));
	MALLOC_CHK(device);

	int res;
	res = config_mcdma(device, payload_length);
	if (res) {
		ERROR("%s", "Could not config MCDMA.");
		return -1;
	}

	// res = start_mcdma(&device);
	// if (res) {
	// 	ERROR("%s", "Could not start MCDMA.\n");
	// 	return -1;
	// }

	// res = verify_payload(&device, payload_length);
	// if (res) {
	// 	ERROR("%s", "Could not verify payload.\n");
	// 	return -1;
	// }

	free_device(device);

	return 0;
}

int config_mcdma(mcdma_device_t * device, int payload_length) {
	INFO("%s", "Creating random payload:");
	// Create random payload
	uint32_t payload[payload_length];
	for (int i = 0; i < payload_length; i++) {
		payload[i] = (uint32_t) rand();
		INFO("%d: 0x%08x", i, payload[i]);
	}

	/*** Memory map address space ***/
	// Open /dev/mem for memory mapping
	int32_t dev_fd = open("/dev/mem", O_RDWR | O_SYNC);
	if (dev_fd < 0) {
		ERROR("%s", "Failed to open /dev/mem.");
		return -1;
	}

	// initialise the axi dma control space
	device->p_baseaddr = AXI_MCDMA_BUF_ADDR_BASE;
	device->v_baseaddr = (uint32_t *) mmap(NULL, (uint32_t) AXI_MCDMA_BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dev_fd, (uint32_t) AXI_MCDMA_BUF_ADDR_BASE); 
	if (device->v_baseaddr == MAP_FAILED) {
		ERROR("%s", "control space map failed.");
		close(dev_fd);
		return -1;
	}

	// intialise mm2s buffer space
	device->p_buffer_src_addr = AXI_MCDMA_BUF_SRC_ADDR;
	device->v_buffer_src_addr = (uint32_t *) mmap(NULL, AXI_MCDMA_BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dev_fd, AXI_MCDMA_BUF_SRC_ADDR); 
	if (device->v_buffer_src_addr == MAP_FAILED) {
		ERROR("%s", "buffer src address map failed.");
		close(dev_fd);
		return -1;
	}

	// initialise s2mm buffer space
	device->p_buffer_dst_addr = AXI_MCDMA_BUF_DST_ADDR;
	device->v_buffer_dst_addr = (uint32_t *) mmap(NULL, AXI_MCDMA_BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dev_fd, AXI_MCDMA_BUF_DST_ADDR); 
	if (device->v_buffer_dst_addr == MAP_FAILED) {
		INFO("%s", "buffer dst address map failed.");
		close(dev_fd);
		return -1;
	}

	close(dev_fd);

	/*** config device struct ***/
	mcdma_channel_t *channel = (mcdma_channel_t *) malloc(sizeof( mcdma_channel_t ));
	MALLOC_CHK(channel);
	device->channels = channel;
	device->num_channels = NUM_CHANNELS;
	device->size = AXI_MCDMA_BUF_SIZE;
	device->channels->next_channel = NULL;

	/*** Initialise address space ***/
	// Copy payload to mm2s buffer space
	memcpy(device->v_buffer_src_addr, (void *) payload, payload_length * sizeof(uint32_t));
	INFO("%s", "Copied payload to src buffer.");

	int res;
	res = config_mcdma_bd_chain(device, payload_length);
	if (res) {
		ERROR("%s", "failed to config bd chain.");
	}

	INFO("configd %d channels.", NUM_CHANNELS);

	res = config_mcdma_mm2s(device);
	if (res) {
		ERROR("%s", "failed to config mm2s.");
	}

	INFO("%s", "configd mm2s.");

	return 0;
}

int config_mcdma_bd_chain(mcdma_device_t *device, int payload_length) {
	// Open /dev/mem for memory mapping
	int32_t dev_fd = open("/dev/mem", O_RDWR | O_SYNC);
	if (dev_fd < 0) {
		ERROR("%s", "Failed to open /dev/mem.");
		return -1;
	}

	mcdma_bd_t *mm2s_bd = (mcdma_bd_t *) malloc(sizeof( mcdma_bd_t ));
	MALLOC_CHK(mm2s_bd);
	device->channels->mm2s_bd_chain = mm2s_bd;

	mcdma_bd_t *s2mm_bd = (mcdma_bd_t *) malloc(sizeof( mcdma_bd_t ));
	MALLOC_CHK(s2mm_bd);
	device->channels->s2mm_bd_chain = s2mm_bd;

	// initialise mm2s bd chain space
	mm2s_bd->p_bd_addr = AXI_MCDMA_MM2S_BD_CHAIN_ADDR;
	mm2s_bd->v_bd_addr = (uint32_t *) mmap(NULL, AXI_MCDMA_BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dev_fd, AXI_MCDMA_MM2S_BD_CHAIN_ADDR); 
	if (mm2s_bd->v_bd_addr == MAP_FAILED) {
		INFO("%s", "mm2s bd chain address map failed.");
		close(dev_fd);
		return -1;
	}

	mm2s_bd->next_mcdma_bd = NULL;
	mm2s_bd->next_bd_addr = mm2s_bd->p_bd_addr;
	mm2s_bd->buffer_addr = AXI_MCDMA_BUF_SRC_ADDR;
	mm2s_bd->buffer_length = (uint32_t) (payload_length * sizeof( uint32_t ));
	mm2s_bd->sof = 1;
	mm2s_bd->eof = 1;
	mm2s_bd->tid = 0;

	INFO("%s", "mm2s_bd fields:");
	INFO("> next_bd_addr: 0x%08x", mm2s_bd->next_bd_addr);
	INFO("> buffer_addr: 0x%08x", mm2s_bd->buffer_addr);
	INFO("> buffer_length: 0x%08x", mm2s_bd->buffer_length);
	INFO("> sof: %d", mm2s_bd->sof);
	INFO("> eof: %d", mm2s_bd->eof);
	INFO("> tid: %d", mm2s_bd->tid);

	INFO("Writing mm2s_bd fields to addr 0x%08x:", mm2s_bd->p_bd_addr);
	_reg_set(mm2s_bd->v_bd_addr, AXI_MCDMA_MM2S_BD_NEXT_DESC_LSB, mm2s_bd->next_bd_addr);
	INFO("reg@0x%08x : 0x%08x", AXI_MCDMA_MM2S_BD_NEXT_DESC_LSB, mm2s_bd->next_bd_addr);
	_reg_set(mm2s_bd->v_bd_addr, AXI_MCDMA_MM2S_BD_BUF_ADDR_LSB, mm2s_bd->buffer_addr);
	INFO("reg@0x%08x : 0x%08x", AXI_MCDMA_MM2S_BD_BUF_ADDR_LSB, mm2s_bd->buffer_addr);
	uint32_t control = (uint32_t) (mm2s_bd->sof << 31) | (uint32_t) (mm2s_bd->eof << 30) | mm2s_bd->buffer_length;
	_reg_set(mm2s_bd->v_bd_addr, AXI_MCDMA_MM2S_BD_CONTROL, control);
	INFO("reg@0x%08x : 0x%08x", AXI_MCDMA_MM2S_BD_CONTROL, control);
	uint32_t control_sideband = (uint32_t) (mm2s_bd->tid << 31);
	_reg_set(mm2s_bd->v_bd_addr, AXI_MCDMA_MM2S_BD_CONTROL_SIDEBAND, control_sideband);
	INFO("reg@0x%08x : 0x%08x", AXI_MCDMA_MM2S_BD_CONTROL_SIDEBAND, control_sideband);

	// initialise s2mm bd chain space
	s2mm_bd->p_bd_addr = AXI_MCDMA_S2MM_BD_CHAIN_ADDR;
	s2mm_bd->v_bd_addr = (uint32_t *) mmap(NULL, AXI_MCDMA_BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dev_fd, AXI_MCDMA_S2MM_BD_CHAIN_ADDR); 
	if (s2mm_bd->v_bd_addr == MAP_FAILED) {
		INFO("%s", "s2mm bd chain address map failed.");
		close(dev_fd);
		return -1;
	}

	s2mm_bd->next_mcdma_bd = NULL;
	s2mm_bd->next_bd_addr = s2mm_bd->p_bd_addr;
	s2mm_bd->buffer_addr = AXI_MCDMA_BUF_SRC_ADDR;
	s2mm_bd->buffer_length = (uint32_t) (payload_length * sizeof( uint32_t ));

	INFO("%s", "s2mm_bd fields:");
	INFO("> next_bd_addr: 0x%08x", s2mm_bd->next_bd_addr);
	INFO("> buffer_addr: 0x%08x", s2mm_bd->buffer_addr);
	INFO("> buffer_length: 0x%08x", s2mm_bd->buffer_length);

	INFO("Writing mm2s_bd fields to addr 0x%08x:", mm2s_bd->p_bd_addr);
	_reg_set(s2mm_bd->v_bd_addr, AXI_MCDMA_MM2S_BD_NEXT_DESC_LSB, mm2s_bd->next_bd_addr);
	INFO("reg@0x%08x : 0x%08x", AXI_MCDMA_MM2S_BD_NEXT_DESC_LSB, mm2s_bd->next_bd_addr);
	_reg_set(s2mm_bd->v_bd_addr, AXI_MCDMA_MM2S_BD_BUF_ADDR_LSB, mm2s_bd->buffer_addr);
	INFO("reg@0x%08x : 0x%08x", AXI_MCDMA_MM2S_BD_BUF_ADDR_LSB, mm2s_bd->buffer_addr);
	_reg_set(s2mm_bd->v_bd_addr, AXI_MCDMA_MM2S_BD_CONTROL, mm2s_bd->buffer_length);
	INFO("reg@0x%08x : 0x%08x", AXI_MCDMA_MM2S_BD_CONTROL, mm2s_bd->buffer_length);

	return 0;
}

int config_mcdma_mm2s(mcdma_device_t *device) {
	// enable channels
	
	return 0;
}

int config_mcdma_s2mm(mcdma_device_t *device) {
	return 0;
}

void free_device(mcdma_device_t *device) {
	INFO("%s", "Freeing device.");
	WARNING("%s", "Assuming only one bd chain per channel.");
	free(device->channels->mm2s_bd_chain);
	free(device->channels->s2mm_bd_chain);
	WARNING("%s", "Assuming only one channel.");
	free(device->channels);
	free(device);
}