#include "IRJ45.h"
IRJ45::IRJ45()
{}
IRJ45::~IRJ45()
{}
Net_Status_t IRJ45::Detect(u16 *dev_id)
{
    return NET_OK;
}
Net_Status_t IRJ45::Init(void *para)
{
    
    return NET_OK;
}
Net_Status_t IRJ45::SendData(NetPackageStruct_t *dat)
{
    return NET_OK;
}
Net_Status_t IRJ45::ReceData(NetPackageStruct_t *dat)
{
    return NET_OK;
}
Net_Status_t IRJ45::NetConfig(NetConfigStruct_t *cfg)
{
    return NET_OK;
}
Net_Status_t IRJ45::NetTest(NetTestStruct_t *test)
{
    return NET_OK;
}