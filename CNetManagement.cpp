#include "CNetManagement.h"

//debug info output
#if DBGUART
extern "C"
{
#define DBG_USE 1

#if DBG_USE
    //是否打开该文件内的调试LOG
    static const char EN_LOG = CYAN;
    //LOG输出文件标记
    static const char TAG[] = "CNetM: ";
#else
#ifdef DBG_Printf
#undef DBG_Printf
#define DBG_Printf(...)
#else
#endif
#endif
}
#endif

CNetManagement::~CNetManagement()
{

}
CNetManagement::CNetManagement(NetParaType_t * para,CSuperAppInterface *g_app,CSuperNetInterface *g_net)
{
    Dprintf(EN_LOG,TAG,"config addr = 0x%08x\r\n",(void *)para);
    g_NetDrv = g_net;
    g_NetApp = g_app;
    Config(para);
}

void CNetManagement::Config(NetParaType_t * para)
{
    g_NetApp->Init(this,(void *)para);//初始化话协议解析必要资源
    g_NetDrv->Init((void *)(para->t_para.dat));//初始化连接驱动必要资源
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
