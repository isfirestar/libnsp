#if !defined (SW_NET_API_HEADER_20130217)
#define SW_NET_API_HEADER_20130217

/*  nshost application interface definition head
    2013-02-17 neo.anderson  
    Copyright (C)2007 Free Software Foundation, Inc. 
    Everyone is permitted to copy and distribute verbatim copies of this license document, but changing it is not allowed.
*/

#include "nisdef.h"

/*---------------------------------------------------------------------------------------------------------------------------------------------------------
  TCP procedure definition	
-----------------------------------------------------------------------------------------------------------------------------------------------------------*/
interface_format(int) tcp_init();
interface_format(void) tcp_uninit();
interface_format(HTCPLINK) tcp_create(tcp_io_callback_t user_callback, const char* l_ipstr, uint16_t l_port);
interface_format(int) tcp_settst(HTCPLINK lnk, const tst_t *tst);
interface_format(int) tcp_gettst(HTCPLINK lnk, tst_t *tst);
interface_format(void) tcp_destroy(HTCPLINK lnk);
interface_format(int) tcp_connect(HTCPLINK lnk, const char* r_ipstr, uint16_t port_remote);
interface_format(int) tcp_connect2(HTCPLINK lnk, const char* r_ipstr, uint16_t port_remote);
interface_format(int) tcp_listen(HTCPLINK lnk, int block);
interface_format(int) tcp_write(HTCPLINK lnk, const void *origin, int cb, const nis_serializer_t serializer);
interface_format(int) tcp_getaddr(HTCPLINK lnk, int type, uint32_t* ipv4, uint16_t* port);
interface_format(int) tcp_setopt(HTCPLINK lnk, int level, int opt, const char *val, int len);
interface_format(int) tcp_getopt(HTCPLINK lnk, int level, int opt, char *val, int *len);
interface_format(int) tcp_setattr(HTCPLINK lnk, int attr, int enable);
interface_format(int) tcp_getattr(HTCPLINK lnk, int attr, int *enabled);

/*---------------------------------------------------------------------------------------------------------------------------------------------------------
  UDP procedure definition																				
---------------------------------------------------------------------------------------------------------------------------------------------------------*/
interface_format(int) udp_init();
interface_format(void) udp_uninit();
interface_format(HUDPLINK) udp_create(udp_io_callback_t user_callback, const char* l_ipstr, uint16_t l_port, int flag);
interface_format(void) udp_destroy(HUDPLINK lnk);
/* @udp_write interface using direct IO mode 
 * @udp_sendto interface like @tcp_write, push memory block into wpoll cache first */ 
interface_format(int) udp_write(HUDPLINK lnk, const void *origin, int cb, const char* r_ipstr, uint16_t r_port, const nis_serializer_t serializer);
interface_format(int) udp_getaddr(HUDPLINK lnk, uint32_t *ipv4, uint16_t *port_output);
interface_format(int) udp_setopt(HUDPLINK lnk, int level, int opt, const char *val, int len);
interface_format(int) udp_getopt(HUDPLINK lnk, int level, int opt, char *val, int *len);
interface_format(int) udp_joingrp(HUDPLINK lnk, const char *g_ipstr, uint16_t g_port);
interface_format(int) udp_dropgrp(HUDPLINK lnk);

/*---------------------------------------------------------------------------------------------------------------------------------------------------------
  UDP GRP procedure definition																				
---------------------------------------------------------------------------------------------------------------------------------------------------------*/
#if _WIN32
interface_format(int) udp_initialize_grp(HUDPLINK lnk, packet_grp_t *grp);
interface_format(void) udp_release_grp(packet_grp_t *grp);
interface_format(int) udp_raise_grp(HUDPLINK lnk, const char *r_ipstr, uint16_t r_port);
interface_format(void) udp_detach_grp(HUDPLINK lnk);
interface_format(int) udp_write_grp(HUDPLINK lnk, packet_grp_t *grp);
#endif

/*---------------------------------------------------------------------------------------------------------------------------------------------------------
 object/global functions
---------------------------------------------------------------------------------------------------------------------------------------------------------*/
interface_format(int) nis_getver(swnet_version_t *version);
/* parse the domain name, get the first parse result of obtained, convert it to Little-Endian*/
interface_format(int) nis_gethost(const char *name, uint32_t *ipv4); 
interface_format(char *) nis_lgethost(char *name, int cb);
/* set/change ECR(event callback routine) for nshost use, return the previous ecr address. */
interface_format(nis_event_callback_t) nis_checr(const nis_event_callback_t ecr); 

/* use @nis_getifmisc to view all local network adapter information
	the @ifv pointer must large enough and specified by @*cbifv to storage all device interface info

	the buffer size indicated by the @*cbifv parameter is too small to hold the adapter information or the @ifv parameter is NULL, the return value will be -EAGAIN
	the @*cbifv parameter returned points to the required size of the buffer to hold the adapter information. 

	on success, the return value is zero, otherwise, set by posix__mkerror(errno) if syscall fatal.
	demo code:
	 [
	 	int i;
	 	ifmisc_t *ifv;
		int cbifv;

		cbifv = 0;
		i = nis_getifmisc(NULL, &cbifv);
		if (i == -EAGAIN && cbifv > 0)
		{
			if (NULL != (ifv = (ifmisc_t *)malloc(cbifv))) {
				i = nis_getifmisc(ifv, &cbifv);
			}
		}

		if (i >= 0) {
			for (i = 0; i < cbifv / sizeof(ifmisc_t); i++) {
				printf(" interface:%s:\n INET:0x%08X\n netmask:0x%08X\n boardcast:0x%08X\n\n", ifv[i].interface_, ifv[i].addr_, ifv[i].netmask_, ifv[i].boardcast_);
			}
		}
	 ] */
interface_format(int) nis_getifmisc(ifmisc_t *ifv, int *cbifv);

#endif