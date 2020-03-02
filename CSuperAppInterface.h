#ifndef __CSUPERAPPINTERFACE_H__
#define __CSUPERAPPINTERFACE_H__
#include "CNetManagement.h"
class CNetManagement;

class CSuperAppInterface
{
public:
    virtual void Init(CNetManagement *pthis,void *para) = 0;
    virtual void ComSend(void) = 0;
    virtual NetPackageStruct_t* Ret_ReceBuf(void) = 0;
    virtual NetPackageStruct_t* Ret_SendBuf(void) = 0;
    virtual BOOL ReceiveCheck(RECEIVE_FRAME *frame) = 0;
    virtual void Agreement(void) = 0;
};
#endif
