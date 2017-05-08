
#include <memory>
#include <mutex>
#include <unordered_map>
#include <cstdlib>
#include <cstring>
#include <map>

#include "network_handler.h"
#include "swnet.h"

namespace nsp {
    namespace tcpip {
        ///////////////////////////////////////		TCP 部分 ///////////////////////////////////////
        static std::atomic_long __tcp_refcnt{ 0}; // GCC4.8中， "= 0"这样的代码被视为废弃

        obtcp::obtcp() {
            if (1 == ++__tcp_refcnt) {
                toolkit::singleton<swnet>::instance()->tcp_init();
            } else {
                --__tcp_refcnt;
            }
        }

        obtcp::obtcp(const HTCPLINK lnk) {
            lnk_ = lnk;
        }

        obtcp::~obtcp() {
            close();
        }

        void obtcp::settst(tst_t *tst) {
            if (INVALID_HTCPLINK != lnk_ && tst) {
                toolkit::singleton<swnet>::instance()->tcp_settst(lnk_, tst);
            }
        }

        int obtcp::create(const char *epstr) {
            if (epstr) {
                endpoint ep;
                if (endpoint::build(epstr, ep) >= 0) {
                    return create(ep);
                }
            }
            return -1;
        }

        int obtcp::create(const endpoint &ep) {
            if (INVALID_HTCPLINK != lnk_) {
                return -1;
            }

            std::string ipstr = ep.ipv4();
            try {
                if (nsp::toolkit::singleton<swnet>::instance()->tcp_create(shared_from_this(), ipstr.size() > 0 ? ipstr.c_str() : nullptr, ep.port()) < 0) {
                    return -1;
                }
            } catch (std::bad_weak_ptr &e) {
                //loerr << "obtcp must be a shared_ptr object." << e.what();
                return -1;
            } catch (...) {
                return -1;
            }

            toolkit::singleton<swnet>::instance()->tcp_settst(lnk_, &tst_);
            return 0;
        }

        int obtcp::create() {
            return create(endpoint("", 0));
        }

        std::weak_ptr<obtcp> obtcp::attach() {
            try {
                auto sptr = shared_from_this();
                if (toolkit::singleton<swnet>::instance()->tcp_attach(lnk_, sptr) < 0) {
                    return std::weak_ptr<obtcp>();
                }

                // 客户连接, 需要拿到对端和本地的地址信息
                uint32_t actip;
                port_t actport;
                toolkit::singleton<swnet>::instance()->tcp_getaddr(lnk_, LINK_ADDR_REMOTE, &actip, &actport);
                remote_.ipv4(actip);
                remote_.port(actport);

                toolkit::singleton<swnet>::instance()->tcp_getaddr(lnk_, LINK_ADDR_LOCAL, &actip, &actport);
                local_.ipv4(actip);
                local_.port(actport);

                return std::weak_ptr<obtcp>(sptr);
            } catch (std::bad_weak_ptr &e) {
                //loerr << "failed to attach tcp object to swnet." << e.what();
            } catch (...) {
                ;
            }

            return std::weak_ptr<obtcp>();
        }

        void obtcp::close() {
            if (INVALID_HTCPLINK != lnk_) {
                toolkit::singleton<swnet>::instance()->tcp_destroy(lnk_);
            }
        }

        int obtcp::connect(const char *epstr) {
            if (INVALID_HTCPLINK == lnk_) {
                return -1;
            }

            if (epstr) {
                endpoint ep;
                if (endpoint::build(epstr, ep) >= 0) {
                    return connect(ep);
                }
            }
            return -1;
        }

        int obtcp::connect(const endpoint &ep) {
            if (INVALID_HTCPLINK == lnk_) {
                return -1;
            }

            std::string ipstr = ep.ipv4();
            port_t port = ep.port();
            if (ipstr.length() <= 0 && port <= 0) {
                return -1;
            }
            if (toolkit::singleton<swnet>::instance()->tcp_connect(lnk_, ipstr.c_str(), port) < 0) {
                return -1;
            }

            // 成功连接后, 可以取出对端和本地的地址信息
            uint32_t actip;
            port_t actport;
            toolkit::singleton<swnet>::instance()->tcp_getaddr(lnk_, LINK_ADDR_REMOTE, &actip, &actport);
            remote_.ipv4(actip);
            remote_.port(actport);
            toolkit::singleton<swnet>::instance()->tcp_getaddr(lnk_, LINK_ADDR_LOCAL, &actip, &actport);
            local_.ipv4(actip);
            local_.port(actport);
            return 0;
        }

        int obtcp::listen() {
            if (INVALID_HTCPLINK == lnk_) {
                return -1;
            }

            if (toolkit::singleton<swnet>::instance()->tcp_listen(lnk_, 5) < 0) {
                return -1;
            }

            // 开始监听后, 肯定不会再出现对端地址
            // 并取出本地地址
            remote_.ipv4("0.0.0.0");
            remote_.port(0);
            uint32_t actip;
            port_t actport;
            toolkit::singleton<swnet>::instance()->tcp_getaddr(lnk_, LINK_ADDR_LOCAL, &actip, &actport);
            local_.ipv4(actip);
            local_.port(actport);
            return 0;
        }

        // 发送例程

        int obtcp::send(int cb, const std::function<int( void *, int) > &fill) {
            if (cb <= 0 || INVALID_HTCPLINK == lnk_ || !fill) {
                return -1;
            }

            return toolkit::singleton<swnet>::instance()->tcp_write(lnk_, cb, [] (void *data, int cb, void *par)->int {
                const std::function<int( void *, int) > *usr_fill = (const std::function<int( void *, int) > *)par;
                return ( *usr_fill)(data, cb);
            }, (void *) &fill);
        }

        int obtcp::send(const std::string &buffer) {
            return send((int) buffer.size(), [&] (void *pkt, int cb) ->int {
                memcpy(pkt, buffer.data(), cb);
                return 0;
            });
        }

        int obtcp::send(const void *data, int cb) {
            return send(cb, [&] (void *pkt, int cb) ->int {
                memcpy(pkt, data, cb);
                return 0;
            });
        }

        const endpoint &obtcp::local() const {
            return local_;
        }

        const endpoint &obtcp::remote() const {
            return remote_;
        }

        void obtcp::on_closed(HTCPLINK previous) {
        }

        void obtcp::on_closed() {
            HTCPLINK previous = lnk_.exchange(INVALID_HTCPLINK);
            if (INVALID_HTCPLINK != previous) {
                toolkit::singleton<swnet>::instance()->tcp_detach(previous);
                on_closed(previous);
            }
        }

        void obtcp::on_recvdata(const std::string &pkt) {
        }

        void obtcp::on_recvdata(const char *data, const int cb) {
            if (data && cb > 0) {
                on_recvdata(std::string(data, cb));
            }
        }

        void obtcp::on_accepted(HTCPLINK lnk) {
        }

        void obtcp::on_accepted(HTCPLINK srv, HTCPLINK client) {
            if (INVALID_HTCPLINK != srv && INVALID_HTCPLINK != client && srv == lnk_) {
                on_accepted(client);
            }
        }

        int obtcp::on_connected() {
            return 0;
        }

        void obtcp::bind_object(const std::shared_ptr<obtcp> &object) {
        }

        void obtcp::setlnk(const HTCPLINK lnk) {
            lnk_ = lnk;
        }

        ///////////////////////////////////////		UDP 部分 ///////////////////////////////////////
        static int __udp_refcnt = 0;

        obudp::obudp() {
            if (1 == ++__udp_refcnt) {
                toolkit::singleton<swnet>::instance()->udp_init();
            } else {
                --__udp_refcnt;
            }
        }

        obudp::~obudp() {
            close();
        }

        int obudp::create(const int flag) {
            return create(endpoint("", 0), flag);
        }

        int obudp::create(const endpoint &ep, const int flag) {
            std::string ipstr = ep.ipv4();
            port_t port = ep.port();

            if (INVALID_HUDPLINK != lnk_) {
                return -1;
            }

            try {
                if (toolkit::singleton<swnet>::instance()->udp_create(shared_from_this(), ipstr.size() > 0 ? ipstr.c_str() : nullptr, port, flag) < 0) {
                    return -1;
                }
            } catch (std::bad_weak_ptr &e) {
                //loerr << "failed to create udp object." << e.what();
                return -1;
            } catch (...) {
                return -1;
            }

            uint32_t actip;
            port_t actport;
            toolkit::singleton<swnet>::instance()->udp_getaddr(lnk_, &actip, &actport);
            local_.ipv4(actip);
            local_.port(actport);
            return 0;
        }

        int obudp::create(const char *epstr, const int flag) {
            if (epstr) {
                endpoint ep;
                if (endpoint::build(epstr, ep) >= 0) {
                    return create(ep, flag);
                }
            }
            return -1;
        }

        void obudp::close() {
            if (INVALID_HUDPLINK != lnk_) {
                toolkit::singleton<swnet>::instance()->udp_destroy(lnk_);
            }
        }

        int obudp::sendto(int cb, const std::function<int( void *, int) > &fill, const endpoint &ep) {
            return toolkit::singleton<swnet>::instance()->udp_write(lnk_, cb, [] (void *pkt, int cb, void *par)->int {
                std::function<int( void *, int) > *fill = (std::function<int( void *, int) > *)par;
                return ( *fill)(pkt, cb);
            }, (void *) &fill, ep.ipv4(), ep.port());
        }

        int obudp::sendto(const std::string &buffer, const endpoint &ep) {
            return sendto(buffer.data(), (int) buffer.size(), ep);
        }

        int obudp::sendto(const char *data, int cb, const endpoint &ep) {
            return sendto(cb, [&] (void *pkt, int cb) ->int {
                memcpy(pkt, data, cb);
                return 0;
            }, ep);
        }

        const endpoint &obudp::local() const {
            return local_;
        }

        void obudp::on_recvdata(const std::string &data, const endpoint &r_ep) {
        }

        void obudp::on_recvdata(const char *data, const int cb, const char *ipaddr, const port_t port) {
            if (INVALID_HUDPLINK != lnk_ && data && cb > 0 && ipaddr && port > 0) {
                on_recvdata(std::string(data, cb), endpoint(ipaddr, port));
            }
        }

        void obudp::on_closed(HUDPLINK previous) {
        }

        void obudp::on_closed() {
            HUDPLINK previous = lnk_.exchange(INVALID_HUDPLINK);
            if (INVALID_HUDPLINK != previous) {
                toolkit::singleton<swnet>::instance()->udp_detach(previous);
                on_closed(previous);
            }
        }

        void obudp::setlnk(const HUDPLINK lnk) {
            lnk_ = lnk;
        }

    } // namespace tcpip
} // namespace nsp