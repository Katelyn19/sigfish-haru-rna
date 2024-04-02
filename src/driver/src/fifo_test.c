/*
author: Katelyn Mak
date: 28/2/2024

This program loads a randomly generated payload onto the axi dma and expects it to be returned exactly the same.

Compile with gcc fifo_test.c –o fifo_test
*/
#include "../include/misc.h"
#include "../include/haru.h"

#include <stdio.h> // todo: remove this once debugging is finished
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

void dma_mm2s_status(uint32_t *axi_dma_v_addr) {
	uint32_t status = _reg_get(axi_dma_v_addr, AXI_DMA_MM2S_SR);
	printf("Memory-mapped to stream status (0x%08x@0x%02x):", status, AXI_DMA_MM2S_SR);
	if (status & 0x00000001)
		printf(" halted");
	else
		printf(" running");

	if (status & 0x00000002)
		printf(" idle");
	if (status & 0x00000008)
		printf(" SGIncld");
	if (status & 0x00000010)
		printf(" DMAIntErr");
	if (status & 0x00000020)
		printf(" DMASlvErr");
	if (status & 0x00000040)
		printf(" DMADecErr");
	if (status & 0x00000100)
		printf(" SGIntErr");
	if (status & 0x00000200)
		printf(" SGSlvErr");
	if (status & 0x00000400)
		printf(" SGDecErr");
	if (status & 0x00001000)
		printf(" IOC_Irq");
	if (status & 0x00002000)
		printf(" Dly_Irq");
	if (status & 0x00004000)
		printf(" Err_Irq");
	printf("\n");
}

void dma_s2mm_status(uint32_t *axi_dma_v_addr) {
	uint32_t status = _reg_get(axi_dma_v_addr, AXI_DMA_S2MM_SR);
	printf("Stream to Memory-mapped status (0x%08x@0x%02x):", status, AXI_DMA_S2MM_SR);
	if (status & (1 << AXI_DMA_SR_HALTED))
		printf(" halted");
	else
		printf(" running");
	if (status & (1 << AXI_DMA_SR_IDLE))
		printf(" idle");
	if (status & (1 << AXI_DMA_SR_SG_ACT))
		printf(" SGIncld");
	if (status & (1 << AXI_DMA_SR_DMA_INT_ERR))
		printf(" DMAIntErr");
	if (status & (1 << AXI_DMA_SR_DMA_SLV_ERR))
		printf(" DMASlvErr");
	if (status & (1 << AXI_DMA_SR_DMA_DEC_ERR))
		printf(" DMADecErr");
	if (status & (1 << AXI_DMA_SR_SG_INT_ERR))
		printf(" SGIntErr");
	if (status & (1 << AXI_DMA_SR_SG_SLV_ERR))
		printf(" SGSlvErr");
	if (status & (1 << AXI_DMA_SR_SG_DEC_ERR))
		printf(" SGDecErr");
	if (status & (1 << AXI_DMA_SR_IOC_IRQ))
		printf(" IOC_Irq");
	if (status & (1 << AXI_DMA_SR_DLY_IRQ))
		printf(" Dly_Irq");
	if (status & (1 << AXI_DMA_SR_ERR_IRQ))
		printf(" Err_Irq");
	printf("\n");
}
int cmpfunc(const void * a, const void * b) {
	return ( *(int*)a - *(int*)b );
}

int fifo_test() {
	printf("Initialising AXI DMA.\n");

	int payload_len = 5;
	uint32_t payload[payload_len];
	
	// create random payload
	for (int i = 0; i < payload_len; i++) {
		payload[i] = (uint32_t) rand();
		printf("%d: %d\n", i, payload[i]);
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

	dma_mm2s_status(axi_dma_v_addr);

	// clear the fifo_interconnect
	printf("Setting fifo interconnect clear to 1.\n");
	_reg_set(fifo_intcn_v_addr, 0x00, 1);
	printf("Setting fifo interconnect clear to 0.\n");
	_reg_set(fifo_intcn_v_addr, 0x00, 0);

	// reset the axi dma
	printf("Resetting and stopping the AXI DMA.\n");
	_reg_set(axi_dma_v_addr, AXI_DMA_MM2S_CR, 1 << AXI_DMA_CR_RESET);
	_reg_set(axi_dma_v_addr, AXI_DMA_S2MM_CR, 1 << AXI_DMA_CR_RESET);

	uint32_t cr;
	cr = _reg_get(axi_dma_v_addr, AXI_DMA_MM2S_CR);
	printf("S2MM Control Register: 00x%08x\n", cr);
	cr = _reg_get(axi_dma_v_addr, AXI_DMA_S2MM_CR);
	printf("MM2S Control Register: 00x%08x\n", cr);

	_reg_set(axi_dma_v_addr, AXI_DMA_S2MM_CR, cr & ~(1 << AXI_DMA_CR_IOC_IRQ_EN));
	_reg_set(axi_dma_v_addr, AXI_DMA_MM2S_CR, cr & ~(1 << AXI_DMA_CR_IOC_IRQ_EN));

	// stop the axi dma
	cr = _reg_get(axi_dma_v_addr, AXI_DMA_MM2S_CR);
	_reg_set(axi_dma_v_addr, AXI_DMA_MM2S_CR, cr & ~(1 << AXI_DMA_CR_RS));

	cr = _reg_get(axi_dma_v_addr, AXI_DMA_S2MM_CR);
	_reg_set(axi_dma_v_addr, AXI_DMA_S2MM_CR, cr & ~(1 << AXI_DMA_CR_RS));

	dma_mm2s_status(axi_dma_v_addr);
	dma_s2mm_status(axi_dma_v_addr);

	//////////////////////////////// MM2S //////////////////////////////////
	printf("Putting payload on the buffer.\n");
	// set the axi dma stream src/dst address
	_reg_set(axi_dma_v_addr, AXI_DMA_MM2S_SRC_ADDR, HARU_AXI_SRC_ADDR);
	_reg_set(axi_dma_v_addr, AXI_DMA_S2MM_DST_ADDR, HARU_AXI_DST_ADDR);

	// // load the data into the buffer
	// for (int i = 0; i < payload_len; i += 8) {
	// 	_reg_set(axis_src_v_addr, i, payload[i]);
	// }

	memcpy(axis_src_v_addr, (void *) payload, payload_len * sizeof(uint32_t));

	// verify payload
	uint32_t data_display;
	printf("Verifying Payload at src address:\n");
	for (int i = 0; i < payload_len*4; i += 4) {
		data_display = _reg_get(axis_src_v_addr, i);
		printf("%d: %d\n", i/8, data_display);
	}
	printf("Staring axi dma transfer\n");

	// start flag the axi dma transfer
	_reg_set(axi_dma_v_addr, AXI_DMA_MM2S_CR, 0xf001);
	_reg_set(axi_dma_v_addr, AXI_DMA_S2MM_CR, 0xf001);

	// set the axi dma stream length - this begins the transfer.
	_reg_set(axi_dma_v_addr, AXI_DMA_MM2S_LENGTH, payload_len*4);
	_reg_set(axi_dma_v_addr, AXI_DMA_S2MM_LENGTH, payload_len*4);

	dma_mm2s_status(axi_dma_v_addr);
	dma_s2mm_status(axi_dma_v_addr);

	cr = _reg_get(axi_dma_v_addr, AXI_DMA_S2MM_LENGTH);
	printf("S2MM Length Register: 00x%08x = %d (decimal)\n", cr, cr);

	cr = _reg_get(axi_dma_v_addr, AXI_DMA_MM2S_LENGTH);
	printf("MM2S Length Register: 00x%08x = %d (decimal)\n", cr, cr);

	printf("Waiting for transfer to be done...\n");

	// busy wait
	volatile uint32_t mm2s_sr = _reg_get(axi_dma_v_addr, AXI_DMA_MM2S_SR);
	while (!(mm2s_sr & (1 << AXI_DMA_SR_IDLE))) {
	// while (!(mm2s_sr)) {
		mm2s_sr = _reg_get(axi_dma_v_addr, AXI_DMA_MM2S_SR);
	}

	printf("laoded onto the axi dma...\n");

	dma_mm2s_status(axi_dma_v_addr);
	dma_s2mm_status(axi_dma_v_addr);

	cr = _reg_get(axi_dma_v_addr, AXI_DMA_MM2S_LENGTH);
	printf("\nBytes written:\n");
	printf("MM2S Length Register: 00x%08x\n", cr);
	printf("MM2S Length Register: %d (decimal)\n\n", cr);

	// busy wait
	volatile uint32_t s2mm_sr = _reg_get(axi_dma_v_addr, AXI_DMA_S2MM_SR);
	while (!(s2mm_sr  & (1 << AXI_DMA_SR_IDLE))) {
	// while (!(s2mm_sr)) {
		s2mm_sr = _reg_get(axi_dma_v_addr, AXI_DMA_S2MM_SR);
		// if (counter == payload_len00) {
		// 	dma_s2mm_status(axi_dma_v_addr);
		// 	counter = 0;
		// } else {
		// 	counter++;
		// }
	}

	printf("AXI DMA transfer done.\n");

	cr = _reg_get(axi_dma_v_addr, AXI_DMA_S2MM_LENGTH);
	printf("\nBytes written:\n");
	printf("S2MM Length Register: 00x%08x\n", cr);
	printf("S2MM Length Register: %d (decimal)\n\n", cr);

	dma_mm2s_status(axi_dma_v_addr);
	dma_s2mm_status(axi_dma_v_addr);

	// //////////////////////////////// S2MM //////////////////////////////////
	// printf("Retrieving payload from the buffer.\n");
	// // set the axi dma stream dst address
	// _reg_set(axi_dma_v_addr, AXI_DMA_S2MM_DST_ADDR, HARU_AXI_DST_ADDR);

	// // set the axi dma stream length
	// _reg_set(axi_dma_v_addr, AXI_DMA_S2MM_LENGTH, payload_len);

	// // start the axi dma load
	// _reg_set(axi_dma_v_addr, AXI_DMA_S2MM_CR, 0xf001);
	// _reg_set(axi_dma_v_addr, AXI_DMA_S2MM_LENGTH, payload_len);
	
	// // busy wait
	// volatile uint32_t s2mm_sr = _reg_get(axi_dma_v_addr, AXI_DMA_S2MM_SR);
	// while (!(s2mm_sr  & (1 << AXI_DMA_SR_IDLE))) {
	// 	s2mm_sr = _reg_get(axis_dst_v_addr, AXI_DMA_S2MM_SR);
	// }

	// check the loaded data is correct
	printf("Error checking.\n");
	int error = 0;
	uint32_t data;
	for (int i = 0; i < payload_len; i++) {
		data = _reg_get(axis_src_v_addr, i*4);
		if (data != payload[i]) {
			error++;
			printf("data mismatch @ payload[%d]: %d %d\n", i, data, payload[i]);
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
