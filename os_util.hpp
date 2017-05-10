#if !defined OS_UTIL_HEADER_02160616
#define OS_UTIL_HEADER_02160616

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <cstdarg>
#include <ctime>
#include <cstdint>
#include <atomic>

#include <string>

#if _WIN32

#include <stddef.h>
#include <Windows.h>

#else

#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h> 
#include <pthread.h>
#include <syscall.h>
#include <dirent.h>
#include <semaphore.h>

#include <signal.h>

#include <sys/time.h>
#include <sys/syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/sysinfo.h>
#include <linux/unistd.h>

#if !defined DUMMYSTRUCTNAME
#define DUMMYSTRUCTNAME
#endif

typedef union _LARGE_INTEGER {

	struct {
		uint32_t LowPart;
		int32_t HighPart;
	} DUMMYSTRUCTNAME;

	struct {
		uint32_t LowPart;
		int32_t HighPart;
	} u;
	int64_t QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;

typedef union _ULARGE_INTEGER {

	struct {
		uint32_t LowPart;
		uint32_t HighPart;
	} DUMMYSTRUCTNAME;

	struct {
		uint32_t LowPart;
		uint32_t HighPart;
	} u;
	int64_t QuadPart;
} ULARGE_INTEGER, *PULARGE_INTEGER;

#endif

#include "icom/compiler.h"
#include "icom/posix_types.h"

namespace nsp {
	namespace os {


		// ��ȡ��ҳ��Ϣ
		uint32_t get_pagesize();

		// ����·�������������� DIR_SYMBOL
		template<class T>
		std::basic_string<T> get_module_fullpath();
		template<class T>
		std::basic_string<T> get_module_directory();
		template<class T>
		std::basic_string<T> get_module_filename();

#if _WIN32
		template<class T>
		std::basic_string<T> get_sysdir();
		template<class T>
		std::basic_string<T> get_appdata_dir();
#endif
		template <class T>
		std::basic_string<T> get_tmpdir();
		template <class T>
		int rmfile( const std::basic_string<T> &target );
		template<class T>
		int mkdir( const std::basic_string<T> &dir );
		template <class T>
		int mkdir_s( const std::basic_string<T> &dir );
		template<class T>
		int is_dir( const std::basic_string<T> &file ); // ����>0 ����Ŀ¼, 0�����ļ��� �������Ǵ���
		template<class T>
		int rmdir_s( const std::basic_string<T> &dir ); // dir ���벻������'/'

		int gettid();
		int getpid();

		int getnpros();

		int getsysmem( uint64_t &total, uint64_t &free, uint64_t &total_swap, uint64_t &free_swap);

		class waitable_handle {
			unsigned char posix_waiter_[256];
		public:
			waitable_handle( int sync = 1 );
			~waitable_handle();
			/* @timeo ָ���ȴ���ʽ������:
			 *  <0 ��ʾ���޵ȴ�
			 *  0  ��ʾ�����ź�
			 *  >0  ��ʾ����ʱ�ȴ�
			 *
			 * ���ض���:
			 * ��� tsc <= 0 ����:
			 * 0: �¼�����
			 * -1: ϵͳ����ʧ��
			 * 
			 * ��� tsc > 0 ���У�
			 * 0: �¼�����
			 * ETIMEOUT: �ȴ���ʱ
			 * -1: ϵͳ����ʧ�� 
			 */
			int wait( uint32_t timeo = 0xFFFFFFFF );
			void sig();
			void reset();
		};

		void pshang();

		template<class T>
		void attempt_syslog( const std::basic_string<T> &msg, uint32_t err );


		// ��ȡϵͳ������Ŀǰʱ��ڵ����ŵ�tick
		// gettick			����Ӧ�ò�δ𾫶ȣ� ���ȵ�λΪ  ms,
		// clock_gettime	�����ں˼��δ𾫶ȣ� ��λΪ     100ns
		uint64_t gettick();
		uint64_t clock_gettime();

		// ��̬��������� gcc -ldl
		void *dlopen( const char *file );
		void* dlsym( void* handle, const char* symbol );
		int dlclose( void *handle );
	}
}

#if !defined PAGE_SIZE
#define PAGE_SIZE nsp::os::get_pagesize()
#endif

#endif