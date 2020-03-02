#include "CNetManagement.h"
CNetManagement::CNetManagement()
{

}
CNetManagement::~CNetManagement()
{

}
void CNetManagement::Config(void)
{
}
void CNetManagement::network_RxTxd(u8 SocketNum,uint8_t* dat,uint16_t *len,BOOL *ComSndRequest,RxTxD_t rtx_f)
{

}

BOOL CNetManagement::ReceiveCheck(RECEIVE_FRAME *frame)
{
    return FALSE;
}
void CNetManagement::ComSend(void)
{

}
void CNetManagement::TimerEvent(u32 ulTimeNum)
{
    switch (ulTimeNum)
    {
    case 1:
    {
        break;
    }
    default:
        break;
    }
}
