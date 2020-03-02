#ifndef __CUDPPROTOCOL_H__
#define __CUDPPROTOCOL_H__
#include "CSuperAppInterface.h"

class CUdpProtocol : public CSuperAppInterface
{
private:
    uint16_t m_SendRandomNum;    //主动发送时的随机数
    uint16_t m_ReceiveRandomNum; //接收到的随机数
    uint32_t m_CommandNum;       //主动发送时的命令流水号
    uint32_t m_CommandNumSave;   //保存主动发送的命令流水号
    uint16_t rx_buf_max;
    uint16_t tx_buf_max;
    CNetManagement *mp_this;
    NetPackageStruct_t m_dat_package;
public:
    RECEIVE_FRAME m_ReceiveFrame; //接收数据缓冲
    SEND_FRAME m_SendFrame;       //发送数据缓冲
    uint8_t m_Request;
    virtual void Init(CNetManagement *pthis,void *para);
    virtual void ComSend(void);
    virtual NetPackageStruct_t* Ret_ReceBuf(void);
    virtual NetPackageStruct_t* Ret_SendBuf(void);
    virtual BOOL ReceiveCheck(RECEIVE_FRAME *frame);
    virtual void Agreement(void);
};

#endif