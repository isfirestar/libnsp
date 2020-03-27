#include "ipc.h"

#include "posix_ifos.h"

struct posix_shared_memory {
	file_descriptor_t shmid;
	int size;
	char filename[256];
	void *vm;
};

int posix__shmcb(const void *shmp)
{
	if (!shmp) {
		return -1;
	}

	return ((struct posix_shared_memory *)shmp)->size;
}

#if _WIN32

#include <Windows.h>

void *posix__shmmk(const char *filename, const int size)
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

typedef enum _SECTION_INFORMATION_CLASS {
	SectionBasicInformation,
	SectionImageInformation
} SECTION_INFORMATION_CLASS;

typedef struct _SECTION_BASIC_INFORMATION {
	PVOID         Base;
	ULONG         Attributes;
	LARGE_INTEGER Size;
} SECTION_BASIC_INFORMATION;

typedef DWORD(WINAPI* NTQUERYSECTION)(HANDLE, SECTION_INFORMATION_CLASS, PVOID, ULONG, PULONG);
static NTQUERYSECTION ZwQuerySection = NULL;

void *posix__shmop(const char *filename)
{
	struct posix_shared_memory *shm;
	HMODULE krnl32;
	SECTION_BASIC_INFORMATION section_info;
	NTSTATUS status;

	if (!filename) {
		return NULL;
	}

	if (!ZwQuerySection) {
		krnl32 = LoadLibraryA("kernel32.dll");
		if (!krnl32) {
			return NULL;
		}
		ZwQuerySection = (NTQUERYSECTION)GetProcAddress(krnl32, "NtQuerySection");
		FreeLibrary(krnl32);
		if (!ZwQuerySection) {
			return NULL;
		}
	}

	shm = (struct posix_shared_memory *)malloc(sizeof(struct posix_shared_memory));
	if (!shm) {
		return NULL;
	}
	memset(shm, 0, sizeof(struct posix_shared_memory));

	sprintf(shm->filename, "Global\\%s", filename);

	shm->shmid = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, shm->filename);
	if (!shm->shmid) {
		free(shm);
		return NULL;
	}

	/* we need get size from mapping handle by NT section */
	status = ZwQuerySection(shm->shmid, SectionBasicInformation, &section_info, sizeof(section_info), NULL);
	if (!NT_SUCCESS(status)) {
		CloseHandle(shm->shmid);
		free(shm);
		return NULL;
	}
	shm->size = section_info.Size.LowPart;

	return shm;
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

#else  /* POSIX */

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

void *posix__shmmk(const char *filename, const int size)
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

void *posix__shmop(const char *filename)
{
	key_t shmkey;
	struct posix_shared_memory *shm;
	struct shmid_ds shmds;

	if (!filename) {
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

	shm->shmid = shmget(shmkey, 0,  SHM_HUGETLB | SHM_NORESERVE);
	if (shm->shmid < 0) {
		return NULL;
	}

	if (shmctl(shm->shmid, IPC_STAT, &shmds) < 0) {
		return NULL;
	}

	shm->size = (int)shmds.shm_segsz;
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
