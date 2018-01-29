#pragma once

#include <memory>
#include <map>
#include <vector>
#include <mutex>

#include "network_handler.h"
#include "serialize.hpp"
#include "log.h"
#include "old.hpp"

namespace nsp {
    namespace tcpip {

        template<class T>
        class tcp_application_service : public obtcp {
            tcp_application_service(HTCPLINK lnk) = delete; // 作为监听对象， 不存在使用链接构造这一说法

            // 下层不再需要关注链接建立的任何细节, 这个虚函数到此终止
            virtual void on_accepted(HTCPLINK lnk) override final {

                std::shared_ptr<T> sptr = std::make_shared<T>(lnk);
                std::weak_ptr<obtcp> wptr = sptr->attach();
                if (wptr.expired()) {
                    return;
                }

                try {
                    sptr->bind_object(shared_from_this());
                    
                    // 服务端有权在会话初次到达阶段拒绝连接
                    if (sptr->on_established() < 0){
                        throw -ENETRESET;
                    }
                } catch (...) {
                    sptr->close();
                    return;
                }

                std::lock_guard < decltype(client_locker_) > guard(client_locker_);
                auto iter = client_set_.find(lnk);
                if (client_set_.end() == iter) {
                    client_set_[lnk] = wptr;
                }
            }

            // 作为监听对象, 一定不会出现收到实际数据包的情况
            virtual void on_recvdata(const std::string &data) override final {
                abort();
            }

            std::map<HTCPLINK, std::weak_ptr<obtcp>> client_set_;
            mutable std::recursive_mutex client_locker_;

        public:

            tcp_application_service() : obtcp() {
				;
            }

            ~tcp_application_service() {
                close();
            }

            // 开始服务
            int begin(const endpoint &ep) {
                return ( (create(ep) >= 0) ? listen() : -1);
            }

            int begin(const char *epstr) {
                if (!epstr)
                    return -1;
                endpoint ep;
                if (endpoint::build(epstr, ep) < 0) {
                    return -1;
                }
                return begin(ep);
            }

            // 客户端关闭后通知服务端
            void on_client_closed(const HTCPLINK lnk) {
                std::lock_guard < decltype(client_locker_) > guard(client_locker_);
                auto iter = client_set_.find(lnk);
                if (client_set_.end() != iter) {
                    client_set_.erase(iter);
                }
            }

            int notify_one(HTCPLINK lnk, const std::function<int( const std::shared_ptr<T> &client)> &todo) {
                std::shared_ptr<T> client;
                {
                    std::lock_guard < decltype(client_locker_) > guard(client_locker_);
                    auto iter = client_set_.find(lnk);
                    if (client_set_.end() == iter) {
                        return -1;
                    }

                    if (iter->second.expired()) {
                        return -1;
                    }

                    auto obptr = iter->second.lock();
                    if (!obptr) {
                        client_set_.erase(iter);
                        return -1;
                    }

                    client = std::static_pointer_cast< T >(obptr);
                }

                if (todo) {
                    return todo(client);
                }
                return -1;
            }

            void notify_all(const std::function<void( const std::shared_ptr<T> &client)> &todo) {
                std::vector<std::weak_ptr < obtcp>> duplicated;
                {
                    std::lock_guard < decltype(client_locker_) > guard(client_locker_);
                    auto iter = client_set_.begin();
                    while (client_set_.end() != iter) {
                        if (iter->second.expired()) {
                            iter = client_set_.erase(iter);
                        } else {
                            duplicated.push_back(iter->second);
                            ++iter;
                        }
                    }
                }

                for (auto &iter : duplicated) {
                    auto obptr = iter.lock();
                    std::shared_ptr<T> client = std::static_pointer_cast< T >(obptr);
                    if (client && todo) {
                        todo(client);
                    }
                }

            }

            // 按链接查找一个客户端
            int search_client_by_link(const HTCPLINK lnk, std::shared_ptr<T> &client) const {
                std::lock_guard < decltype(client_locker_) > guard(client_locker_);
                auto iter = client_set_.find(lnk);
                if (client_set_.end() != iter) {
                    client = std::static_pointer_cast< T >(iter->second.lock());
                    return 0;
                } else {
                    return -1;
                }
            }
        };
    }
}

namespace nsp {
    namespace tcpip {

        /*指定底层协议类型， 定义TCP连接会话*/
        template<class T>
        class tcp_application_client : public obtcp {
            std::weak_ptr<tcp_application_service<tcp_application_client<T>>> tcp_application_server_;

        public:

            tcp_application_client() : obtcp() {
                settst<T>();
            }

            tcp_application_client(HTCPLINK lnk) : obtcp(lnk) {
                settst<T>();
            }

            virtual ~tcp_application_client() {
				;
            }

            // 如果使用 proto::proto_interface 的流化和反流化模型， 则可以直接使用这样的对象进行发包操作
            // 这种操作是框架最推荐的
            int psend(const proto::proto_interface *package) {
                if (!package) return -1;
                return obtcp::send(package->length(), [&] (void *buffer, int cb) ->int {
					if (!package->serialize((unsigned char *) buffer)) {
						return -1;
					}
					return 0;
                });
            }

            virtual void bind_object(const std::shared_ptr<obtcp> &object) override final {
                tcp_application_server_ = std::static_pointer_cast< tcp_application_service<tcp_application_client < T>> >(object);
            }
            
            // server 创建 session 后， 首次主动通知会话对象
            virtual int on_established() {
                return 0;
            }
        protected:
            // 如果服务端还在， 则通知服务端,有客户链接断开， 同时允许继续重写下行
            virtual void on_closed(HTCPLINK previous) override final {
                auto sptr = tcp_application_server_.lock();
                if (sptr) {
                    std::shared_ptr<tcp_application_service<tcp_application_client < T>>> srvptr =
                            std::static_pointer_cast< tcp_application_service<tcp_application_client < T>> >(sptr);
                    srvptr->on_client_closed(previous);
                }

                on_disconnected(previous);
            }

			// 客户端对象收到连接请求， 肯定是严重错误
            virtual void on_accepted(HTCPLINK lnk) override final {
                abort();
            } 

            virtual void on_disconnected(const HTCPLINK previous) {
				;
            }
        };

        class udp_application_client : public obudp {
        public:
            udp_application_client() : obudp() {
                ;
            }

            ~udp_application_client() {
                ;
            }

            // 类似于 tcp, 这里也提供 psend 方法支持 proto_interface 的内置流化
            int psend(const proto::proto_interface *package, const endpoint &ep) {
                if (!package) {
                    return -1;
                }

                return obudp::sendto(package->length(), [&] (void *buffer, int cb) ->int {
                    if (!package->serialize((unsigned char *) buffer)) {
                        return -1;
                    }
                    return 0;
                }, ep);
            }
        };

        typedef udp_application_client udp_application_server;

    } // namespace tcpip
} // namespace nsp