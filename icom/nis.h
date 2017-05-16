#if !defined (SW_NET_API_HEADER_20130217)
#define SW_NET_API_HEADER_20130217

/*
        ����Ӧ�ó���ӿڹ淶 (nis, network interface specification) ����

        �淶������ swnet.dll ultimate 2,2,5,0(2013-02-17) �汾(˳��9ϵ�и���ǰ���հ�)
                ֧�ֿ�ά���� ccl runtime 1,3,3,0(2010-09-01) �汾(�ȶ���)
                        NIS�淶 2015-05-26 ���� / neo.anderson

        ���е� swnet2250+ Ӧ��ʵ���� ����ѭ���淶���壬�������� win32/x64/POSIX

        �汾����������ԭ mxx ��ϵ�й��ܣ� TransmitPackets������ʱ�������� UDP �п���

        UNIX like ����ϵͳƽ̨�²���֧�� GRP ��ʽ�Ĳ���ģ��
 */
#include "nisdef.h"



/*---------------------------------------------------------------------------------------------------------------------------------------------------------
  TCP ���̶��岿��	
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
  UDP ���̶��岿��																				
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
  UDP GRP ���̶��岿��																				
---------------------------------------------------------------------------------------------------------------------------------------------------------*/
#if WIN32
interface_format(int) udp_initialize_grp(HUDPLINK lnk, packet_grp_t *grp);
interface_format(void) udp_release_grp(packet_grp_t *grp);
interface_format(int) udp_raise_grp(HUDPLINK lnk, const char *r_ipstr, uint16_t r_port);
interface_format(void) udp_detach_grp(HUDPLINK lnk);
interface_format(int) udp_write_grp(HUDPLINK lnk, packet_grp_t *grp);
#endif

/*---------------------------------------------------------------------------------------------------------------------------------------------------------
 �������/ȫ�ֲ��� ����
---------------------------------------------------------------------------------------------------------------------------------------------------------*/
interface_format(int) nis_setctx(HLNK lnk, const void * user_context, int user_context_size);
interface_format(int) nis_getctx(HLNK lnk, void * user_context, int *user_context_size/*opt*/);
interface_format(void *) nis_refctx(HLNK lnk, int *user_context_size);
interface_format(int) nis_ctxsize(HLNK lnk);
interface_format(int) nis_getver(swnet_version_t *version);
interface_format(int) nis_gethost(const char *name, uint32_t *ipv4); /*������������������ȡ�׸�����IP��ַ, �õ�ַ���ڹ����ڲ���תΪС��*/


#endif