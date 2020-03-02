#ifndef __CNETMANAGEMENT_H__
#define __CNETMANAGEMENT_H__
#include "CUdpProtocol.h"
#include "CSuperNetInterface.h"
#include "CSuperAppInterface.h"

class CNetManagement
{
private:
    NetType_t this_net;
    CSuperNetInterface *g_NetDrv;
    CSuperAppInterface *g_NetApp;
public:
    CNetManagement();
    ~CNetManagement();
    void Config(void);
    virtual void TimerEvent(u32 ulTimeNum);
    virtual void ComSend(void);
    virtual BOOL ReceiveCheck(RECEIVE_FRAME *frame);
};

#endif