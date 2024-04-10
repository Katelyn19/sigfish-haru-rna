// #include "fifo_test.h"
// #include "../include/axi_mcdma_fifo_test.h"
#include "mcdma_test.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Default payload length: 10. Otherwise, please enter payload length.");
        mcdma_test(10);
    } else {
        int payload_length = atoi(argv[1]);
        mcdma_test(payload_length);
    }
}