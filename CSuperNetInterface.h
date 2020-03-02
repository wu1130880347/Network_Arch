#ifndef __CSUPERNETINTERFACE_H__
#define __CSUPERNETINTERFACE_H__
#include <stdint.h>
#include <stdbool.h>
#include "CNetManagement_Config.h"
#include "dbg_tools.h"



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

//网络中心数据结构
#pragma pack(1)
typedef struct
{
    
}NetPackageStruct_t;

typedef struct
{
    
}NetConfigStruct_t;

typedef struct
{
    
}NetTestStruct_t;
#pragma pack()

class CSuperNetInterface
{
public:
    virtual Net_Status_t Detect(u16 *dev_id) = 0;
    virtual Net_Status_t Init(u16 dev_id) = 0;
    virtual Net_Status_t SendData(NetPackageStruct_t *dat) = 0;
    virtual Net_Status_t ReceData(NetPackageStruct_t *dat) = 0;
    virtual Net_Status_t NetConfig(NetConfigStruct_t *cfg) = 0;
    virtual Net_Status_t NetTest(NetTestStruct_t *test) = 0;
};
#endif
