#include "compiler.h"

#include "posix_ifos.h"
#include "posix_string.h"
#include "posix_atomic.h"
#include "clist.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>


#include <time.h>

#if _WIN32
#include <Windows.h>
#pragma comment(lib, "Advapi32.lib")
#else
#include <sys/types.h>
#include <syscall.h>
#include <sys/syscall.h>
#include <sys/sysinfo.h>
#include <sys/syslog.h>
#include <dlfcn.h> 
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <iconv.h>
#include <locale.h>
#include <unistd.h>
#endif

static
int __posix__rmdir(const char *dir) {
#if _WIN32
    char all_file[MAXPATH];
    HANDLE find;
    WIN32_FIND_DATAA wfd;

    if (!dir) return -1;
    if (posix__isdir(dir) <= 0) return -1;

    posix__sprintf(all_file, cchof(all_file), "%s\\*.*", dir);

    find = FindFirstFileA(all_file, &wfd);
    if (INVALID_HANDLE_VALUE == find) {
        return -1;
    }
    while (FindNextFileA(find, &wfd)) {
        char target_file[MAXPATH];
        posix__sprintf(target_file, cchof(target_file), "%s\\%s", dir, wfd.cFileName);
        if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (0 == strcmp(".", wfd.cFileName) || 0 == strcmp("..", wfd.cFileName)) {
                continue;
            }
            if (__posix__rmdir(target_file) < 0) {
                break;
            }
        } else {
            if (posix__rm(target_file) < 0) {
                break;
            }
        }
    }
    FindClose(find);
    return ( (RemoveDirectoryA(dir) > 0) ? (0) : (-1));

#else
    /* > rm -rf dir */
    struct dirent *ent;
    DIR *dirp;

    if (!dir) {
        return RE_ERROR(EINVAL);
    }
    
    dirp = opendir(dir);
    if (!dirp) {
        return make_error_result(errno);
    }
    
    while (NULL != (ent = readdir(dirp))) {
        if (0 == posix__strcmp(ent->d_name, ".") || 0 == posix__strcmp(ent->d_name, "..")) {
            continue;
        }

        char filename[260];
        posix__sprintf(filename, cchof(filename), "%s/%s", dir, ent->d_name);

        if (posix__isdir(filename)) {
            __posix__rmdir(filename);
        } else {
            if (posix__rm(filename) < 0) {
                break;
            }
        }
    }

    remove(dir);
    closedir(dirp);
    return 0;
#endif
}

long posix__gettid() {
#if _WIN32
    return (int) GetCurrentThreadId();
#else
    return syscall(SYS_gettid);
#endif
}

long posix__getpid() {
#if _WIN32
    return (int) GetCurrentProcessId();
#else
    return syscall(SYS_getpid);
#endif          
}

void posix__sleep(uint64_t ms) {
#if _WIN32
    Sleep(MAXDWORD & ms);
#else
    usleep(ms * 1000);
#endif
}

void *posix__dlopen(const char *file) {
#if _WIN32
    HMODULE mod;
    mod = LoadLibraryA(file);
    return (void *) mod;
#else

    return dlopen(file, /*RTLD_LAZY*/RTLD_NOW);
#endif
}

void* posix__dlsym(void* handle, const char* symbol) {
    if (!handle || !symbol) return NULL;
#if _WIN32
    return (void *) GetProcAddress(handle, symbol);
#else
    return dlsym(handle, symbol);
#endif
}

int posix__dlclose(void *handle) {
    if (!handle) return -1;

#if _WIN32
    if (FreeLibrary((HMODULE) handle)) {
        return 0;
    }
    return -1;
#else
    return dlclose(handle);
#endif
}

const char *posix__dlerror() {
#if _WIN32
    return posix__strerror();
#else
    return dlerror();
#endif
}

int posix__mkdir(const char *const dir) {
#if _WIN32
    if (!dir) return -1;

    if (CreateDirectoryA(dir, NULL)) {
        return 0;
    }

    if (ERROR_ALREADY_EXISTS == GetLastError()) {
        return 0;
    }

    return -1;
#else
    if (dir) {
        return mkdir(dir, S_IRWXU | S_IRGRP | S_IWGRP | S_IROTH);
    }
    return -1;
#endif
}

#if _WIN32

struct dir_stack_node {
    struct list_head link;
    char dir[255];
};

int posix__pmkdir(const char *const dir) {
    struct list_head stack;
    struct dir_stack_node *node; /* 不允许使用栈对象 */
    struct dir_stack_node *pos;
    char *p_next_dir_symb;
    int retval;

    if (!dir) {
        return -1;
    }

    retval = 0;
    INIT_LIST_HEAD(&stack);

    node = (struct dir_stack_node *) malloc(sizeof ( struct dir_stack_node));
    if (!node) {
        return -1;
    }
    posix__strcpy(node->dir, cchof(node->dir), dir);
    list_add(&node->link, &stack);
    while (!list_empty(&stack)) {
        pos = list_first_entry(&stack, struct dir_stack_node, link);
        if (posix__mkdir(pos->dir) >= 0) {
            list_del(&pos->link);
            free(pos);
        } else {
            /* 不是目录结构性错误, 一律认定为失败 */
            if (ERROR_PATH_NOT_FOUND != GetLastError()) {
                retval = -1;
                break;
            }
            p_next_dir_symb = strrchr(pos->dir, POSIX__DIR_SYMBOL);
            if (!p_next_dir_symb) {
                retval = -1;
                break;
            }
            *p_next_dir_symb = 0;
            node = (struct dir_stack_node *) malloc(sizeof ( struct dir_stack_node));
            if (!node) {
                retval = -1;
                break;
            }
            posix__strncpy(node->dir, (uint32_t) (cchof(node->dir)), dir, (uint32_t) (p_next_dir_symb - pos->dir));
            list_add(&node->link, &stack);
        }
    }

    /* 返回前， 必须保证所有的内存被清理 
            无论前置操作是否成功 */
    while (!list_empty(&stack)) {
        pos = list_first_entry(&stack, struct dir_stack_node, link);
        if (posix__mkdir(pos->dir)) {
            list_del(&pos->link);
            free(pos);
        }
    }
    return retval;
}
#else

int posix__pmkdir(const char *const dir) {
    FILE *p;
    char command[255];
    sprintf(command, "mkdir -p %s", dir);
    p = popen(command, "r");
    if (p) {
        pclose(p);
        return 0;
    }
    return -1;
}
#endif

int posix__rm(const char *const target) {
    if (!target) {
        return -1;
    }
#if _WIN32
    if (posix__isdir(target)) {
        return __posix__rmdir(target);
    } else {
        return DeleteFileA(target);
    }
#else
    if (posix__isdir(target)) {
        return __posix__rmdir(target);
    } else {
        return remove(target);
    }
#endif
}

void posix__close(int fd) {
#if _WIN32
    if ((HANDLE) fd != INVALID_HANDLE_VALUE) {
        CloseHandle((HANDLE) fd);
    }
#else
    if (fd > 0) {
        close(fd);
    }
#endif
}

int posix__fflush(int fd) {
    if (fd < 0) {
        return EINVAL;
    }
#if _WIN32
    return convert_boolean_condition_to_retval(FlushFileBuffers((HANDLE) fd));
#else
    return fdatasync(fd);
#endif
}

const char *posix__fullpath_current() {
    static char fullpath[255];
#if _WIN32
    uint32_t length;

    fullpath[0] = 0;

    length = GetModuleFileNameA(NULL, fullpath, sizeof ( fullpath) / sizeof ( fullpath[0]));
    if (0 != length) {
        return fullpath;
    } else {
        return NULL;
    }
#else
    long pid;
    char link[64];

    memset(fullpath, 0, 255);
    pid = posix__getpid();
    if (pid < 0) {
        return NULL;
    }

    posix__sprintf(link, cchof(link), "/proc/%d/exe", pid);
    if (readlink(link, fullpath, sizeof ( fullpath)) < 0) {
        return NULL;
    }
    return fullpath;
#endif
}

char *posix__fullpath_current2(char *holder, int cb) {
    if (!holder || cb <= 0) {
        return NULL;
    }

    memset(holder, 0, cb);
#if _WIN32
    uint32_t length;
    length = GetModuleFileNameA(NULL, holder, cb );
    if (0 == length) {
        return NULL;
    }
#else
    long pid;
    char link[64];

    pid = posix__getpid();
    if (pid < 0) {
        return NULL;
    }

    posix__sprintf(link, cchof(link), "/proc/%d/exe", pid);
    if (readlink(link, holder, cb) < 0) {
        return NULL;
    }
#endif
    return holder;
}

const char *posix__getpedir() {
    char *p;
    static char dir[MAXPATH];
    const char *fullpath = posix__fullpath_current();
    if (!fullpath) {
        return NULL;
    }
    p = strrchr(fullpath, POSIX__DIR_SYMBOL);
    if (!p) {
        return NULL;
    }
    posix__strncpy(dir, (uint32_t) (cchof(dir)), fullpath, (uint32_t) (p - fullpath));
    return dir;
}

char *posix__getpedir2(char *holder, int cb) {
    char *p;
    char fullpath[255];

    if (!holder || cb <= 0) {
        return NULL;
    }

    p = posix__fullpath_current2(fullpath, sizeof(fullpath));
    if (!p) {
        return NULL;
    }
    p = strrchr(fullpath, POSIX__DIR_SYMBOL);
    if (!p) {
        return NULL;
    }
    posix__strncpy(holder, (uint32_t) (cb), fullpath, (uint32_t) (p - fullpath));
    return holder;
}

const char *posix__getpename() {
    const char *p;
    static char name[MAXPATH];
    const char *fullpath = posix__fullpath_current();
    if (!fullpath) {
        return NULL;
    }

    p = strrchr(fullpath, POSIX__DIR_SYMBOL);
    if (!p) {
        return NULL;
    }

    posix__strcpy(name, cchof(name), p + 1);
    return &name[0];
}

char *posix__getpename2(char *holder, int cb) {
    char *p;
    char fullpath[255];

    if (!holder || cb <= 0) {
        return NULL;
    }

    p = posix__fullpath_current2(fullpath, sizeof(fullpath));
    if (!p) {
        return NULL;
    }

    p = strrchr(fullpath, POSIX__DIR_SYMBOL);
    if (!p) {
        return NULL;
    }

    posix__strcpy(holder, cb, p + 1);
    return holder;
}

const char *posix__getelfname() {
    return posix__getpename();
}

const char *posix__gettmpdir() {
    static char buffer[MAXPATH];
#if _WIN32
    if (0 == GetTempPathA(_countof(buffer), buffer)) {
        return buffer;
    }
    return NULL;
#else
    posix__strcpy(buffer, cchof(buffer), "/tmp");
    return buffer;
#endif     
}

char *posix__gettmpdir2(char *holder, int cb) {
    if (!holder || cb <= 0) {
        return NULL;
    }

#if _WIN32
    if (0 == GetTempPathA(cb, holder)) {
        return holder;
    }
    return NULL;
#else
    posix__strcpy(holder, cb, "/tmp");
    return holder;
#endif 
}

int posix__isdir(const char *const file) {
#if _WIN32
    unsigned long attr;

    if (!file) return -1;

    attr = GetFileAttributesA(file);
    if (INVALID_FILE_ATTRIBUTES == attr) {
        return -1;
    }

    if (attr & FILE_ATTRIBUTE_DIRECTORY) {
        return 1;
    }
    return 0;
#else
    struct stat st;

    if (!file) return -1;

    if (stat(file, &st) < 0) {
        return -1;
    }

    /* 如果符号链接目标是一个目录， 同样会解释为一个目录， 而不是 __S_IFLNK
     * __S_IFLNK 仅针对指向文件的符号链接
     * 使用符号链接目录的相对路径同样可以正常open文件
     * 例如： 
     * /home/Julie/escape/configs -> /etc
     * int fd = open("/home/Julie/escape/configs/passwd", O_RDONLY); 可以正常打开文件
     * 
     * shell 中查找所有符号链接的命令:
     * find . -type l
     * 删除所有的符号链接
     * find . -type l | xargs rm
    */
    if (st.st_mode & __S_IFDIR) {
        return 1;
    }

    return 0;
#endif
}

int posix__getpriority(int *priority) {
#if _WIN32
    DWORD retval;

    if (!priority) {
        return RE_ERROR(EINVAL);
    }

    retval = GetPriorityClass(GetCurrentProcess());
    if (0 == retval) {
        return -1;
    }

    *priority = retval;
    return 0;
#else
    int who;
    int retval;

    if (!priority) {
        return RE_ERROR(EINVAL);
    }

    who = 0;
    retval = getpriority(PRIO_PROCESS, who);
    if (0 == errno) {
        *priority = retval;
        return 0;
    }
    return -1;
#endif
}

int posix__setpriority_below() {
#if _WIN32
    return convert_boolean_condition_to_retval(SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS));
#else
    return nice(5);
#endif  
}

int posix__setpriority_normal() {
#if _WIN32
    return convert_boolean_condition_to_retval(SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS));
#else
    return nice(0);
#endif  
}

int posix__setpriority_critical() {
#if _WIN32
    return convert_boolean_condition_to_retval(SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS));
#else
    return nice(-5);
#endif
}

int posix__setpriority_realtime() {
#if _WIN32
    return convert_boolean_condition_to_retval(SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS));
#else
    return nice(-10);
#endif
}

int posix__getnprocs() {
#if _WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return (int) sysinfo.dwNumberOfProcessors;
#else
    return get_nprocs();
#endif
}

int posix__getsysmem(sys_memory_t *sysmem) {
#if _WIN32
    MEMORYSTATUSEX s_info;
    if (!GlobalMemoryStatusEx(&s_info)) {
        return -1;
    }

    memset(sysmem, 0, sizeof ( sys_memory_t));
    sysmem->totalram = s_info.ullTotalPhys;
    sysmem->freeram = s_info.ullAvailPhys;
    sysmem->totalswap = s_info.ullTotalPageFile;
    sysmem->freeswap = s_info.ullAvailPageFile;
    return 0;
#else
    struct sysinfo s_info;
    if (sysinfo(&s_info) < 0) {
        return -1;
    }

    memset(sysmem, 0, sizeof ( sys_memory_t));
    if (s_info.totalhigh > 0) {
        sysmem->totalram = s_info.totalhigh;
        sysmem->totalram <<= 32;
    }
    sysmem->totalram |= s_info.totalram;

    if (s_info.freehigh > 0) {
        sysmem->freeram = s_info.freehigh;
        sysmem->freeram <<= 32;
    }
    sysmem->freeram |= s_info.freeram;
    sysmem->totalswap = s_info.totalswap;
    sysmem->freeswap = s_info.freeswap;
    /*  FILE *fp;
        char str[81];
        memset(str,0,81);
        fp=popen("cat /proc/meminfo | grep MemTotal:|sed -e 's/.*:[^0-9]//'","r");
        if(fp >= 0)
        {
            fgets(str,80,fp);
            fclose(fp);
        }
    */
    return 0;
#endif
}

uint32_t posix__getpagesize() {
    uint32_t ps = 0;
#if _WIN32
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    ps = sys_info.dwPageSize;
#else
    ps = sysconf(_SC_PAGE_SIZE);
#endif
    return ps;
}

void posix__syslog(const char *const logmsg) {
#if _WIN32
    HANDLE shlog;
    const char *strerrs[1];

    if (!logmsg) {
        return;
    }

    shlog = RegisterEventSourceA(NULL, "Application");
    if (INVALID_HANDLE_VALUE == shlog) {
        return;
    }

    strerrs[0] = logmsg;
    BOOL b = ReportEventA(shlog, EVENTLOG_ERROR_TYPE, 0, 0xC0000001, NULL,
            1, 0, strerrs, NULL);

    DeregisterEventSource(shlog);
#else
    /*
     * cat /var/log/messages | tail -n1
     */
    if (logmsg) {
        syslog(LOG_USER | LOG_ERR, "[%d]# %s", getpid(), logmsg);
    }
#endif
}

static
int __posix__gb2312_to_uniocde(char **from, size_t input_bytes, char **to, size_t *output_bytes) {
#if _WIN32
    int min;
    int need;

    if (!output_bytes) {
        return RE_ERROR(EINVAL);
    }

    min = MultiByteToWideChar(CP_ACP, 0, *from, -1, NULL, 0);
    need = 2 * min;

    if (!to || *output_bytes < (size_t) need) {
        *output_bytes = need;
        return RE_ERROR(EAGAIN);
    }

    return MultiByteToWideChar(CP_ACP, 0, *from, -1, (LPWSTR) * to, (int) *output_bytes);
#else
    int retval;
    iconv_t cd;

    cd = iconv_open("gb2312", "utf8");
    if (!cd) {
        return -1;
    }

    setlocale(LC_ALL, "zh_CN.gb18030");
    retval = iconv(cd, from, &input_bytes, to, output_bytes);
    if (retval < 0) {
        printf("%d\n", errno);
    }
    iconv_close(cd);
    return retval;
#endif
}

static
int __posix__unicode_to_gb2312(char **from, size_t input_bytes, char **to, size_t *output_bytes) {
#if _WIN32
    int min;

    if (!output_bytes) {
        return RE_ERROR(EINVAL);
    }

    min = WideCharToMultiByte(CP_OEMCP, 0, (LPCWCH) * from, -1, NULL, 0, NULL, FALSE);
    if (!to || *output_bytes < (size_t) min) {
        *output_bytes = min;
        return RE_ERROR(EAGAIN);
    }

    return WideCharToMultiByte(CP_OEMCP, 0, (LPCWCH) * from, -1, *to, (int) *output_bytes, NULL, FALSE);
#else
    return -1;
#endif
}

int posix__iconv(const char *from_encode, const char *to_encode, char **from, size_t from_bytes, char **to, size_t *to_bytes) {
    if (0 == posix__strcasecmp(from_encode, "gb2312") && 0 == posix__strcasecmp(to_encode, "unicode")) {
        return __posix__gb2312_to_uniocde(from, from_bytes, to, to_bytes);
    } else if (0 == posix__strcasecmp(from_encode, "unicode") && 0 == posix__strcasecmp(to_encode, "gb2312")) {
        return __posix__unicode_to_gb2312(from, from_bytes, to, to_bytes);
    }
    return EINVAL;
}

int posix__write_file(int fd, const char *buffer, int size) {
    int wcb;
    int n;
    int wtotal;

    if (fd < 0 || size <= 0 || !buffer) {
        return RE_ERROR(EINVAL);
    }

    wtotal = 0;
    n = size;
    while (n > 0) {
#if _WIN32
        if (!WriteFile((HANDLE) fd, (buffer + (size - n)), n, (LPDWORD) & wcb, NULL)) {
            break;
        }
#else
        wcb = write(fd, (buffer + (size - n)), n);
        if (wcb <= 0) {
            break;
        }
#endif
        n -= wcb;
        wtotal += wcb;
    }
    return wtotal;
}

int posix__read_file(int fd, char *buffer, int size) {

    int rcb;
    int rtotal;
    int n;

    rtotal = 0;
    n = size;
    while (n > 0) {
#if _WIN32
        if (!ReadFile((HANDLE) fd, (buffer + (size - n)), n, (LPDWORD) & rcb, NULL)) {
            break;
        }
#else
        rcb = read(fd, (buffer + (size - n)), n);
        if (rcb <= 0) {
            break;
        }
#endif
        n -= rcb;
        rtotal += rcb;
    }

    return rtotal;
}

/*  Generate random numbers in the half-closed interva
 *  [range_min, range_max). In other words,
 *  range_min <= random number < range_max
 */
int posix__random(const int range_min, const int range_max) {
    static int rand_begin = 0;
    if (1 == posix__atomic_inc(&rand_begin)) {
        srand((unsigned int) time(NULL));
    } else {
        posix__atomic_dec(&rand_begin);
    }

    int u;
    int r = rand();

    if (range_min == range_max) {
        u = ((0 == range_min) ? r : range_min);
    } else {
        if (range_max < range_min) {
            u = r;
        } else {
#if _WIN32
            /* 区间差值大于7FFFH, 如果不进行调整，则取值区间被截断为 [min, min+7FFFH) */
            if (range_max - range_min > RAND_MAX) {
                u = (int) ((double) rand() / (RAND_MAX + 1) * (range_max - range_min) + range_min);
            } else {
                u = (r % (range_max - range_min)) + range_min;
            }
#else
            u = (r % (range_max - range_min)) + range_min;
#endif
        }
    }

    return u;
}

uint64_t posix__get_filesize(const char *path) {
    uint64_t filesize = (uint64_t) (-1);
#if _WIN32
    LARGE_INTEGER size;
    HANDLE fd;

    fd = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (fd == INVALID_HANDLE_VALUE) {
        return filesize;
    }
    if (GetFileSizeEx(fd, &size)) {
        filesize = size.HighPart;
        filesize <<= 32;
        filesize |= size.LowPart;
        CloseHandle(fd);
    }
#else
    struct stat statbuff;
    if (stat(path, &statbuff) < 0) {
        return filesize;
    } else {
        filesize = statbuff.st_size;
    }
#endif
    return filesize;
}

int posix__seek_file_offset(int fd, uint64_t offset) {
#if _WIN32
    LARGE_INTEGER move, pointer;
    move.QuadPart = offset;
    if (!SetFilePointerEx((HANDLE) fd, move, &pointer, FILE_BEGIN)) {
        return -1;
    }
#else
    __off_t newoff;
    newoff = lseek(fd, (__off_t) offset, SEEK_SET);
    if (newoff != offset) {
        return -1;
    }
#endif
    return 0;
}

int posix__file_open(const char *path, void *descriptor) {
    if (!path || !descriptor) {
        return RE_ERROR(EINVAL);
    }

#if _WIN32
	HANDLE fd;
    fd = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE == fd) {
        return make_error_result(GetLastError());
    }
    *((HANDLE *) descriptor) = fd;
#else
    int fd;
    fd = open(path, O_RDWR);
    if (fd < 0) {
        return make_error_result(errno);
    }
    *((int *) descriptor) = fd;
#endif
    return 0;
}

int posix__file_open_always(const char *path, void *descriptor) {
    if (!path || !descriptor) {
        return RE_ERROR(EINVAL);
    }

#if _WIN32
    HANDLE fd;
	fd = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE == fd) {
        return make_error_result(GetLastError());
    }
    *((HANDLE *) descriptor) = fd;
#else
    int fd;
    fd = open(path, O_RDWR | O_CREAT, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    if (fd < 0) {
        return make_error_result(errno);
    }
    *((int *) descriptor) = fd;
#endif
    return 0;
}

int posix__file_create(const char *path, void *descriptor) {
    if (!path || !descriptor) {
        return RE_ERROR(EINVAL);
    }

#if _WIN32
    HANDLE fd;
	fd = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE == fd) {
        return make_error_result(GetLastError());
    }
    *((HANDLE *) descriptor) = fd;
#else
    int fd;
    fd = open(path, O_RDWR | O_CREAT | O_EXCL, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    if (fd < 0) {
        return make_error_result(errno);
    }
    *((int *) descriptor) = fd;
#endif
    return 0;
}

int posix__file_create_always(const char *path, void *descriptor) {
    if (!path || !descriptor) {
        return RE_ERROR(EINVAL);
    }

#if _WIN32
    HANDLE fd;
	fd = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE == fd) {
        return make_error_result(GetLastError());
    }
    *((HANDLE *) descriptor) = fd;
#else
    int fd;
    fd = open(path, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    if (fd < 0) {
        return make_error_result(errno);
    }
    *((int *) descriptor) = fd;
#endif
    return 0;
}
