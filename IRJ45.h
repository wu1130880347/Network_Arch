#ifndef __IRJ45_H__
#define __IRJ45_H__
#include "CSuperNetInterface.h"

class IRJ45 : public CSuperNetInterface
{
public:
    IGPRS();
    ~IGPRS();
    virtual Net_Status_t Detect(u16 *dev_id);
    virtual Net_Status_t Init(u16 dev_id);
    virtual Net_Status_t SendData(NetPackageStruct_t *dat);
    virtual Net_Status_t ReceData(NetPackageStruct_t *dat);
    virtual Net_Status_t NetConfig(NetConfigStruct_t *cfg);
    virtual Net_Status_t NetTest(NetTestStruct_t *test);
};
#endif