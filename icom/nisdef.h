#if !defined SWNET_DEF_HEADER_2016_5_24
#define SWNET_DEF_HEADER_2016_5_24

#if defined __cplusplus
#define STD_C_FORMAT extern "C"
#include <cstdint>
#else
#define STD_C_FORMAT
#include <stdint.h>
#endif

/* bytes size of network protocol layer */
#define MTU       			(1500)   
#define ETHERNET_P_SIZE     (14)   /*14 bytes of ethrent layer */
#define IP_P_SIZE      		(20)   /* 20 bytes of IP layer */
#define UDP_P_SIZE      	(8)   /* 8 bytes of UDP layer */
#define TCP_P_SIZE      	(20)   /* 20 bytes of TCP Layer */

/* types of nshost handle */
typedef uint32_t HLNK;
typedef uint32_t HTCPLINK;
typedef int32_t HUDPLINK;

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
#define EVT_CREATED     (0x0001)    /* created */
#define EVT_PRE_CLOSE   (0x0002)    /* ready to close*/
#define EVT_CLOSED      (0x0003)    /* has been closed */
#define EVT_RECEIVEDATA (0x0004)    /* receive data*/
#define EVT_SENDDATA    (0x0005)    /* sent data*/
#define EVT_DEBUG_LOG   (0x0006)    /* report debug information */
#define EVT_EXCEPTION   (0xFFFF)    /* exceptions*/

/* TCP events */
#define EVT_TCP_ACCEPTED  (0x0013)   /* has been Accepted */
#define EVT_TCP_CONNECTED  (0x0014)  /* success connect to remote */

/* option to get link address */
#define LINK_ADDR_LOCAL   (0x0001)   /* get local using endpoint pair */
#define LINK_ADDR_REMOTE  (0x0002)   /* get remote using endpoint pair */

#pragma pack(push, 1)

typedef struct _nis_event_t {
    int Event; 

    union {

        struct {
            HTCPLINK Link;
        } Tcp;

        struct {
            HUDPLINK Link;
        } Udp;
    } Ln;
} nis_event_t;

/* user callback definition for network events */
typedef void( STD_CALL *nis_callback_t)(const nis_event_t *naio_event, const void *pParam2);
typedef nis_callback_t tcp_io_callback_t;
typedef nis_callback_t udp_io_callback_t;

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

typedef struct _TCP_STREAM_TEMPLATE {
    tcp_ppt_parser_t parser_;
    tcp_ppt_builder_t builder_;
    int cb_;
} tst_t;

typedef int( STD_CALL *nis_sender_maker_t)(void *data, int cb, void *context);

typedef struct {
    union {
        struct {
            const char * Data;
            int Size; 
        } Packet;

        struct {
            HTCPLINK AcceptLink;
        } Accept;

        struct {
            int SubEvent;
            uint32_t ErrorCode;
        } Exception;

        struct {
            HTCPLINK OptionLink;
        } LinkOption;

        struct {
            const char *logstr;
        } DebugLog;

    } e;
} tcp_data_t;

/*---------------------------------------------------------------------------------------------------------------------------------------------------------
    UDP implement
---------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define UDP_FLAG_NONE           (0)
#define UDP_FLAG_UNITCAST       (UDP_FLAG_NONE)
#define UDP_FLAG_BROADCAST      (1)
#define UDP_FLAG_MULTICAST      (2)

typedef struct {
    union {
        struct {
            const char * Data;
            int Size;
            char RemoteAddress[16];
            uint16_t RemotePort;
        } Packet;

        struct {
            int SubEvent;
            uint32_t ErrorCode;
        } Exception;

        struct {
            HUDPLINK OptionLink;
        } LinkOption;

        struct {
            const char *logstr;
        } DebugLog;
    } e;

} udp_data_t;

/*---------------------------------------------------------------------------------------------------------------------------------------------------------
    GRP implement
---------------------------------------------------------------------------------------------------------------------------------------------------------*/

typedef struct _packet_grp_node {
    char *Data;
    int Length;
} packet_grp_node_t;

typedef struct _packet_grp {
    packet_grp_node_t *Items;
    int Count;
} packet_grp_t;

/* 支持库版本协议	*/
typedef struct _swnet_version {
    short procedure_;
    short main_;
    short sub_;
    short leaf_;
} swnet_version_t;

/*  receiving notification text informations from nshost moudle
    version > 9.6.0
*/
typedef void( STD_CALL *nis_event_callback_t)(const char *host_event, const char *reserved, int rescb);

#pragma pack(pop)

#endif