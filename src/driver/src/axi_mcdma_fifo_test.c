/*
author: Katelyn Mak
date created: 12/3/2024

This program loads a random payload onto a Multi-Channel AXI DMA (MCDMA) project and verifies the return values.
Currently, only one channel is used for the MCDMA.

*/

#include "../include/misc.h"
#include "../include/axi_mcdma.h"
#include "../include/axi_mcdma_fifo_test.h"

#include <stdio.h> // todo: remove this once debugging is finished
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>


int axi_mcdma_fifo_test(int payload_len) {
	printf("============================= MCDMA FIFO TEST =============================\n");

	printf("Creating random payload:\n");
	// Create random payload
	uint32_t payload[payload_len];
	for (int i = 0; i < payload_len; i++) {
		payload[i] = (uint32_t) rand();
		printf("%d: 0x%08x\n", i, payload[i]);
	}

	/*** Prepare MCDMA buffer ***/
	// Open /dev/mem for memory mapping
	int32_t dev_fd = open("/dev/mem", O_RDWR | O_SYNC);
	if (dev_fd < 0) {
		printf("Error: Failed to open /dev/mem.\n");
		return -1;
	}

	// initialise the axi dma control space
	uint32_t * axi_mcdma_v_addr = (uint32_t *) mmap(NULL, (uint32_t) AXI_MCDMA_BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dev_fd, (uint32_t) AXI_MCDMA_BUF_ADDR_BASE); 
	if (axi_mcdma_v_addr == MAP_FAILED) {
		printf("Error: AXI DMA map failed.\n");
		close(dev_fd);
		return -1;
	}

	// intialise the axi dma stream space
	// axi mcdma source addr
	uint32_t * axis_src_v_addr = (uint32_t *) mmap(NULL, AXI_MCDMA_BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dev_fd, AXI_MCDMA_BUF_SRC_ADDR); 
	if (axis_src_v_addr  == MAP_FAILED) {
		printf("Error: AXI DMA source address map failed.\n");
		close(dev_fd);
		return -1;
	}

	// axi mcdma destination addr
	uint32_t * axis_dst_v_addr = (uint32_t *) mmap(NULL, AXI_MCDMA_BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dev_fd, AXI_MCDMA_BUF_DST_ADDR); 
	if (axis_dst_v_addr == MAP_FAILED) {
		printf("Error: AXI DMA destination address map failed.\n");
		close(dev_fd);
		return -1;
	}

	// axi mcdma destination addr
	uint32_t * axis_buf_desc_mm2s_v_addr = (uint32_t *) mmap(NULL, AXI_MCDMA_BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dev_fd, AXI_MCDMA_MM2S_BUF_DESC_CHAIN_ADDR); 
	if (axis_buf_desc_mm2s_v_addr == MAP_FAILED) {
		printf("Error: AXI DMA buffer descriptor mm2s chain address map failed.\n");
		close(dev_fd);
		return -1;
	}

	uint32_t * axis_buf_desc_s2mm_v_addr = (uint32_t *) mmap(NULL, AXI_MCDMA_BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dev_fd, AXI_MCDMA_S2MM_BUF_DESC_CHAIN_ADDR); 
	if (axis_buf_desc_s2mm_v_addr == MAP_FAILED) {
		printf("Error: AXI DMA buffer descriptor s2mm chain address map failed.\n");
		close(dev_fd);
		return -1;
	}

	/*** Set up buffer ***/
	// copy payload to memory
	memcpy(axis_src_v_addr, (void *) payload, payload_len * sizeof(uint32_t));

	// Create descriptor chain
	_reg_set(axis_buf_desc_mm2s_v_addr, AXI_MCDMA_MM2S_BD_NEXT_DESC_LSB, AXI_MCDMA_MM2S_BUF_DESC_CHAIN_ADDR);
	_reg_set(axis_buf_desc_mm2s_v_addr, AXI_MCDMA_MM2S_BD_BUF_ADDR_LSB, AXI_MCDMA_BUF_SRC_ADDR);
	_reg_set(axis_buf_desc_mm2s_v_addr, AXI_MCDMA_MM2S_BD_CONTROL, (uint32_t) (payload_len * sizeof(uint32_t)) | 0x40000000);
	fprintf(stderr, "mm2s Buffer Length: 0x%08x\n", (uint32_t) (payload_len*2 * sizeof(uint16_t)) | 0x40000000);

	_reg_set(axis_buf_desc_s2mm_v_addr, AXI_MCDMA_MM2S_BD_NEXT_DESC_LSB, AXI_MCDMA_S2MM_BUF_DESC_CHAIN_ADDR);
	_reg_set(axis_buf_desc_s2mm_v_addr, AXI_MCDMA_MM2S_BD_BUF_ADDR_LSB, AXI_MCDMA_BUF_DST_ADDR);
	_reg_set(axis_buf_desc_mm2s_v_addr, AXI_MCDMA_MM2S_BD_CONTROL, (uint32_t) (payload_len * sizeof(uint32_t)));
	fprintf(stderr, "s2mm Buffer Length: 0x%08x\n", (uint32_t) (payload_len*2 * sizeof(uint16_t)));

	// reset mcdma
	fprintf(stderr, "Reset mm2s and s2mm.\n");
	_reg_set(axi_mcdma_v_addr, AXI_MCDMA_MM2S_CCR, AXI_MCDMA_MM2S_RESET);
	_reg_set(axi_mcdma_v_addr, AXI_MCDMA_S2MM_CCR, AXI_MCDMA_MM2S_RESET);

	/*** Initialise MCDMA mm2s **/
	// Enable Channels
	_reg_set(axi_mcdma_v_addr, AXI_MCDMA_MM2S_CHEN, 0x1);
	fprintf(stderr, "mm2s: Enabled channel 0. Disabled all other channels.\n");

	// Map Current Descriptor to buffer space
	_reg_set(axi_mcdma_v_addr, AXI_MCDMA_MM2S_CH1CURDESC_LSB, AXI_MCDMA_MM2S_BUF_DESC_CHAIN_ADDR);
	fprintf(stderr, "mm2s: Set channel 0 current bd to address: 0x%08x\n", AXI_MCDMA_MM2S_BUF_DESC_CHAIN_ADDR);

	// program the CHANNEL.fetch bit (CH.RS)
	_reg_set(axi_mcdma_v_addr, AXI_MCDMA_MM2S_CH1CR, AXI_MCDMA_MM2S_CH1RS);
	fprintf(stderr, "mm2s: Enabled channel 0 bd fetch.\n");

	// status update
	volatile uint32_t mm2s_sr = _reg_get(axi_mcdma_v_addr, AXI_MCDMA_MM2S_CH1SR);
	fprintf(stderr, "mm2s: status = 0x%08x\n", mm2s_sr);

	// Start MCDMA (set MCDMA.RS bit)
	_reg_set(axi_mcdma_v_addr, AXI_MCDMA_MM2S_CCR, AXI_MCDMA_MM2S_RS);
	fprintf(stderr, "mm2s: Started axi mcdma with RS bit.\n");

	// Program interrupt threshold (enable interrupts)

	// Porgram the Queue Scheduler register if applicable

	// Program the TD register of channels
	// MCDMA starts when TD is programmed
	// uint32_t td_value_mm2s = AXI_MCDMA_BUF_SRC_ADDR + (payload_len * sizeof(uint32_t))/64;
	uint32_t td_value_mm2s = AXI_MCDMA_MM2S_BUF_DESC_CHAIN_ADDR;
	// uint32_t td_value_mm2s = AXI_MCDMA_MM2S_BUF_DESC_CHAIN_ADDR;
	_reg_set(axi_mcdma_v_addr, AXI_MCDMA_MM2S_CH1TAILDESC_LSB, (uint32_t) td_value_mm2s);
	fprintf(stderr, "mm2s: Set channel 0 bd tail to tail adddress: 0x%08x\n", td_value_mm2s);

	/*** Wait for Transfer to be complete ***/
	fprintf(stderr, "mm2s: Waiting for mm2s channel go idle.\n");
	// busy wait
	while (!(mm2s_sr & AXI_MCDMA_MM2S_CHIDLE)) {
	// while (!(mm2s_sr)) {
		mm2s_sr = _reg_get(axi_mcdma_v_addr, AXI_MCDMA_MM2S_CH1SR);
		fprintf(stderr, "mm2s: status = 0x%08x\n", mm2s_sr);
	}

	fprintf(stderr, "mm2s: Transfer completed\n");

	// status update
	mm2s_sr = _reg_get(axi_mcdma_v_addr, AXI_MCDMA_MM2S_CH1SR);
	fprintf(stderr, "mm2s: status = 0x%08x\n", mm2s_sr);

	/*** Retrive MCDMA s2mm ***/
	// Enable Channels
	_reg_set(axi_mcdma_v_addr, AXI_MCDMA_S2MM_CHEN, 0x1);
	fprintf(stderr, "s2mm: Enabled channel 0. Disabled all other channels.\n");

	_reg_set(axi_mcdma_v_addr, AXI_MCDMA_S2MM_CH1CURDESC_LSB, AXI_MCDMA_S2MM_BUF_DESC_CHAIN_ADDR << 6);
	fprintf(stderr, "s2mm: Set channel 0 bd to dst address: 00x%08x\n", AXI_MCDMA_S2MM_BUF_DESC_CHAIN_ADDR << 6);

	// Porgram the CHANNEL.fetch bit
	_reg_set(axi_mcdma_v_addr, AXI_MCDMA_S2MM_CH1CR, AXI_MCDMA_MM2S_CH1RS);
	fprintf(stderr, "s2mm: Enabled channel 0 bd fetch.\n");

	// status update
	volatile uint32_t s2mm_sr = _reg_get(axi_mcdma_v_addr, AXI_MCDMA_S2MM_CH1SR);
	fprintf(stderr, "s2mm: status = 0x%08x\n", s2mm_sr);

	// Start MCDMA (set MCDMA.RS bit)
	_reg_set(axi_mcdma_v_addr, AXI_MCDMA_S2MM_CCR, AXI_MCDMA_MM2S_RS);
	fprintf(stderr, "s2mm: Started axi mcdma with RS bit.\n");

	// Porgram interrupt thresholds (enable interrupt)

	// Porgram the TD register of channels
	uint32_t td_value_s2mm = AXI_MCDMA_S2MM_BUF_DESC_CHAIN_ADDR << 6;
	// _reg_set(axi_mcdma_v_addr, AXI_MCDMA_S2MM_CH1TAILDESC_LSB, td_value_s2mm);
	// fprintf(stderr, "s2mm: Set channel 0 bd tail to tail adddress: 00x%08x\n", td_value_s2mm);

	/*** Wait for Transfer to be complete ***/
	// busy wait
	s2mm_sr = _reg_get(axi_mcdma_v_addr, AXI_MCDMA_S2MM_CH1SR);
	while (!(s2mm_sr & AXI_MCDMA_MM2S_CHIDLE)) {
	// while (!(mm2s_sr)) {
		mm2s_sr = _reg_get(axi_mcdma_v_addr, AXI_MCDMA_S2MM_CH1SR);
	}

	fprintf(stderr, "s2mm: Transfer completed\n");

	// status update
	s2mm_sr = _reg_get(axi_mcdma_v_addr, AXI_MCDMA_MM2S_CH1SR);
	fprintf(stderr, "s2mm: status = 0x%08x\n", s2mm_sr);


	uint32_t error_reg = _reg_get(axi_mcdma_v_addr, 0x00C);
	fprintf(stderr, "mm2s: channel in progress register = 0x%08x\n", error_reg);
	error_reg = _reg_get(axi_mcdma_v_addr, 0x010);
	fprintf(stderr, "mm2s: error register = 0x%08x\n", error_reg);

	error_reg = _reg_get(axi_mcdma_v_addr, 0x50C);
	fprintf(stderr, "s2mm: channel in progress register = 0x%08x\n", error_reg);
	error_reg = _reg_get(axi_mcdma_v_addr, 0x510);
	fprintf(stderr, "s2mm: error register = 0x%08x\n", error_reg);


	// check the loaded data is correct
	fprintf(stderr, "Error checking.\n");
	int error = 0;
	uint32_t data;
	for (int i = 0; i < payload_len; i++) {
		data = _reg_get(axis_dst_v_addr, i*4);
		if (data != payload[i]) {
			error++;
			printf("data mismatch @ payload[%d]: 0x%08x 0x%08x\n", i, data, payload[i]);
		}
	}

	if (error == 0) {
		printf("Success!\n");
	} else {
		printf("Oh no! Data mismatches: %d\n", error);
	}

	close(dev_fd);

	return 0;
}
