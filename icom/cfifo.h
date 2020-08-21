#if !defined LIB_CKFIFO_H
#define LIB_CKFIFO_H

#include <stdint.h>

#include "compiler.h"

struct ckfifo {
    unsigned char  *buffer;
    uint32_t     	size;
    uint32_t     	in;
    uint32_t       	out;
    void 		   *spin_lock;
};

PORTABLEAPI(struct ckfifo*) ckfifo_init(void *buffer, uint32_t size);
PORTABLEAPI(void) ckfifo_uninit(struct ckfifo *ckfifo_ring_buffer);
PORTABLEAPI(uint32_t) ckfifo_len(const struct ckfifo *ckfifo_ring_buffer);
PORTABLEAPI(uint32_t) ckfifo_get(struct ckfifo *ckfifo_ring_buffer, void *buffer, uint32_t size);
PORTABLEAPI(uint32_t) ckfifo_put(struct ckfifo *ckfifo_ring_buffer, const void *buffer, uint32_t size);

#endif
