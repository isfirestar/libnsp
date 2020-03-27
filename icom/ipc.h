/*
 * the integration of portable inter-process comunications(IPC)
 * neo.anderson 2020-3-27
 */

#if !defined POSIX_IPC_H
	#define POSIX_IPC_H
#endif

#include "compiler.h"

#if _WIN32
	typedef HANDLE posix__shmid_t;
	#define INVALID_SHMID	((posix__shmid_t)INVALID_HANDLE_VALUE)
#else
	typedef int posix__shmid_t;
	#define INVALID_SHMID	((posix__shmid_t)-1)
#endif

__extern__
posix__shmid_t posix__shmget(const char *filename, const int size);
__extern__
void *posix__shmat(const posix__shmid_t shmid);
__extern__
int posix__shmdt(const void *ptr);
__extern__
int posix__shmrm(const posix__shmid_t shmid);
