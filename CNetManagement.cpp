#include "CNetManagement.h"
CNetManagement::~CNetManagement()
{

}
CNetManagement::CNetManagement(NetParaType_t * para,CSuperAppInterface *g_app,CSuperNetInterface *g_net)
{
    g_NetDrv = g_net;
    g_NetApp = g_app;
    Config(para);
}

void CNetManagement::Config(NetParaType_t * para)
{
    g_NetApp->Init(this,(void *)para);//初始化话协议解析必要资源
    g_NetDrv->Init((void *)para);//舒适化连接驱动必要资源
}

void CNetManagement::ComSend(void)
{
    g_NetDrv->SendData(g_NetApp->Ret_SendBuf());
}

void CNetManagement::Agreement(void)
{
    if(g_NetDrv->ReceData(g_NetApp->Ret_ReceBuf()) == NET_OK)
    {
        g_NetApp->Agreement();
    }
}

void CNetManagement::TimerEvent(uint32_t ulTimeNum)
{
    return ;
}
