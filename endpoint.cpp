#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "endpoint.h"
#include "toolkit.h"
#include "os_util.hpp"
#include "swnet.h"

namespace nsp {
    namespace tcpip {

        //////////////////////////////////////////////// IP check ////////////////////////////////////////////////

        int endpoint::parse_ep(const std::string & epstr, std::string &ipv4, port_t &port) {
            static const std::string delim = ":";

            // 区分开ip和port
            std::string hoststr;
            std::string portstr;
            std::size_t loc = 0, offset = 0;

            // 找不到有效的端口分割符
            loc = epstr.find(delim, offset);
            if ((0 == loc) || (std::string::npos == loc)) {
                return -1;
            }
            hoststr = epstr.substr(offset, loc - offset);
            offset = loc + 1;
            portstr = epstr.substr(offset);

            // 必须要有host域
            if (0 == hoststr.length()) {
                return -1;
            }

            // 端口必须保证有效
            if (toolkit::is_valid_portstr(portstr, port) < 0) {
                return -1;
            }

            // 允许解析域名/主机名
            if (isalpha(hoststr.at(0))) {
                uint32_t ip;

                // "localhost" 可以得到正确解析
                if (toolkit::singleton<tcpip::swnet>::instance()->nis_gethost(hoststr.c_str(), &ip) < 0) {
                    return -1;
                }
                char iptxt[16] = {0};
                toolkit::ipv4_tostring(ip, iptxt, cchof(iptxt));
                if (0 == iptxt[0]) {
                    return -1;
                }
                ipv4.assign(iptxt);
            } else {
                // 对IP字符串和端口进行有效性判定
                // 数字的网络地址， 不作 gethostbyaddr 有效性判断
                if (toolkit::is_effective_ipv4str(hoststr) < 0) {
                    return -1;
                }
                ipv4 = hoststr;
            }

            return 0;
        }

        endpoint::endpoint() {
            ipv4(0);
            port(INVALID_TCPIP_PORT_NUMBER);
        }

        endpoint::endpoint(const char *ipstr, const port_t po) {
            ipv4(std::string().assign(ipstr ? ipstr : ""));
            port(po);
        }

        endpoint::endpoint(const endpoint &rf) {
            if (&rf != this) {
                toolkit::posix_strcpy(ipstr_, cchof(ipstr_), rf.ipstr_);
                address_ = rf.address_;
                port_ = rf.port_;
            }
        }

        endpoint::endpoint(endpoint &&rf) {
            toolkit::posix_strcpy(ipstr_, cchof(ipstr_), rf.ipstr_);
            address_ = rf.address_;
            port_ = rf.port_;
        }

        endpoint &endpoint::operator=(endpoint &&rf) {
            toolkit::posix_strcpy(ipstr_, cchof(ipstr_), rf.ipstr_);
            address_ = rf.address_;
            port_ = rf.port_;
            return *this;
        }

        endpoint &endpoint::operator=(const endpoint &rf) {
            if (&rf != this) {
                ipv4(rf.ipv4());
                port(rf.port());
            }
            return *this;
        }

        bool endpoint::operator==(const endpoint &rf) const {
            if (&rf != this) {
                return ( (0 == strcmp(ipstr_, rf.ipv4())) && address_ == rf.address_ && port_ == rf.port());
            } else {
                return true;
            }
        }

        bool endpoint::operator<(const endpoint &rf) const {
            if (address_ < rf.address_) {
                return true;
            } else {
                if (address_ > rf.address_) {
                    return false;
                } else {
                    return port_ < rf.port_;
                }
            }
        }

        endpoint::operator bool() const {
            return ( (0 != address_) ? (true) : (INVALID_TCPIP_PORT_NUMBER != port_));
        }

        const bool endpoint::connectable() const {
            return ( (0 != address_) && (BOARDCAST_TCPIP_ADDRESS != address_) && (INVALID_TCPIP_PORT_NUMBER != port_));
        }

        const bool endpoint::bindable() const {
            return ( (BOARDCAST_TCPIP_ADDRESS != address_) && (INVALID_TCPIP_PORT_NUMBER != port_));
        }

        const bool endpoint::manual() const {
            return ( MANUAL_NOTIFY_TARGET == address_);
        }

        const char *endpoint::ipv4() const {
            return &ipstr_[0];
        }

        const u32_ipv4_t endpoint::ipv4_uint32() const {
            return address_;
        }

        void endpoint::ipv4(const std::string &ipstr) {
            if (ipstr.length() > 0) {
                toolkit::posix_strcpy(ipstr_, cchof(ipstr_), ipstr.c_str());
                address_ = toolkit::ipv4_touint(ipstr_, kByteOrder_LittleEndian);
                return;
            }
            memset(ipstr_, 0, sizeof ( ipstr_));
            address_ = 0;
        }

        void endpoint::ipv4(const char *ipstr, int cpcch) {
            if (ipstr && cpcch > 0) {
                toolkit::posix_strncpy(ipstr_, cchof(ipstr_), ipstr, cpcch);
                address_ = toolkit::ipv4_touint(ipstr_, kByteOrder_LittleEndian);
            }
        }

        void endpoint::ipv4(const u32_ipv4_t uint32_address) {
            address_ = uint32_address;
            if (uint32_address > 0) {
                char buffer[16] = {0};
                std::string ipv4str = toolkit::ipv4_tostring(address_, buffer, sizeof ( buffer));
                if (ipv4str.length()) {
                    toolkit::posix_strcpy(ipstr_, cchof(ipstr_), ipv4str.c_str());
                }
            } else {
                memset(ipstr_, 0, sizeof ( ipstr_));
            }
        }

        const port_t endpoint::port() const {
            return port_;
        }

        void endpoint::port(const port_t po) {
            port_ = po;
        }

        const std::string endpoint::to_string() const {
            std::string epstr;
            epstr += ipstr_;
            epstr += ":";
            epstr += toolkit::to_string<char>(port_);
            return epstr;
        }

        int endpoint::build(const std::string &epstr, endpoint &ep) {
            std::string ipstr;
            port_t port;
            if (endpoint::parse_ep(epstr, ipstr, port) < 0) {
                return -1;
            }
            ep.ipv4(ipstr);
            ep.port(port);
            return 0;
        }

        endpoint endpoint::boardcast(const port_t po) {
            if (0 == po || INVALID_TCPIP_PORT_NUMBER == po) {
                return endpoint("0.0.0.0", INVALID_TCPIP_PORT_NUMBER);
            } else {
                return endpoint("255.255.255.255", po);
            }
        }

        void endpoint::disable() {
            ipv4((uint32_t) 0);
            port(INVALID_TCPIP_PORT_NUMBER);
        }

    } // namespace tcpip
} // nsp