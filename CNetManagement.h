#ifndef __CNETMANAGEMENT_H__
#define __CNETMANAGEMENT_H__
#include "CSuperNetInterface.h"
#include "CSuperAppInterface.h"

class CSuperNetInterface;
class CSuperAppInterface;

class CNetManagement
{
private:
    NetParaType_t this_net;
    CSuperNetInterface *g_NetDrv;
    CSuperAppInterface *g_NetApp;
public:
    ~CNetManagement();
    CNetManagement(NetParaType_t * para,CSuperAppInterface *g_app,CSuperNetInterface *g_net);
    void Config(NetParaType_t * para);
    virtual void Agreement(void);
    virtual void ComSend(void);
    virtual BOOL ReceiveCheck(RECEIVE_FRAME *frame);
    virtual void TimerEvent(uint32_t ulTimeNum);
};

#endif