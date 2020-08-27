#ifndef POSIX_WAIT_H
#define POSIX_WAIT_H

#include "posix_thread.h"

#if _WIN32

struct __waitable_handle {
    int sync_; /* as boolean check */
    HANDLE cond_;
};

#else

struct __waitable_handle {
    int sync_; /* as boolean check */
    pthread_cond_t cond_;
    int pass_;
    posix__pthread_mutex_t mutex_;
    int effective;
}__POSIX_TYPE_ALIGNED__;

#endif

typedef struct __waitable_handle posix__waitable_handle_t;

/* initialize wait object @waiter, the calling thread have responsibility to guarantee pointer @waiter aligned to 4 bytes
*/
PORTABLEAPI(int) posix__init_synchronous_waitable_handle(posix__waitable_handle_t *waiter);
PORTABLEAPI(int) posix__init_notification_waitable_handle(posix__waitable_handle_t *waiter);

/* assign a wait object and than initialize it, store and retun by @waiter,
    object pointer assigned by success call MUST explicit call @posix__release_waitable_handle to free it's memory
*/
PORTABLEAPI(int) posix__allocate_synchronous_waitable_handle(posix__waitable_handle_t **waiter);
PORTABLEAPI(int) posix__allocate_notification_waitable_handle(posix__waitable_handle_t **waiter);

/* uninitialize wait object either synchronous or nitifications */
PORTABLEAPI(void) posix__uninit_waitable_handle(posix__waitable_handle_t *waiter);
/* uninitialize and release wait object which assigned by above two allocation interface */
PORTABLEAPI(void) posix__release_waitable_handle(posix__waitable_handle_t *waiter);

/* hang-up calling thread until synchronous event trigger or @timeout condition meet
 * @waiter : the wait object assign by posix__allocate_synchronous_waitable_handle/posix__allocate_notification_waitable_handle
 *           or local variable initialized by posix__init_synchronous_waitable_handle/posix__init_notification_waitable_handle
 * @interval : the interval in milliseconds for wait timeout
 *
 * definition of return value :
 *  in case of (interval <= 0):
 *  0 : the event occured
 *  -1: syscall failed.
 *
 * in case of (interval > 0):
 * 0: the event occured
 * ETIMEOUT: wait timeout
 * -1: syscall failed.
 */
#if !defined INFINITE
#define INFINITE (0xFFFFFFFF)
#endif
PORTABLEAPI(int) posix__waitfor_waitable_handle(posix__waitable_handle_t *waiter, int interval/*ms*/);

/* awaken the waitting thread or threads specified by @waiter
 */
PORTABLEAPI(int) posix__sig_waitable_handle(posix__waitable_handle_t *waiter);

/* posix__block_waitable_handle/posix__reset_waitable_handle use to mark the waiter-object to block status,this only effective on notification-object.
 * inner codes didn't examine the waiter-obejct category, the behavior are unspecified when calling thread invoke these functions with synchronous-object
 */
PORTABLEAPI(void) posix__block_waitable_handle(posix__waitable_handle_t *waiter);
PORTABLEAPI(void) posix__reset_waitable_handle(posix__waitable_handle_t *waiter);

#define DECLARE_SYNC_WAITER(name)   \
    struct __waitable_handle name; \
    posix__init_synchronous_waitable_handle(&name)

#define DECLARE_NOTI_WATIER(name) \
    struct __waitable_handle name; \
    posix__init_notification_waitable_handle(&name)

/* hang up calling thread, make it upon a dead-wait status. */
PORTABLEAPI(void) posix__hang();

/* High precision delay implementation, in microseconds */
PORTABLEAPI(int) posix__delay_execution(uint64_t us);

#endif /* POSIX_WAIT_H */

