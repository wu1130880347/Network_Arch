#include "CUdpProtocol.h"
#include "CNetManagement.h"
#include "driver.h"

void CUdpProtocol::Init(CNetManagement *pthis,void *para)
{
    mp_this = pthis;
    m_dat_package.p_sendFrame= &m_SendFrame;
    m_dat_package.p_receFrame = &m_ReceiveFrame;
}
void CUdpProtocol::ComSend(void) 
{
    mp_this->ComSend();
}
NetPackageStruct_t *CUdpProtocol::Ret_ReceBuf(void)
{
    return &m_dat_package;
}
NetPackageStruct_t *CUdpProtocol::Ret_SendBuf(void)
{
    return &m_dat_package;
}
BOOL CUdpProtocol::ReceiveCheck(RECEIVE_FRAME *frame)
{
    u8 temp_buff[4] = {0xab, 0x12, 0xcd, 0x34};
    u8 temp_HMAC[8];

    //�����ж�
    if (m_Request == 0)
        return FALSE;
    //��������
    frame->Len = m_ReceiveFrame.Len;
    memcpy(frame->Data, m_ReceiveFrame.Data, m_ReceiveFrame.Len);
    m_Request = 0;

    //֡ͷ�ж�
    if (memcmp(frame->Data, temp_buff, 4))
        return FALSE;
    //�����ж�
    if (*(u16 *)(frame->Data + 4) != (frame->Len - 6))
    { //ճ������(ֻ����һ����)
        frame->Len = *(u16 *)(frame->Data + 4) + 6;
    }
    //�����ж�
    if ((frame->Len == 0) || (frame->Len > rx_buf_max))
        return FALSE;
    //DMAC�ж�
    if (memcmp(frame->Data + 6, 0x00000000ul, 8))
        return FALSE;
    //������������
    m_ReceiveRandomNum = *(u16 *)(frame->Data + 14);
    //HMAC�ж�
    g_CTool.HMAC((*(u16 *)(frame->Data + 4)) - 6, frame->Data + 4, 8, temp_HMAC);
    if (memcmp(frame->Data + (*(u16 *)(frame->Data + 4)) - 2, temp_HMAC, 8))
        return FALSE;

    return TRUE;
}

void CUdpProtocol::Agreement(void)
{
    if (ReceiveCheck(&m_ReceiveFrame))
        switch (*(u16 *)(m_ReceiveFrame.Data + 20))
        {
        case 0x0001: //�����ظ�
            //HeartBeatReply(m_ReceiveFrame.Data + 16);
            break;
        // case 0x0002: //ע��ظ�
        //     RegisterReply(m_ReceiveFrame.Data + 16);
        //     break;
        // case 0x0003: //��ȡ�豸��Ϣ
        //     GetPosState(m_ReceiveFrame.Data + 16);
        //     break;
        // case 0x0004: //�޸��豸���
        //     SetAddr(m_ReceiveFrame.Data + 16);
        //     break;
        // case 0x0005: //�޸ķ�����IP���˿�
        //     SetServer(m_ReceiveFrame.Data + 16);
        //     break;
        // case 0x0006: //��ȡ�豸ʱ��
        //     ReadTime(m_ReceiveFrame.Data + 16);
        //     break;
        // case 0x0007: //�޸��豸ʱ��
        //     SetTime(m_ReceiveFrame.Data + 16);
        //     break;
        // case 0x0008: //����ѯ�ظ�
        //     BalanceQueryReply(m_ReceiveFrame.Data + 16);
        //     break;
        // case 0x0009: //��������ظ�
        //     TradeReply(m_ReceiveFrame.Data + 16);
        //     break;
        // case 0x000A: //��ս��׼�¼
        //     EmptyNote(m_ReceiveFrame.Data + 16);
        //     break;
        // case 0x000B: //�زɽ��׼�¼
        //     RereadNote(m_ReceiveFrame.Data + 16);
        //     break;
        // case 0x000C: //�������ѯ�ظ�
        //     IncomeReply(m_ReceiveFrame.Data + 16);
        //     break;
        // case 0x000D: //�·��ļ���Ϣ
        //     DownloadFileName(m_ReceiveFrame.Data + 16);
        //     break;
        // case 0x000E: //�·��ļ�����
        //     DownloadFileData(m_ReceiveFrame.Data + 16);
        //     break;
        // case 0x000F: //Զ������
        //     LongRangeReset(m_ReceiveFrame.Data + 16);
        //     break;
        // case 0x0010: //����ʱ�����
        //     SetTimePara(m_ReceiveFrame.Data + 16);
        //     break;
        // case 0x0011: //������������
        //     CancelReply(m_ReceiveFrame.Data + 16);
        //     break;
        // case 0x0012: //�����Ʒ��Ϣ
        //     MenuEmpty(m_ReceiveFrame.Data + 16);
        //     break;
        // case 0x0013: //������Ʒ��Ϣ
        //     MenuSet(m_ReceiveFrame.Data + 16);
        //     break;
        // case 0x0014: //������Ʒ��Ϣ�汾��
        //     MenuVersionSet(m_ReceiveFrame.Data + 16);
        //     break;
        // case 0x0015: //���¿�����
        //     CardPara(m_ReceiveFrame.Data + 16);
        //     break;
        // case 0x0016: //�ɼ����׼�¼
        //     ReadNote(m_ReceiveFrame.Data + 16);
        //     break;
        // case 0x0017: //�������
        //     EmptyVoice(m_ReceiveFrame.Data + 16);
        //     break;
        // case 0x001E: //��ʱ�����ظ�
        //     TimeCancelReply(m_ReceiveFrame.Data + 16);
        //     break;
        // case 0x0024: //��ȡICCID
        //     GetICCIDResponse(m_ReceiveFrame.Data + 16);
        //     break;
        // case 0x0025: //ICCID�ϱ��ɹ���Ĵ���
        //     SendICCIDResponse();
        //     break;
        // case 0x0200: //���·������б�2017.07.21
        //     SetServerList(m_ReceiveFrame.Data + 16);
        //     break;
        // case 0x0201: //�޸�ͨѶ��Կ
        //     SetComKey(m_ReceiveFrame.Data + 16);
        //     break;
        // case 0x0202: //�޸�DMAC
        //     SetDMAC(m_ReceiveFrame.Data + 16);
        //     break;
        // case 0x0203: //�޸Ķ�ά���Ƿ���
        //     SetQRencode(m_ReceiveFrame.Data + 16);
        //     break;
        // case 0x0204: //��Ӫ����Կ�
        //     PairCardReply(m_ReceiveFrame.Data + 16);
        //     break;
        default:
            break;
        }
}