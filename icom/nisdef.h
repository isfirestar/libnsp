#if !defined SWNET_DEF_HEADER_2016_5_24
#define SWNET_DEF_HEADER_2016_5_24

#include "compiler.h"
#include "object.h"

#if defined __cplusplus
#define STD_C_FORMAT extern "C"
#include <cstdint>
#else
#define STD_C_FORMAT
#include <stdint.h>
#endif

/* bytes size of network protocol layer */
#define MTU       			               (1500)
#define UDP_PROTOCOL_LAYER_SIZE             (28)
#define TCP_PROTOCOL_LAYER_SIZE             (40)

#define MAX_UDP_UNIT        ((MTU - UDP_PROTOCOL_LAYER_SIZE))

#define IIS_MTU             (576)
#define IIS_MAX_UDP_UNIT    ((IIS_MTU - UDP_PROTOCOL_LAYER_SIZE))

/* types of nshost handle */
typedef objhld_t HLNK;
typedef HLNK HTCPLINK;
typedef HLNK HUDPLINK;
typedef HLNK HARPLINK;

#if !defined STD_CALL
#if _WIN32
#define STD_CALL __stdcall
#else
#define STD_CALL
#endif
#endif

#if defined __cplusplus
typedef bool nis_boolean_t;
#else
typedef int nis_boolean_t;
#endif

/* macro of export format */
#define interface_format(_Ty) STD_C_FORMAT _Ty STD_CALL

#if !defined INVALID_HTCPLINK
#define INVALID_HTCPLINK ((HTCPLINK)(~0))
#endif

#if !defined INVALID_HUDPLINK
#define INVALID_HUDPLINK ((HUDPLINK)(~0))
#endif

/* common network events */
#define EVT_PRE_CLOSE   (0x0002)    /* ready to close*/
#define EVT_CLOSED      (0x0003)    /* has been closed */
#define EVT_RECEIVEDATA (0x0004)    /* receive data*/
/* TCP events */
#define EVT_TCP_ACCEPTED    (0x0013)   /* has been Accepted */
#define EVT_TCP_CONNECTED   (0x0014)  /* success connect to remote */

/* option to get link address */
#define LINK_ADDR_LOCAL   (1)   /* get local using endpoint pair */
#define LINK_ADDR_REMOTE  (2)   /* get remote using endpoint pair */

/* optional  attributes of tcp link */
#define LINKATTR_TCP_FULLY_RECEIVE                      (1) /* receive fully packet include low-level head */
#define LINKATTR_TCP_NO_BUILD                           (2) /* not use @tst::builder when calling @tcp_write */
#define LINKATTR_TCP_UPDATE_ACCEPT_CONTEXT              (4) /* copy tst and attr to accepted link when syn */
#define LINKATTR_UDP_BAORDCAST                          (1)
#define LINKATTR_UDP_MULTICAST                          (2)

/* the definition control types for @nis_cntl */
#define NI_SETATTR      (1)
#define NI_GETATTR      (2)
#define NI_SETCTX       (3)
#define NI_GETCTX       (4)
#define NI_SETTST       (5)
#define NI_GETTST       (6)
#define NI_DUPCTX       (7)

/* the dotted decimal notation for IPv4 or IPv6 */
struct nis_inet_addr {
    char i_addr[INET_ADDRSTRLEN];
};

struct nis_inet6_addr {
    char i6_addr[INET6_ADDRSTRLEN];
};

struct nis_event {
    int Event;

    union {
        struct {
            HTCPLINK Link;
        } Tcp;

        struct {
            HUDPLINK Link;
        } Udp;
    } Ln;
} __POSIX_TYPE_ALIGNED__;

typedef struct nis_event nis_event_t;

/* user callback definition for network events */
typedef void( STD_CALL *nis_callback_t)(const struct nis_event *event, const void *data);
typedef nis_callback_t tcp_io_callback_t;
typedef nis_callback_t udp_io_callback_t;
typedef nis_callback_t arp_io_callback_t;

/*---------------------------------------------------------------------------------------------------------------------------------------------------------
    TCP implement
---------------------------------------------------------------------------------------------------------------------------------------------------------*/

/*  private protocol template(PPT,) support
    protocol parse template: tcp_ppt_parser_t
            @data               data stream
            @cb                 bytes of data stream
            @user_data_size     bytes of user data stream (eliminate length of protocol)

    protocol builder template: tcp_ppt_builder_t
            @data               data stream
            @cb                 bytes of data stream for build

    Any negative return of PPT templates will terminate the subsequent operation
 */
typedef int( STD_CALL *tcp_ppt_parser_t)(void *data, int cb, int *user_data_size);
typedef int( STD_CALL *tcp_ppt_builder_t)(void *data, int cb);

struct __tcp_stream_template {
    tcp_ppt_parser_t parser_;
    tcp_ppt_builder_t builder_;
    int cb_;
} __POSIX_TYPE_ALIGNED__;

typedef struct __tcp_stream_template tst_t;

/*  @nis_serializer_t target object use for @tcp_write or @udp_write procedure
 *  when parameter @origin of these write function is a pointer to a C style strcuture object without 1 byte aligned,
 *  or, @origin it's a pointer to a simple C++ object.
 *  the @serializer parameter should specify a method how to serialization @origin into byte-string @packet
 *  @packet will be the data buffer that is actually delivered to the kernel after @serializer call.
 */
typedef int( STD_CALL *nis_serializer_t)(unsigned char *packet, const void *origin, int cb);

struct nis_tcp_data {
    union {
        /* only used in case of EVT_RECEIVEDATA, @Packet.Size of bytes of data storage in @Packet.Data has been received from kernel,  */
        struct {
            const unsigned char *Data;
            int Size;
        } Packet;

        /* only used in case of EVT_TCP_ACCEPTED,
            @Accept.AcceptLink specify the remote link which accepted by listener @nis_event.Ln.Tcp.Link */
        struct {
            HTCPLINK AcceptLink;
        } Accept;

        /* only used in case of EVT_PRE_CLOSE, @PreClose.Context  pointer to user defined context of each link object */
        struct {
            void *Context;
        } PreClose;
    } e;
}__POSIX_TYPE_ALIGNED__;

typedef struct nis_tcp_data tcp_data_t;

/*---------------------------------------------------------------------------------------------------------------------------------------------------------
    UDP implement
---------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define UDP_FLAG_NONE           (0)
#define UDP_FLAG_UNITCAST       (UDP_FLAG_NONE)
#define UDP_FLAG_BROADCAST      (LINKATTR_UDP_BAORDCAST)
#define UDP_FLAG_MULTICAST      (LINKATTR_UDP_MULTICAST)

struct nis_udp_data {
    union {
        /* only used in case of EVT_RECEIVEDATA, @Packet.Size of bytes of data storage in @Packet.Data has been received from kernel,
            the sender endpoint is @RemoteAddress:@RemotePort */
        struct {
            const unsigned char *Data;
            int Size;
            char RemoteAddress[16];
            uint16_t RemotePort;
        } Packet;

        /* only used in case of EVT_PRE_CLOSE, @PreClose.Context  pointer to user defined context of each link object */
        struct {
            void *Context;
        } PreClose;
    } e;
} __POSIX_TYPE_ALIGNED__;

typedef struct nis_udp_data udp_data_t;

/*---------------------------------------------------------------------------------------------------------------------------------------------------------
    ARP implement
---------------------------------------------------------------------------------------------------------------------------------------------------------*/
struct nis_arp_data {
    union {
        struct {
            uint16_t    Arp_Hardware_Type;
            uint16_t    Arp_Protocol_Type;
            uint8_t     Arp_Hardware_Size;
            uint8_t     Arp_Protocol_Size;
            uint16_t    Arp_Op_Code;
            uint8_t     Arp_Sender_Mac[6];
            uint32_t    Arp_Sender_Ip;
            uint8_t     Arp_Target_Mac[6];
            uint32_t    Arp_Target_Ip;
        } Packet;

        /* only used in case of EVT_PRE_CLOSE, @PreClose.Context  pointer to user defined context of each link object */
        struct {
            void *Context;
        } PreClose;
    } e;
} __POSIX_TYPE_ALIGNED__;

typedef struct nis_arp_data arp_data_t;

/*---------------------------------------------------------------------------------------------------------------------------------------------------------
    GRP implement
---------------------------------------------------------------------------------------------------------------------------------------------------------*/
struct __packet_grp_node {
    char *Data;
    int Length;
} __POSIX_TYPE_ALIGNED__;
typedef struct __packet_grp_node packet_grp_node_t;

struct __packet_grp {
    packet_grp_node_t *Items;
    int Count;
} __POSIX_TYPE_ALIGNED__;
typedef struct __packet_grp packet_grp_t;

/* 支持库版本协议	*/
struct __swnet_version {
    short major_;
    short minor_;
    short revision_;
} __POSIX_TYPE_ALIGNED__;
typedef struct __swnet_version swnet_version_t;

/*  receiving notification text informations from nshost moudle
    version > 9.6.0
*/
typedef void( STD_CALL *nis_event_callback_t)(const char *host_event, const char *reserved, int rescb);

struct __ifmisc {
    char interface_[64];
    uint32_t addr_;
    uint32_t netmask_;
    uint32_t boardcast_;
}__POSIX_TYPE_ALIGNED__;
typedef struct __ifmisc ifmisc_t;

#endif
