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
        printf("%d: %d\n", i, payload[i]);
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

    /*** Initialise MCDMA mm2s **/
    // Enable Channels
    _reg_set(axi_mcdma_v_addr, AXI_MCDMA_MM2S_CHEN, 0x1);
    fprintf(stderr, "mm2s: Enabled channel 0. Disabled all other channels.\n");

    // Map Current Descriptor to buffer space
    _reg_set(axi_mcdma_v_addr, AXI_MCDMA_MM2S_CH1CURDESC_LSB, AXI_MCDMA_BUF_SRC_ADDR);
    fprintf(stderr, "mm2s: Set channel 0 bd to src address: 00x%08x\n", AXI_MCDMA_BUF_SRC_ADDR);

    // program the CHANNEL.fetch bit (CH.RS)
    _reg_set(axi_mcdma_v_addr, AXI_MCDMA_MM2S_CH1CR, AXI_MCDMA_MM2S_RS);
    fprintf(stderr, "mm2s: Enabled channel 0 bd fetch.\n");

    // Start MCDMA (set MCDMA.RS bit)
    _reg_set(axi_mcdma_v_addr, AXI_MCDMA_MM2S_CCR, AXI_MCDMA_MM2S_RS);
    fprintf(stderr, "mm2s: Started axi mcdma with RS bit.\n");

    // Program interrupt threshold (enable interrupts)

    // Porgram the Queue Scheduler register if applicable

    // Program the TD register of channels
    // MCDMA starts when TD is programmed
    _reg_set(axi_mcdma_v_addr, AXI_MCDMA_MM2S_CH1TAILDESC_LSB, (uint32_t) (AXI_MCDMA_BUF_SRC_ADDR + (payload_len * sizeof(uint32_t))));
    fprintf(stderr, "mm2s: Set channel 0 bd tail to tail adddress: 00x%08x\n", (uint32_t) (AXI_MCDMA_BUF_SRC_ADDR + (payload_len * sizeof(uint32_t))));

    /*** Wait for Transfer to be complete ***/
	// busy wait
	volatile uint32_t mm2s_sr = _reg_get(axi_mcdma_v_addr, AXI_MCDMA_MM2S_CH1SR);
	while (!(mm2s_sr & (1 << AXI_MCDMA_MM2S_IDLE))) {
	// while (!(mm2s_sr)) {
		mm2s_sr = _reg_get(axi_mcdma_v_addr, AXI_MCDMA_MM2S_CH1SR);
	}

	fprintf(stderr, "mm2s: Transfer completed\n");

    /*** Retrive MCDMA s2mm ***/
    // Enable Channels
    _reg_set(axi_mcdma_v_addr, AXI_MCDMA_S2MM_CHEN, 0x1);
    fprintf(stderr, "s2mm: Enabled channel 0. Disabled all other channels.\n");

    _reg_set(axi_mcdma_v_addr, AXI_MCDMA_S2MM_CH1CURDESC_LSB, AXI_MCDMA_BUF_DST_ADDR);
    fprintf(stderr, "s2mm: Set channel 0 bd to src address: 00x%08x\n", AXI_MCDMA_BUF_DST_ADDR);

    // Porgram the CHANNEL.fetch bit
    _reg_set(axi_mcdma_v_addr, AXI_MCDMA_S2MM_CH1CR, AXI_MCDMA_MM2S_RS);
    fprintf(stderr, "s2mm: Enabled channel 0 bd fetch.\n");

    // Start MCDMA (set MCDMA.RS bit)
    _reg_set(axi_mcdma_v_addr, AXI_MCDMA_S2MM_CCR, AXI_MCDMA_MM2S_RS);
    fprintf(stderr, "s2mm: Started axi mcdma with RS bit.\n");

    // Porgram interrupt thresholds (enable interrupt)

    // Porgram the TD register of channels
    _reg_set(axi_mcdma_v_addr, AXI_MCDMA_S2MM_CH1TAILDESC_LSB, (uint32_t) (AXI_MCDMA_BUF_DST_ADDR + (payload_len * sizeof(uint32_t))));
    fprintf(stderr, "s2mm: Set channel 0 bd tail to tail adddress: 00x%08x\n", (uint32_t) (AXI_MCDMA_BUF_DST_ADDR + (payload_len * sizeof(uint32_t))));

    close(dev_fd);
    
    return 0;
}
