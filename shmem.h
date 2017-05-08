#if _WIN32
#else
#include <sys/mman.h>
#endif

#if !defined MAP_FAILED
#define MAP_FAILED   ((void *)0)
#endif

/*
 *	��ģ���ṩһ���ļ�ӳ�䷽ʽ���ڴ湲����
 *	neo-anderson 2017-03-06
 */

namespace nsp {
    namespace os {
        // �ڴ湲�����

        class shared_memory {
#if _WIN32
            HANDLE mapping_ = INVALID_HANDLE_VALUE;
#endif
            void *handle_ = MAP_FAILED;
            char file_[255]; // WIN32��Ϊ������, GNU��Ϊ�ļ�·��
            int cb_ = -1;
        public:
            shared_memory();

            // ������������: (����̲����ڿ�������)
            // WIN32	:����ӳ����,ʹ�ø��ƺ�ľ��ӳ�������ڴ�, �����׳��쳣�� ʹ�� DWORD ���������
            // GNU		:ֱ��ʹ������Դ��ӳ�������ַ(��fork��������ʹ�ø������ַ���Ǽ̳н��̻�exec*����Ҫ���´�)
            shared_memory(const shared_memory &lref);

            // �ƶ�����:(����̲������ƶ���������)
            // WIN32	:�ƶ�ӳ�����������ַ�� �Ƴ�����Դ�ľ���������ַ
            // GNU		:�ƶ������ַ��  �Ƴ�����Դ�������ַ
            shared_memory(shared_memory &&rref);
            ~shared_memory();

            // ͬ������������
            shared_memory &operator=(const shared_memory &lref);
            // ͬ�ƶ���������
            shared_memory &operator=(shared_memory &&rref);

            // ������Σ�յķ�ʽֱ������ӳ���ַ
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