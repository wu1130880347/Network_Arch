#include <map>
#include "CUartProtocol.h"
#include "CNetManagement.h"
#include "driver.h"


//debug info output
#if DBGUART
extern "C"
{
#define DBG_USE 1

#if DBG_USE
    //�Ƿ�򿪸��ļ��ڵĵ���LOG
    static const char EN_LOG = GREEN;
    //LOG����ļ����
    static const char TAG[] = "UartPro: ";
#else
#ifdef DBG_Printf
#undef DBG_Printf
#define DBG_Printf(...)
#else
#endif
#endif
}

#endif

static std::map<void*,CUartProtocol*> m_udpmap;
CUartProtocol::CUartProtocol()
{
    m_SendRandomNum = 0;    //��������ʱ�������
    m_ReceiveRandomNum = 0; //���յ��������
    m_CommandNum = 0;       //��������ʱ��������ˮ��
    m_CommandNumSave = 0;   //�����������͵�������ˮ��
    rx_buf_max = 0;
    tx_buf_max = 0;
    mp_this = nullptr;
    m_dat_package.p_sendFrame= nullptr;
    m_dat_package.p_receFrame = nullptr;
}
CUartProtocol::~CUartProtocol()
{

}
CUartProtocol *CUartProtocol::get_this(void* t_target)
{
    Dprintf(EN_LOG,TAG,"Cnet get_this = 0x%08x\r\n",t_target);
    static CUartProtocol * p_temp;
    p_temp = m_udpmap[t_target];
    return p_temp;
}

void CUartProtocol::Init(CNetManagement *pthis,void *para)
{
    NetParaType_t *temp = (NetParaType_t*)para;
    Dprintf(EN_LOG,TAG,"Cnet addr = 0x%08x\r\n",(void *)pthis);
    Dprintf(EN_LOG,TAG,"txf_max = %d rxf_max = %d\r\n",temp->t_rdt_num.tx_max,temp->t_rdt_num.rx_max);

    m_SendFrame.Data = temp->t_rdt_num.tx_buf;//��ʼ����������buffer
    m_ReceiveFrame.Data = temp->t_rdt_num.rx_buf;
    rx_buf_max = temp->t_rdt_num.tx_max;
    tx_buf_max = temp->t_rdt_num.rx_max;
    mp_this = pthis;
    m_dat_package.p_sendFrame= &m_SendFrame;
    m_dat_package.p_receFrame = &m_ReceiveFrame;
    m_udpmap[para] = this;//���������ͼ��
}
void CUartProtocol::ComSend(void)
{
    mp_this->ComSend();
}
NetPackageStruct_t *CUartProtocol::Ret_ReceBuf(void)
{
    return &m_dat_package;
}
NetPackageStruct_t *CUartProtocol::Ret_SendBuf(void)
{
    return &m_dat_package;
}
BOOL CUartProtocol::ReceiveCheck(RECEIVE_FRAME *frame)
{
    //�����ж�
    // Dprintf(EN_LOG,TAG, "Agreement Rece data len = %d\r\n", frame->Len);
    // for (uint16_t i = 0; i < frame->Len; i++)
    //     Dprintf(EN_LOG,"", "%02x ", frame->Data[i]);
    // Dprintf(EN_LOG,"", "\r\n");
    if (frame->Data[4] == g_CTool.CRC8Value((uint8_t *)&frame->Data[5], *(uint16_t *)&(frame->Data[2])))
    {
        Dprintf(EN_LOG,TAG, "check rece suc...\r\n");
        return TRUE;
    }
    else
    {   
        Dprintf(EN_LOG,TAG, "check rece fal...\r\n");
        return FALSE;
    }
}

void CUartProtocol::Agreement(void)
{
    if (!ReceiveCheck(&m_ReceiveFrame))
        return;
    TestReturnInfo(&m_ReceiveFrame);
    uint8_t *p_tmp = m_ReceiveFrame.Data + 5;
    u16 ret = p_tmp[0] | (u16)p_tmp[1] << 8;
    switch (ret)
    {
    case 0x0001: //�����ظ�
        //HeartBeatReply(m_ReceiveFrame.Data + 16);
        break;
    case 0x0a00: //�·������ļ���Ϣ�ظ�
    case 0x0a01: //�·������ļ����ݻظ�
        Transfer_FileData(p_tmp + 2);
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
//��������(��ڲ���ֻ��������)
//mode 0:�ظ����ͣ�1:�������� ����(������ˮ�Ź̶�0���ϲ��Ѹ�ֵ)��2:������������ָ��(������ˮ���ڴ˴���ֵ)
void CUartProtocol::SendFrame(u16 len, u8 *pData, u8 mode)
{
	//m_SendFrame.Len = len + 18 + 6;
    m_SendFrame.Len = len + 5;
    memcpy(m_SendFrame.Data + 5, pData, len);
	// if (m_SendFrame.Len > tx_buf_max)
	// 	return;
	// //֡ͷ
	m_SendFrame.Data[0] = 0x5a;
	m_SendFrame.Data[1] = 0xa5;
	// m_SendFrame.Data[2] = 0x5e;
	// m_SendFrame.Data[3] = 0xb4;
	// //֡����(D-MAC(8)�������(2)��������MAC(8))
	 *(u16*)(m_SendFrame.Data+2) = len;
     *(u8*)(m_SendFrame.Data+4) = g_CTool.CRC8Value(m_SendFrame.Data + 5,len);
	// //D-MAC
    // static uint8_t temp_dmac[8] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
	// memcpy(m_SendFrame.Data+6, temp_dmac, 8);
	// //�����
	// if (mode)//��������
	// 	*(u16*)(m_SendFrame.Data+14) = ++m_SendRandomNum;
	// else	//�ظ�����
	// 	*(u16*)(m_SendFrame.Data+14) = m_ReceiveRandomNum;
	// //������
	// memcpy(m_SendFrame.Data+16, pData, len);
	// //�������������ˮ��
	// if (mode == 2)
	// {
	// 	*(u32*)(m_SendFrame.Data+16) = ++m_CommandNum;
	// 	m_CommandNumSave = m_CommandNum;
	// }
    // //static uint8_t temp[8] = {0,1,2,3,4,5,6,7};
    
    // //memcpy(m_SendFrame.Data+len+16, temp, 8);
	// //HMAC(֡���ȡ�D-MAC���������������)
	// g_CTool.HMAC(len+12, m_SendFrame.Data+4, 8, m_SendFrame.Data+len+16);

     ComSend();
}
void CUartProtocol::HeartBeatSend(void)
{
    u8 temp_buff[8];

    *(u32 *)(temp_buff) = 0;                                     //������ˮ��
    *(u16 *)(temp_buff + 4) = 0x0001;                            //������
    temp_buff[6] = 0x00; //����GPRS��ʼ��ʧ��
    temp_buff[7] = 0x00;

    SendFrame(8, temp_buff, 1);
}
void CUartProtocol::TestReturnInfo(RECEIVE_FRAME *frame)
{
    //SendFrame(frame->Len, frame->Data, 0);
}
void CUartProtocol::Transfer_FileData(u8 *pData)
{
    uint8_t temp_dat[3];
    *(uint16_t *)temp_dat = 0x0a01;
    temp_dat[2] = 0;
    SendFrame(3, temp_dat, 1);
    Dprintf(EN_LOG,TAG, "rece pos = %d\r\n",*(uint32_t *)pData);
}