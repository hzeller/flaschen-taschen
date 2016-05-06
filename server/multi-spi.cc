// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
// Copyright (C) 2016 Henner Zeller <h.zeller@acm.org>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation version 2.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://gnu.org/licenses/gpl-2.0.txt>

#include "multi-spi.h"

#include <assert.h>
#include <strings.h>
#include <stdlib.h>
#include <string.h>

// mmap-bcm-register
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BCM2708_PI1_PERI_BASE  0x20000000
#define BCM2709_PI2_PERI_BASE  0x3F000000

#define PERI_BASE BCM2709_PI2_PERI_BASE

#define PAGE_SIZE 4096

// ---- GPIO specific defines
#define GPIO_REGISTER_BASE 0x200000
#define GPIO_SET_OFFSET 0x1C
#define GPIO_CLR_OFFSET 0x28
#define PHYSICAL_GPIO_BUS (0x7E000000 + GPIO_REGISTER_BASE)

// ---- DMA specific defines
#define DMA_CHANNEL       5   // That usually is free.
#define DMA_BASE          0x007000

// Return a pointer to a periphery subsystem register.
// TODO: consolidate with gpio file.
static void *mmap_bcm_register(off_t register_offset) {
  const off_t base = PERI_BASE;

  int mem_fd;
  if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
    perror("can't open /dev/mem: ");
    fprintf(stderr, "You need to run this as root!\n");
    return NULL;
  }

  uint32_t *result =
    (uint32_t*) mmap(NULL,                  // Any adddress in our space will do
                     PAGE_SIZE,
                     PROT_READ|PROT_WRITE,  // Enable r/w on GPIO registers.
                     MAP_SHARED,
                     mem_fd,                // File to map
                     base + register_offset // Offset to bcm register
                     );
  close(mem_fd);

  if (result == MAP_FAILED) {
    fprintf(stderr, "mmap error %p\n", result);
    return NULL;
  }
  return result;
}

struct MultiSPI::GPIOData {
    uint32_t set;
    uint32_t ignored_upper_set_bits; // bits 33..54 of GPIO. Not needed.
    uint32_t reserved_area;          // gap between GPIO registers.
    uint32_t clr;
};

MultiSPI::MultiSPI(int clock_gpio)
    : clock_gpio_(clock_gpio), size_(0), gpio_dma_(NULL), gpio_shadow_(NULL) {
    alloced_.mem = NULL;
    bool success = gpio_.Init();
    assert(success);  // gpio couldn't be initialized
    success = gpio_.AddOutput(clock_gpio);
    assert(success);  // clock pin not valid
}

MultiSPI::~MultiSPI() {
    UncachedMemBlock_free(&alloced_);
    free(gpio_shadow_);
}

bool MultiSPI::RegisterDataGPIO(int gpio, size_t serial_byte_size) {
    if (serial_byte_size > size_)
        size_ = serial_byte_size;
    return gpio_.AddOutput(gpio);
}

void MultiSPI::FinishRegistration() {
    assert(alloced_.mem == NULL);  // Registered twice ?
    // One DMA operation can only span a limited amount of range.
    const int kMaxOpsPerBlock = (2<<15) / sizeof(GPIOData);
    const int gpio_operations = size_ * 8 * 2 + 1;
    const int control_blocks
        = (gpio_operations + kMaxOpsPerBlock - 1) / kMaxOpsPerBlock;
    const int alloc_size = (control_blocks * sizeof(struct dma_cb)
                            + gpio_operations * sizeof(GPIOData));
    alloced_ = UncachedMemBlock_alloc(alloc_size);
    gpio_dma_ = (struct GPIOData*) ((uint8_t*)alloced_.mem 
                                    + control_blocks * sizeof(struct dma_cb));

    struct dma_cb* previous = NULL;
    struct dma_cb* cb = NULL;
    struct GPIOData *start_gpio = gpio_dma_;
    int remaining = gpio_operations;
    for (int i = 0; i < control_blocks; ++i) {
        cb = (struct dma_cb*) ((uint8_t*)alloced_.mem + i * sizeof(dma_cb));
        if (previous) {
            previous->next = UncachedMemBlock_to_physical(&alloced_, cb);
        }
        const int n = remaining > kMaxOpsPerBlock ? kMaxOpsPerBlock : remaining;
        cb->info   = (DMA_CB_TI_SRC_INC | DMA_CB_TI_DEST_INC |
                      DMA_CB_TI_NO_WIDE_BURSTS | DMA_CB_TI_TDMODE);
        cb->src    = UncachedMemBlock_to_physical(&alloced_, start_gpio);
        cb->dst    = PHYSICAL_GPIO_BUS + GPIO_SET_OFFSET;
        cb->length = DMA_CB_TXFR_LEN_YLENGTH(n)
            | DMA_CB_TXFR_LEN_XLENGTH(sizeof(GPIOData));
        cb->stride = DMA_CB_STRIDE_D_STRIDE(-16) | DMA_CB_STRIDE_S_STRIDE(0);
        previous = cb;
        start_gpio += n;
        remaining -= n;
    }
    cb->next = 0;

    // First block in our chain.
    start_block_ = (struct dma_cb*) alloced_.mem;

    // --- All the memory operations we do in cached memory as direct operations
    // on the uncached memory (which is used by the DMA operation) is pretty
    // slow - and we need multiple read/write operations.
    gpio_shadow_ = (struct GPIOData*)calloc(gpio_operations, sizeof(GPIOData));
    gpio_copy_size_ = gpio_operations * sizeof(GPIOData);

    // Now, we can already prepare every other element to set the CLK pin
    for (int i = 0; i < gpio_operations; ++i) {
        if (i % 2 == 0)
            gpio_shadow_[i].clr = (1<<clock_gpio_);
        else
            gpio_shadow_[i].set = (1<<clock_gpio_);
    }

    // 4.2.1.2
    char *dmaBase = (char*)mmap_bcm_register(DMA_BASE);
    dma_channel_ = (struct dma_channel_header*)(dmaBase + 0x100 * DMA_CHANNEL);
}

void MultiSPI::SetBufferedByte(int data_gpio, size_t pos, uint8_t data) {
    assert(pos < size_);
    GPIOData *buffer_pos = gpio_shadow_ + 2 * 8 * pos;
    for (uint8_t bit = 0x80; bit; bit >>= 1, buffer_pos += 2) {
        if (data & bit) {   // set
            buffer_pos->set |= (1 << data_gpio);
            buffer_pos->clr &= ~(1 << data_gpio);
        } else {            // reset
            buffer_pos->set &= ~(1 << data_gpio);
            buffer_pos->clr |= (1 << data_gpio);
        }
    }
}

void MultiSPI::SendBuffers() {
    memcpy(gpio_dma_, gpio_shadow_, gpio_copy_size_);

    dma_channel_->cs |= DMA_CS_END;
    dma_channel_->cblock = UncachedMemBlock_to_physical(&alloced_, start_block_);
    dma_channel_->cs = DMA_CS_PRIORITY(7) | DMA_CS_PANIC_PRIORITY(7) | DMA_CS_DISDEBUG;
    dma_channel_->cs |= DMA_CS_ACTIVE;
    while ((dma_channel_->cs & DMA_CS_ACTIVE)
           && !(dma_channel_->cs & DMA_CS_ERROR)) {
        usleep(10);
    }

    dma_channel_->cs |= DMA_CS_ABORT;
    usleep(100);
    dma_channel_->cs &= ~DMA_CS_ACTIVE;
    dma_channel_->cs |= DMA_CS_RESET;
}
