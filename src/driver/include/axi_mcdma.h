/*
author: Katelyn Mak

This header file defines the register addresses and values to control the MCDMA.

*/
#ifndef AXI_MCDMA
#define AXI_MCDMA

#include <stdint.h>

/*
    AXI MCDMA Buffer Address Space
*/
#define AXI_MCDMA_BUF_ADDR_BASE                     0xa0000000
#define AXI_MCDMA_BUF_SIZE                          0xffff
#define AXI_MCDMA_BUF_SRC_ADDR                      0x10000000
#define AXI_MCDMA_BUF_DST_ADDR                      0x20000000

#define AXI_MCDMA_BUF_INIT_ERROR     0x01

/*
    AXI Multichannel DMA MM2S
*/
// MM2S Address Space
#define AXI_MCDMA_MM2S_CCR                          0x000   // Common Control Register
#define AXI_MCDMA_MM2S_CSR                          0x004   // Common Status Register
#define AXI_MCDMA_MM2S_CHEN                         0x008   // Channel Enable/Disable
#define AXI_MCDMA_MM2S_CHSER                        0x00C   // Channel in Progress REgister
#define AXI_MCDMA_MM2S_ERR                          0x010   // Error Register
#define AXI_MCDMA_MM2S_CH_SCHD_TYPE                 0x014   // Channel Queue Scheduler type
#define AXI_MCDMA_MM2S_WWR_REG1                     0x018   // Weight of each channel (CH1-8)
#define AXI_MCDMA_MM2S_WWR_REG2                     0x01C   // Weight of each channel (CH9-16)
#define AXI_MCDMA_MM2S_CHANNELS_SERVICE             0x020   // MM2S Channels Completed Register
#define AXI_MCDMA_MM2S_ARCACHE_ARUSER               0x024   // Set the ARCHACHE and ARUSER values for AXI4 read
#define AXI_MCDMA_MM2S_INTR_STATUS                  0x028   // MM2S Channel Interrupt Monitor Register
#define AXI_MCDMA_MM2S_CH1CR                        0x040   // CH1 Control Register

// MM2S Channel 1 Address Space
#define AXI_MCDMA_MM2S_CH1SR                        0x044   // CH1 Status Register
#define AXI_MCDMA_MM2S_CH1CURDESC_LSB               0x048   // CH1 Current Descriptor (LSB)
#define AXI_MCDMA_MM2S_CH1CURDESC_MSB               0x04C   // CH1 Current Descriptor (MSB)
#define AXI_MCDMA_MM2S_CH1TAILDESC_LSB              0x050   // CH1 Tail Descriptor (LSB)
#define AXI_MCDMA_MM2S_CH1TAILDESC_MSB              0x054   // CH1 Tail Descriptor (MSB)
#define AXI_MCDMA_MM2S_CH1PKTCOUNT_STAT             0x058   // CH1 Packet Processed count

// MM2S Common Control Register
#define AXI_MCDMA_MM2S_RS                           0x001   // Run = 1, Stop = 0
#define AXI_MCDMA_MM2S_RESET                        0x004   // Reset in progress = 1, Reset not in progress = 0

// MM2S Common Status Register
#define AXI_MCDMA_MM2S_HALTED                       0x000   // Halted = 1, Running = 0
#define AXI_MCDMA_MM2S_IDLE                         0x004   // Idle = = 1, Not Idle = 0

// MM2S Error Register
#define AXI_MCDMA_MM2S_DMA_INTR_ERR                 0x00    // MCDMA Internal Error
#define AXI_MCDMA_MM2S_DMA_SLV_ERR                  0x02    // MCDMA Slave Error
#define AXI_MCDMA_MM2S_DMA_DEC_ERR                  0x04    // MCDMA Decode Error
#define AXI_MCDMA_MM2S_SG_INT_ERR                   0x08    // Scatter gather Internal Error
#define AXI_MCDMA_MM2S_SG_SLV_ERR                   0x10    // Scatter gather Slave Error
#define AXI_MCDMA_MM2S_SG_DEC_ERR                   0x20    // Scatter gather Decode Error

/*
    AXI Multichannel DMA S2MM
*/
// S2MM Address Space
#define AXI_MCDMA_S2MM_CCR                          0x500   // Common Control Register
#define AXI_MCDMA_S2MM_CSR                          0x504   // Common Status Register
#define AXI_MCDMA_S2MM_CHEN                         0x508   // Channel Enable/Disable
#define AXI_MCDMA_S2MM_CHSER                        0x50C   // Channel in Progress REgister
#define AXI_MCDMA_S2MM_ERR                          0x510   // Error Register
#define AXI_MCDMA_S2MM_PKTDROP                      0x514   // S2MM Packet Drop Stat
#define AXI_MCDMA_S2MM_CHANNELS_SERVICE             0x518   // S2MM Channels Completed Register
#define AXI_MCDMA_S2MM_AWCACHE_AWUSER               0x51C   // Set the values for awcache and awuser ports
#define AXI_MCDMA_S2MM_INTR_STATUS                  0x520   // S2MM Channel Interrupt Monitor Register

// S2MM Channel 1 Address Space
#define AXI_MCDMA_S2MM_CH1CR                        0x540   // CH1 Status Register
#define AXI_MCDMA_S2MM_CH1SR                        0x544   // CH1 Status Register
#define AXI_MCDMA_S2MM_CH1CURDESC_LSB               0x548   // CH1 Current Descriptor (LSB)
#define AXI_MCDMA_S2MM_CH1CURDESC_MSB               0x54C   // CH1 Current Descriptor (MSB)
#define AXI_MCDMA_S2MM_CH1TAILDESC_LSB              0x550   // CH1 Tail Descriptor (LSB)
#define AXI_MCDMA_S2MM_CH1TAILDESC_MSB              0x554   // CH1 Tail Descriptor (MSB)
#define AXI_MCDMA_S2MM_CH1PKTDROP_STAT              0x558   // CH1 Packet Drop Stat
#define AXI_MCDMA_S2MM_CH1PKTCOUNT_STAT             0x55C   // CH1 Packet Processed count

#endif