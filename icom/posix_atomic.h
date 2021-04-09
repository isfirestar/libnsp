/* -march=i686 */

#ifndef POSIX_ATOMIC_H
#define POSIX_ATOMIC_H

#if _WIN32

#include <Windows.h>

#define posix__atomic_get(ptr)					InterlockedExchangeAdd((volatile LONG *)ptr, 0)
#define posix__atomic_get64(ptr)					InterlockedExchangeAdd64((volatile LONG64 *)ptr, 0)
#define posix__atomic_set(ptr, value)       InterlockedExchange(( LONG volatile *)ptr, (LONG)value)
#define posix__atomic_set64(ptr, value)       InterlockedExchange64(( LONG64 volatile *)ptr, (LONG64)value)
#define posix__atomic_inc(ptr)                  InterlockedIncrement(( LONG volatile *)ptr)
#define posix__atomic_inc64(ptr)                InterlockedIncrement64(( LONG64 volatile *)ptr)
#define posix__atomic_dec(ptr)                  InterlockedDecrement(( LONG volatile *) ptr)
#define posix__atomic_dec64(ptr)                 InterlockedDecrement64(( LONG64 volatile *) ptr)
#define posix__atomic_xchange(ptr, val)       InterlockedExchange(( LONG volatile *) ptr, (LONG)val)
#define posix__atomic_xchange64(ptr, val)       InterlockedExchange64(( LONG64 volatile *) ptr, (LONG64)val)
#define posix__atomic_compare_xchange(ptr, oldval,  newval) InterlockedCompareExchange( ( LONG volatile *)ptr, (LONG)newval, (LONG)oldval )
#define posix__atomic_compare_xchange64(ptr, oldval,  newval) InterlockedCompareExchange64( ( LONG64 volatile *)ptr, (LONG64)newval, (LONG64)oldval )
#define posix__atomic_ptr_xchange(ptr, val)     InterlockedExchangePointer((PVOID volatile* )tar, (PVOID)src)
#define posix__atomic_compare_ptr_xchange(ptr, oldptr, newptr) InterlockedCompareExchangePointer((PVOID volatile*)ptr, (PVOID)newptr, (PVOID)oldptr)

#else /* POSIX */

#define posix__atomic_get(ptr)					__atomic_load_n(ptr, __ATOMIC_ACQUIRE)
#define posix__atomic_get64(ptr)				__atomic_load_n(ptr, __ATOMIC_ACQUIRE)
#define posix__atomic_set(ptr,value) 			__atomic_store_n(ptr, value, __ATOMIC_RELAXED)
#define posix__atomic_set64(ptr,value) 			__atomic_store_n(ptr, value, __ATOMIC_RELAXED)
#define posix__atomic_inc(ptr)                  __sync_add_and_fetch(ptr, 1)
#define posix__atomic_inc64(ptr)                  __sync_add_and_fetch(ptr, 1)
#define posix__atomic_dec(ptr)                  __sync_sub_and_fetch(ptr, 1)
#define posix__atomic_dec64(ptr)                  __sync_sub_and_fetch(ptr, 1)
#define posix__atomic_xchange(ptr, val)       __sync_lock_test_and_set(ptr, val)
#define posix__atomic_xchange64(ptr, val)       __sync_lock_test_and_set(ptr, val)
#define posix__atomic_compare_xchange(ptr, oldval,  newval)   __sync_val_compare_and_swap(ptr, oldval, newval )
#define posix__atomic_compare_xchange64(ptr, oldval,  newval)   __sync_val_compare_and_swap(ptr, oldval, newval )
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

#endif /* end POSIX */

#define POSIX__ATOMIC_INIT_FAILED		((long)-2L)
#define POSIX__ATOMIC_INIT_RUNNING		((long)-1L)
#define POSIX__ATOMIC_INIT_TODO			((long)0L)
#define POSIX__ATOMIC_INIT_SUCCESS		((long)1L)

#define posix__atomic_initial_declare_variable(initial_variable_name)	static long (initial_variable_name) = POSIX__ATOMIC_INIT_TODO
/*
 * if return value is POSIX__ATOMIC_INIT_TODO, means need initial now
 * if return value is POSIX__ATOMIC_INIT_RUNNING, means initial operation is in progress
 * if return value is POSIX__ATOMIC_INIT_SUCCESS, means initial has been completed
 * if return value is POSIX__ATOMIC_INIT_FAILED, means initial has been completed, but some error happended in progress
 */
#define posix__atomic_initial_test(initial_variable_pointer) \
	posix__atomic_compare_xchange((volatile long *)initial_variable_pointer, POSIX__ATOMIC_INIT_TODO, POSIX__ATOMIC_INIT_RUNNING)

#define posix__atomic_initial_try(initial_variable_pointer) \
	(POSIX__ATOMIC_INIT_TODO == posix__atomic_initial_test(initial_variable_pointer))

#define posix__atomic_initial_complete(initial_variable_pointer)	\
	posix__atomic_compare_xchange((volatile long *)initial_variable_pointer, POSIX__ATOMIC_INIT_RUNNING, POSIX__ATOMIC_INIT_SUCCESS)

#define posix__atomic_initial_exception(initial_variable_pointer) \
	posix__atomic_compare_xchange((volatile long *)initial_variable_pointer, POSIX__ATOMIC_INIT_RUNNING, POSIX__ATOMIC_INIT_FAILED)

#define posix__atomic_initial_rtest(initial_variable_pointer) \
	posix__atomic_compare_xchange((volatile long *)initial_variable_pointer, POSIX__ATOMIC_INIT_SUCCESS, POSIX__ATOMIC_INIT_TODO)

#define posix__atomic_initial_regress(initial_variable_pointer) \
	(POSIX__ATOMIC_INIT_SUCCESS == posix__atomic_initial_rtest(initial_variable_pointer))

#define posix__atomic_initial_passed(initial_variable) \
	(POSIX__ATOMIC_INIT_SUCCESS == initial_variable)

#define posix__atomic_initial_awaiting(initial_variable) \
	(POSIX__ATOMIC_INIT_TODO == initial_variable)

#endif /* POSIX_ATOMIC_H */

