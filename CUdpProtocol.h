#ifndef __CUDPPROTOCOL_H__
#define __CUDPPROTOCOL_H__
#include "CSuperAppInterface.h"

class CUdpProtocol : public CSuperAppInterface
{
private:
    uint16_t m_SendRandomNum;    //��������ʱ�������
    uint16_t m_ReceiveRandomNum; //���յ��������
    uint32_t m_CommandNum;       //��������ʱ��������ˮ��
    uint32_t m_CommandNumSave;   //�����������͵�������ˮ��
    uint16_t rx_buf_max;
    uint16_t tx_buf_max;
    CNetManagement *mp_this;
    NetPackageStruct_t m_dat_package;
public:
    RECEIVE_FRAME m_ReceiveFrame; //�������ݻ���
    SEND_FRAME m_SendFrame;       //�������ݻ���
    uint8_t m_Request;
    virtual void Init(CNetManagement *pthis,void *para);
    virtual void ComSend(void);
    virtual NetPackageStruct_t* Ret_ReceBuf(void);
    virtual NetPackageStruct_t* Ret_SendBuf(void);
    virtual BOOL ReceiveCheck(RECEIVE_FRAME *frame);
    virtual void Agreement(void);
};

#endif