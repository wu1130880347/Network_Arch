#ifndef __CSUPERAPPINTERFACE_H__
#define __CSUPERAPPINTERFACE_H__
#include "CSuperNetInterface.h"

//�����������ݽṹ
#pragma pack(1)
typedef struct
{
    uint16_t Len;      //���ݳ���
    uint8_t Data[600]; //������
} RECEIVE_FRAME;
#pragma pack()

class CSuperAppInterface
{
public:
    virtual void Init(void) = 0;
    virtual void ComSend(void) = 0;
    virtual BOOL ReceiveCheck(RECEIVE_FRAME *frame) = 0;
    virtual void Agreement(void) = 0;
};
#endif
