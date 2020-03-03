#include "IUarts.h"

//debug info output
#if DBGUART
extern "C"
{
#define DBG_USE 1

#if DBG_USE
    //是否打开该文件内的调试LOG
    static const char EN_LOG = YELLOW;
    //LOG输出文件标记
    static const char TAG[] = "UartDrv: ";
#else
#ifdef DBG_Printf
#undef DBG_Printf
#define DBG_Printf(...)
#else
#endif
#endif
}

#endif
IUarts::IUarts()
{}
IUarts::~IUarts()
{}
Net_Status_t IUarts::Detect(u16 *dev_id)
{
    return NET_OK;
}
Net_Status_t IUarts::Init(void *para)
{
    
    return NET_OK;
}
Net_Status_t IUarts::SendData(NetPackageStruct_t *dat)
{
    uint16_t t_len = dat->p_sendFrame->Len;
    uint8_t *t_dat = dat->p_sendFrame->Data;
    Dprintf(EN_LOG,TAG,"Uart send data len = %d\r\n",t_len);
    for(uint16_t i = 0;i<t_len;i++)
        Dprintf(EN_LOG,"","%02x ",t_dat[i]);
    Dprintf(EN_LOG,"","\r\n");
    return NET_OK;
}
Net_Status_t IUarts::ReceData(NetPackageStruct_t *dat)
{
    return NET_OK;
}
Net_Status_t IUarts::NetConfig(NetConfigStruct_t *cfg)
{
    return NET_OK;
}
Net_Status_t IUarts::NetTest(NetTestStruct_t *test)
{
    return NET_OK;
}