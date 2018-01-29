/* -march=i686 */

#ifndef POSIX_ATOMIC_H
#define POSIX_ATOMIC_H

#if _WIN32

#include <Windows.h>

#define posix__atomic_inc(ptr)                  InterlockedIncrement(( LONG volatile *)ptr)
#define posix__atomic_dec(ptr)                  InterlockedDecrement(( LONG volatile *) ptr)
#define posix__atomic_xchange(ptr, val)       InterlockedExchange(( LONG volatile *) ptr, (LONG)val)
#define posix__atomic_compare_xchange(ptr, oldval,  newval) InterlockedCompareExchange( ( LONG volatile *)ptr, (LONG)newval, (LONG)oldval )
#define posix__atomic_ptr_xchange(ptr, val)     InterlockedExchangePointer((PVOID volatile* )tar, (PVOID)src)
#define posix__atomic_compare_ptr_xchange(ptr, oldptr, newptr) InterlockedCompareExchangePointer((PVOID volatile*)ptr, (PVOID)newptr, (PVOID)oldptr)

#else

#define posix__atomic_inc(ptr)                  __sync_add_and_fetch(ptr, 1)
#define posix__atomic_dec(ptr)                  __sync_sub_and_fetch(ptr, 1)
#define posix__atomic_xchange(ptr, val)       __sync_lock_test_and_set(ptr, val)
#define posix__atomic_compare_xchange(ptr, oldval,  newval)   __sync_val_compare_and_swap(ptr, oldval, newval )
#define posix__atomic_ptr_xchange(ptr, val)     __sync_lock_test_and_set(ptr, val)
#define posix__atomic_compare_ptr_xchange(ptr, oldptr, newptr) __sync_val_compare_and_swap(ptr, oldptr, newptr )

/*
 * type __sync_lock_test_and_set (type *ptr, type value, ...)
 *          行为: *ptr = value, 返回 *ptr交换前的值
 * 
 * bool __sync_bool_compare_and_swap (type*ptr, type oldval, type newval, ...)
 *          行为: 如果 (*ptr == oldval) 则 *ptr = newval, 返回1
 *                否则 返回 0, ptr/ *ptr不变
 * 
 * type __sync_val_compare_and_swap (type *ptr, type oldval,  type newval, ...)
 *          行为: 如果 (*ptr == oldval) 则 *ptr = newval, 返回 *ptr 交换前的值
 *                否则 返回 *ptr, ptr/ *ptr不变
 * 
 * void __sync_lock_release (type *ptr, ...)
 *          行为: *ptr = 0
 *  */

#endif

#endif /* POSIX_ATOMIC_H */

