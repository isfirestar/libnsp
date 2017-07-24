#include <mutex>
#include <vector>

#include "singleton.hpp"
#include "log.h"

#include "icom/logger.h"
#include "icom/posix_string.h"

#ifdef _WIN32
#define snprintf(buffer, size, ...) \
    _snprintf_s(buffer, size, _TRUNCATE, __VA_ARGS__)
#endif

namespace nsp {
    namespace toolkit {
        namespace xlog {
            /////////////////////// loex ///////////////////////

            loex::loex(enum log__levels level) : level_(level) {
                posix__strcpy(module_, cchof(module_), "nsp");
                str_[0] = 0;
            }

            loex::loex(const char *module, enum log__levels level) : level_(level) {
                if (module) {
                    posix__strcpy(module_, cchof(module_), module);
                }
                str_[0] = 0;
            }

            loex::~loex() {
                if (0 != str_[0]) { // 以此限制设置日志分片的对象，析构阶段不会真实调用日志输出
                    log__write(module_, level_, kLogTarget_Filesystem | kLogTarget_Stdout, "%s", str_);
                }
            }

            loex &loex::operator<<(const wchar_t *str) {
                if (str) {
                    ::posix__sprintf(str_, sizeof ( str_) - strlen(str_), "%s%ws", str_, str);
                }
                return *this;
            }

            loex &loex::operator<<(const char *str) {
                if (str) {
                    ::posix__sprintf(str_, sizeof ( str_) - strlen(str_), "%s%s", str_, str);
                }
                return *this;
            }

            loex &loex::operator<<(int32_t n) {
                ::posix__sprintf(str_, sizeof ( str_) - strlen(str_), "%s%d", str_, n);
                return *this;
            }

            loex &loex::operator<<(uint32_t n) {
                ::posix__sprintf(str_, sizeof ( str_) - strlen(str_), "%s%u", str_, n);
                return *this;
            }

            loex &loex::operator<<(int16_t n) {
                ::posix__sprintf(str_, sizeof ( str_) - strlen(str_), "%s%d", str_, n);
                return *this;
            }

            loex &loex::operator<<(uint16_t n) {
                ::posix__sprintf(str_, sizeof ( str_) - strlen(str_), "%s%u", str_, n);
                return *this;
            }

            loex &loex::operator<<(int64_t n) {
#if _WIN32
                ::posix__sprintf(str_, sizeof ( str_) - strlen(str_), "%s%I64d", str_, n);
#else
                ::posix__sprintf(str_, sizeof ( str_) - strlen(str_), "%s%lld", str_, n);
#endif
                return *this;
            }

            loex &loex::operator<<(uint64_t n) {
#if _WIN32
                ::posix__sprintf(str_, sizeof ( str_) - strlen(str_), "%s%I64u", str_, n);
#else
                ::posix__sprintf(str_, sizeof ( str_) - strlen(str_), "%s%llu", str_, n);
#endif
                return *this;
            }

            loex &loex::operator<<(const std::basic_string<char> &str) {
                if (str.size() > 0) {
                    ::posix__sprintf(str_, sizeof ( str_) - strlen(str_), "%s%s", str_, str.c_str());
                }
                return *this;
            }

            loex &loex::operator<<(void *ptr) {
                ::posix__sprintf(str_, sizeof ( str_) - strlen(str_), "%s%p", str_, ptr);
                return *this;
            }

            loex &loex::operator<<(void **ptr) {
                ::posix__sprintf(str_, sizeof ( str_) - strlen(str_), "%s%p", str_, ptr);
                return *this;
            }

            loex &loex::operator<<(const hex &ob) {
                ::posix__sprintf(str_, sizeof ( str_) - strlen(str_), "%s0x%08X", str_, ob.__auto_t);
                return *this;
            }

            loex &loex::operator<<(float f) {
                ::posix__sprintf(str_, sizeof ( str_) - strlen(str_), "%s%g", str_, f);
                return *this;
            }

            loex &loex::operator<<(double lf) {
                ::posix__sprintf(str_, sizeof ( str_) - strlen(str_), "%s%g", str_, lf);
                return *this;
            }

            //			loex &loex::operator << ( const std::_Smanip<std::streamsize> &sp )
            //			{
            //				strsize_ = sp._Manarg;
            //				return *this;
            //			}

        } // xlog
    } // toolkit
} // namespace nsp
