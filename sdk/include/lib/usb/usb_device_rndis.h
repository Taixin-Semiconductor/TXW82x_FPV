/*****************************************************************************
* Module    : usb
* File      : usb_dev_rndis.h
* Author    : 
* Function  : USB RNDIS驱动的一些定义
*****************************************************************************/
#ifndef USB_DEV_RNDIS_H
#define USB_DEV_RNDIS_H

#ifdef __cplusplus
 extern "C" {
#endif

/*---------------------------------------------------------------------*/
/*  CDC_RNDIS definitions                                              */
/*---------------------------------------------------------------------*/

/** Implemented CDC_RNDIS Version Major */
#define CDC_RNDIS_VERSION_MAJOR                     0x01U

/* Implemented CDC_RNDIS Version Minor */
#define CDC_RNDIS_VERSION_MINOR                     0x00U

/* Maximum size in bytes of a CDC_RNDIS control message
   which can be sent or received */
#define CDC_RNDIS_MESSAGE_BUFFER_SIZE               128U

/* Maximum size in bytes of an Ethernet frame
   according to the Ethernet standard */
#define CDC_RNDIS_ETH_FRAME_SIZE_MAX                1536U

/* Maximum size allocated for buffer
   inside Query messages structures */
#define CDC_RNDIS_MAX_INFO_BUFF_SZ                  200U
#define CDC_RNDIS_MAX_DATA_SZE                      2000U

/* Notification request value for a CDC_RNDIS
   Response Available notification */
#define CDC_RNDIS_NOTIFICATION_RESP_AVAILABLE       0x00000001UL


#define CDC_RNDIS_PACKET_MSG_ID                     0x00000001UL
#define CDC_RNDIS_INITIALIZE_MSG_ID                 0x00000002UL
#define CDC_RNDIS_HALT_MSG_ID                       0x00000003UL
#define CDC_RNDIS_QUERY_MSG_ID                      0x00000004UL
#define CDC_RNDIS_SET_MSG_ID                        0x00000005UL
#define CDC_RNDIS_RESET_MSG_ID                      0x00000006UL
#define CDC_RNDIS_INDICATE_STATUS_MSG_ID            0x00000007UL
#define CDC_RNDIS_KEEPALIVE_MSG_ID                  0x00000008UL

#define CDC_RNDIS_INITIALIZE_CMPLT_ID               0x80000002UL
#define CDC_RNDIS_QUERY_CMPLT_ID                    0x80000004UL
#define CDC_RNDIS_SET_CMPLT_ID                      0x80000005UL
#define CDC_RNDIS_RESET_CMPLT_ID                    0x80000006UL
#define CDC_RNDIS_KEEPALIVE_CMPLT_ID                0x80000008UL

#define CDC_RNDIS_STATUS_SUCCESS                    0x00000000UL
#define CDC_RNDIS_STATUS_FAILURE                    0xC0000001UL
#define CDC_RNDIS_STATUS_INVALID_DATA               0xC0010015UL
#define CDC_RNDIS_STATUS_NOT_SUPPORTED              0xC00000BBUL
#define CDC_RNDIS_STATUS_MEDIA_CONNECT              0x4001000BUL
#define CDC_RNDIS_STATUS_MEDIA_DISCONNECT           0x4001000CUL
/** Media state */
#define CDC_RNDIS_MEDIA_STATE_CONNECTED             0x00000000UL
#define CDC_RNDIS_MEDIA_STATE_DISCONNECTED          0x00000001UL

/** Media types */
#define CDC_RNDIS_MEDIUM_802_3                      0x00000000UL

#define CDC_RNDIS_DF_CONNECTIONLESS                 0x00000001UL
#define CDC_RNDIS_DF_CONNECTION_ORIENTED            0x00000002UL

/** Hardware status of the underlying NIC */
#define CDC_RNDIS_HW_STS_READY                      0x00000000UL
#define CDC_RNDIS_HW_STS_INITIALIZING               0x00000001UL
#define CDC_RNDIS_HW_STS_RESET                      0x00000002UL
#define CDC_RNDIS_HW_STS_CLOSING                    0x00000003UL
#define CDC_RNDIS_HW_STS_NOT_READY                  0x00000004UL

/** Packet filter */
#define CDC_RNDIS_PACKET_DIRECTED                   0x00000001UL
#define CDC_RNDIS_PACKET_MULTICAST                  0x00000002UL
#define CDC_RNDIS_PACKET_ALL_MULTICAST              0x00000004UL
#define CDC_RNDIS_PACKET_BROADCAST                  0x00000008UL
#define CDC_RNDIS_PACKET_SOURCE_ROUTING             0x00000010UL
#define CDC_RNDIS_PACKET_PROMISCUOUS                0x00000020UL
#define CDC_RNDIS_PACKET_SMT                        0x00000040UL
#define CDC_RNDIS_PACKET_ALL_LOCAL                  0x00000080UL
#define CDC_RNDIS_PACKET_GROUP                      0x00001000UL
#define CDC_RNDIS_PACKET_ALL_FUNCTIONAL             0x00002000UL
#define CDC_RNDIS_PACKET_FUNCTIONAL                 0x00004000UL
#define CDC_RNDIS_PACKET_MAC_FRAME                  0x00008000UL

#define OID_GEN_SUPPORTED_LIST                      0x00010101UL
#define OID_GEN_HARDWARE_STATUS                     0x00010102UL
#define OID_GEN_MEDIA_SUPPORTED                     0x00010103UL
#define OID_GEN_MEDIA_IN_USE                        0x00010104UL
#define OID_GEN_MAXIMUM_FRAME_SIZE                  0x00010106UL
#define OID_GEN_LINK_SPEED                          0x00010107UL
#define OID_GEN_TRANSMIT_BLOCK_SIZE                 0x0001010AUL
#define OID_GEN_RECEIVE_BLOCK_SIZE                  0x0001010BUL
#define OID_GEN_VENDOR_ID                           0x0001010CUL
#define OID_GEN_VENDOR_DESCRIPTION                  0x0001010DUL
#define OID_GEN_CURRENT_PACKET_FILTER               0x0001010EUL
#define OID_GEN_MAXIMUM_TOTAL_SIZE                  0x00010111UL
#define OID_GEN_MEDIA_CONNECT_STATUS                0x00010114UL
#define OID_GEN_MAXIMUM_SEND_PACKETS                0x00010115UL
#define OID_GEN_VENDOR_DRIVER_VERSION               0x00010116UL
#define OID_GEN_PHYSICAL_MEDIUM                     0x00010202UL
#define OID_GEN_XMIT_OK                             0x00020101UL
#define OID_GEN_RCV_OK                              0x00020102UL
#define OID_GEN_XMIT_ERROR                          0x00020103UL
#define OID_GEN_RCV_ERROR                           0x00020104UL
#define OID_GEN_RCV_NO_BUFFER                       0x00020105UL
#define OID_GEN_CDC_RNDIS_CONFIG_PARAMETER          0x0001021BUL
#define OID_802_3_PERMANENT_ADDRESS                 0x01010101UL
#define OID_802_3_CURRENT_ADDRESS                   0x01010102UL
#define OID_802_3_MULTICAST_LIST                    0x01010103UL
#define OID_802_3_MAXIMUM_LIST_SIZE                 0x01010104UL
#define OID_802_3_MAC_OPTIONS                       0x01010105UL  // deprecated ndis6
#define OID_802_3_RCV_ERROR_ALIGNMENT               0x01020101UL
#define OID_802_3_XMIT_ONE_COLLISION                0x01020102UL
#define OID_802_3_XMIT_MORE_COLLISIONS              0x01020103UL


#define CDC_RNDIS_SEND_ENCAPSULATED_COMMAND         0x00U
#define CDC_RNDIS_GET_ENCAPSULATED_RESPONSE         0x01U

#define CDC_RNDIS_NET_DISCONNECTED                  0x00U
#define CDC_RNDIS_NET_CONNECTED                     0x01U

#define CDC_RNDIS_BMREQUEST_TYPE_RNDIS              0xA1U
#define CDC_RNDIS_PCKTMSG_DATAOFFSET_OFFSET         8U

/* CDC_RNDIS State values */
typedef enum
{
    CDC_RNDIS_STATE_UNINITIALIZED    = 0,
    CDC_RNDIS_STATE_BUS_INITIALIZED  = 1,
    CDC_RNDIS_STATE_INITIALIZED      = 2,
    CDC_RNDIS_STATE_DATA_INITIALIZED = 3
} USBD_CDC_RNDIS_StateTypeDef;

typedef struct
{
    uint8_t   bmRequest;
    uint8_t   bRequest;
    uint16_t  wValue;
    uint16_t  wIndex;
    uint16_t  wLength;
    uint8_t   data[8];
} USBD_CDC_RNDIS_NotifTypeDef;

/* Messages Sent by the Host ---------------------*/

/* Type define for a CDC_RNDIS Initialize command message */
typedef struct
{
    uint32_t MsgType;
    uint32_t MsgLength;
    uint32_t ReqId;
    uint32_t MajorVersion;
    uint32_t MinorVersion;
    uint32_t MaxTransferSize;
} USBD_CDC_RNDIS_InitMsgTypeDef;

/* Type define for a CDC_RNDIS Halt Message */
typedef struct
{
    uint32_t MsgType;
    uint32_t MsgLength;
    uint32_t ReqId;
} USBD_CDC_RNDIS_HaltMsgTypeDef;

/* Type define for a CDC_RNDIS Query command message */
typedef struct
{
    uint32_t MsgType;
    uint32_t MsgLength;
    uint32_t RequestId;
    uint32_t Oid;
    uint32_t InfoBufLength;
    uint32_t InfoBufOffset;
    uint32_t DeviceVcHandle;
    uint32_t InfoBuf[0];
} USBD_CDC_RNDIS_QueryMsgTypeDef;

/* Type define for a CDC_RNDIS Set command message */
typedef struct
{
    uint32_t MsgType;
    uint32_t MsgLength;
    uint32_t ReqId;
    uint32_t Oid;
    uint32_t InfoBufLength;
    uint32_t InfoBufOffset;
    uint32_t DeviceVcHandle;
    uint32_t InfoBuf[0];
} USBD_CDC_RNDIS_SetMsgTypeDef;

/* Type define for a CDC_RNDIS Reset message */
typedef struct
{
    uint32_t MsgType;
    uint32_t MsgLength;
    uint32_t Reserved;
} USBD_CDC_RNDIS_ResetMsgTypeDef;

/* Type define for a CDC_RNDIS Keepalive command message */
typedef struct
{
    uint32_t MsgType;
    uint32_t MsgLength;
    uint32_t ReqId;
} USBD_CDC_RNDIS_KpAliveMsgTypeDef;


/* Messages Sent by the Device ---------------------*/

/* Type define for a CDC_RNDIS Initialize complete response message */
typedef struct
{
    uint32_t MsgType;
    uint32_t MsgLength;
    uint32_t ReqId;
    uint32_t Status;
    uint32_t MajorVersion;
    uint32_t MinorVersion;
    uint32_t DeviceFlags;
    uint32_t Medium;
    uint32_t MaxPacketsPerTransfer;
    uint32_t MaxTransferSize;
    uint32_t PacketAlignmentFactor;
    uint32_t AFListOffset;
    uint32_t AFListSize;
} USBD_CDC_RNDIS_InitCpltMsgTypeDef;

/* Type define for a CDC_RNDIS Query complete response message */
typedef struct
{
    uint32_t MsgType;
    uint32_t MsgLength;
    uint32_t ReqId;
    uint32_t Status;
    uint32_t InfoBufLength;
    uint32_t InfoBufOffset;
    uint32_t InfoBuf[0];
} USBD_CDC_RNDIS_QueryCpltMsgTypeDef;

/* Type define for a CDC_RNDIS Set complete response message */
typedef struct
{
    uint32_t MsgType;
    uint32_t MsgLength;
    uint32_t ReqId;
    uint32_t Status;
} USBD_CDC_RNDIS_SetCpltMsgTypeDef;

/* Type define for a CDC_RNDIS Reset complete message */
typedef struct
{
    uint32_t MsgType;
    uint32_t MsgLength;
    uint32_t Status;
    uint32_t AddrReset;
} USBD_CDC_RNDIS_ResetCpltMsgTypeDef;

/* Type define for CDC_RNDIS struct to indicate a change
   in the status of the device */
typedef struct
{
    uint32_t MsgType;
    uint32_t MsgLength;
    uint32_t Status;
    uint32_t StsBufLength;
    uint32_t StsBufOffset;
} USBD_CDC_RNDIS_StsChangeMsgTypeDef;

/* Type define for a CDC_RNDIS Keepalive complete message */
typedef struct
{
    uint32_t MsgType;
    uint32_t MsgLength;
    uint32_t ReqId;
    uint32_t Status;
} USBD_CDC_RNDIS_KpAliveCpltMsgTypeDef;


/* Messages Sent by both Host and Device ---------------------*/

/* Type define for a CDC_RNDIS packet message, used to encapsulate
   Ethernet packets sent to and from the adapter */
typedef struct
{
    uint32_t MsgType;
    uint32_t MsgLength;
    uint32_t DataOffset;
    uint32_t DataLength;
    uint32_t OOBDataOffset;
    uint32_t OOBDataLength;
    uint32_t NumOOBDataElements;
    uint32_t PerPacketInfoOffset;
    uint32_t PerPacketInfoLength;
    uint32_t VcHandle;
    uint32_t Reserved;
} USBD_CDC_RNDIS_PacketMsgTypeDef;

/* Miscellaneous types used for parsing ---------------------*/

/* The common part for all CDC_RNDIS messages Complete response */
typedef struct
{
    uint32_t MsgType;
    uint32_t MsgLength;
    uint32_t ReqId;
    uint32_t Status;
} USBD_CDC_RNDIS_CommonCpltMsgTypeDef;

/* Type define for a single parameter structure */
typedef struct
{
    uint32_t ParamNameOffset;
    uint32_t ParamNameLength;
    uint32_t ParamType;
    uint32_t ParamValueOffset;
    uint32_t ParamValueLength;
} USBD_CDC_RNDIS_ParamStructTypeDef;


/* Type define of a single CDC_RNDIS OOB data record */
typedef struct
{
    uint32_t Size;
    uint32_t Type;
    uint32_t ClassInfoType;
    uint32_t OOBData[sizeof(uint32_t)];
} USBD_CDC_RNDIS_OOBPacketTypeDef;

/* Type define for notification structure */
typedef struct
{
    uint32_t notification;
    uint32_t reserved;
} USBD_CDC_RNDIS_NotifStructTypeDef;

/* This structure will be used to store the type, the size and ID for any
   received message from the control endpoint */
typedef struct
{
    uint32_t MsgType;
    uint32_t MsgLength;
} USBD_CDC_RNDIS_CtrlMsgTypeDef;

typedef union
{
    USBD_CDC_RNDIS_PacketMsgTypeDef         Packet;
    USBD_CDC_RNDIS_InitMsgTypeDef           Init;
    USBD_CDC_RNDIS_InitCpltMsgTypeDef       InitCplt;
    USBD_CDC_RNDIS_HaltMsgTypeDef           Halt;
    USBD_CDC_RNDIS_QueryMsgTypeDef          Query;
    USBD_CDC_RNDIS_QueryCpltMsgTypeDef      QueryCplt;
    USBD_CDC_RNDIS_SetMsgTypeDef            Set;
    USBD_CDC_RNDIS_SetCpltMsgTypeDef        SetCplt;
    USBD_CDC_RNDIS_ResetMsgTypeDef          Reset;
    USBD_CDC_RNDIS_ResetCpltMsgTypeDef      ResetCplt;
    USBD_CDC_RNDIS_StsChangeMsgTypeDef      Indicate;
    USBD_CDC_RNDIS_KpAliveMsgTypeDef        KpAlive;
    USBD_CDC_RNDIS_KpAliveCpltMsgTypeDef    KpAliveCplt;
    USBD_CDC_RNDIS_CtrlMsgTypeDef           Ctrl;
} USBD_CDC_RNDIS_MsgTypeDef;


#ifdef __cplusplus
}
#endif


#endif
