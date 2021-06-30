#if !defined (SW_NET_API_HEADER_20130217)
#define SW_NET_API_HEADER_20130217

/*  nshost application interface definition head
    2013-02-17 neo.anderson
    Copyright (C)2007 Free Software Foundation, Inc.
    Everyone is permitted to copy and distribute verbatim copies of this license document, but changing it is not allowed.

    summary of known bugs :
    1. Even without invoking the initialize the interface or initialize call fail,
    	other IO-independent interfaces still may return success and establishing legitimate internal object.
	2. When call to @tcp_connect in synchronous mode, cause by the mutlithreading reason,
		the order of arrival of callback events may be in chaos, for example, EVT_CLOSED is earlier than EVT_TCP_CONNECTED.
*/

#include "nisdef.h"

/* @tcp_init use to initialzie TCP framework, invoke before any TCP relate function call.
   @tcp_uninit use to uninitialize TCP low-level framework and release resource
   usually, large than or equal to zero return value indicate success, negative return value indicate error detected.
   potential return value including:
   -EPROTOTYPE / -ENOENT : TCP protocol are not support.
   EALREADY: protocol has been initialized before this time invocation. */
PORTABLEAPI(int) tcp_init();
PORTABLEAPI(void) tcp_uninit();

/* @tcp_create and @tcp_create2 use to create a TCP object point to by return value @HTCPLINK, this link will use everywhere which employ TCP functions.
	@tcp_destroy notify framework close the TCP connection and release resource of this link as soon as possible.
				in fact, the link usually can NOT be close completely immediately,
				framework have resposibility to guarantee the availability of link and it's callback procedure running in memory-safe environment.
	parameters:
	@callback input the callback function address to respond the TCP network event, see "common network events" in @nisdef.h
	@ipstr specify the local IPv4 address format with "dotted decimal notation" specify the initial address which this link expect to bind on.
			if "NULL" specified, framework will use default address "0.0.0.0" assicoated with any network-adpater
	@port specify the port which this link expect to bind on, if 0 specified, framework will use "random-selection" by underlying-system
	@tst specify the low-level protocol parse template, explicit set in create stage are much simple than invoke @tcp_settst later
	return:
	on success, the return value large than zero, otherwise should be @INVALID_HTCPLINK. */
PORTABLEAPI(HTCPLINK) tcp_create(tcp_io_callback_t callback, const char* ipstr, uint16_t port);
PORTABLEAPI(HTCPLINK) tcp_create2(tcp_io_callback_t callback, const char* ipstr, uint16_t port, const tst_t *tst);
PORTABLEAPI(void) tcp_destroy(HTCPLINK link);

/* @tcp_connect try to connect target host indicate by @ipstr and @port in synchronous model and @tcp_connect2 try it by asynchronous
	parameters:
	@link is the local TCP object symbol create by invoke @tcp_create or @tcp_create2
	@ipstr specify the target host IPv4 address which acquire to connect to
	@port specify the target port which acquire to connect to
	return:
	on success, the return value large than or equal to zero, otherwise, negative value should be return.
	potential errors including:
	-EINVAL : the input parameter illegal
	-EPROTOTYPE : @link not a TCP object
	-ENOENT : @link are already closed or states are not available
	-EISCONN : connection has been established.
	-EBADFD : low-level socket states are not TCP_CLOSE
	any other errors of syscall bind(2) and/or connect(2)  */
PORTABLEAPI(int) tcp_connect(HTCPLINK link, const char* ipstr, uint16_t port);
PORTABLEAPI(int) tcp_connect2(HTCPLINK link, const char* ipstr, uint16_t port);

/* @tcp_listen set TCP object point by @link to TCP_LISTEN state.
	parameters:
	@link is the local TCP object symbol create by @tcp_create or @tcp_create2
	@block defines the maximum length to which the queue of pending connections for sockfd may grow, this parameter can be 0, when it is 0,
		the behavior of @tcp_listen is use default backlog number "SOMAXCONN"
	return:
	on success, the return value large than or equal to zero, otherwise, negative value should be return.
	potential errors including:
	-EINVAL : the input parameter illegal
	-EPROTOTYPE : @link not a TCP object
	-ENOENT : @link are already closed or states are not available
	-EBADFD : low-level socket states are not TCP_CLOSE
	any other errors of syscall bind(2) and/or listen(2)		*/
PORTABLEAPI(int) tcp_listen(HTCPLINK link, int block);

/* @tcp_write send @size bytes of user data @origin on TCP object associated with @link to target host which already connected.
		@tcp_write ensure operation under asynchronous model, NOT-blocking calling thread and return immediately,
		but NOT guarantee request complete in system kernel at the time point of function returned
		on client peer, it's calling sequence MUST be later than success invoke @tcp_connect or @tcp_connect2.
		on server peer, the link MUST be the remote object symbol which callback in EVT_TCP_ACCEPTED event.
	@tcp_write have much different behavior between Win32 and POSIX operation system:
	1. in POSIX , @tcp_write will direct call send(2) to try to write out data immediately when the socket IO state are not "blocking",
		the "blocking" IO state only happen when send-Q is full, and send(2) obtain error "EAGAIN".
		when the "blocking" state established, @tcp_write will package the request data into a queue which manage by framework itself and waiting for
		the best opportunity to dispatch to system-kernel automatically.
	2. in Win32, @tcp_write always package the request memory queued and managed by framework, the actual send operation will be trigger by:
		2.1 receive a IOCP send complete signal.
		2.2 no any request pending in current subsequence.
		2.3 WSASend return success and not pending on system kernel.

	parameters:
	@link is the TCP object symbol which have TCP_ESTABLISHED state.
	@origin specify the user data wich acqure to send to target host.
	@size indicate the length in bytes which @origin contain
	@serializer only use by C++ framework, it support in order to the way of @nsp::tcpip::proto_interface template class to serialize write out data.

	return:
	on success, the return value should be either a positive integer indicate the sent size of direct-completed inner syscall
		or zero indicate successful push node into internal queue managed by framework.
	on fatal, negative integer will return, potential errors including:
	-EINVAL : the input parameter illegal
	-EPROTOTYPE : @link not a TCP object
	-ENOENT : @link are already closed or states are not available
	-ENOMEM : framework can not allocate virtual memory from system-kernel
	-EBUSY : send-Q of system kernel are already full, the cache queue of framework also arrived maximum pending limit, user request can NOT be perform now */
PORTABLEAPI(int) tcp_write(HTCPLINK link, const void *origin, int size, const nis_serializer_t serializer);

/* @tcp_awaken schedules the pointer to @pipedata of @cb bytes of user data to receive thread which assign to @link,
	this implement is helpful for thread merge and/or lockless design in some complex program.
	parameters:
	@link is the TCP object symbol
	@pipedata NOT-null pointer to user data buffer.
	@cb indicate the length in bytes of @pipedata pointer to
	return:
	on success, the return value large than or equal to zero, otherwise, negative value should be return. */
PORTABLEAPI(int) tcp_awaken(HTCPLINK link, const void *pipedata, int cb);

/* @tcp_getaddr acknowledge the actual IPv4 tuple be bound on local or tuple be connected to remote.
	parameters:
	@link a symbol pointer to the TCP object.
	@type canbe one of LINK_ADDR_LOCAL or LINK_ADDR_REMOTE
	@ip pointer use to obtain the return IP address string.
	@port pointer use to obtain the return port number
	return:
	on success, the return value large than or equal to zero, otherwise, negative value should be return.
	potential errors including:
	-EINVAL : the input parameter illegal
	-EPROTOTYPE : @link not a TCP object
	-ENOENT : @link are already closed or states are not available */
PORTABLEAPI(int) tcp_getaddr(HTCPLINK link, int type, uint32_t* ip, uint16_t* port);

/* @tcp_setopt and @tcp_getopt are the simple calling chains to getsockopt(2) and setsockopt(2).
	all parameters and return value definitions are the same as syscall */
PORTABLEAPI(int) tcp_setopt(HTCPLINK link, int level, int opt, const char *val, int len);
PORTABLEAPI(int) tcp_getopt(HTCPLINK link, int level, int opt, char *val, int *len);

/*  the following are some obsolete interface definition:
	NOTE: New applications should use @nis_cntl interface (available since version 9.8.1),
	which provides a much superior interface for user control operation for every link.
	@NI_SETTST to instead @tcp_settst
	@NI_GETTST to instead @tcp_gettst
	@NI_SETATTR to instead @tcp_setattr
	@NI_GETATTR to instead @tcp_getattr */
PORTABLEAPI(int) tcp_settst(HTCPLINK link, const tst_t *tst);
PORTABLEAPI(int) tcp_gettst(HTCPLINK link, tst_t *tst);
PORTABLEAPI(int) tcp_setattr(HTCPLINK link, int cmd, int enable);
PORTABLEAPI(int) tcp_getattr(HTCPLINK link, int cmd, int *enabled);

/* @udp_init use to initialzie UDP framework, invoke before any other UDP relate function call.
   @udp_uninit use to uninitialize UDP framework and release resource
   usually, large than or equal to zero return value indicate success, negative return value indicate error detected.
   potential return value including:
   -EPROTOTYPE / -ENOENT : UDP protocol are not support.
   EALREADY: protocol has been initialized before this time invocation. */
PORTABLEAPI(int) udp_init();
PORTABLEAPI(void) udp_uninit();

/* NOTE: New applications should NOT set the @flag when calling @udp_create  (available since version 9.8.1),
 			every udp link can change the attributes(flag) any time by interface @nis_cntl call with parameter @NI_SETATTR,
 			more useful: that broadcast attributes can now be cancelled.
 	@udp_create use to create a UDP object point to by return value @HUDPLINK, this link will use everywhere which employ UDP functions.
	@udp_destroy notify framework close the UDP socket and release resource of this link as soon as possible.
				in fact, the link usually can NOT be close completely immediately,
				framework have resposibility to guarantee the availability of link and it's callback procedure running in memory-safe environment.
	parameters:
	@callback input the callback function address to respond the UDP network event, see "common network events" in @nisdef.h
	@ipstr specify the local IPv4 address format with "dotted decimal notation" specify the initial address which this link expect to bind on.
			if "NULL" specified, framework will use default address "0.0.0.0" assicoated with any network-adpater
	@port specify the port which this link expect to bind on, if 0 specified, framework will use "random-selection" by underlying-system
	@flag indicate some external attributes that creater ask framework to build for this UDP socket, by default, set it to 0.
	return:
	on success, the return value large than zero, otherwise should be @INVALID_HUDPLINK. */
PORTABLEAPI(HUDPLINK) udp_create(udp_io_callback_t user_callback, const char* ipstr, uint16_t port, int flag);
PORTABLEAPI(void) udp_destroy(HUDPLINK link);

/* @udp_write send @size bytes of user datagram @origin from local address tuple associated by @link to remote target host @ipstr and it's UDP port @port.
		@udp_write using synchronous model but NOT-blocking calling thread and return immediately,
		framework guarantee request post to system kernel at the time point of function returned.
		@udp_write are available after @udp_create successful return.
	@udp_write have much different behavior between Win32 and POSIX operation system:
	1. in POSIX , @udp_write will direct call sendto(2) to try to write out data immediately when the socket IO state are not "blocking",
		the "blocking" IO state only happen when send-Q is full, and sendto(2) obtain error "EAGAIN".
		when the "blocking" state established, @udp_write will package the request data into a queue which manage by framework itself and waiting for
		the best opportunity to dispatch to system-kernel automatically.
	2. in Win32, @udp_write always schedule the datagram to system kernel immediately.

	parameters:
	@link is the UDP object symbol.
	@origin specify the user data wich acqure to send to target host.
	@size indicate the length in bytes which @origin contain
	@ipstr specify the remote target host IPv4 address, this parameter can NOT be NULL or "0.0.0.0"
	@port specify the remote target host UDP port, this parameter can NOT be 0.
	@serializer only use by C++ framework, it support in order to the way of @nsp::tcpip::proto_interface template class to serialize write out data.

	return:
	on success, the return value should be either a positive integer indicate the sent size of direct-completed inner syscall
		or zero indicate successful push node into internal queue managed by framework.
	on fatal, negative integer will return, potential errors including:
	-EINVAL : the input parameter illegal
	-EPROTOTYPE : @link not a UDP object
	-ENOENT : @link are already closed or states are not available
	-ENOMEM : framework can not allocate virtual memory from system-kernel
	-EBUSY : send-Q of system kernel are already full, the cache queue of framework also arrived maximum pending limit, user request can NOT be perform now */
PORTABLEAPI(int) udp_write(HUDPLINK link, const void *origin, int size, const char* ipstr, uint16_t port, const nis_serializer_t serializer);

/* @udp_awaken schedules the pointer to @pipedata of @cb bytes of user data to receive thread which assign to @link,
	this implement is helpful for thread merge and/or lockless design in some complex program.
	parameters:
	@link is the UDP object symbol
	@pipedata NOT-null pointer to user data buffer.
	@cb indicate the length in bytes of @pipedata pointer to
	return:
	on success, the return value large than or equal to zero, otherwise, negative value should be return. */
PORTABLEAPI(int) udp_awaken(HUDPLINK link, const void *pipedata, int cb);

/* @udp_getaddr acknowledge the actual IPv4 tuple be bound on local or tuple be connected to remote.
	parameters:
	@link a symbol pointer to the UDP object.
	@type canbe one of LINK_ADDR_LOCAL or LINK_ADDR_REMOTE
	@ip pointer use to obtain the return IP address string.
	@port pointer use to obtain the return port number
	return:
	on success, the return value large than or equal to zero, otherwise, negative value should be return.
	potential errors including:
	-EINVAL : the input parameter illegal
	-EPROTOTYPE : @link not a UDP object
	-ENOENT : @link are already closed or states are not available */
PORTABLEAPI(int) udp_getaddr(HUDPLINK link, uint32_t *ipv4, uint16_t *port);

/* @udp_setopt and @udp_getopt are the simple calling chains to getsockopt(2) and setsockopt(2).
	all parameters and return value definitions are the same as syscall */
PORTABLEAPI(int) udp_setopt(HUDPLINK link, int level, int opt, const char *val, int len);
PORTABLEAPI(int) udp_getopt(HUDPLINK link, int level, int opt, char *val, int *len);

PORTABLEAPI(int) udp_joingrp(HUDPLINK link, const char *ipstr, uint16_t port);
PORTABLEAPI(int) udp_dropgrp(HUDPLINK link);

#if _WIN32
PORTABLEAPI(int) udp_initialize_grp(HUDPLINK link, packet_grp_t *grp);
PORTABLEAPI(void) udp_release_grp(packet_grp_t *grp);
PORTABLEAPI(int) udp_raise_grp(HUDPLINK link, const char *ipstr, uint16_t port);
PORTABLEAPI(void) udp_detach_grp(HUDPLINK link);
PORTABLEAPI(int) udp_write_grp(HUDPLINK link, packet_grp_t *grp);
#endif

/* NOTE: Before using ARP low level network protocol, application MUST call @udp_init method
 *			any ARP io/file-descriptor willbe bind on UDP threads.
 *			ensure that calling thread HAVE root/administrator access right/equal or or higher execution priority
 */
PORTABLEAPI(HARPLINK) arp_create(arp_io_callback_t user_callback, const char *ipstr);
PORTABLEAPI(int) arp_nrequest(HARPLINK link, uint32_t target);
PORTABLEAPI(int) arp_request(HARPLINK link, const char *target);
PORTABLEAPI(void) arp_destroy(HARPLINK link);

PORTABLEAPI(int) nis_getver(swnet_version_t *version);
/* parse the domain name, get the first parse result of obtained, convert it to Little-Endian*/
PORTABLEAPI(int) nis_gethost(const char *name, uint32_t *ipv4);
PORTABLEAPI(char *) nis_lgethost(char *name, int cb);
/* set/change ECR(event callback routine) for nshost use, return the previous ecr address. */
PORTABLEAPI(nis_event_callback_t) nis_checr(const nis_event_callback_t ecr);

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
PORTABLEAPI(int) nis_getifmisc(ifmisc_t *ifv, int *cbifv);

/*  @nis_getifmac interface implementation a method to get the physical address(MAC) of the specified network interface named @eth_name.
 *  for POSIX, the @eth_name should be one of the network interface name by command ifconfig(8)
 *		for example: "enp0s8"
 *  for win32, the @eth_name should be one of the network adapter description by command ipconfig /all, The string to the right of the colon.
 *		for example: "Intel(R) Dual Band Wireless-AC 3165"
 *	input paramter @pyhaddr is a buffer larger than 6 bytes use to save the mac address when interface success invoked.
 *	on success, the return value should be zero, otherwise, negative integer number return, it's absolute value indicate the error number definied in <errno.h>
*/
PORTABLEAPI(int) nis_getifmac(char *eth_name, unsigned char *pyhaddr);

/*
 *	NI_SETATTR(int)
 *		set the attributes of specify object, return the operation result
 *	NI_GETATTR(void)
 *		get the attributes of speicfy object, return the object attributes in current on successful, otherwise, -1 will be return
 *	NI_SETCTX(const void *)
 *		set the user define context pointer and binding with target object
 *		NOTE: 	that @NI_SETCTX with @nis_cntl call failure during neither EVT_PRE_CLOSE nor EVT_CLOSED
 *	NI_GETCTX(void **)
 *		get the user define context pointer which is binding with target object
 *		NOTE: 	that @NI_GETCTX with @nis_cntl call always failure on EVT_CLOSED,in this procedure, PreClose::Context it's SURE be NULL.
 *				calling thread should use or save or free the context pointer through PreClose::Context in event handler EVT_PRE_CLOSE,
 *					and EVT_PRE_CLOSE is the last chance to safely visit user context pointer
 *	NI_SETTST(const tst_t *)
 *		set the tcp stream template of specify object
 *	NI_GETTST(tst_t *)
 *		get the tcp stream template of specify object current set
 *
 *	attributes added after version 991
 *	NI_RISECTX(void **)
 *	NI_SINKCTX(void)
 *		these two attributes use to resolve the problem during using  @NI_GETCTX, take account of the follow scenario:
 *		one thread call @NI_GETCTX and obtain the context pointer--with lockless, but another thread trigger the link close event.
 *		in this situation, wild pointer obtained by the first thread may cause application crash.
 *		by use @NI_RISECTX instead, framework shall automatic raise up the reference count of link and then prevent the link closed by any event.
 *		but calling thread has responsibility to explicit invoke @NI_SINKCTX to release the reference count of this link.
 */
PORTABLEAPI(int) nis_cntl(objhld_t link, int cmd, ...);

#endif
