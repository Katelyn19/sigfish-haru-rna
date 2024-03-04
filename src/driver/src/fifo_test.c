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
	printf("initialising axi_dma.\n");

	uint32_t ret;

	uint32_t payload[100];
	
	// create random payload
	for (int i = 0; i < 100; i++) {
		payload[i] = (uint32_t) rand();
		printf("%d: %d\n", i, payload[i]);
	}

	// initialise the axi dma - allocate space in mem for the axi to read/write from

	// Open /dev/mem for memory mapping
	int32_t dev_fd = open("/dev/mem", O_RDWR | O_SYNC);
	if (dev_fd < 0) {
		return -1;
	}

	// initialise the axi dma control space
	void * axi_dma_v_addr = (uint32_t *) mmap(NULL, HARU_AXI_DMA_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dev_fd, HARU_AXI_DMA_ADDR_BASE); 
	if (axi_dma_v_addr == MAP_FAILED) {
		close(dev_fd);
		return -1;
	}

	// intialise the axi dma stream space
	void * axis_src_v_addr = (uint32_t *) mmap(NULL, HARU_AXI_BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dev_fd, HARU_AXI_SRC_ADDR); 
	if (axis_src_v_addr  == MAP_FAILED) {
		close(dev_fd);
		return -1;
	}

	void * axis_dst_v_addr = (uint32_t *) mmap(NULL, HARU_AXI_BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dev_fd, HARU_AXI_SRC_ADDR); 
	if (axis_dst_v_addr == MAP_FAILED) {
		close(dev_fd);
		return -1;
	}

	// initialise the fifo_interconnect - allocate space in mem for the fifo_interconnect to read from
	void * fifo_intcn_v_addr = (uint32_t *) mmap(NULL, HARU_DTW_ACCEL_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dev_fd, HARU_DTW_ACCEL_ADDR_BASE);
	if (fifo_intcn_v_addr == MAP_FAILED) {
		close(dev_fd);
		return -1;
	}

	// clear the fifo_interconnect
	_reg_set(fifo_intcn_v_addr, 0x00, 1);
	_reg_set(fifo_intcn_v_addr, 0x00, 0);


	//////////////////////////////// MM2S //////////////////////////////////
	// set the axi dma stream src address
	_reg_set(axi_dma_v_addr, AXI_DMA_MM2S_SRC_ADDR, HARU_AXI_SRC_ADDR);

	// set the axi dma stream length
	_reg_set(axi_dma_v_addr, AXI_DMA_MM2S_LENGTH, 100);

	// load the data into the buffer
	for (int i = 0; i < 100; i++) {
		_reg_set(axis_src_v_addr, i, payload[i]);
	}
	// start the axi dma transfer
	_reg_set(axi_dma_v_addr, AXI_DMA_MM2S_CR, 0xf001);

	// busy wait
	volatile uint32_t mm2s_sr = _reg_get(axi_dma_v_addr, AXI_DMA_MM2S_SR);
	while (!(mm2s_sr & (1 << AXI_DMA_SR_IDLE))) {
		mm2s_sr = _reg_get(axi_dma_v_addr, AXI_DMA_MM2S_SR);
	}

	//////////////////////////////// S2MM //////////////////////////////////
	// set the axi dma stream dst address
	_reg_set(axi_dma_v_addr, AXI_DMA_S2MM_DST_ADDR, HARU_AXI_DST_ADDR);

	// set the axi dma stream length
	_reg_set(axi_dma_v_addr, AXI_DMA_S2MM_LENGTH, 100);

	// start the axi dma load
	_reg_set(axi_dma_v_addr, AXI_DMA_S2MM_CR, 0xf001);
	_reg_set(axi_dma_v_addr, AXI_DMA_S2MM_LENGTH, 100);
	
	// busy wait
	volatile uint32_t s2mm_sr = _reg_get(axi_dma_v_addr, AXI_DMA_S2MM_SR);
	while (!(s2mm_sr  & (1 << AXI_DMA_SR_IDLE))) {
		s2mm_sr = _reg_get(axis_dst_v_addr, AXI_DMA_S2MM_SR);
	}

	// check the loaded data is correct
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
}


int main(int argc, char *argv[]) {
	fifo_test();
}