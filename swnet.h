#if !defined SWNET_INTERFACE_CALL
#define SWNET_INTERFACE_CALL

#include <memory>
#include <unordered_map>
#include <mutex>

#include "icom/nisdef.h"

#include "singleton.hpp"
#include "network_handler.h"
#include "os_util.hpp"

#define NSP_DECLARE_INTERFACE(_Ret, _DefaultRet, _Routine, _Arg, _Par) \
                        _Ret _Routine _Arg { \
                                typedef _Ret (STD_CALL *_Routine_Type)_Arg; \
                                _Ret r = _DefaultRet; \
                                static void *f = nullptr; \
                                                                if (!f) { \
                                                                        f = nsp::os::dlsym( shared_library_, #_Routine ); \
                                                                } \
                                if (f) { \
                                        r = ((_Routine_Type)f)_Par; \
                                } \
                                return r; \
                        } \

#define NSP_DECLARE_COMM_INTERFACE(_Routine, _Arg, _Par) \
                        int _Routine _Arg { \
                                typedef int (STD_CALL *_Routine_Type)_Arg; \
                                int r = -1; \
                                static void *f = nullptr; \
                                                                if (!f) { \
                                                                        f = nsp::os::dlsym( shared_library_, #_Routine ); \
                                                                } \
                                if (f) { \
                                        r = ((_Routine_Type)f)_Par; \
                                } \
                                return r; \
                        }\

#define NSP_DECLARE_VOID_INTERFACE(_Routine, _Arg, _Par) \
                        void _Routine _Arg { \
                                typedef void (STD_CALL *_Routine_Type)_Arg; \
                                static void *f = nullptr; \
                                                                if (!f) { \
                                                                        f = nsp::os::dlsym( shared_library_, #_Routine ); \
                                                                } \
                                if (f) { \
                                        ((_Routine_Type)f)_Par; \
                                } \
                        }\

namespace nsp {
    namespace tcpip {

        class swnet {
            // dl
            void *shared_library_ = nullptr;

            // TCP
            mutable std::recursive_mutex lock_tcp_redirection_;
            std::unordered_map<HTCPLINK, std::shared_ptr<obtcp>> tcp_object_;

            int tcp_search(const HTCPLINK lnk, std::shared_ptr<obtcp> &object) const;
            void tcp_refobj(const HTCPLINK lnk, const std::function<void( const std::shared_ptr<obtcp>)> &todo);

            // UDP
            mutable std::recursive_mutex lock_udp_redirection_;
            std::unordered_map<HUDPLINK, std::shared_ptr<obudp>> udp_object_;

            int udp_search(const HUDPLINK lnk, std::shared_ptr<obudp> &object) const;
            void udp_refobj(const HUDPLINK lnk, const std::function<void( const std::shared_ptr<obudp>)> &todo);

            // c-d
            friend class nsp::toolkit::singleton<swnet>;
            swnet();
            ~swnet();

            //io
            static void STD_CALL tcp_io(const nis_event_t *pParam1, const void *pParam2);
            static void STD_CALL udp_io(const nis_event_t *pParam1, const void *pParam2);
            static void STD_CALL ecr(const char *host_event, const char *reserved, int rescb);

        public:
            NSP_DECLARE_COMM_INTERFACE(tcp_init, (), ());
            NSP_DECLARE_VOID_INTERFACE(tcp_uninit, (), ());
            NSP_DECLARE_INTERFACE(HTCPLINK, INVALID_HTCPLINK, tcp_create, (tcp_io_callback_t user_callback, const char* l_ipstr, uint16_t l_port), (user_callback, l_ipstr, l_port));
            NSP_DECLARE_COMM_INTERFACE(tcp_settst, (HTCPLINK lnk, const tst_t *tst), (lnk, tst));
            NSP_DECLARE_COMM_INTERFACE(tcp_gettst, (HTCPLINK lnk, tst_t *tst), (lnk, tst));
            NSP_DECLARE_VOID_INTERFACE(tcp_destroy, (HTCPLINK lnk), (lnk));
            NSP_DECLARE_COMM_INTERFACE(tcp_connect, (HTCPLINK lnk, const char* r_ipstr, uint16_t port_remote), (lnk, r_ipstr, port_remote));
			NSP_DECLARE_COMM_INTERFACE(tcp_connect2, (HTCPLINK lnk, const char* r_ipstr, uint16_t port_remote), (lnk, r_ipstr, port_remote));
            NSP_DECLARE_COMM_INTERFACE(tcp_listen, (HTCPLINK lnk, int block), (lnk, block));
            NSP_DECLARE_COMM_INTERFACE(tcp_write, (HTCPLINK lnk, int cb, nis_sender_maker_t maker, void *par), (lnk, cb, maker, par));
            NSP_DECLARE_COMM_INTERFACE(tcp_getaddr, (HTCPLINK lnk, int type, uint32_t* ipv4, uint16_t* port), (lnk, type, ipv4, port));
            NSP_DECLARE_COMM_INTERFACE(tcp_setopt, (HTCPLINK lnk, int level, int opt, const char *val, int len), (lnk, level, opt, val, len));
            NSP_DECLARE_COMM_INTERFACE(tcp_getopt, (HTCPLINK lnk, int level, int opt, char *val, int *len), (lnk, level, opt, val, len));

            NSP_DECLARE_COMM_INTERFACE(udp_init, (), ());
            NSP_DECLARE_VOID_INTERFACE(udp_uninit, (), ());
            NSP_DECLARE_INTERFACE(HUDPLINK, INVALID_HUDPLINK, udp_create, (udp_io_callback_t user_callback, const char* l_ipstr, uint16_t l_port, int flag), (user_callback, l_ipstr, l_port, flag));
            NSP_DECLARE_VOID_INTERFACE(udp_destroy, (HUDPLINK lnk), (lnk));
            NSP_DECLARE_COMM_INTERFACE(udp_write, (HUDPLINK lnk, int cb, nis_sender_maker_t maker, void *par, const char* r_ipstr, uint16_t r_port), (lnk, cb, maker, par, r_ipstr, r_port));
            NSP_DECLARE_COMM_INTERFACE(udp_getaddr, (HUDPLINK lnk, uint32_t *ipv4, uint16_t *port_output), (lnk, ipv4, port_output));
            NSP_DECLARE_COMM_INTERFACE(udp_setopt, (HUDPLINK lnk, int level, int opt, const char *val, int len), (lnk, level, opt, val, len));
            NSP_DECLARE_COMM_INTERFACE(udp_getopt, (HUDPLINK lnk, int level, int opt, char *val, int *len), (lnk, level, opt, val, len));

            NSP_DECLARE_COMM_INTERFACE(nis_setctx, (HLNK lnk, const void * user_context, int user_context_size), (lnk, user_context, user_context_size));
            NSP_DECLARE_COMM_INTERFACE(nis_getctx, (HLNK lnk, void * user_context, int* user_context_size), (lnk, user_context, user_context_size));
            NSP_DECLARE_INTERFACE(void *, NULL, nis_refctx, (HLNK lnk, int *user_context_size), (lnk, user_context_size));
            NSP_DECLARE_COMM_INTERFACE(nis_ctxsize, (HLNK lnk), (lnk));
            NSP_DECLARE_COMM_INTERFACE(nis_getver, (swnet_version_t *version), (version));
            NSP_DECLARE_COMM_INTERFACE(nis_gethost, (const char *name, uint32_t *ipv4), (name, ipv4)); /*可用于域名解析，获取首个解析IP地址, 该地址将在过程内部被转为小端*/
            NSP_DECLARE_INTERFACE(char *, NULL, nis_lgethost, (char *name, int cb), (name, cb));
            NSP_DECLARE_INTERFACE(nis_event_callback_t, NULL, nis_checr, (const nis_event_callback_t ecr), (ecr));
            
        public:
            // TCP
            int tcp_create(const std::shared_ptr<obtcp> &object, const char *ipstr, const port_t port);
            int tcp_attach(HTCPLINK lnk, const std::shared_ptr<obtcp> &object);
            void tcp_detach(HTCPLINK lnk);

            // UDP
            int udp_create(const std::shared_ptr<obudp> &object, const char* ipstr, const port_t port, int flag = UDP_FLAG_NONE);
            void udp_detach(HUDPLINK lnk);
        };
    }
}

#endif
