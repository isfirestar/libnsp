#include "ipc.h"

#include "posix_ifos.h"

#if _WIN32
#else

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

posix__shmid_t posix__shmget(const char *filename, const int size)
{
	char filepath[256];
	file_descriptor_t fd;
	key_t shmkey;

	if (!filename || size <= 0 || 0 != (size % PAGE_SIZE)) {
		return -1;
	}

	sprintf(filepath, "/dev/shm/%s", filename);
	if (posix__file_open(filepath, FF_RDACCESS | FF_OPEN_EXISTING, 0600, &fd) < 0) {
		if ( posix__file_open(filepath, FF_RDACCESS | FF_CREATE_NEWONE, 0600, &fd) < 0) {
			return -1;
		}
	}

	/* afterward, the file object are meaningless */
	posix__file_close(fd);

	shmkey = ftok(filepath, ';');
	if (shmkey < 0) {
		return -1;
	}

	return shmget(shmkey, size, SHM_HUGETLB);;
}

void *posix__shmat(const posix__shmid_t shmid)
{
	void *ptr;

	if (INVALID_SHMID == shmid) {
		return NULL;
	}

	ptr = shmat(shmid, NULL, 0);
	if ((void *)-1 == ptr) {
		return NULL;
	}
	return ptr;
}

int posix__shmdt(const void *ptr)
{
	return shmdt(ptr);
}

int posix__shmrm(const posix__shmid_t shmid)
{
	struct shmid_ds shmds;
	return shmctl(shmid, IPC_RMID, &shmds);
}

#endif
