// -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; -*-
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

#include "rpi-dma.h"

#include "mailbox.h"

#include <assert.h>
#include <string.h>

#define PAGE_SIZE 4096

// ---- Memory mappping defines
#define BUS_TO_PHYS(x) ((x)&~0xC0000000)

// ---- Memory allocating defines
// https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
#define MEM_FLAG_DIRECT           (1 << 2)
#define MEM_FLAG_COHERENT         (2 << 2)
#define MEM_FLAG_L1_NONALLOCATING (MEM_FLAG_DIRECT | MEM_FLAG_COHERENT)

static int mbox_fd = -1;   // used internally by the UncachedMemBlock-functions.

// Allocate a block of memory of the given size (which is rounded up to the next
// full page). The memory will be aligned on a page boundary and zeroed out.
struct UncachedMemBlock UncachedMemBlock_alloc(size_t size) {
    if (mbox_fd < 0) {
        mbox_fd = mbox_open();
        assert(mbox_fd >= 0);  // Uh, /dev/vcio not there ?
    }
    // Round up to next full page.
    size = size % PAGE_SIZE == 0 ? size : (size + PAGE_SIZE) & ~(PAGE_SIZE - 1);

    struct UncachedMemBlock result;
    result.size = size;
    result.mem_handle = mem_alloc(mbox_fd, size, PAGE_SIZE,
                                  MEM_FLAG_L1_NONALLOCATING);
    result.bus_addr = mem_lock(mbox_fd, result.mem_handle);
    result.mem = mapmem(BUS_TO_PHYS(result.bus_addr), size);
    assert(result.bus_addr);  // otherwise: couldn't allocate contiguous block.
    memset(result.mem, 0x00, size);

    return result;
}

// Free block previously allocated with UncachedMemBlock_alloc()
void UncachedMemBlock_free(struct UncachedMemBlock *block) {
    if (block->mem == NULL) return;
    assert(mbox_fd >= 0);  // someone should've initialized that on allocate.
    unmapmem(block->mem, block->size);
    mem_unlock(mbox_fd, block->mem_handle);
    mem_free(mbox_fd, block->mem_handle);
    block->mem = NULL;
}


// Given a pointer to memory that is in the allocated block, return the
// physical bus addresse needed by DMA operations.
uintptr_t UncachedMemBlock_to_physical(const struct UncachedMemBlock *blk,
				       void *p) {
    uint32_t offset = (uint8_t*)p - (uint8_t*)blk->mem;
    assert(offset < blk->size);   // pointer not within our block.
    return blk->bus_addr + offset;
}
