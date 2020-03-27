/*
 * the integration of portable inter-process comunications(IPC)
 * neo.anderson 2020-3-27
 */

#if !defined POSIX_IPC_H
	#define POSIX_IPC_H
#endif

#include "compiler.h"

/*
 *	@posix__shmmk implement use to create a new shared memory segment.
 *	@filename, pass a null-terminated string relate to the name of shared memory segment
 *				in POSIX platform, "/dev/shm/filename" file are going to open or create.
 *				in WIN32 platform, "Global\filename" section are going to create.
 *	@size: bytes of this shared memory segment acquirement. high 32 bits will be ignored
 *  @return: on success, the shm manager object are returned, it can be use to other calls like: posix__shmat/posix__shmdt/posix__shmrm/posix__shmcb,
 *			otherwise, NULL returned.
 *	@remark: if the kernel object shared memory segment associated with @filename are existed, @posix__shmmk call will failure.
 *				in POSIX platform, use ipcs(1) to see the detail informations of all msg/shm/sem objects.
 *				in WIN32 platform, use "Process Monitor" to see the detail informations of all kernel sections.
 *				after success called, @posix__shmcb can be effect use to get the size of shared memory segment
 */
__extern__ void *posix__shmmk(const char *filename, const int size);

/*
 * @posix__shmop implement use to open a existing shared memory segment
 * @filename, pass a null-terminated string relate to the name of shared memory segment
 *				in POSIX platform, "/dev/shm/filename" file are going to open
 *				in WIN32 platform, "Global\filename" section are going to open.
 *	@return: on success, the shm manager object are returned, it can be use to other calls like: posix__shmat/posix__shmdt/posix__shmrm/posix__shmcb,
 *			otherwise, NULL returned.
 * @remakr: if the kernel object shared memory segment associated with @filename are NOT existed, @posix__shmmk call will failure.
 *			after success called, @posix__shmcb can be effect use to get the size of shared memory segment
 */
__extern__ void *posix__shmop(const char *filename);

/*
 *	@posix__shmat attach virtual memory to a shard memory segment
 *	@shmp: the shm object return by @posix__shmmk or @posix__shmop
 *	@return: on success, the attached virtual in process space are returned. otherwise, NULL returned.
 */
__extern__ void *posix__shmat(const void *shmp);

/*
 * 
 */
__extern__ int posix__shmdt(const void *shmp);
__extern__ int posix__shmrm(void *shmp);
__extern__ int posix__shmcb(const void *shmp);
