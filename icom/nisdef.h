#if !defined SWNET_DEF_HEADER_2016_5_24
#define SWNET_DEF_HEADER_2016_5_24

#if defined __cplusplus
#define STD_C_FORMAT extern "C"
#include <cstdint>
#else
#define STD_C_FORMAT
#include <stdint.h>
#endif

/* 前置网络协议层及其长度 */
#define MTU       			(1500)   
#define ETHERNET_P_SIZE     (14)   /*网络层14个字节 */
#define IP_P_SIZE      		(20)   /* IP层20个字节 */
#define UDP_P_SIZE      	(8)   /* UDP层8个字节 */
#define TCP_P_SIZE      	(20)   /* TCP层20个字节 */

/* 下层句柄定义 */
typedef uint64_t HLNK;
typedef uint64_t HTCPLINK;
typedef int64_t HUDPLINK;

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

#define interface_format(_Ty) STD_C_FORMAT _Ty STD_CALL

#if !defined INVALID_HTCPLINK
#define INVALID_HTCPLINK ((HTCPLINK)(~0))
#endif

#if !defined INVALID_HUDPLINK
#define INVALID_HUDPLINK ((HUDPLINK)(~0))
#endif

/* 通用网络事件 */
#define EVT_CREATED   (0x0001)   /* 已经创建 */
#define EVT_PRE_CLOSE   (0x0002)   /* 即将关闭*/
#define EVT_CLOSED   (0x0003)   /* 已经关闭*/
#define EVT_RECEIVEDATA   (0x0004)   /* 接收数据*/
#define EVT_SENDDATA   (0x0005)   /* 发送数据*/
#define EVT_DEBUG_LOG   (0x0006)   /* 反馈的调试信息 */
#define EVT_EXCEPTION   (0xFFFF)   /* 异常*/

/* TCP 网络事件 */
#define EVT_TCP_ACCEPTED  (0x0013)   /* 已经Accept */
#define EVT_TCP_CONNECTED  (0x0014)   /* 已经连接成功 */
#define EVT_TCP_DISCONNECTED            (0x0015)   /* 已经断开连接, 主要用于标记10054(远端主机主动断开)事件 */

/* 获取地址信息选项 */
#define LINK_ADDR_LOCAL   (0x0001)   /* 得到绑定本机地址端口信息 */
#define LINK_ADDR_REMOTE  (0x0002)   /* 得到绑定对端地址端口信息 */

#pragma pack(push, 1)

typedef struct _nis_event_t {
    int Event; /* 事件类型 */

    union {

        struct {
            HTCPLINK Link;
        } Tcp;

        struct {
            HUDPLINK Link;
        } Udp;
    } Ln;
} nis_event_t;

/* 协议回调例程定义 */
typedef void( STD_CALL *nis_callback_t)(const nis_event_t *naio_event, const void *pParam2);
typedef nis_callback_t tcp_io_callback_t;
typedef nis_callback_t udp_io_callback_t;

/*---------------------------------------------------------------------------------------------------------------------------------------------------------
    TCP 部分
---------------------------------------------------------------------------------------------------------------------------------------------------------*/

/* 私有协议模板(PPT, private protocol template) 支持

        协议解析模板 tcp_ppt_parser_t
                @data               数据流
                @cb                 数据流长度
                @user_data_size   用户部分的长度(除去协议长度后)

        协议构建模板 tcp_ppt_builder_t
                @data               构建协议的数据流
                @cb                 构建协议的数据字节长度

        私有协议模板的操作返回<0, 将会使下级网络流程失败中止
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
            const char * Data; /* 接收数据Buf */
            int Size; /* 该数据包字节大小 */
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
 UDP 部分
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
 GRP相关部分
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

#pragma pack(pop)

#endif