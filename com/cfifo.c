#include "posix_thread.h"
#include "cfifo.h"

struct ckfifo* ckfifo_init(void *buffer, uint32_t size)
{
    struct ckfifo *ckfifo_ring_buffer;

    assert(buffer);
    if (!is_powerof_2(size)) {
        return NULL;
    }

    ckfifo_ring_buffer = (struct ckfifo *)malloc(sizeof(struct ckfifo));
    if (!ckfifo_ring_buffer) {
        return NULL;
    }

    memset(ckfifo_ring_buffer, 0, sizeof(struct ckfifo));
    ckfifo_ring_buffer->buffer = buffer;
    ckfifo_ring_buffer->size = size;
    ckfifo_ring_buffer->in = 0;
    ckfifo_ring_buffer->out = 0;
    ckfifo_ring_buffer->spin_lock = malloc(sizeof(posix__pthread_mutex_t));
    if (!ckfifo_ring_buffer->spin_lock) {
        free(ckfifo_ring_buffer);
        return NULL;
    }
    posix__pthread_mutex_init(ckfifo_ring_buffer->spin_lock);
    return ckfifo_ring_buffer;
}

void ckfifo_uninit(struct ckfifo *ckfifo_ring_buffer)
{
    if (ckfifo_ring_buffer) {
        if (ckfifo_ring_buffer->spin_lock) {
            posix__pthread_mutex_release(ckfifo_ring_buffer->spin_lock);
        }
        free(ckfifo_ring_buffer);
    }
}

uint32_t __ckfifo_len(const struct ckfifo *ckfifo_ring_buffer)
{
    return (ckfifo_ring_buffer->in - ckfifo_ring_buffer->out);
}

uint32_t __ckfifo_get(struct ckfifo *ckfifo_ring_buffer, unsigned char *buffer, uint32_t size)
{
    uint32_t len, n;
    assert(ckfifo_ring_buffer && buffer);
    n  = min(size, ckfifo_ring_buffer->in - ckfifo_ring_buffer->out);
    len = min(n, ckfifo_ring_buffer->size - (ckfifo_ring_buffer->out & (ckfifo_ring_buffer->size - 1)));
    memcpy(buffer, ckfifo_ring_buffer->buffer + (ckfifo_ring_buffer->out & (ckfifo_ring_buffer->size - 1)), len);
    memcpy(buffer + len, ckfifo_ring_buffer->buffer, n - len);
    ckfifo_ring_buffer->out += n;
    return n;
}

uint32_t __ckfifo_put(struct ckfifo *ckfifo_ring_buffer, const unsigned char *buffer, uint32_t size)
{
    uint32_t len, n;
    assert(ckfifo_ring_buffer && buffer);
    n = min(size, ckfifo_ring_buffer->size - ckfifo_ring_buffer->in + ckfifo_ring_buffer->out);
    len  = min(n, ckfifo_ring_buffer->size - (ckfifo_ring_buffer->in & (ckfifo_ring_buffer->size - 1)));
    memcpy(ckfifo_ring_buffer->buffer + (ckfifo_ring_buffer->in & (ckfifo_ring_buffer->size - 1)), buffer, len);
    memcpy(ckfifo_ring_buffer->buffer, buffer + len, n - len);
    ckfifo_ring_buffer->in += n;
    return n;
}

uint32_t ckfifo_len(const struct ckfifo *ckfifo_ring_buffer)
{
    uint32_t len;
    posix__pthread_mutex_lock(ckfifo_ring_buffer->spin_lock);
    len = __ckfifo_len(ckfifo_ring_buffer);
    posix__pthread_mutex_unlock(ckfifo_ring_buffer->spin_lock);
    return len;
}

uint32_t ckfifo_get(struct ckfifo *ckfifo_ring_buffer, void *buffer, uint32_t size)
{
    uint32_t n;
    posix__pthread_mutex_lock(ckfifo_ring_buffer->spin_lock);
    n = __ckfifo_get(ckfifo_ring_buffer, buffer, size);
    if (ckfifo_ring_buffer->in == ckfifo_ring_buffer->out) {
        ckfifo_ring_buffer->in = ckfifo_ring_buffer->out = 0;
    }
    posix__pthread_mutex_unlock(ckfifo_ring_buffer->spin_lock);
    return n;
}

uint32_t ckfifo_put(struct ckfifo *ckfifo_ring_buffer, const void *buffer, uint32_t size)
{
    uint32_t n;
    posix__pthread_mutex_lock(ckfifo_ring_buffer->spin_lock);
    n = __ckfifo_put(ckfifo_ring_buffer, buffer, size);
    posix__pthread_mutex_unlock(ckfifo_ring_buffer->spin_lock);
    return n;
}
