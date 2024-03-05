/*
author: Katelyn Mak
date: 28/2/2024

This file loads a randomly generated payload onto the axi dma and expects it to be returned exactly the same.

Compile with gcc fifo_test.c –o fifo_test
*/
#include "../include/misc.h"
#include "../include/haru.h"

#include <stdio.h> // todo: remove this once debugging is finished
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

int fifo_test() {
	printf("Initialising AXI DMA.\n");

	uint32_t payload[100];
	
	// create random payload
	for (int i = 0; i < 100; i++) {
		payload[i] = (uint32_t) rand();
	}

	// initialise the axi dma - allocate space in mem for the axi to read/write from

	// Open /dev/mem for memory mapping
	int32_t dev_fd = open("/dev/mem", O_RDWR | O_SYNC);
	if (dev_fd < 0) {
		printf("Error: Failed to open /dev/mem.\n");
		return -1;
	}

	// initialise the axi dma control space
	// uint32_t * axi_dma_v_addr = (uint32_t *) mmap(NULL, (uint32_t) HARU_AXI_DMA_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dev_fd, (uint32_t) HARU_AXI_DMA_ADDR_BASE); 
	uint32_t * axi_dma_v_addr = (uint32_t *) mmap(NULL, (uint32_t) HARU_AXI_DMA_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dev_fd, (uint32_t) HARU_AXI_DMA_ADDR_BASE); 
	if (axi_dma_v_addr == MAP_FAILED) {
		printf("Error: AXI DMA map failed.\n");
		close(dev_fd);
		return -1;
	}

	// intialise the axi dma stream space
	uint32_t * axis_src_v_addr = (uint32_t *) mmap(NULL, HARU_AXI_BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dev_fd, HARU_AXI_SRC_ADDR); 
	if (axis_src_v_addr  == MAP_FAILED) {
		printf("Error: AXI DMA source address map failed.\n");
		close(dev_fd);
		return -1;
	}

	uint32_t * axis_dst_v_addr = (uint32_t *) mmap(NULL, HARU_AXI_BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dev_fd, HARU_AXI_DST_ADDR); 
	if (axis_dst_v_addr == MAP_FAILED) {
		printf("Error: AXI DMA destination address map failed.\n");
		close(dev_fd);
		return -1;
	}

	// initialise the fifo_interconnect - allocate space in mem for the fifo_interconnect to read from
	printf("Initialising the fifo interconnect.\n");
	uint32_t * fifo_intcn_v_addr = (uint32_t *) mmap(NULL, HARU_DTW_ACCEL_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dev_fd, HARU_DTW_ACCEL_ADDR_BASE);
	if (fifo_intcn_v_addr == MAP_FAILED) {
		printf("Error: fifo interconnect map failed.\n");
		close(dev_fd);
		return -1;
	}

	// clear the fifo_interconnect
	printf("Setting fifo interconnect clear to 1.\n");
	_reg_set(fifo_intcn_v_addr, 0x00, 1);
	printf("Setting fifo interconnect clear to 0.\n");
	_reg_set(fifo_intcn_v_addr, 0x00, 0);

	// reset the axi dma
	printf("Resetting and stopping the AXI DMA.\n");
	_reg_set(axi_dma_v_addr, AXI_DMA_MM2S_CR, 1 << AXI_DMA_CR_RESET);
	_reg_set(axi_dma_v_addr, AXI_DMA_S2MM_CR, 1 << AXI_DMA_CR_RESET);
	_reg_set(axi_dma_v_addr, AXI_DMA_MM2S_CR, 0 << AXI_DMA_CR_RESET);
	_reg_set(axi_dma_v_addr, AXI_DMA_S2MM_CR, 0 << AXI_DMA_CR_RESET);

	// stop the axi dma
	uint32_t cr = _reg_get(axi_dma_v_addr, AXI_DMA_MM2S_CR);
	_reg_set(axi_dma_v_addr, AXI_DMA_MM2S_CR, cr & ~(1 << AXI_DMA_CR_RS));
	cr = _reg_get(axi_dma_v_addr, AXI_DMA_S2MM_CR);
	_reg_set(axi_dma_v_addr, AXI_DMA_S2MM_CR, cr & ~(1 << AXI_DMA_CR_RS));


	//////////////////////////////// MM2S //////////////////////////////////
	printf("Putting payload on the buffer.\n");
	// set the axi dma stream src/dst address
	_reg_set(axi_dma_v_addr, AXI_DMA_MM2S_SRC_ADDR, HARU_AXI_SRC_ADDR);
	_reg_set(axi_dma_v_addr, AXI_DMA_S2MM_DST_ADDR, HARU_AXI_DST_ADDR);

	// load the data into the buffer
	for (int i = 0; i < 100; i++) {
		_reg_set(axis_src_v_addr, i, payload[i]);
	}

	printf("Staring axi dma transfer\n");

	// set the axi dma stream length
	_reg_set(axi_dma_v_addr, AXI_DMA_MM2S_LENGTH, 100);
	_reg_set(axi_dma_v_addr, AXI_DMA_S2MM_LENGTH, 100);

	// start the axi dma transfer
	_reg_set(axi_dma_v_addr, AXI_DMA_MM2S_CR, 0xf001);
	_reg_set(axi_dma_v_addr, AXI_DMA_S2MM_CR, 0xf001);

	printf("Waiting for transfer to be done...\n");

	// busy wait
	volatile uint32_t mm2s_sr = _reg_get(axi_dma_v_addr, AXI_DMA_MM2S_SR);
	while (!(mm2s_sr & (1 << AXI_DMA_SR_IDLE))) {
		mm2s_sr = _reg_get(axi_dma_v_addr, AXI_DMA_MM2S_SR);
	}

	printf("laoded onto the axi dma...\n");

	// busy wait
	volatile uint32_t s2mm_sr = _reg_get(axi_dma_v_addr, AXI_DMA_S2MM_SR);
	while (!(s2mm_sr  & (1 << AXI_DMA_SR_IDLE))) {
		s2mm_sr = _reg_get(axi_dma_v_addr, AXI_DMA_S2MM_SR);
	}

	printf("AXI DMA transfer done.\n");

	// //////////////////////////////// S2MM //////////////////////////////////
	// printf("Retrieving payload from the buffer.\n");
	// // set the axi dma stream dst address
	// _reg_set(axi_dma_v_addr, AXI_DMA_S2MM_DST_ADDR, HARU_AXI_DST_ADDR);

	// // set the axi dma stream length
	// _reg_set(axi_dma_v_addr, AXI_DMA_S2MM_LENGTH, 100);

	// // start the axi dma load
	// _reg_set(axi_dma_v_addr, AXI_DMA_S2MM_CR, 0xf001);
	// _reg_set(axi_dma_v_addr, AXI_DMA_S2MM_LENGTH, 100);
	
	// // busy wait
	// volatile uint32_t s2mm_sr = _reg_get(axi_dma_v_addr, AXI_DMA_S2MM_SR);
	// while (!(s2mm_sr  & (1 << AXI_DMA_SR_IDLE))) {
	// 	s2mm_sr = _reg_get(axis_dst_v_addr, AXI_DMA_S2MM_SR);
	// }

	// check the loaded data is correct
	printf("Error checking.\n");
	int error = 0;
	uint32_t data;
	for (int i = 0; i < 100; i++) {
		data = _reg_get(axis_src_v_addr, i);
		if (data != payload[i]) {
			error++;
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

