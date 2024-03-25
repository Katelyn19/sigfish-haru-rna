/*
author: Katelyn Mak
date created: 12/3/2024

This program loads a random payload onto a Multi-Channel AXI DMA (MCDMA) project and verifies the return values.
Currently, only one channel is used for the MCDMA.

*/

#include "../include/misc.h"
#include "../include/haru.h"

#include <stdio.h>
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

    // Initialise MCDMA
    // Enable Channels

    // Map Cursor Descriptor

    // program the CHANNEL.fetch bit?
}

void main(void) {
    axi_mcdma_fifo_test(10);
}