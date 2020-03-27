#include "ipc.h"

#include "posix_ifos.h"

struct posix_shared_memory {
	file_descriptor_t shmid;
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

	shm->shmid = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, size, shm->filename);
	if (!shm->shmid) {
		free(shm);
		return NULL;
	}

	if (ERROR_ALREADY_EXISTS == GetLastError()) {
		CloseHandle(shm->shmid);
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

	shm->shmid = (PAGE_READWRITE, FALSE, shm->filename);
}

void *posix__shmat(const void *shmp)
{
	struct posix_shared_memory *shm;

	shm = (struct posix_shared_memory *)shmp;

	shm->vm = MapViewOfFile(shm->shmid, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, shm->size);
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
	CloseHandle(((struct posix_shared_memory *)shmp)->shmid);
	return 0;
}

#else

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

void *posix__shmct(const char *filename, const int size)
{
	key_t shmkey;
	struct posix_shared_memory *shm;
	int fd;

	if (!filename || size <= 0 || 0 != (size % PAGE_SIZE)) {
		return NULL;
	}

	shm = (struct posix_shared_memory *)malloc(sizeof(struct posix_shared_memory));
	if (!shm) {
		return NULL;
	}
	memset(shm, 0, sizeof(struct posix_shared_memory));

	sprintf(shm->filename, "/dev/shm/%s", filename);
	if (posix__file_open(shm->filename, FF_RDACCESS | FF_OPEN_EXISTING, 0600, &fd) < 0) {
		if ( posix__file_open(shm->filename, FF_RDACCESS | FF_CREATE_NEWONE, 0600, &fd) < 0) {
			return NULL;
		}
	}

	/* afterward, the file object are meaningless */
	posix__file_close(fd);

	shmkey = ftok(shm->filename, ';');
	if (shmkey < 0) {
		return NULL;
	}

	shm->shmid = shmget(shmkey, size, IPC_CREAT | IPC_EXCL | SHM_HUGETLB | SHM_NORESERVE);
	if (shm->shmid < 0) {
		return NULL;
	}

	return shm;
}

void *posix__shmop(const char *filename, const int size)
{
	key_t shmkey;
	struct posix_shared_memory *shm;

	if (!filename || size <= 0 || 0 != (size % PAGE_SIZE)) {
		return NULL;
	}

	shm = (struct posix_shared_memory *)malloc(sizeof(struct posix_shared_memory));
	if (!shm) {
		return NULL;
	}
	memset(shm, 0, sizeof(struct posix_shared_memory));

	sprintf(shm->filename, "/dev/shm/%s", filename);
	shmkey = ftok(shm->filename, ';');
	if (shmkey < 0) {
		return NULL;
	}

	shm->shmid = shmget(shmkey, size,  SHM_HUGETLB | SHM_NORESERVE);
	if (shm->shmid < 0) {
		return NULL;
	}

	return shm;
}

void *posix__shmat(const void *shmp)
{
	struct posix_shared_memory *shm;

	shm = (struct posix_shared_memory *)shmp;

	shm->vm = shmat(shm->shmid, NULL, 0);
	if ((void *)-1 == shm->vm) {
		return NULL;
	}
	return shm->vm;
}

int posix__shmdt(const void *shmp)
{
	return (NULL != shmp) ? shmdt(((struct posix_shared_memory *)shmp)->vm) : -1;
}

int posix__shmrm(void *shmp)
{
	struct shmid_ds shmds;
	return shmctl(((struct posix_shared_memory *)shmp)->shmid, IPC_RMID, &shmds);
}

#endif
