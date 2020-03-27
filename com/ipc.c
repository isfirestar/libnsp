#include "ipc.h"

#include "posix_ifos.h"

struct posix_shared_memory {
	file_descriptor_t fd;
	int size;
	char filename[256];
	void *vm;
};

#if _WIN32

#include <Windows.h>

void *posix__shmct(const char *filename, const int size)
{
	struct posix_shared_memory *shm;

	if (!filename || size <= 0 || 0 != (size % PAGE_SIZE)) {
		return NULL;
	}

	shm = (struct posix_shared_memory *)malloc(sizeof(struct posix_shared_memory));
	if (!shm) {
		return NULL;
	}
	memset(shm, 0, sizeof(struct posix_shared_memory));

	sprintf(shm->filename, "\\\\.\\GLOBAL\\%s", filename);

	shm->fd = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, size, shm->filename);
	if (!shm->fd) {
		free(shm);
		return NULL;
	}

	if (ERROR_ALREADY_EXISTS == GetLastError()) {
		CloseHandle(shm->fd);
		free(shm);
		return NULL;
	}

	shm->size = size;
	return shm;
}

void *posix__shmop(const char *filename)
{
	struct posix_shared_memory *shm;

	if (!filename) {
		return NULL;
	}

	shm = (struct posix_shared_memory *)malloc(sizeof(struct posix_shared_memory));
	if (!shm) {
		return NULL;
	}
	memset(shm, 0, sizeof(struct posix_shared_memory));

	shm->fd = (PAGE_READWRITE, FALSE, shm->filename);
}

void *posix__shmat(const void *shmp)
{
	struct posix_shared_memory *shm;

	shm = (struct posix_shared_memory *)shmp;

	shm->vm = MapViewOfFile(shm->fd, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, shm->size);
	return shm->vm;
}

int posix__shmdt(const void *shmp)
{
	struct posix_shared_memory *shm;

	shm = (struct posix_shared_memory *)shmp;

	if (shm) {
		if (UnmapViewOfFile(shm->vm)) {
			shm->vm = NULL;
			return 0;
		}
 	}
	
	return -1;
}

int posix__shmrm(void *shmp)
{
	CloseHandle(((struct posix_shared_memory *)shmp)->fd);
	return 0;
}

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
