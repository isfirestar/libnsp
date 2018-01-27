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
interface_format(int) tcp_write(HTCPLINK lnk, int cb, nis_sender_maker_t maker, void *par);
interface_format(int) tcp_getaddr(HTCPLINK lnk, int type, uint32_t* ipv4, uint16_t* port);
interface_format(int) tcp_setopt(HTCPLINK lnk, int level, int opt, const char *val, int len);
interface_format(int) tcp_getopt(HTCPLINK lnk, int level, int opt, char *val, int *len);

/*---------------------------------------------------------------------------------------------------------------------------------------------------------
  UDP procedure definition																				
---------------------------------------------------------------------------------------------------------------------------------------------------------*/
interface_format(int) udp_init();
interface_format(void) udp_uninit();
interface_format(HUDPLINK) udp_create(udp_io_callback_t user_callback, const char* l_ipstr, uint16_t l_port, int flag);
interface_format(void) udp_destroy(HUDPLINK lnk);
interface_format(int) udp_write(HUDPLINK lnk, int cb, nis_sender_maker_t maker, void *par, const char* r_ipstr, uint16_t r_port);
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
interface_format(int) nis_setctx(HLNK lnk, const void * user_context, int user_context_size);
interface_format(int) nis_getctx(HLNK lnk, void * user_context, int *user_context_size/*opt*/);
interface_format(void *) nis_refctx(HLNK lnk, int *user_context_size);
interface_format(int) nis_ctxsize(HLNK lnk);
interface_format(int) nis_getver(swnet_version_t *version);
interface_format(int) nis_gethost(const char *name, uint32_t *ipv4); /* parse the domain name, get the first parse result of obtained, convert it to Little-Endian*/
interface_format(char *) nis_lgethost(char *name, int cb);

/* set/change ECR(event callback routine) for nshost use, return the previous ecr address.
	version > 9.6.0
*/
interface_format(nis_event_callback_t) nis_checr(const nis_event_callback_t ecr);

#endif