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
            tcp_application_service(HTCPLINK lnk) = delete; // ��Ϊ�������� ������ʹ�����ӹ�����һ˵��

            // �²㲻����Ҫ��ע���ӽ������κ�ϸ��, ����麯��������ֹ

            virtual void on_accepted(HTCPLINK lnk) override final {

                std::shared_ptr<T> sptr = std::make_shared<T>(lnk);
                std::weak_ptr<obtcp> wptr = sptr->attach();
                if (wptr.expired()) {
                    return;
                }

                try {
                    sptr->bind_object(shared_from_this());
                    if (sptr->on_connected() < 0) {
                        throw std::logic_error("user terminated connection.");
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

            // ��Ϊ��������, һ����������յ�ʵ�����ݰ������

            virtual void on_recvdata(const std::string &data) override final {
                abort();
            }

            std::map<HTCPLINK, std::weak_ptr<obtcp>> client_set_;
            mutable std::recursive_mutex client_locker_;

        public:

            tcp_application_service() : obtcp() {
            }

            ~tcp_application_service() {
                close();
            }

            // ��ʼ����

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

            // �ͻ��˹رպ�֪ͨ�����

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

            // �����Ӳ���һ���ͻ���

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

        /*ָ���ײ�Э�����ͣ� ����TCP���ӻỰ*/
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
            }

            // ���ʹ�� proto::proto_interface �������ͷ�����ģ�ͣ� �����ֱ��ʹ�������Ķ�����з�������
            // ���ֲ����ǿ�����Ƽ���

            int psend(const proto::proto_interface *package) {
                if (!package) return -1;
                return obtcp::send(package->length(), [&] (void *buffer, int cb) ->int {
                    return ( package->serialize((unsigned char *) buffer) < 0) ? -1 : 0;
                });
            }

            virtual void bind_object(const std::shared_ptr<obtcp> &object) override final {
                tcp_application_server_ = std::static_pointer_cast< tcp_application_service<tcp_application_client < T>> >(object);
            }
        protected:
            // �������˻��ڣ� ��֪ͨ�����,�пͻ����ӶϿ��� ͬʱ���������д����

            virtual void on_closed(HTCPLINK previous) override final {
                auto sptr = tcp_application_server_.lock();
                if (sptr) {
                    std::shared_ptr<tcp_application_service<tcp_application_client < T>>> srvptr =
                            std::static_pointer_cast< tcp_application_service<tcp_application_client < T>> >(sptr);
                    srvptr->on_client_closed(previous);
                }

                on_disconnected(previous);
            }

            virtual void on_accepted(HTCPLINK lnk) override final {
                abort();
            } // �ͻ��˶����յ��������� �϶������ش���

            virtual void on_disconnected(const HTCPLINK previous) {
            }
        };

        typedef obudp udp_application_client;
        typedef obudp udp_application_server;

    } // namespace tcpip
} // namespace nsp