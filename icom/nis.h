#if !defined (SW_NET_API_HEADER_20130217)
#define SW_NET_API_HEADER_20130217

/*
        网络应用程序接口规范 (nis, network interface specification) 定义

        规范衍生自 swnet.dll ultimate 2,2,5,0(2013-02-17) 版本(顺网9系列更新前最终版)
                支持库维持在 ccl runtime 1,3,3,0(2010-09-01) 版本(稳定版)
                        NIS规范 2015-05-26 定稿 / neo.anderson

        所有的 swnet2250+ 应用实例， 都遵循本规范定义，并适用于 win32/x64/POSIX

        版本整体弱化了原 mxx 的系列功能， TransmitPackets功能暂时被限制在 UDP 中可用

        UNIX like 操作系统平台下不再支持 GRP 方式的操作模型
 */
#include "nisdef.h"



/*---------------------------------------------------------------------------------------------------------------------------------------------------------
  TCP 过程定义部分	
-----------------------------------------------------------------------------------------------------------------------------------------------------------*/
interface_format(int) tcp_init();
interface_format(void) tcp_uninit();
interface_format(HTCPLINK) tcp_create(tcp_io_callback_t user_callback, const char* l_ipstr, uint16_t l_port);
interface_format(int) tcp_settst(HTCPLINK lnk, const tst_t *tst);
interface_format(int) tcp_gettst(HTCPLINK lnk, tst_t *tst);
interface_format(void) tcp_destroy(HTCPLINK lnk);
interface_format(int) tcp_connect(HTCPLINK lnk, const char* r_ipstr, uint16_t port_remote);
interface_format(int) tcp_listen(HTCPLINK lnk, int block);
interface_format(int) tcp_write(HTCPLINK lnk, int cb, nis_sender_maker_t maker, void *par);
interface_format(int) tcp_getaddr(HTCPLINK lnk, int type, uint32_t* ipv4, uint16_t* port);
interface_format(int) tcp_setopt(HTCPLINK lnk, int level, int opt, const char *val, int len);
interface_format(int) tcp_getopt(HTCPLINK lnk, int level, int opt, char *val, int *len);

/*---------------------------------------------------------------------------------------------------------------------------------------------------------
  UDP 过程定义部分																				
---------------------------------------------------------------------------------------------------------------------------------------------------------*/
interface_format(int) udp_init();
interface_format(void) udp_uninit();
interface_format(HUDPLINK) udp_create(udp_io_callback_t user_callback, const char* l_ipstr, uint16_t l_port, int flag);
interface_format(void) udp_destroy(HUDPLINK lnk);
interface_format(int) udp_write(HUDPLINK lnk, int cb, nis_sender_maker_t maker, void *par, const char* r_ipstr, uint16_t r_port);
interface_format(int) udp_getaddr(HUDPLINK lnk, uint32_t *ipv4, uint16_t *port_output);
interface_format(int) udp_setopt(HUDPLINK lnk, int level, int opt, const char *val, int len);
interface_format(int) udp_getopt(HUDPLINK lnk, int level, int opt, char *val, int *len);

/*---------------------------------------------------------------------------------------------------------------------------------------------------------
  UDP GRP 过程定义部分																				
---------------------------------------------------------------------------------------------------------------------------------------------------------*/
#if WIN32
interface_format(int) udp_initialize_grp(HUDPLINK lnk, packet_grp_t *grp);
interface_format(void) udp_release_grp(packet_grp_t *grp);
interface_format(int) udp_raise_grp(HUDPLINK lnk, const char *r_ipstr, uint16_t r_port);
interface_format(void) udp_detach_grp(HUDPLINK lnk);
interface_format(int) udp_write_grp(HUDPLINK lnk, packet_grp_t *grp);
#endif

/*---------------------------------------------------------------------------------------------------------------------------------------------------------
 对象相关/全局采样 部分
---------------------------------------------------------------------------------------------------------------------------------------------------------*/
interface_format(int) nis_setctx(HLNK lnk, const void * user_context, int user_context_size);
interface_format(int) nis_getctx(HLNK lnk, void * user_context, int *user_context_size/*opt*/);
interface_format(void *) nis_refctx(HLNK lnk, int *user_context_size);
interface_format(int) nis_ctxsize(HLNK lnk);
interface_format(int) nis_getver(swnet_version_t *version);
interface_format(int) nis_gethost(const char *name, uint32_t *ipv4); /*可用于域名解析，获取首个解析IP地址, 该地址将在过程内部被转为小端*/


#endif