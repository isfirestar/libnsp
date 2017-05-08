#pragma once

#include <string>
#include <cstdint>

#if !defined MAXIMUM_IPV4_STRLEN
#define MAXIMUM_IPV4_STRLEN     (0x10)
#endif // !MAXIMUM_IPV4_STRLEN

#if !defined MAXIMU_TCPIP_PORT_NUMBER
#define MAXIMU_TCPIP_PORT_NUMBER   (0xFFFF)
#endif // !MAXIMU_TCPIP_PORT_NUMBER

#define INVALID_TCPIP_PORT_NUMBER   (MAXIMU_TCPIP_PORT_NUMBER)

#define BOARDCAST_TCPIP_ADDRESS    (0xFFFFFFFF)
#define MANUAL_NOTIFY_TARGET    (BOARDCAST_TCPIP_ADDRESS)

#define INVALID_ENDPOINT_STD    ("0.0.0.0:65535")

namespace nsp {
    namespace tcpip {

        typedef uint16_t port_t;
        typedef uint32_t u32_ipv4_t;

        class endpoint {
        public:
            endpoint();
            endpoint(const char *ipstr, const port_t po);
            endpoint(const endpoint &rf);
            endpoint &operator=(const endpoint &rf);
            endpoint(endpoint &&rf);
            endpoint &operator=(endpoint &&rf);

        public:
            bool operator==(const endpoint &rf) const; // ���� endpoint ��Ϊ std::find ����
            bool operator<(const endpoint &rf) const; // ���� endpoint ֱ����Ϊ std::map �� KEY ֵ
            operator bool() const; // 0.0.0.0:65535 ������Ϊ����Ч��IP��ַ, 255.255.255.255:65535��Ϊ�ֶ���ַ��Ч
            const bool connectable() const; // ����Ϊ TcpConnect ����ĵ�ַ�ṹ
            const bool bindable() const; // ����Ϊ���ذ�
            const bool manual() const; // �ӳ�ȷ�������ַ��Ϣ�Ķ���

        public:
            const char *ipv4() const;
            const u32_ipv4_t ipv4_uint32() const;
            void ipv4(const u32_ipv4_t uint32_address);
            void ipv4(const std::string &ipstr); // ��Ϊ�Ѿ��� uint32 �����ĺ���, ���������� const char * ������
            void ipv4(const char *ipstr, int cpcch);
            const port_t port() const;
            void port(const port_t po);
            const std::string to_string() const;
            void disable(); // ��������Ϊ��Ч

        public:
            // ����: Ϊʲô���ṩ epstr ��Ϊ������ endpoint ���캯��
            // epstr �Ľ�����һ���ܳɹ�, ����ṩ���캯��, ��û���κλ��᷵���쳣, ���ֻ���׳��쳣, �����˿ͻ����벶���쳣�ĸ��Ӷ�
            static int build(const std::string &epstr, endpoint &ep);
            static endpoint boardcast(const port_t po);

        private:
            static int parse_ep(const std::string & epstr, std::string &ipv4, port_t &port);

        private:
            char ipstr_[MAXIMUM_IPV4_STRLEN];
            port_t port_;
            u32_ipv4_t address_;
        };

    } // namespace tcpip
} // nsp