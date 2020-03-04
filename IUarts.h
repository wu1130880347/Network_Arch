#ifndef __IUarts_H__
#define __IUarts_H__
#include "CSuperNetInterface.h"

#pragma pack(1)
typedef struct
{
    uint8_t uart_type;  //0x01 普通串口
    uint8_t uart_id;    //0x01 串口1 0x02 串口2
    uint32_t baud_rate; //波特率
    uint8_t *rx_buffer; 
    uint16_t rx_max;
    uint8_t *tx_buffer;
    uint16_t tx_max;
} NetDrvUartStruct_t;
#pragma pack()

class IUarts : public CSuperNetInterface
{
private:
public:
    IUarts();
    ~IUarts();
    virtual Net_Status_t Detect(u16 *dev_id);
    virtual Net_Status_t Init(void *para);
    virtual Net_Status_t SendData(NetPackageStruct_t *dat);
    virtual Net_Status_t ReceData(NetPackageStruct_t *dat);
    virtual Net_Status_t NetConfig(NetConfigStruct_t *cfg);
    virtual Net_Status_t NetTest(NetTestStruct_t *test);
};
#endif