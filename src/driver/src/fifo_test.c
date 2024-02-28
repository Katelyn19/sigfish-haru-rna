/*
author: Katelyn Mak
date: 28/2/2024

This file loads a randomly generated payload onto the axi dma and expects it to be returned exactly the same.

*/
#include "../include/misc.h"

#include <stdio.h> // todo: remove this once debugging is finished
#include <stdlib.h>
#include <fcntl.h>

int fifo_test() {
	printf("initialising axi_dma.\n");

	uint32_t ret;

	uint16_t payload[100];
	
	// create random payload
	for (int i = 0; i < 100; i++) {
		payload[i] = rand();
		printf("%d: %d\n", i, payload[i]);
	}

	// initialise the axi dma - allocate space in mem for the axi to read/write from

	// Open /dev/mem for memory mapping
	int32_t dev_fd = open("/dev/mem", O_RDWR | O_SYNC);
	if (dev_fd < 0) {
		return -1;
	}

	// initialise the fifo_interconnect - allocate space in mem for the fifo_interconnect to read from
	// clear the fifo_interconnect
	// set the axi dma stream src address
	// set the axi dma stream length
	// start the axi dma transfer
	// check the loaded data is correct
}


int main(int argc, char *argv[]) {
	fifo_test();
}