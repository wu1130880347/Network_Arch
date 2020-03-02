#include "IGPRS.h"
IGPRS::IGPRS()
{}
IGPRS::~IGPRS()
{}
Net_Status_t IGPRS::Detect(u16 *dev_id)
{
    return NET_OK;
}
Net_Status_t IGPRS::Init(void *para)
{
    
    return NET_OK;
}
Net_Status_t IGPRS::SendData(NetPackageStruct_t *dat)
{
    return NET_OK;
}
Net_Status_t IGPRS::ReceData(NetPackageStruct_t *dat)
{
    return NET_OK;
}
Net_Status_t IGPRS::NetConfig(NetConfigStruct_t *cfg)
{
    return NET_OK;
}
Net_Status_t IGPRS::NetTest(NetTestStruct_t *test)
{
    return NET_OK;
}