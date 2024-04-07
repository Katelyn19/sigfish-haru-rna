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
int configure_mcdma(mcdma_device_t * device, int payload_length);
int start_mcdma(mcdma_device_t * device);
int verify_payload(mcdma_device_t * device, int payload_length);

int mcdma_test (int payload_length) {
	fprintf(stderr, "============================= MCDMA FIFO TEST =============================\n");

	// Create device struct
	mcdma_device_t device;

	int res;
	res = configure_mcdma(&device, payload_length);
	if (res) {
		ERROR("%s", "Could not configure MCDMA.\n");
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

	return 0;
}

int configure_mcdma(mcdma_device_t * device, int payload_length) {
	INFO("%s", "Creating random payload:\n");
	// Create random payload
	uint32_t payload[payload_length];
	for (int i = 0; i < payload_length; i++) {
		payload[i] = (uint32_t) rand();
		INFO("%d: 0x%08x\n", i, payload[i]);
	}

	/*** Memory map address space ***/
	// Open /dev/mem for memory mapping
	int32_t dev_fd = open("/dev/mem", O_RDWR | O_SYNC);
	if (dev_fd < 0) {
		ERROR("%s", "Failed to open /dev/mem.\n");
		return -1;
	}

	// initialise the axi dma control space
	device->p_baseaddr = AXI_MCDMA_BUF_ADDR_BASE;
	device->v_baseaddr = (uint32_t *) mmap(NULL, (uint32_t) AXI_MCDMA_BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dev_fd, (uint32_t) AXI_MCDMA_BUF_ADDR_BASE); 
	if (device->v_baseaddr == MAP_FAILED) {
		ERROR("%s", "control space map failed.\n");
		close(dev_fd);
		return -1;
	}

	// intialise mm2s buffer space
	device->p_buffer_src_addr = AXI_MCDMA_BUF_SRC_ADDR;
	device->v_buffer_src_addr = (uint32_t *) mmap(NULL, AXI_MCDMA_BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dev_fd, AXI_MCDMA_BUF_SRC_ADDR); 
	if (device->v_buffer_src_addr == MAP_FAILED) {
		ERROR("%s", "buffer src address map failed.\n");
		close(dev_fd);
		return -1;
	}

	// initialise s2mm buffer space
	device->p_buffer_dst_addr = AXI_MCDMA_BUF_DST_ADDR;
	device->v_buffer_dst_addr = (uint32_t *) mmap(NULL, AXI_MCDMA_BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dev_fd, AXI_MCDMA_BUF_DST_ADDR); 
	if (device->v_buffer_dst_addr == MAP_FAILED) {
		INFO("%s", "buffer dst address map failed.\n");
		close(dev_fd);
		return -1;
	}

	// initialise mm2s bd chain space
	device->channels->mm2s_bd_chain->p_bd_addr = AXI_MCDMA_MM2S_BD_CHAIN_ADDR;
	device->channels->mm2s_bd_chain->v_bd_addr = (uint32_t *) mmap(NULL, AXI_MCDMA_BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dev_fd, AXI_MCDMA_MM2S_BD_CHAIN_ADDR); 
	if (device->channels->mm2s_bd_chain->v_bd_addr == MAP_FAILED) {
		INFO("%s", "mm2s bd chain address map failed.\n");
		close(dev_fd);
		return -1;
	}

	// initialise s2mm bd chain space
	device->channels->s2mm_bd_chain->p_bd_addr = AXI_MCDMA_S2MM_BD_CHAIN_ADDR;
	device->channels->s2mm_bd_chain->v_bd_addr = (uint32_t *) mmap(NULL, AXI_MCDMA_BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dev_fd, AXI_MCDMA_S2MM_BD_CHAIN_ADDR); 
	if (device->channels->s2mm_bd_chain->v_bd_addr == MAP_FAILED) {
		INFO("%s", "s2mm bd chain address map failed.\n");
		close(dev_fd);
		return -1;
	}

	return 0;
}