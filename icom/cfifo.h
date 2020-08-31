#if !defined LIB_C_KFIFO_H
#define LIB_C_KFIFO_H

#include "compiler.h"

struct ckfifo {
    unsigned char  *buffer;
    uint32_t     	size;
    uint32_t     	in;
    uint32_t       	out;
    void 		   *spin_lock;
};

__interface__ struct ckfifo* ckfifo_init(void *buffer, uint32_t size);
__interface__ void ckfifo_uninit(struct ckfifo *ckfifo_ring_buffer);
__interface__ uint32_t ckfifo_len(const struct ckfifo *ckfifo_ring_buffer);
__interface__ uint32_t ckfifo_get(struct ckfifo *ckfifo_ring_buffer, void *buffer, uint32_t size);
__interface__ uint32_t ckfifo_put(struct ckfifo *ckfifo_ring_buffer, const void *buffer, uint32_t size);


#endif
