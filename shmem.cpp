#include "os_util.hpp"
#include "toolkit.h"

#include <errno.h>

#include "shmem.h"

#include "icom/posix_ifos.h"
#include "icom/posix_string.h"

#if _WIN32
static std::atomic<int> shared_memory_name_key
{
    0
};
#endif

namespace nsp {
    namespace os {

        shared_memory::shared_memory() {
            memset(file_, 0, sizeof ( file_));
        }

        shared_memory::shared_memory(const shared_memory &lref) {
            cb_ = lref.cb_;
            if (lref.filename()) {
                toolkit::posix_strcpy(file_, cchof(file_), lref.file_);
            } else {
                memset(file_, 0, cchof(file_));
            }

#if _WIN32
            if (lref.mapping_ != INVALID_HANDLE_VALUE) {
                if (!DuplicateHandle(GetCurrentProcess(), lref.mapping_,
                        GetCurrentProcess(), &mapping_,
                        0, FALSE, DUPLICATE_SAME_ACCESS)) {
                    throw GetLastError();
                }
            }

            handle_ = MapViewOfFile(mapping_, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, cb_);
            if (!handle_) {
                CloseHandle(mapping_);
                mapping_ = INVALID_HANDLE_VALUE;
                throw GetLastError();
            }
#else
            handle_ = lref.handle_;
#endif
        }

        shared_memory::shared_memory(shared_memory &&rref) {
#if _WIN32
            mapping_ = rref.mapping_;
            rref.mapping_ = INVALID_HANDLE_VALUE;
#endif
            handle_ = rref.handle_;
            rref.handle_ = MAP_FAILED;

            cb_ = rref.cb_;
            rref.cb_ = -1;

            if (rref.filename()) {
                toolkit::posix_strcpy(file_, cchof(file_), rref.file_);
                memset(rref.file_, 0, cchof(rref.file_));
            } else {
                memset(file_, 0, cchof(file_));
            }
        }

        shared_memory::~shared_memory() {
            close();
        }

        shared_memory &shared_memory::operator=(const shared_memory &lref) {
            cb_ = lref.cb_;
            if (lref.filename()) {
                toolkit::posix_strcpy(file_, cchof(file_), lref.file_);
            } else {
                memset(file_, 0, cchof(file_));
            }

#if _WIN32
            if (lref.mapping_ != INVALID_HANDLE_VALUE) {
                if (!DuplicateHandle(GetCurrentProcess(), lref.mapping_,
                        GetCurrentProcess(), &mapping_,
                        0, FALSE, DUPLICATE_SAME_ACCESS)) {
                    throw GetLastError();
                }
            }

            handle_ = MapViewOfFile(mapping_, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, cb_);
            if (!handle_) {
                CloseHandle(mapping_);
                mapping_ = INVALID_HANDLE_VALUE;
                throw GetLastError();
            }
#else
            handle_ = lref.handle_;
#endif
            return *this;
        }

        shared_memory &shared_memory::operator=(shared_memory &&rref) {
#if _WIN32
            mapping_ = rref.mapping_;
            rref.mapping_ = INVALID_HANDLE_VALUE;
#endif
            handle_ = rref.handle_;
            rref.handle_ = MAP_FAILED;

            cb_ = rref.cb_;
            rref.cb_ = -1;

            if (rref.filename()) {
                toolkit::posix_strcpy(file_, cchof(file_), rref.file_);
                memset(rref.file_, 0, cchof(rref.file_));
            } else {
                memset(file_, 0, cchof(file_));
            }

            return *this;
        }

        shared_memory::operator void *() {
            return handle_;
        }

        shared_memory::operator const void *() const {
            return handle_;
        }

        int shared_memory::create(int cb, const char *file) {
            if ((cb <= 0) || (MAP_FAILED != handle_) || (0 != (cb % PAGE_SIZE))) {
                return -1;
            }

            cb_ = cb;
#if _WIN32
            // 确定共享名字
            char shared_name[MAX_PATH];
            toolkit::posix_strcpy(shared_name, cchof(shared_name), "Global\\nspshm_");
            if (file) {
                // 不对指定名字的映射作ID处理
                auto symb = ::strrchr(file, '\\');
                if (!symb) {
                    toolkit::posix_strcat(shared_name, cchof(shared_name), file);
                } else {
                    toolkit::posix_strcat(shared_name, cchof(shared_name), symb);
                }
                toolkit::posix_strcpy(file_, cchof(file_), shared_name);
            } else {
                ::posix__sprintf(shared_name, cchof(shared_name), "%s%d", shared_name, ++shared_memory_name_key);
                toolkit::posix_strcpy(file_, cchof(file_), shared_name);
                mapping_ = (void *) CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, cb_, shared_name);
                if (!mapping_) {
                    int err = GetLastError();
                    return -1;
                }

                handle_ = MapViewOfFile(mapping_, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, cb_);
                if (!handle_) {
                    CloseHandle(mapping_);
                    mapping_ = INVALID_HANDLE_VALUE;
                    return -1;
                }

                return 0;
            }

            HANDLE fd = CreateFileA(file_, GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            while (INVALID_HANDLE_VALUE == fd) {
                if (ERROR_FILE_NOT_FOUND == GetLastError()) {
                    fd = CreateFileA(file_, GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
                    if (INVALID_HANDLE_VALUE != fd) {
                        try {
                            char *init_buff = new char[cb_];
                            memset(init_buff, 0, cb_);
                            if (::posix__write_file((int) fd, init_buff, sizeof ( init_buff)) < 0) {
                                CloseHandle(fd);
                                delete[]init_buff;
                                return -1;
                            }
                            delete[]init_buff;
                            break;
                        } catch (...) {
                            CloseHandle(fd);
                            return -1;
                        }
                        break;
                    }
                }
                return -1;
            }

            mapping_ = (void *) CreateFileMappingA(fd, NULL, PAGE_READWRITE, 0, cb_, shared_name);
            if (!mapping_) {
                return -1;
            }

            handle_ = MapViewOfFile(mapping_, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, cb_);
            if (!handle_) {
                CloseHandle(mapping_);
                mapping_ = INVALID_HANDLE_VALUE;
                return -1;
            }

            return 0;
#else
            if (!file) {
                toolkit::posix_strcpy(file_, cchof(file_), "/dev/zero");
            } else {
                toolkit::posix_strcpy(file_, cchof(file_), file);
            }

            int fd = ::open(file_, O_EXCL | O_RDWR);
            while (fd < 0) {
                if (ENOENT == errno) {
                    fd = ::open(file_, O_CREAT | O_RDWR, S_IRWXU);
                    if (fd > 0) {
                        try {
                            char *init_buff = new char[cb_];
                            bzero(init_buff, cb_);
                            if (posix__write_file(fd, init_buff, sizeof (init_buff)) < 0) {
                                ::close(fd);
                                delete []init_buff;
                                return -1;
                            }
                            delete []init_buff;
                            break;
                        } catch (...) {
                            ::close(fd);
                            return -1;
                        }
                        break;
                    }
                }
                return -1;
            }

            handle_ = mmap(NULL, cb_, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            if (handle_ == MAP_FAILED) {
                ::close(fd);
                return -1;
            }

            ::close(fd);

#endif
            return 0;
        }

        int shared_memory::open(int cb, const char *file) {
            if ((cb <= 0) || (MAP_FAILED != handle_) || (0 != (cb % PAGE_SIZE))) {
                return -1;
            }

            cb_ = cb;
#if _WIN32
            // 打开阶段， 共享名必须指定
            if ((INVALID_HANDLE_VALUE != mapping_) || !file) {
                return -1;
            }
            toolkit::posix_strcpy(file_, cchof(file_), file);

            // 打开共享节
            mapping_ = OpenFileMappingA(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, file_);
            if (INVALID_HANDLE_VALUE == mapping_) {
                return -1;
            }

            handle_ = MapViewOfFile(mapping_, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, cb_);
            if (!handle_) {
                CloseHandle(mapping_);
                mapping_ = INVALID_HANDLE_VALUE;
                return -1;
            }

            return 0;
#else
            return create(cb, file);
#endif
        }

        int shared_memory::read(char *buffer, int cblen, int offset) {
            if ((MAP_FAILED == handle_) || !buffer || (cblen <= 0) || (offset + cblen > cb_)) {
                return -1;
            }
            memcpy(buffer, ((char *) handle_ + offset), cblen);
            return 0;
        }

        int shared_memory::write(const char *buffer, int cblen, int offset) {
            if ((MAP_FAILED == handle_) || !buffer || (cblen <= 0) || (offset + cblen > cb_)) {
                return -1;
            }
            memcpy(((char *) handle_ + offset), buffer, cblen);
            return 0;
        }

        void shared_memory::close() {
#if _WIN32
            if (handle_) {
                UnmapViewOfFile(handle_);
                handle_ = MAP_FAILED;
            }
            if (INVALID_HANDLE_VALUE != mapping_) {
                CloseHandle(mapping_);
                mapping_ = INVALID_HANDLE_VALUE;
            }
#else
            if (handle_ != MAP_FAILED) {
                munmap(handle_, cb_);
                cb_ = -1;
                handle_ = MAP_FAILED;
            }
#endif
        }

        const char *const shared_memory::filename() const {
            return ( (0 != file_[0]) ? file_ : nullptr);
        }

        const char *const shared_memory::shared_name() const {
            return filename();
        }

        int shared_memory::getsize() const {
            return cb_;
        }
    }
}