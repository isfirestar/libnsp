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
                    ::log__save(module_, level_, kLogTarget_Filesystem | kLogTarget_Stdout, "%s", str_);
                }
            }

            loex &loex::operator<<(const wchar_t *str) {
                if (str) {
                    ::posix__sprintf(&str_[strlen(str_)], sizeof ( str_) - strlen(str_), "%ws", str);
                }
                return *this;
            }

            loex &loex::operator<<(const char *str) {
                if (str) {
                    ::posix__sprintf(&str_[strlen(str_)], sizeof ( str_) - strlen(str_), "%s", str);
                }
                return *this;
            }

            loex &loex::operator<<(int32_t n) {
                ::posix__sprintf(&str_[strlen(str_)], sizeof ( str_) - strlen(str_), "%d", n);
                return *this;
            }

            loex &loex::operator<<(uint32_t n) {
                ::posix__sprintf(&str_[strlen(str_)], sizeof ( str_) - strlen(str_), "%u", n);
                return *this;
            }

            loex &loex::operator<<(int16_t n) {
                ::posix__sprintf(&str_[strlen(str_)], sizeof ( str_) - strlen(str_), "%d", n);
                return *this;
            }

            loex &loex::operator<<(uint16_t n) {
                ::posix__sprintf(&str_[strlen(str_)], sizeof ( str_) - strlen(str_), "%u", n);
                return *this;
            }

            loex &loex::operator<<(int64_t n) {
#if _WIN32
                ::posix__sprintf(&str_[strlen(str_)], sizeof ( str_) - strlen(str_), "%I64d", n);
#else
                ::posix__sprintf(&str_[strlen(str_)], sizeof ( str_) - strlen(str_), "%lld", n);
#endif
                return *this;
            }

            loex &loex::operator<<(uint64_t n) {
#if _WIN32
                ::posix__sprintf(&str_[strlen(str_)], sizeof ( str_) - strlen(str_), "%I64u", n);
#else
                ::posix__sprintf(&str_[strlen(str_)], sizeof ( str_) - strlen(str_), "%llu", n);
#endif
                return *this;
            }

            loex &loex::operator<<(const std::basic_string<char> &str) {
                if (str.size() > 0) {
                    ::posix__sprintf(&str_[strlen(str_)], sizeof ( str_) - strlen(str_), "%s", str.c_str());
                }
                return *this;
            }

            loex &loex::operator<<(void *ptr) {
                ::posix__sprintf(&str_[strlen(str_)], sizeof ( str_) - strlen(str_), "%p", ptr);
                return *this;
            }

            loex &loex::operator<<(void **ptr) {
                ::posix__sprintf(&str_[strlen(str_)], sizeof ( str_) - strlen(str_), "%p", ptr);
                return *this;
            }

            loex &loex::operator<<(const hex &ob) {
                ::posix__sprintf(&str_[strlen(str_)], sizeof ( str_) - strlen(str_), "%08X", ob.__auto_t);
                return *this;
            }

            loex &loex::operator<<(float f) {
                ::posix__sprintf(&str_[strlen(str_)], sizeof ( str_) - strlen(str_), "%g", f);
                return *this;
            }

            loex &loex::operator<<(double lf) {
                ::posix__sprintf(&str_[strlen(str_)], sizeof ( str_) - strlen(str_), "%g", lf);
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
