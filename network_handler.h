#if !defined SWNET_IMPL_NETWORK_OBJECTS
#define SWNET_IMPL_NETWORK_OBJECTS

#if !defined WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <string>
#include <functional>
#include <atomic>
#include <memory>

#include "endpoint.h"
#include "icom/nisdef.h"

namespace nsp {
    namespace tcpip {

        class obtcp : public std::enable_shared_from_this<obtcp> {
        public:
            obtcp();
            obtcp(const HTCPLINK lnk);
            virtual ~obtcp();

        public:
            int create();
            int create(const char *epstr);
            int create(const endpoint &ep);
            std::weak_ptr<obtcp> attach();
            void close();
            int connect(const char *epstr);
            int connect(const endpoint &ep);
            int connect2(const char *epstr);
            int connect2(const endpoint &ep);
            int listen();
            int send(int cb, const std::function<int( void *, int) > &fill);
            int send(const std::string &buffer);
            int send(const void *data, int cb);
            const endpoint &local() const;
            const endpoint &remote() const;

            template<class T>
            void settst() {
                tst_.cb_ = T::Length();
                tst_.builder_ = &T::builder;
                tst_.parser_ = &T::parser;
                settst(&tst_);
            }

            void setlnk(const HTCPLINK lnk);

            void on_recvdata(const char *data, const int cb);
            void on_accepted(HTCPLINK srv, HTCPLINK client);
            void on_closed();
            void on_debug_output(const char *info);
            void on_connected2();

            virtual void on_connected();
            virtual void bind_object(const std::shared_ptr<obtcp> &object);
            virtual void on_pre_close();

        protected:
            virtual void on_closed(HTCPLINK previous);
            virtual void on_recvdata(const std::string &pkt);
            virtual void on_accepted(HTCPLINK lnk);
            virtual void on_lowlevel_debug(const char *info);

            std::atomic<HTCPLINK> lnk_{INVALID_HTCPLINK};
            endpoint remote_;
            endpoint local_;
            tst_t tst_;

        private:
            void settst(tst_t *tst);
            obtcp(const obtcp &rf) = delete;
            obtcp(obtcp &&) = delete;
            obtcp &operator=(const obtcp &) = delete;
        };

        class obudp : public std::enable_shared_from_this<obudp> {
        public:
            obudp();
            virtual ~obudp();

        public:
            int create(const int flag = UDP_FLAG_NONE);
            int create(const endpoint &ep, const int flag = UDP_FLAG_NONE);
            int create(const char *epstr, const int flag = UDP_FLAG_NONE);
            void close();
            int sendto(int cb, const std::function<int( void *, int) > &fill, const endpoint &ep);
            int sendto(const std::string &buffer, const endpoint &ep);
            int sendto(const char *data, int cb, const endpoint &ep);

            const endpoint &local() const;
            void setlnk(const HUDPLINK lnk);
            void on_recvdata(const char *data, const int cb, const char *ipaddr, const port_t port);
            void on_closed();
            void on_debug_output(const char *info);

            virtual void on_pre_close();
               
        protected:
            std::atomic<HUDPLINK> lnk_{INVALID_HUDPLINK};
            endpoint local_;

            virtual void on_recvdata(const std::string &data, const endpoint &r_ep);
            virtual void on_closed(HUDPLINK previous);
            virtual void on_lowlevel_debug(const char *info);
            
        private:
            obudp(const obudp &) = delete;
            obudp(obudp &&) = delete;
            obudp &operator=(const obudp &) = delete;
        };

    } // namespace tcpip
} // namespace nsp

#endif // !SWNET_IMPL_NETWORK_OBJECTS