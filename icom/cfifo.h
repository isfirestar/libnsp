#ifndef KFIFO_HEADER_H
#define KFIFO_HEADER_H

#include "compiler.h"

struct ckfifo {
    void           *buffer;
    uint32_t     	size;
    uint32_t     	in;
    uint32_t       	out;
    void 		   *spin_lock;
};

struct ckfifo* ckfifo_init(void *buffer, uint32_t size);
void ckfifo_uninit(struct ckfifo *ckfifo_ring_buffer);
uint32_t ckfifo_len(const struct ckfifo *ckfifo_ring_buffer);
uint32_t ckfifo_get(struct ckfifo *ckfifo_ring_buffer, void *buffer, uint32_t size);
uint32_t ckfifo_put(struct ckfifo *ckfifo_ring_buffer, const void *buffer, uint32_t size);


#endif
