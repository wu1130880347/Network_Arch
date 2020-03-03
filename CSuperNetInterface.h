#ifndef __CSUPERNETINTERFACE_H__
#define __CSUPERNETINTERFACE_H__
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "CNetManagement_Config.h"
#include "dbg_tools.h"
#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t
 typedef enum {FALSE = 0, TRUE} BOOL;

typedef enum
{
    NET_TX,
    NET_RX
}RxTxD_t;

typedef enum
{
    NET_OK,
    NET_ERR,
    NET_BUSY
}Net_Status_t;
//网络类型
typedef enum
{
    NET_NBIOT,
    NET_RJ45,
    NET_LORE,
    NET_GPRS,
    NET_WIFI,
    NET_BT,
    NET_UART
}NetType_t;

typedef enum
{
    APP_NBIOT,
    APP_RJ45,
    APP_LORE,
    APP_GPRS,
    APP_WIFI,
    APP_BT,
    APP_UART
}AppType_t;

//网络中心数据结构
#pragma pack(1)
//接受发送最大buffer
typedef struct
{
    uint16_t rx_max;
    uint16_t tx_max;
}Buffer_rtd_t;
//网络配置参数表
typedef struct
{
    void *dat;
}NetConfig_t;
//连接中心配置属性
typedef struct
{
    NetType_t t_net;
    AppType_t t_app;
    Buffer_rtd_t t_rdt_num;
    NetConfig_t t_para;    
}NetParaType_t;
//网络中心数据结构
typedef struct
{
    NetType_t t_net;
}NetTarget_t;
//网络主动发送到达使用协议

typedef struct
{
    uint16_t Len; //数据长度
    uint8_t *Data; //数据体
} RECEIVE_FRAME;
typedef struct
{
    uint16_t Len; //数据长度
    uint8_t *Data;
} SEND_FRAME;

typedef struct
{
    RECEIVE_FRAME *p_receFrame;
    SEND_FRAME *p_sendFrame;
}NetPackageStruct_t;

typedef struct
{
    uint16_t Len; //数据长度
}NetConfigStruct_t;

typedef struct
{
    uint16_t Len; //数据长度
}NetTestStruct_t;
#pragma pack()

class CSuperNetInterface
{
public:
    virtual Net_Status_t Detect(u16 *dev_id) = 0;
    virtual Net_Status_t Init(void *para) = 0;
    virtual Net_Status_t SendData(NetPackageStruct_t *dat) = 0;
    virtual Net_Status_t ReceData(NetPackageStruct_t *dat) = 0;
    virtual Net_Status_t NetConfig(NetConfigStruct_t *cfg) = 0;
    virtual Net_Status_t NetTest(NetTestStruct_t *test) = 0;
};
#endif
