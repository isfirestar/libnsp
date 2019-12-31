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

struct ckfifo* ring_buffer_init(void *buffer, uint32_t size);
void ring_buffer_free(struct ckfifo *ckfifo_ring_buffer);
uint32_t ring_buffer_len(const struct ckfifo *ckfifo_ring_buffer);
uint32_t ring_buffer_get(struct ckfifo *ckfifo_ring_buffer, void *buffer, uint32_t size);
uint32_t ring_buffer_put(struct ckfifo *ckfifo_ring_buffer, const void *buffer, uint32_t size);


#endif
