#if !defined SWNET_DEF_HEADER_2016_5_24
#define SWNET_DEF_HEADER_2016_5_24

#if defined __cplusplus
#define STD_C_FORMAT extern "C"
#include <cstdint>
#else
#define STD_C_FORMAT
#include <stdint.h>
#endif

/* ǰ������Э��㼰�䳤�� */
#define MTU       			(1500)   
#define ETHERNET_P_SIZE     (14)   /*�����14���ֽ� */
#define IP_P_SIZE      		(20)   /* IP��20���ֽ� */
#define UDP_P_SIZE      	(8)   /* UDP��8���ֽ� */
#define TCP_P_SIZE      	(20)   /* TCP��20���ֽ� */

/* �²������� */
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

/* ͨ�������¼� */
#define EVT_CREATED   (0x0001)   /* �Ѿ����� */
#define EVT_PRE_CLOSE   (0x0002)   /* �����ر�*/
#define EVT_CLOSED   (0x0003)   /* �Ѿ��ر�*/
#define EVT_RECEIVEDATA   (0x0004)   /* ��������*/
#define EVT_SENDDATA   (0x0005)   /* ��������*/
#define EVT_DEBUG_LOG   (0x0006)   /* �����ĵ�����Ϣ */
#define EVT_EXCEPTION   (0xFFFF)   /* �쳣*/

/* TCP �����¼� */
#define EVT_TCP_ACCEPTED  (0x0013)   /* �Ѿ�Accept */
#define EVT_TCP_CONNECTED  (0x0014)   /* �Ѿ����ӳɹ� */
#define EVT_TCP_DISCONNECTED            (0x0015)   /* �Ѿ��Ͽ�����, ��Ҫ���ڱ��10054(Զ�����������Ͽ�)�¼� */

/* ��ȡ��ַ��Ϣѡ�� */
#define LINK_ADDR_LOCAL   (0x0001)   /* �õ��󶨱�����ַ�˿���Ϣ */
#define LINK_ADDR_REMOTE  (0x0002)   /* �õ��󶨶Զ˵�ַ�˿���Ϣ */

#pragma pack(push, 1)

typedef struct _nis_event_t {
    int Event; /* �¼����� */

    union {

        struct {
            HTCPLINK Link;
        } Tcp;

        struct {
            HUDPLINK Link;
        } Udp;
    } Ln;
} nis_event_t;

/* Э��ص����̶��� */
typedef void( STD_CALL *nis_callback_t)(const nis_event_t *naio_event, const void *pParam2);
typedef nis_callback_t tcp_io_callback_t;
typedef nis_callback_t udp_io_callback_t;

/*---------------------------------------------------------------------------------------------------------------------------------------------------------
    TCP ����
---------------------------------------------------------------------------------------------------------------------------------------------------------*/

/* ˽��Э��ģ��(PPT, private protocol template) ֧��

        Э�����ģ�� tcp_ppt_parser_t
                @data               ������
                @cb                 ����������
                @user_data_size   �û����ֵĳ���(��ȥЭ�鳤�Ⱥ�)

        Э�鹹��ģ�� tcp_ppt_builder_t
                @data               ����Э���������
                @cb                 ����Э��������ֽڳ���

        ˽��Э��ģ��Ĳ�������<0, ����ʹ�¼���������ʧ����ֹ
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
            const char * Data; /* ��������Buf */
            int Size; /* �����ݰ��ֽڴ�С */
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
 UDP ����
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
 GRP��ز���
---------------------------------------------------------------------------------------------------------------------------------------------------------*/

typedef struct _packet_grp_node {
    char *Data;
    int Length;
} packet_grp_node_t;

typedef struct _packet_grp {
    packet_grp_node_t *Items;
    int Count;
} packet_grp_t;

/* ֧�ֿ�汾Э��	*/
typedef struct _swnet_version {
    short procedure_;
    short main_;
    short sub_;
    short leaf_;
} swnet_version_t;

#pragma pack(pop)

#endif