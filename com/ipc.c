#include "ipc.h"

#include "posix_ifos.h"

struct posix_shared_memory {
	file_descriptor_t shmid;
	int size;
	char filename[256];
	void *vma;
};

int posix__shmcb(const void *shmp)
{
	if (!shmp) {
		return -1;
	}

	return ((struct posix_shared_memory *)shmp)->size;
}

void *posix__shmvma(const void *shmp)
{
	if (!shmp) {
		return NULL;
	}

	return ((struct posix_shared_memory *)shmp)->vma;
}

#define SHM_HOST_PREDEFINITION	"nshostSharedMemorySection"

#if _WIN32

#include <Windows.h>

#define SHM_FILE_PREFIX	"\\\\.\\GLOBAL\\"

void *posix__shmmk(const char *filename, const int size)
{
	struct posix_shared_memory *shmptr;

	if (size <= 0 || 0 != (size % PAGE_SIZE)) {
		return NULL;
	}

	shmptr = (struct posix_shared_memory *)malloc(sizeof(struct posix_shared_memory));
	if (!shmptr) {
		return NULL;
	}
	memset(shmptr, 0, sizeof(struct posix_shared_memory));
	sprintf(shmptr->filename, SHM_FILE_PREFIX"%s", filename ? filename : SHM_HOST_PREDEFINITION );

	do {
		shmptr->shmid = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, size, shm->filename);
		if (!shmptr->shmid) {
			break;
		}

		if (ERROR_ALREADY_EXISTS == GetLastError()) {
			CloseHandle(shmptr->shmid);
			break;
		}

		shmptr->size = size;
		return shmptr;
	}while (0);

	free(shmptr);
	return NULL;
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
	struct posix_shared_memory *shmptr;
	HMODULE krnl32;
	SECTION_BASIC_INFORMATION section_info;
	NTSTATUS status;

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

	shmptr = (struct posix_shared_memory *)malloc(sizeof(struct posix_shared_memory));
	if (!shmptr) {
		return NULL;
	}
	memset(shmptr, 0, sizeof(struct posix_shared_memory));
	sprintf(shmptr->filename, SHM_FILE_PREFIX"%s", filename ? filename : SHM_HOST_PREDEFINITION );

	shmptr->shmid = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, shm->filename);
	if (!shmptr->shmid) {
		free(shmptr);
		return NULL;
	}

	/* we need get size from mapping handle by NT section */
	status = ZwQuerySection(shmptr->shmid, SectionBasicInformation, &section_info, sizeof(section_info), NULL);
	if (!NT_SUCCESS(status)) {
		CloseHandle(shmptr->shmid);
		free(shmptr);
		return NULL;
	}
	shmptr->size = section_info.Size.LowPart;

	return shmptr;
}

void *posix__shmat(const void *shmp)
{
	struct posix_shared_memory *shmptr;

	shmptr = (struct posix_shared_memory *)shmp;

	shmptr->vma = MapViewOfFile(shmptr->shmid, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, shm->size);
	return shmptr->vma;
}

int posix__shmdt(const void *shmp)
{
	struct posix_shared_memory *shmptr;

	shmptr = (struct posix_shared_memory *)shmp;

	if (shmptr) {
		if (UnmapViewOfFile(shmptr->vma)) {
			shmptr->vma = NULL;
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
#include <errno.h>
#include <string.h>

#define SHM_FILE_PREFIX 			"/dev/shm/"
#define SHM_PROCFS_HUGEPAGES		"/proc/sys/vm/nr_hugepages"
#define SHM_TOK_SYMLOB				';'

static int determine_vm_hugepages()
{
	int fd;
	int retval;
	char text[16];
	int n;

	retval = posix__file_open(SHM_PROCFS_HUGEPAGES, FF_RDACCESS | FF_OPEN_EXISTING, 0600, &fd);
	if (retval < 0) {
		return -1;
	}

	n = -1;
	retval = read(fd, text, sizeof(text));
	if (retval > 0 && retval < sizeof(text)) {
		text[retval - 1] = 0;  /* drop last byte : '\n'  */
		n = atoi(text);
	}

	close(fd);
	return n;
}

void *posix__shmmk(const char *filename, int size)
{
	key_t shmkey;
	struct posix_shared_memory *shmptr;
	int fd;
	int shmflg;
	int hugepages;

	if ( size <= 0 || 0 != (size % PAGE_SIZE)) {
		return NULL;
	}

	shmptr = (struct posix_shared_memory *)malloc(sizeof(struct posix_shared_memory));
	if (!shmptr) {
		return NULL;
	}
	memset(shmptr, 0, sizeof(struct posix_shared_memory));
	sprintf(shmptr->filename, SHM_FILE_PREFIX"%s", filename ? filename : SHM_HOST_PREDEFINITION );

	do {
		if (posix__file_open(shmptr->filename, FF_RDACCESS | FF_OPEN_EXISTING, 0600, &fd) < 0) {
			if ( posix__file_open(shmptr->filename, FF_RDACCESS | FF_CREATE_NEWONE, 0600, &fd) < 0) {
				break;
			}
		}

		/* afterward, the file object are meaningless */
		posix__file_close(fd);

		shmkey = ftok(shmptr->filename, SHM_TOK_SYMLOB);
		if (shmkey < 0) {
			break;
		}

		shmflg = IPC_CREAT | IPC_EXCL | SHM_NORESERVE;
		/* examine if the kernel support hugepages. */
		hugepages = determine_vm_hugepages();
		if (hugepages > 0) {
			/* every hugepages size is 2MB. acquirement size MUST less than kernel permission */
			if (size <= (hugepages << 21)) {
				shmflg |= SHM_HUGETLB;
			}
		}
		shmptr->shmid = shmget(shmkey, size, shmflg);
		if (shmptr->shmid < 0) {
			break;
		}

		return shmptr;
	} while(0);

	free(shmptr);
	return NULL;
}

void *posix__shmop(const char *filename)
{
	key_t shmkey;
	struct posix_shared_memory *shmptr;
	struct shmid_ds shmds;

	shmptr = (struct posix_shared_memory *)malloc(sizeof(struct posix_shared_memory));
	if (!shmptr) {
		return NULL;
	}
	memset(shmptr, 0, sizeof(struct posix_shared_memory));
	sprintf(shmptr->filename, SHM_FILE_PREFIX"%s", filename ? filename : SHM_HOST_PREDEFINITION );

	do {
		shmkey = ftok(shmptr->filename, SHM_TOK_SYMLOB);
		if (shmkey < 0) {
			break;
		}

		/* open existing only */
		shmptr->shmid = shmget(shmkey, 0, 0);
		if (shmptr->shmid < 0) {
			break;
		}

		/* load attributes from segment, main purpose is get size of it */
		if (shmctl(shmptr->shmid, IPC_STAT, &shmds) < 0) {
			break;
		}

		shmptr->size = (int)shmds.shm_segsz;
		return shmptr;
	} while(0);

	free(shmptr);
	return NULL;
}

void *posix__shmat(const void *shmp)
{
	struct posix_shared_memory *shmptr;

	shmptr = (struct posix_shared_memory *)shmp;

	shmptr->vma = shmat(shmptr->shmid, NULL, 0);
	if ((void *)-1 == shmptr->vma) {
		return NULL;
	}

	return shmptr->vma;
}

int posix__shmdt(const void *shmp)
{
	struct posix_shared_memory *shmptr;

	shmptr = (struct posix_shared_memory *)shmp;
	if (!shmptr->vma || (((void *)-1) == shmptr->vma)) {
		return -1;
	}

	return shmdt(shmptr->vma);
}

int posix__shmrm(void *shmp)
{
	struct shmid_ds shmds;
	return shmctl(((struct posix_shared_memory *)shmp)->shmid, IPC_RMID, &shmds);
}

#endif
