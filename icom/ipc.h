/*
 * the integration of portable inter-process comunications(IPC)
 * neo.anderson 2020-3-27
 */

#if !defined POSIX_IPC_H
	#define POSIX_IPC_H
#endif

#include "compiler.h"

__extern__
void *posix__shmct(const char *filename, const int size);
__extern__
void *posix__shmop(const char *filename);
__extern__
void *posix__shmat(const void *shmp);
__extern__
int posix__shmdt(const void *shmp);
__extern__
int posix__shmrm(void *shmp);
