#if _WIN32
#else
#include <sys/mman.h>
#endif

#if !defined MAP_FAILED
#define MAP_FAILED   ((void *)0)
#endif

/*
 *	本模块提供一个文件映射方式的内存共享工作
 *	neo-anderson 2017-03-06
 */

namespace nsp {
    namespace os {
        // 内存共享对象

        class shared_memory {
#if _WIN32
            HANDLE mapping_ = INVALID_HANDLE_VALUE;
#endif
            void *handle_ = MAP_FAILED;
            char file_[255]; // WIN32作为共享名, GNU作为文件路径
            int cb_ = -1;
        public:
            shared_memory();

            // 拷贝构造语义: (跨进程不存在拷贝构造)
            // WIN32	:复制映射句柄,使用复制后的句柄映射虚拟内存, 可能抛出异常， 使用 DWORD 捕获错误码
            // GNU		:直接使用引用源的映射虚拟地址(仅fork可以延续使用该虚拟地址，非继承进程或exec*均需要重新打开)
            shared_memory(const shared_memory &lref);

            // 移动语义:(跨进程不存在移动拷贝构造)
            // WIN32	:移动映射句柄和虚拟地址， 移除引用源的句柄和虚拟地址
            // GNU		:移动虚拟地址，  移除引用源的虚拟地址
            shared_memory(shared_memory &&rref);
            ~shared_memory();

            // 同拷贝构造语义
            shared_memory &operator=(const shared_memory &lref);
            // 同移动构造语义
            shared_memory &operator=(shared_memory &&rref);

            // 允许以危险的方式直接引用映射地址
            operator void *();
            operator const void *() const;
        public:
            int create(int cb, const char *file = nullptr);
            int open(int cb, const char *file = nullptr);
            int read(char *buffer, int cblen, int offset);
            int write(const char *buffer, int cblen, int offset);
            void close();

        public:
            const char *const filename() const;
            const char *const shared_name() const;
            int getsize() const;
        };
    }
}

#if !defined convert_from_shmem
#define convert_from_shmem(type, shm)   ((type *)(void *)shm)
#endif