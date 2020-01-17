#include "CUdpProtocol.h"
#include "driver.h"
#include "IUpdate.h"
#include "INoteStream.h"
#include "IVoice.h"
#include "IChannels.h"
#include "IShortRecord.h"
#include "IGID.h"
#include "IUsart.h"
#include "IAbnormalInf.h"

u16 CUdpProtocol::m_SendRandomNum;		//��������ʱ�������
u16 CUdpProtocol::m_ReceiveRandomNum;	//���յ��������
u32 CUdpProtocol::m_CommandNum;		//��������ʱ��������ˮ��
u32 CUdpProtocol::m_CommandNumSave;	//�����������͵�������ˮ��
__no_init UPGRADE_LIST structUpgrade @ (0x20008D00);//��������(4bytes)
u8 CUdpProtocol::u8ServerUpdate = 0;//������IP����:0�����£�1��Ҫ������2�����У�3����IP��flash

CUdpProtocol::CUdpProtocol()
{
	m_SendRandomNum = 0;
	m_CommandNum = 0;
	m_CommandNumSave = 0;
	m_SendFlag = 0;
	m_Request = 0;
	memset(onlinetrade_fg, 0, sizeof(onlinetrade_fg));
	memset(pNoteStream, NULL, sizeof(pNoteStream));
	memset(wait_timeout, 0, sizeof(wait_timeout));
	memset(reSend_request, NULL, sizeof(reSend_request));
	memset(online_channels, NULL, sizeof(online_channels));
}
void CUdpProtocol::ComSend(void)
{	

}

void CUdpProtocol::powoff_ComSend(void)
{

}
//��������(��ڲ���֮��������)
//mode 0:�ظ����ͣ�1:������������(������ˮ�Ź̶�0���ϲ��Ѹ�ֵ)��2:������������ָ��(������ˮ���ڴ˴���ֵ)
void CUdpProtocol::SendFrame(u16 len, u8 *pData, u8 mode)
{
	/*u8 temp_buff[150];
	//֡ͷ
	temp_buff[0] = 0x49;
	temp_buff[1] = 0x8a;
	temp_buff[2] = 0x5e;
	temp_buff[3] = 0xb4;
	//֡����(D-MAC(8)�������(2)��������MAC(8))
	*(u16*)(temp_buff+4) = len + 18;
	//D-MAC
	memcpy(temp_buff+6, g_CSystem.m_DMAC, 8);
	//�����
	if (mode)//��������
		*(u16*)(temp_buff+14) = ++m_SendRandomNum;
	else	//�ظ�����
		*(u16*)(temp_buff+14) = m_ReceiveRandomNum;
	//������
	memcpy(temp_buff+16, pData, len);
	//�������������ˮ��
	if (mode == 2){
		*(u32*)(temp_buff+16) = ++m_CommandNum;
		m_CommandNumSave = m_CommandNum;
	}
	//HMAC(֡���ȡ�D-MAC���������������)
	g_CTool.HMAC(len+12, temp_buff+4, temp_buff+len+16);

	//�øշ��͹��ı�־
	m_SendFlag = 1; 
	
	ComSend();*/

	//if (m_LoginFlag == 0xff)
		//return;//ע��ʧ�ܣ����ٶԷ�������������
	//�����ܳ���
	m_SendFrame.Len = len + 18 + 6;
	if (m_SendFrame.Len > SND_MAX_LEN)
		return;
	//֡ͷ
	m_SendFrame.Data[0] = 0x49;
	m_SendFrame.Data[1] = 0x8a;
	m_SendFrame.Data[2] = 0x5e;
	m_SendFrame.Data[3] = 0xb4;
	//֡����(D-MAC(8)�������(2)��������MAC(8))
	*(u16*)(m_SendFrame.Data+4) = len + 18;
	//D-MAC
	memcpy(m_SendFrame.Data+6, g_CSystem.m_DMAC, 8);
	//�����
	if (mode)//��������
		*(u16*)(m_SendFrame.Data+14) = ++m_SendRandomNum;
	else	//�ظ�����
		*(u16*)(m_SendFrame.Data+14) = m_ReceiveRandomNum;
	//������
	memcpy(m_SendFrame.Data+16, pData, len);
	//�������������ˮ��
	if (mode == 2)
	{
		*(u32*)(m_SendFrame.Data+16) = ++m_CommandNum;
		m_CommandNumSave = m_CommandNum;
	}
	//HMAC(֡���ȡ�D-MAC���������������)
	g_CTool.HMAC(len+12, m_SendFrame.Data+4, m_SendFrame.Data+len+16);
	
	//�øշ��͹��ı�־
	m_SendFlag = 1;
	
	ComSend();
	//�ط����崦��
	m_ReSndLen = len+24;
	if (m_ReSndLen > SND_MAX_LEN)
		m_ReSndLen = SND_MAX_LEN;
	memcpy(m_ReSndBuf, m_SendFrame.Data, m_ReSndLen);
}

//�ط���������
void CUdpProtocol::ReSend(void)
{
	//�øշ��͹��ı�־
	m_SendFlag = 1;
	
	ComSend();
}

//��������У��
//pData����
BOOL CUdpProtocol::ReceiveCheck(RECEIVE_FRAME *frame)
{
	u8 temp_buff[4] = {0x49, 0x8a, 0x5e, 0xb4};
	u8 temp_HMAC[8];
	
	//�����ж�
	if (m_Request == FALSE)
		return FALSE;
	frame->Len = m_ReceiveFrame.Len;
	memcpy(frame->Data, m_ReceiveFrame.Data, m_ReceiveFrame.Len);
	m_Request = 0;
	
	//֡ͷ�ж�
	if (memcmp(frame->Data, temp_buff, 4))
		return FALSE;
	//�����ж�
	if (*(u16*)(frame->Data+4) != (frame->Len-6))
	{	//ճ������(ֻ����һ����)
		frame->Len = *(u16*)(frame->Data+4)+6;
	}
	//�����ж�
	if ((frame->Len == 0) || (frame->Len > RCV_MAX_LEN))
		return FALSE;
	//DMAC�ж�
	if (memcmp(frame->Data+6, g_CSystem.m_DMAC, 8))
		return FALSE;
	//������������
	m_ReceiveRandomNum = *(u16*)(frame->Data+14);
	//HMAC�ж�
	g_CTool.HMAC((*(u16*)(frame->Data+4))-6, frame->Data+4, temp_HMAC);
	if (memcmp(frame->Data+(*(u16*)(frame->Data+4))-2, temp_HMAC, 8))
		return FALSE;
	return TRUE;
	//�����²��ϱ�������
	/*frame->Len = m_ReceiveFrame.Len;
	memcpy(frame->Data, m_ReceiveFrame.Data, m_ReceiveFrame.Len);
	m_Request = FALSE;
	
	//�����ж�
	if ((frame.len == 0) || (frame.len > RCV_MAX_LEN))
	{
		return FALSE;
	}
	//ճ�����ȥ�������ظ�
	while ((*(u16*)(frame.rcvdata+4)) < (frame.len-6))
	{
		if ((*(u32*)(frame.rcvdata+16)) == 0)
		{	//�����ظ�(���ȹ̶�31�ֽ�)
			frame.len -= 31;
			memcpy(temp_receive_buff, frame.rcvdata+31, frame.len);
			memcpy(frame.rcvdata, temp_receive_buff, frame.len);
		}
		else	//�����ظ�
			break;
	}
	
	//��������
	memcpy(temp_receive_buff, frame.rcvdata, RCV_MAX_LEN);
	m_Request = FALSE;
	
	//֡ͷ�ж�
	if (memcmp(temp_receive_buff, temp_buff, 4))
		return FALSE;
	//�����ж�
	if ((*(u16*)(temp_receive_buff+4)) > RCV_MAX_LEN)
		return FALSE;
	//DMAC�ж�
	if (memcmp(temp_receive_buff+6, g_CSystem.m_DMAC, 8))
		return FALSE;
	//������������
	m_ReceiveRandomNum = *(u16*)(temp_receive_buff+14);
	//HMAC�ж�
	g_CTool.HMAC((*(u16*)(temp_receive_buff+4))-6, temp_receive_buff+4, temp_HMAC);
	if(0)//((memcmp(temp_receive_buff+(*(u16*)(temp_receive_buff+4))-2, temp_HMAC, 8)))
		return FALSE;
	
	//���ݸ��Ƶ�����Ҫʹ�õĻ���
	memcpy(pData, temp_receive_buff+16, (*(u16*)(temp_receive_buff+4))-18);
	return TRUE;*/
	
}

//Э�����
void CUdpProtocol::Agreement(void)
{
	RECEIVE_FRAME temp_frame;
	if (!ReceiveCheck(&temp_frame))
		return;//û���յ���Ч����
		
	g_CSystem.m_uiPortChangedCounter = 5;//ͨѶ����Ϊ5
	g_CSystem.CommunitTimerReset();
	//����һ֡����
	switch(*(u16*)(temp_frame.Data+20))
	{
	case 0x0001://�����ظ�
		HeartBeatReply(temp_frame.Data+16);
		break;
	case 0x0002://ע��ظ�
		RegisterReply(temp_frame.Data+16);
		break;
	case 0x0003://��ȡ�豸״̬
		GetPosState(temp_frame.Data+16);
		break;
	case 0x0004://�����豸��� 2016-5-19 �޸����
		SetNodeAddr(temp_frame.Data+16);
		break;
	case 0x0005://�޸�ģ�����
		SetPosPara(temp_frame.Data+16);
		break;
	case 0x0008://����ѯ
		BalanceQueryReply(temp_frame.Data+16);	
		break;
	case 0x000A://��ս��׼�¼
		EmptyNote(temp_frame.Data+16);
		break;
	case 0x000D://�·��ļ���Ϣ
		DownloadFileName(temp_frame.Data+16);
		break;
	case 0x000E://�·��ļ�����
		DownloadFileData(temp_frame.Data+16);
		break;
	case 0x000F://Զ��������׮
		LongRangeReset(temp_frame.Data+16);
		break;
	case 0x0011:
		CancelTradeReply(temp_frame.Data+16); //2016-5-22 
		break;
	case 0x0016://�ɼ����׼�¼
		ReadNote(temp_frame.Data+16);
		break;
	case 0x0017://�������
		EmptyVoice(temp_frame.Data+16);
		break;
	case 0x0024: //��ȡICCID
	  	GetICCIDResponse(temp_frame.Data+16);
		break;
	case 0x0025://ICCID�ϱ��ɹ���Ĵ���
		SendICCIDResponse();
		break;
	case 0x0200://���·������б�2017.07.21
		SetServerList(temp_frame.Data+16);
		break;
	case 0x0201://����ͨѶ������Կ
		SetComKey(temp_frame.Data+16);
		break;
	case 0x0202://����D-MAC
		SetDMAC(temp_frame.Data+16);
		break;
	case 0x0204://��Ӫ��ˢ���ظ�
	  	OperatorCardReply(temp_frame.Data+16);
		break;
	case 0x0401://�޸ĳ��ģʽ(Ҫ������Ч)
		SetTradeMode(temp_frame.Data+16);
		break;
	case 0x0402://�޸ĵ�λ
		SetTradeUnit(temp_frame.Data+16);
		break;
	case 0x0403://�޸��޵����Զ��ر�ʱ��
		SetTradeOffTime(temp_frame.Data+16);
		break;
	case 0x0404://���Ӳ������
		ClearCoin(temp_frame.Data+16);
		break;
	case 0x040a://���׮�����������ˢ���۷ѻظ�
		CardTradeReply(temp_frame.Data+16);
		break;
	case 0x0406://΢�ſ������
		StartCharge(temp_frame.Data+16);
		break;
	case 0x0407://΢�Ž������
		StopCharge(temp_frame.Data+16);
		break;
	case 0x0408://��Ӳ������
	  	UpLoadCoins(temp_frame.Data+16);
		break;
	case 0x0409://�޸ĵ�λ����
		SetTradePerUnit(temp_frame.Data+16);
		break;
	case 0x40C:
		Get_Meter_E_totalDispose(temp_frame.Data+16);
		break;
    case 0x040d://��ȡλ����Ϣ
        Get_Position(temp_frame.Data+16);
        break;
    case 0x0410://�޸�ͨ���������ֵ����Сֵ2018        
        Set_MaxPower(temp_frame.Data+16);
        break;
	case 0x0411: //��ȡ�豸������Ϣ2018
		Get_Parameter(temp_frame.Data + 16);
		break;
    case 0x0412://������Ϣ�ظ�
        TestInfo_Reply(temp_frame.Data+16);
        break;
    case 0x0415://�쳣��Ϣ�ϱ��ظ�
        Abnormal_reply(temp_frame.Data+16);
        break;
	case 0x0416: //���߽���ظ�
		OnlineTradeReturn(temp_frame.Data + 16);
		break;
    case 0x0417: //���߽���2�ظ�
        OnlineTradeReturn(temp_frame.Data + 16);
        break;
	default:
		break;
	}
}

//��������0001
//2016-5-22��Ϊ�仯����
void CUdpProtocol::SendHeartBeat(void)
{
	u8 i,j,cnt = 0,len = 9;//֡�̶�����    
	
	//�ȼ��㷵��֡�仯����
	for(i=0;i<g_CSystem.m_MaxChl;i++)//2016-5-30 �޸�
	{
		if(g_CChargingPile.m_SwitchState[i] == POWER_ON)
			cnt++;
	}
	if(cnt)
	  len += cnt*2;

	u8 temp_SendBuff[29];
	*(u32*)(temp_SendBuff) = 0;//������ˮ��
	*(u16*)(temp_SendBuff+4) = 0x0001;//������
	temp_SendBuff[6] = g_CSystem.m_State;
	if(cnt)
	{
		u8 Index = 0;
		for(j=0;j<g_CSystem.m_MaxChl;j++)
		{
			i = g_CSystem.m_MaxChl- j - 1;//ͨ��״̬λ�ĸ�λ��Ӧ���ݵ�λ//�޸�2·׮���ϴ�״̬��bug
			if(g_CChargingPile.m_SwitchState[i] == POWER_ON)
			{
				*(u16*)(temp_SendBuff+7) |= (0x0001<<i);
				if (g_CSystem.m_Mode == 0)//����������2016-5-27
					*(u16*)(temp_SendBuff+(9+(Index++)*2)) = g_CChargingPile.m_PulseNow[i]/16;
				else
				  	*(u16*)(temp_SendBuff+(9+(Index++)*2)) = g_CChargingPile.TimerCharged[i];
			}
			else
			{
				*(u16*)(temp_SendBuff+7) &= (~(0x0001<<i));
			}
		}
	}
	else
	{
		*(u16*)(temp_SendBuff+7) = 0;
	}

	SendFrame(len, temp_SendBuff, 1);
}


//�����ظ�
void CUdpProtocol::HeartBeatReply(u8 *pData)
{
	if (pData[6])//�豸δע��
		m_LoginFlag = 0;//�޸�ע��״̬
    else
    {
        if(g_CSystem.head_err_type)
        {//����Ӳ���쳣��Ϣ            
            u8 tmp_buff = (g_CSystem.head_err_type & 0x01)?2:(g_CSystem.head_err_type & 0x02)?1:3;
            //�����쳣��Ϣ
            g_IAbnormalInf.CreateAbnormal(A_HARDERR,&tmp_buff);
            g_CSystem.head_err_type = 0;
        }
    }
}
//�ϵ緢��CCID
void CUdpProtocol::SendCCID(u8* CCID)
{
	u8 temp_SendBuff[30];
	u8 temp_ip[4];
	g_CSystem.GetDefaultServerIP(temp_ip);
	*(u32*)(temp_SendBuff) = 0;//������ˮ��
	*(u16*)(temp_SendBuff+4) = 0x0025;//������
	memcpy(temp_SendBuff+6, temp_ip, 4);
	memcpy(temp_SendBuff+10, CCID, 20);
	SendFrame(30, temp_SendBuff, 2);		
}

//����ע��
void CUdpProtocol::SendRegister(void)
{
	//u8 temp_SendBuff[25];
	u8 temp_SendBuff[29];
	*(u32*)(temp_SendBuff) = 0;//������ˮ��
	*(u16*)(temp_SendBuff+4) = 0x0002;//������
	*(u16*)(temp_SendBuff+6) = g_CSystem.m_unNodeAddr;//�豸���
	temp_SendBuff[8] = EQUIPMENT_TYPE_H;//������(���):����׮
	temp_SendBuff[9] = EQUIPMENT_TYPE_L;//С����(�ͺ�):���ⵥ׮
	temp_SendBuff[10] = VERSION_NUMBER_H;//���汾��
	temp_SendBuff[11] = VERSION_NUMBER_L;//�ΰ汾��
	temp_SendBuff[12] = RELEASE_DATE_YEAR;//��������(��)
	temp_SendBuff[13] = RELEASE_DATE_MONTH;//��������(��)
	temp_SendBuff[14] = RELEASE_DATE_DAY;//��������(��)
	temp_SendBuff[15] = 0;//���λΪ���Ʊ�ʶλ:1Ϊ���ư汾��0Ϊͨ�ð汾
	temp_SendBuff[16] = g_CSystem.m_Mode;
	//temp_SendBuff[17] = g_CSystem.m_Unit;
	*(u32*)(temp_SendBuff+17) = g_CSystem.m_Unit;
	//temp_SendBuff[18] = g_CSystem.m_OffTime;
	temp_SendBuff[21] = g_CSystem.m_OffTime;
	//*(u32*)(temp_SendBuff+19) = g_CSystem.m_TimeStamp;
	*(u32*)(temp_SendBuff+22) = g_CSystem.m_TimeStamp;
	//*(u16*)(temp_SendBuff+23) = g_CSystem.m_Switch;
	*(u16*)(temp_SendBuff+26) = g_CSystem.m_Switch;
	
	//Send(25, temp_SendBuff, 2);

	temp_SendBuff[28] = 0x01;//�����֣����λΪ1����ʾ��֡��Ҫ�������Ļظ�

	SendFrame(29, temp_SendBuff, 2);
}

//ע��ظ�
void CUdpProtocol::RegisterReply(u8 *pData)
{
	if (pData[6])//ע��ʧ��
	{
		if(pData[6] == 2 || pData[6] == 4)//����������ظ��豸���ڻ�˿ڲ�ƥ��
		{
			m_LoginFlag = 0xff;//�޸�ע��״̬��ֻ�����������ٷ�����	
		}
	}
	else
	{
		m_LoginFlag = 0x80;//�޸�ע��״̬
		g_CPowerCheck->set_ledstate(LED_FREQ_0_1);//��Գɹ���10sһ��        
        g_CGPRS.SendHeartBeat();//���������޸��ƶ�״̬���ӿ��豸����
		if (u8ServerUpdate == 2)
			u8ServerUpdate = 3;
        u8 *temp = 0; 
        Get_Position(temp);//�ϱ�λ��
	}
}

//��ȡ�豸״̬
void CUdpProtocol::GetPosState(u8 *pData)
{
	//u8 temp_SendBuff[23];
	u8 temp_SendBuff[26];
	*(u32*)(temp_SendBuff) = 0;//������ˮ��
	*(u16*)(temp_SendBuff+4) = 0x0003;//������
	*(u16*)(temp_SendBuff+6) = g_CSystem.m_unNodeAddr;//�豸���
	temp_SendBuff[8] = EQUIPMENT_TYPE_H;//������(���):����׮
	temp_SendBuff[9] = EQUIPMENT_TYPE_L;//С����(�ͺ�):���ⵥ׮
	temp_SendBuff[10] = VERSION_NUMBER_H;//���汾��
	temp_SendBuff[11] = VERSION_NUMBER_L;//�ΰ汾��
	temp_SendBuff[12] = RELEASE_DATE_YEAR;//��������(��)
	temp_SendBuff[13] = RELEASE_DATE_MONTH;//��������(��)
	temp_SendBuff[14] = RELEASE_DATE_DAY;//��������(��)
	temp_SendBuff[15] = 0;//���λΪ���Ʊ�ʶλ:1Ϊ���ư汾��0Ϊͨ�ð汾
	temp_SendBuff[16] = g_CSystem.m_Mode;
	//temp_SendBuff[17] = g_CSystem.m_Unit;
	*(u32*)(temp_SendBuff+17) = g_CSystem.m_Unit;
	//temp_SendBuff[18] = g_CSystem.m_OffTime;
	temp_SendBuff[21] = g_CSystem.m_OffTime;
	//*(u32*)(temp_SendBuff+19) = g_CSystem.m_TimeStamp;
	*(u32*)(temp_SendBuff+22) = g_CSystem.m_TimeStamp;
	
	//Send(23, temp_SendBuff, 0);
	SendFrame(26, temp_SendBuff, 0);
}


//�޸��豸��� 2016-5-19
void CUdpProtocol::SetNodeAddr(u8 *pData)
{
	u8 temp_buff[7];
	memcpy(temp_buff, pData, 6);//������ˮ�š�������
	temp_buff[6] = 0;//ִ�гɹ�
	SendFrame(7, temp_buff, 0);
	
	g_CSystem.ModifyNodeAddr(*((u16 *)(pData+6))); //ȷ��һ�� 2016-5-19
}

//�޸ĳ��ģʽ(Ҫ����)
void CUdpProtocol::SetTradeMode(u8 *pData)
{
	u8 temp_buff[7];
	
	memcpy(temp_buff, pData, 6);//������ˮ�š�������
	g_CSystem.ModifyMode(pData[6]);//0=>2·��; 1=>10·С��
	
	temp_buff[6] = 0;//ִ�гɹ�
	SendFrame(7, temp_buff, 0);

	//����
	(&g_CUdpProtocol)->m_ReStart = 1;
	SetTimer (1, 100, (&g_CUdpProtocol));
}

//�޸ĵ�λ(��ʱ��Ч)
void CUdpProtocol::SetTradeUnit(u8 *pData)
{
	u8 temp_buff[7];
	
	memcpy(temp_buff, pData, 6);//������ˮ�š�������
	g_CSystem.ModifyUnit(pData[6]);// 1~200
	
	temp_buff[6] = 0;//ִ�гɹ�
	SendFrame(7, temp_buff, 0);
}

//�޸ĵ�λ����
void CUdpProtocol::SetTradePerUnit(u8 *pData)
{
	u8 temp_buff[7];
	
	UnitRate temp_perunit;
	temp_perunit.PerUse = *(u32*)(pData+6);
	temp_perunit.PerMoney = *(u32*)(pData+10); //�˴���֪���ɷ�
	g_CSystem.ModifyPerUnit(temp_perunit);// 1~200
	memcpy(temp_buff, pData, 6);//������ˮ�š�������
	temp_buff[6] = 0;//ִ�гɹ�
	SendFrame(7, temp_buff, 0);
}

//�޸��޵����Զ��ر�ʱ��(��ʱ��Ч)
void CUdpProtocol::SetTradeOffTime(u8 *pData)
{
	u8 temp_buff[7];
	
	memcpy(temp_buff, pData, 6);//������ˮ�š�������
	g_CSystem.ModifyOffTime(pData[6]);// 3~200
	
	temp_buff[6] = 0;//ִ�гɹ�
	SendFrame(7, temp_buff, 0);
}

//���Ӳ������
void CUdpProtocol::ClearCoin(u8 *pData)
{
	u8 temp_buff[7];
	
	memcpy(temp_buff, pData, 6);//������ˮ�š�������
	g_CSystem.ClearCoinNum();
	
	temp_buff[6] = 0;//ִ�гɹ�
	SendFrame(7, temp_buff, 0);
}

//�޸�ģ�����
void CUdpProtocol::SetPosPara(u8 *pData)
{
	u8 temp_buff[7];
	
	memcpy(temp_buff, pData, 6);//������ˮ�š�������
	//���������ʱ�����޸�
	g_CSystem.ModifyServerIP(pData+6);//������IP
	g_CSystem.ModifyPort(*(u16*)(pData+10));//�������˿�
	g_CSystem.ModifyCommunicationMode(0);//�޸�ͨѶ��ʽ(��ΪIP��portͨѶ)
	
	//temp_buff[6] = 0;//ִ�гɹ�
	SendFrame(6, temp_buff, 0);
}

//����ָ��:���׮�����������ˢ���۷�
//money��λԪ  channel 0~9
//���money�ĵ�λ�Ƿ�
void CUdpProtocol::CardTrade(u8 *pCardNO, u32 money,u8 channel)
{
	u8 temp_buff[23];
	
	*(u32*)(temp_buff) = 0;//������ˮ���ɷ���ʱ��ֵ
	*(u16*)(temp_buff+4) = 0x040a;//������
	temp_buff[6] = channel+1;
	memcpy(temp_buff+7, pCardNO, 4);//������
	*(u32*)(temp_buff+11) = g_CSystem.m_TimeStamp;//ʱ���
	*(u32*)(temp_buff+15) = IChannels::s_Channel[channel].ShortRecordSn;//������ˮ��
	//temp_buff[19] = money;//�����
	memcpy(temp_buff+19, (u8*)&money, 4);//�����
	
	ICard::s_CardMsg.State = 0xff;
	SendFrame(23, temp_buff, 2);
}

//����ˢ������ظ�
void CUdpProtocol::CardTradeReply(u8 *pData)
{// ��������롢�û���š�ʱ������̼�¼��ˮ�š����׶�
	memcpy((u8*)&ICard::s_CardMsg.State, pData+6, 17); 
	if (pData[6] == 22)
		m_LoginFlag = 0x00; 
}

void CUdpProtocol::CancelTradeReq(void)
{
    ;
}

//���ؽ���������������󷵻أ�PC=>DVR�� 0011  2015-5-22
void CUdpProtocol::CancelTradeReply(u8 *pData)
{
	;
}

//����ѯ
void CUdpProtocol::BalanceQueryRequest(u8 *CardNO)
{
	u8 temp_buff[12];
	
	*(u32*)(temp_buff) = 0;//������ˮ��
	*(u16*)(temp_buff+4) = 0x0008;//������
	*(u16*)(temp_buff+6) = g_CSystem.m_unNodeAddr;//�豸���
	memcpy(temp_buff+8, CardNO, 4);//������
	ICard::s_CardMsg.State = 0xff;
	SendFrame(12, temp_buff, 2);
}

//����ѯ�ظ�
void CUdpProtocol::BalanceQueryReply(u8 *pData)
{
	if (m_CommandNumSave != *(u32*)pData)
		return;
	memcpy((u8*)&ICard::s_CardMsg.State, pData+6, 1);//�����
	memcpy((u8*)&ICard::s_CardMsg.CardNO, pData+7, 8);//�û���Ϣ
}

//��Ӫ���������� DVR=>PC��0204
void CUdpProtocol::OperatorCardReq(u8 *pCardNO)
{
	u8 temp_buff[10];

	*(u32*)(temp_buff) = 0;//������ˮ���ɷ���ʱ��ֵ
	*(u16*)(temp_buff+4) = 0x0204;//������
	memcpy(temp_buff+6, pCardNO, 4);//������
	
	ICard::s_CardMsg.State = 0xff;
	SendFrame(10, temp_buff, 2);
}

//��Ӫ�̿�����ظ�
void CUdpProtocol::OperatorCardReply(u8 *pData)
{
	memcpy((u8*)&ICard::s_CardMsg.State, pData+6, 1); //ȡ���ظ����
}

//΢�ſ������ �û����(4)���������(1)�������(4)
void CUdpProtocol::StartCharge(u8 *pData)
{
    u8 repeat_flag=0;//���ر�־
	u8 temp_buff[16];
	u8 len = 7;
	u8 ch = pData[10]-1;
//	INoteStream temp_INoteStream;
	
	memcpy(temp_buff, pData, 6);//������ˮ�š�������
	if (g_CSystem.m_State & 0x02)
		temp_buff[6] = 4;//��¼����Ч
	if (g_CSystem.m_State & 0x20)
		temp_buff[6] = 5;//�г�����¼δ�ϴ�

	if (pData[10] !=0 && pData[10] <= g_CSystem.m_MaxChl)
	{
		if ((g_CSystem.m_Switch>>ch) & 0x01)
		{	//������Ʋ�������
			if (g_CChargingPile.m_SwitchState[ch] == POWER_OFF)
			{
				temp_buff[6] = 0;//����Ͷ������ �� ˢ���˻�����
				//temp_INoteStream.IncreaseShortRecord();//���������ˮ������
				//g_CSystem.m_SendReply[pData[10]].ShortRecordBuff = temp_INoteStream.ShortRecordView();//��ˮ���뻺��
			}
			else if (g_CChargingPile.m_SwitchState[ch] == POWER_ON)
			{	//�Ѿ�������Ҫ����(ע:Ͷ������ֻ֧��Ͷ������)//�˻���ͬ��������
				if (*(u32*)(pData+6) == IChannels::s_Channel[ch].UserNO)
                {
					temp_buff[6] = 0;
                    repeat_flag = 1;
                }
				else
					temp_buff[6] = 3;//�ѱ�����˻�����
			}
			else
				temp_buff[6] = 3;//���ڵȴ��ر�
		}
		else
			temp_buff[6] = 2;//������Ʋ�����
	}
	else
		temp_buff[6] = 1;//������Ų�����
	
	if (temp_buff[6] == 0)
	{	//���Կ������
		IChannels::s_Channel[ch].UserNO = *(u32*)(pData+6);//�û����Ϊ0xFFFFFFFF�������û������˿�
        if(repeat_flag == 0)
        {
            if(g_CSystem.m_Mode)
                //IChannels::s_Channel[ch].MaxValue += *(u32*)(pData+11)/100.0*g_CSystem.m_Unit*60;
                IChannels::s_Channel[ch].MaxValue += (*(u32*)(pData+11))/g_CSystem.m_PerUnit.PerMoney*g_CSystem.m_PerUnit.PerUse*60;
            else
                IChannels::s_Channel[ch].MaxValue += *(u32*)(pData+11)/g_CSystem.m_PerUnit.PerMoney*g_CSystem.m_PerUnit.PerUse*10;   
            if (*(pData+15) == 1)
            {
                //���жϳ����С����
                g_CSystem.m_LowCurrentOff |= 1<<ch;
                g_CSystem.ModifyLowCurrentOff(g_CSystem.m_LowCurrentOff);
                
            }
            else
            {
                g_CSystem.m_LowCurrentOff &= ~(1<<ch);
                g_CSystem.ModifyLowCurrentOff(g_CSystem.m_LowCurrentOff);
            }
            g_CChargingPile.RechargeStart(ch);//�򿪿���
		}//end if(repeat_falg==0)
		IShortRecord tmp_SR;
		//���ݻظ�
		len += 9;
		*(u32*)(temp_buff+7) = g_CSystem.m_TimeStamp;//ʱ���
		*(u32*)(temp_buff+11) = tmp_SR.View();//������ˮ�� 
		temp_buff[15] = ch+1;	
		if(repeat_flag == 0)
        {
            //�洢ͨ������
            IChannels::s_Channel[ch].State = CHANNEL_STATE_VALID;
            IChannels::s_Channel[ch].CancelFlag = CHANNEL_CANCELFLAG_FALSE;
            IChannels::s_Channel[ch].ShortRecordSn = tmp_SR.View();
            IChannels::Save(ch, &IChannels::s_Channel[ch]);
            tmp_SR.Add();//����̼�¼��ˮ��
        }
	}
	SendFrame(len, temp_buff, 0);
}

//΢�Ž������ �û����(4)���������(1)
void CUdpProtocol::StopCharge(u8 *pData)
{
	u8 temp_buff[8];
	
	memcpy(temp_buff, pData, 6);//������ˮ�š�������
	if ((pData[10]!=0) && (pData[10]<=g_CSystem.m_MaxChl))
	{
		//ͨ���źϷ�
		u8 ch = pData[10]-1;//0~9
		if (g_CChargingPile.m_SwitchState[ch] == POWER_ON)//2016-5-31ȥ��ģʽ�жϣ���С������һ��
		{
			if (*(u32*)(pData+6) == IChannels::s_Channel[ch].UserNO)
			{	//�˴����ظ�ʣ��������ڽ��׼�¼�лظ�
				temp_buff[6] = 0;//�رճɹ�
				g_CChargingPile.m_SwitchState[ch] = POWER_WAIT_OFF;
                g_CChargingPile.m_ErrorState[ch] = WECHAT_END;//����ԭ��΢�Ž������
			}
			else
				temp_buff[6] = 2;//���Ǵ��û�������
		}
		else
			temp_buff[6] = 1;//�Ѿ��ر�
	}
	else 
		temp_buff[6] = 4;//�Ƿ�ͨ����

	temp_buff[7] = pData[10];//���Ӳ������
	SendFrame(8, temp_buff, 0);
}

//��ս��׼�¼
void CUdpProtocol::EmptyNote(u8 *pData)
{
	u8 temp_buff[7];
	u8 temp_compare_buff[4] = {0xAA, 0x55, 0xBB, 0x44};
	
	memcpy(temp_buff, pData, 6);//������ˮ��//������
	if (memcmp(pData+6, temp_compare_buff, 4))//�������в��Ϸ�
	{
		temp_buff[6] = 1;
		SendFrame(7, temp_buff, 0);
	}
	else
	{
		temp_buff[6] = 0;
		SendFrame(7, temp_buff, 0);
		INoteStream temp_INoteStream;
		temp_INoteStream.EmptyNote();//��ս��׼�¼
		g_CSystem.ModifyTimeStamp(*(u32*)(pData+10));//�޸�ʱ���
		IShortRecord tmp_stream;
		tmp_stream.Set(1);
		//���±�־������
		g_CSystem.m_State &= ~0x02;
		g_CSystem.m_State &= ~0x04;
        g_IAbnormalInf.Set(1);//��λ�쳣��Ϣ��ˮ��
	}
	
}

//���׼�¼�ɼ�
void CUdpProtocol::ReadNote(u8 *pData)
{
	u8 temp_buff[150];
	
    if(g_CSystem.m_Version == RN8209C_VERSION || USE_HW_CSE7761)
    {
        memcpy(temp_buff, pData, 4);//������ˮ��
        *(u16* )(temp_buff + 4) = 0x0418;//�ɼ����׼�¼2
    }
    else
    {
        memcpy(temp_buff, pData, 6);//������ˮ��//������
    }		
	INoteStream temp_INoteStream;
	temp_INoteStream.GetNote(INoteStream::s_structNotesAddrRead, &temp_INoteStream.m_note);
	if (pData[6] & 0x01)
	{	//ֹͣ���׼�¼�ɼ� ���ظ�
		if (pData[6] & 0x10)
		{	//ȷ��λ��У���ϴ��Ķ̽�����ˮ
			if (temp_INoteStream.m_note.SerialNum == *(u32*)(pData+7))
			{	//�ƶ���ָ��
				temp_INoteStream.NextReadNoteAddr();
			}
		}
		return;
	}
	if (temp_INoteStream.m_note.NoteState == NOTES_STATE_EMPTY)
	{	//�޽��׼�¼
		temp_buff[6] = 1;
		SendFrame(7, temp_buff, 0);
		g_CSystem.m_State &= ~0x04;
		return;
	}
	if (pData[6] & 0x10)
	{	//ȷ��λ��У���ϴ��Ķ̽�����ˮ
		if (temp_INoteStream.m_note.SerialNum != *(u32*)(pData+7))
		{	//ȷ�ϵĶ̽�����ˮ����
			temp_buff[6] = 2;
			SendFrame(7, temp_buff, 0);
			return;
		}
		temp_INoteStream.NextReadNoteAddr();//�ƶ���ָ��
	}
	temp_INoteStream.GetNote(INoteStream::s_structNotesAddrRead, &temp_INoteStream.m_note);
	if (temp_INoteStream.m_note.NoteState == NOTES_STATE_EMPTY)
	{	//�޽��׼�¼
		temp_buff[6] = 1;
		SendFrame(7, temp_buff, 0);
		g_CSystem.m_State &= ~0x04;
		return;
	}
	temp_buff[6] = 0;
    if(g_CSystem.m_Version == RN8209C_VERSION || USE_HW_CSE7761)
    {
        //�����׼�¼�еĶ����������޳�
        //�������0xFE������ǳ�����߽���
        u8 len = 0;
        for (u8 i = 0; i < CHARGING_LEN; i++)//����¼һ����48��
        {
                if (temp_INoteStream.m_note.ChargingRecording[i] !=0xFEFE)
                    len++;
                else
                    break;
        }
        if (len == CHARGING_LEN)//û���ҵ���������
            len = 0;//�����������и���������
        //��һ�������Ǽ�ȥ��¼״̬��
        //�ڶ��������Ǽ�ȥ������߶���Ĳ���
        //�������Ǽ�ȥCRC
        // �Ȱ��г���¼��ǰ�벿�ֿ�����Ȼ���ٿ���crc
        u8 Sendlen = (NOTES_LEN_SIZE-1)-(CHARGING_LEN-len)*2-1;
        memcpy(temp_buff+7, (u8*)&temp_INoteStream.m_note.Addr, Sendlen);
        //��crc������ȥ
        memcpy(temp_buff+7+Sendlen, (u8*)&temp_INoteStream.m_note.Addr+NOTES_LEN_SIZE-1, 1);
        
        SendFrame(7+Sendlen+1, temp_buff, 0);//һ��crc
    }
    else
    {
        memcpy(temp_buff+7, (u8*)&temp_INoteStream.m_note.Addr, NOTES_LEN_SIZE-1-1-48-4-1);
        memcpy(temp_buff+7+(NOTES_LEN_SIZE-1-1-48-4-1), (u8*)&temp_INoteStream.m_note.CRC8, 1);
        SendFrame(7+NOTES_LEN_SIZE-1-48-4-1, temp_buff, 0);
    }
}

//�������
void CUdpProtocol::EmptyVoice(u8 *pData)
{
	u8 temp_buff[7];
	
	memcpy(temp_buff, pData, 6);//������ˮ�š�������
	if (pData[6] != 0xAA || pData[7] != 0x55 || pData[8] != 0xBB || pData[9] != 0x44)
		temp_buff[6] = 1;//ʧ��(�������в���)
	else
	{
		temp_buff[6] = 0;//�ɹ�
		IVoice temp_IVoice;
		temp_IVoice.EmptyWavDate();
	}
	SendFrame(7, temp_buff, 0);
}

//�·��ļ���Ϣ
void CUdpProtocol::DownloadFileName(u8 *pData)
{
	u8 temp_buff[7];
	
	memcpy(temp_buff, pData, 6);//������ˮ��//������
	temp_buff[6] = 0;
	m_FileType = pData[6];
	switch (m_FileType)
	{
		case 0:	//�����ļ�
			IUpdate temp_IUpdate;
#ifdef UPDATE_CHECK_TRACE
			g_CSystem.g_update_fg = 0;
#endif            
			if (temp_IUpdate.DownloadName(pData+7))
				temp_buff[6] = 1;//����
			break;
		case 1:	//�����ļ�
			IVoice temp_IVoice;
			if (temp_IVoice.DownloadWavName(pData+7))
				temp_buff[6] = 1;//����
			break;
		default:
			temp_buff[6] = 2;//�ļ����Ͳ�����
			break;
	}
	SendFrame(7, temp_buff, 0);
}

//�·��ļ�����
void CUdpProtocol::DownloadFileData(u8 *pData)
{
	u8 temp_buff[11];
	
	memcpy(temp_buff, pData, 6);//������ˮ��//������
	memcpy(temp_buff+7, pData+6, 4);//ƫ�Ƶ�ַ
	temp_buff[6] = 0;
	switch (m_FileType)
	{
		case 0:	//�����ļ�
		{
			IUpdate temp_IUpdate;
			int ret = temp_IUpdate.DownloadUpdateFile(*(u32*)(pData+6), pData+14, *(u32*)(pData+10));
#ifdef UPDATE_CHECK_TRACE
			if (ret == -3)
			{
				g_CSystem.g_update_fg = 1;
				return;
			}
#endif			
            if (ret < 0)
				temp_buff[6] = 1;
			else if (ret == 1)
			{	//�����ļ��������
				//д����״̬�֣���������
				structUpgrade.ucListA		= 0xaa;
				structUpgrade.ucAntitoneA	= 0x55;
				structUpgrade.ucListB		= 0xbb;
				structUpgrade.ucAntitoneB	= 0x44;
				
				SendFrame(11, temp_buff, 0);//������תǰ�ظ�
				(&g_CUdpProtocol)->m_ReStart= 1;
				SetTimer(1, 100, (&g_CUdpProtocol)); // 1s֮����ת
			}
			break;
		}
		case 1:	//�����ļ�
			IVoice temp_IVoice;
			if (temp_IVoice.DownloadWavData(*(u32*)(pData+6), pData+14, *(u32*)(pData+10)) < 0)
				temp_buff[6] = 1;
			break;
		default:
			temp_buff[6] = 2;
			break;
	}
	SendFrame(11, temp_buff, 0);
}

//Զ��������׮
void CUdpProtocol::LongRangeReset(u8 *pData)
{
	u8 temp_buff[7];
	
	memcpy(temp_buff, pData, 6);//������ˮ�š�������
	if (pData[6] != 0xaa || pData[7] != 0x55 || pData[8] != 0xbb || pData[9] != 0x44)
	{
		temp_buff[6] = 1;//ʧ��(�������в���)
		SendFrame(7, temp_buff, 0);
		return;
	}
	
	temp_buff[6] = 0;//�ɹ�(�յ���תָ��)
	SendFrame(7, temp_buff, 0);
	
	(&g_CUdpProtocol)->m_ReStart = 1;
	SetTimer (1, 100, (&g_CUdpProtocol));
}

//�޸�ͨѶ��Կ
void CUdpProtocol::SetComKey(u8 *pData)
{
	u8 temp_buff[7];
	
	memcpy(temp_buff, pData, 6);//������ˮ�š�������
	if (pData[6] != 0xAA || pData[7] != 0x55 || pData[8] != 0xBB || pData[9] != 0x44)
		temp_buff[6] = 1;//ʧ��(�������в���)
	else
	{
		temp_buff[6] = 0;//�ɹ�
		IGID temp_IGID;
		temp_IGID.Modify_Com_Key(1, pData+10);//�޸�ͨѶ��Կ
	}
	SendFrame(7, temp_buff, 0);
}

//�޸�DMAC
void CUdpProtocol::SetDMAC(u8 *pData)
{
	u8 temp_buff[7];
	
	memcpy(temp_buff, pData, 6);//������ˮ�š�������
	if (pData[6] != 0xA1 || pData[7] != 0x51 || pData[8] != 0xB1 || pData[9] != 0x41)
		temp_buff[6] = 1;//ʧ��(�������в���)
	else
	{
		IGID temp_IGID;
		if (temp_IGID.Modify_DMAC(pData+10) == 0)
			temp_buff[6] = 2;//ʧ��(DMAC���Ϸ�)
		else
		{
			temp_buff[6] = 0;//�ɹ�
			temp_IGID.Get_DMAC(g_CSystem.m_DMAC);//���»�ȡ��DMAC
		}
	}
	SendFrame(7, temp_buff, 0);
}

//�ϴ�Ӳ������ 2016-5-20
//��Ӳ��������PC=>DVR��0408
void CUdpProtocol::UpLoadCoins(u8 *pData)
{
	u8 temp_buff[10];
	
	memcpy(temp_buff, pData, 6);//������ˮ�š�������
	*(u32*)(temp_buff+6) = g_CSystem.m_CoinNumNow;
	SendFrame(10, temp_buff, 0);
}

// ��ǰע��״̬:0δע�᣻0x80��ע�᣻0xffע�ᱻ�ܾ�
u8 CUdpProtocol::GetLoginState(void)
{
	return m_LoginFlag;
}

void CUdpProtocol::TimerEvent(u32 ulTimeNum)
{
	switch(ulTimeNum)
	{
		case 1:
		{
			KillTimer(this, 1);
			if ((&g_CUdpProtocol)->m_Reboot == 1)
			{
				(&g_CUdpProtocol)->m_Reboot = 0;
				*(u32*)(0xE000ED0C) = 0x05FA0001;//��ת��boot
				//�ں˸�λ���費��λ
				while (1);
				
			}
			else if((&g_CUdpProtocol)->m_ReStart == 1)
			{
				(&g_CUdpProtocol)->m_ReStart = 0;
				NVIC_SystemReset();//����
				//�ں������趼�Ḵλ
				while(1);
			}
			break;
		}
		case 5:
		{
			KillTimer(this, 5);
			g_CGPRS.Get_Meter_E_totalReply();
			break;
		}
		case 10://�ȴ����߽���ɹ�
		{
			u8 temp_kill_fg = 0;
//			DBG_Printf("�ȴ�������...\r\n");
			for (u8 i = 0; i < g_CSystem.m_MaxChl; i++)
			{
				if (online_channels[i]) //������������ͨ��
				{
					if (onlinetrade_fg[i] == 0) //����ɹ�
					{
						wait_timeout[i] = 0;
						reSend_request[i] = 0;
						online_channels[i] = 0;
						delete pNoteStream[i];
						pNoteStream[i] = NULL;
					}
					if (++wait_timeout[i] >= 10)//3s��ʱ���ط�
					{
						wait_timeout[i] = 0;
						if (++reSend_request[i] >= 3)//3�γ�ʱ�����ɽ����¼
						{
							online_channels[i] = 0;
							reSend_request[i] = 0;
							onlinetrade_fg[i] = 0; //��ʱ�ͷ�
//							DBG_Printf("���㳬ʱ������flash\r\n");
							//��ʱ�����ɽ����¼
							INoteStream tmp_Note;
							memcpy(&tmp_Note.m_note, pNoteStream[i], sizeof(*pNoteStream[i]));
							tmp_Note.CreateMainTradeNote();
							delete pNoteStream[i];
							pNoteStream[i] = NULL;
						}
						else
						{
							OnlineTradeRequest(i, pNoteStream[i]);
						}
					}
					if (temp_kill_fg == 0)
						temp_kill_fg = 1;
				}
			}
			if (temp_kill_fg == 0)
				KillTimer(this, 10);
			break;
		}
        case 16:
        {
            u8 temp_kill_fg = 0;
			for (u8 i = 0; i < ABNORMAL_NUM; i++)
			{
				if (abnor_sendtype[i]) //������������ͨ��
				{
					if (sendabnor_fg[i] == 0) //����ɹ�
					{
						abnor_waitcount[i] = 0;
						abnor_waittime[i] = 0;
						abnor_sendtype[i] = 0;
						delete pAbnorNote[i];
						pAbnorNote[i] = NULL;
					}
					if (++abnor_waittime[i] > 9)//3s��ʱ���ط�
					{
						abnor_waittime[i] = 0;
						if (++abnor_waitcount[i] > 2)//3�γ�ʱ�����ɽ����¼
						{
							abnor_sendtype[i] = 0;
							abnor_waitcount[i] = 0;
							sendabnor_fg[i] = 0; //��ʱ�ͷ�
							//��ʱ�����ɽ����¼
//							INoteStream tmp_Note;
//							memcpy(&tmp_Note.m_note, pNoteStream[i], sizeof(*pNoteStream[i]));
//							tmp_Note.CreateMainTradeNote();
							delete pAbnorNote[i];
							pAbnorNote[i] = NULL;
						}
						else
						{
							Send_abnormal_inf(pAbnorNote[i]);
						}
					}
					if (temp_kill_fg == 0)
						temp_kill_fg = 1;
				}
			}
			if (temp_kill_fg == 0)
				KillTimer(this, 16);
            break;
        }
		default:
			break;
	}
}

/************************************************************
�������ƣ�SetServerList
�������ܣ����յ��������������ķ������б���Ϣ�󣬽��д���
��ڲ���������ָ��
���ڲ������� 
��д  �ˣ�furongwei
�������ڣ�2017.08.03
************************************************************/
void CUdpProtocol::SetServerList(u8* pData)
{	
	u8 result = 1;
	u8 temp_buff[61] = {0};
	u8 reply_buff[7] = {0};
	
	memcpy(reply_buff, pData, 6);// ����ظ���������Ϣ��������ˮ�š������룩														
	temp_buff[0] = pData[10];
	if ((pData[6] == 0xAA) && (pData[7] == 0x55) && (pData[8] == 0xBB) && (pData[9] == 0x44))// ��������У��
	{
		if((pData[10] > 0) && (pData[10] <= 10))// ������IP����У��
		{
			u8 i;
			for(i = 0; i < pData[10]; i++)
			{
				if(g_CSystem.LegalCheck(pData + 11 + i * 6) == 0)// ����Ϸ�
				{
					memcpy(((temp_buff+1) + i * 6), (pData + 11 + i * 6), 6);// ����IP��������
				}
				else// �����Ƿ����ݣ���������Ч���ݽ���
				{
					break;
				}
			}
			if(i == pData[10])// ���ʵ�ʼ�⵽��IP�����봫���������Ƿ�һ��
			{// ���һ��
				if(g_CSystem.ModifyServerIpList(temp_buff,  pData[10]) == 0)// �������б�д��flash	
					result = 0;// �ɹ�����0
			}
		}
	}
	
	reply_buff[6] = result;// ����ظ���������Ϣ��ִ�н����														
	SendFrame(7, reply_buff, 0);// �����÷������б�����Ľ�������͸�������
	g_CSystem.JudgeIfNeedToSwitchIP();	
}

/************************************************************
�������ƣ�GetICCIDResponse
�������ܣ��ӷ��������������ȡICCID
��ڲ���������
���ڲ�������
��д  �ˣ�furongwei
�������ڣ�2017.06.12
************************************************************/
void CUdpProtocol::GetICCIDResponse(u8 *pData)
{
	u8 temp_buff[27] = {0};
	char iccid_buff[CCIDLEN] = {0};
	
	memcpy(temp_buff, pData, 6);
	
	g_CGPRS.GetCCID(iccid_buff);
	
	if(iccid_buff)// �����ȡ����ICCID
	{
		temp_buff[6] = 1;// ��ȡ�ɹ���1
		memcpy(temp_buff+7, iccid_buff, CCIDLEN);
	}
	else							
	{
		temp_buff[6] = 0;// ��ȡʧ�ܣ�0
	}
	SendFrame(27, temp_buff, 0);
}

void CUdpProtocol::SendICCIDResponse(void)
{
	g_CGPRS.WriteICCIDToFlash();
}

void CUdpProtocol::Get_Meter_E_totalDispose(u8 *pData)
{
//	u8 temp_buff[10];
	
	memcpy(m_Cmd_Fuc_Buf, pData, 6);//������ˮ�š�������
	g_IUsart.use_hardOrsoft = g_IUsart.use_hardOrsoft_default;//���³�ʼ��
	g_IUsart.Config(RS485);//����������Ϊ485ͨ��
	//���������������ʱ
	SetTimer(5, 500, (&g_CUdpProtocol));//��Ҫһ��ʱ����ʱ���Ա�ͨ��485����ָ���ȡ��ֵ
}

void CUdpProtocol::Get_Meter_E_totalReply(void)
{
	u8 temp_buff[10];
	memcpy(temp_buff, m_Cmd_Fuc_Buf, 6);//������ˮ�š�������
	
	//if(g_IUsart.m_HandShake == 2)
		*(u32*)(temp_buff+6) = g_IUsart.m_Meter_E_total;
	//else
		//*(u32*)(temp_buff+6) = 0;
	SendFrame(10, temp_buff, 0);
	g_IUsart.Config(RS232);//���������û�232ͨ��
}

void CUdpProtocol::Get_Parameter(u8 *pData)//��ȡ�豸����
{
    u8 temp_buff[45];
    u8 aTmp[4];
    char tempid[20]={0};
    memcpy(temp_buff, pData, 6);//������ˮ�š�������  6�ֽ�
    *(u16*)(temp_buff+6) = g_CSystem.m_unNodeAddr;//�豸��� 2�ֽ�
    g_CSystem.GetDefaultServerIP(aTmp); 
    memcpy(temp_buff + 8, aTmp, 4);//ip 4�ֽ�
    *(u16*)(temp_buff + 12) = g_CSystem.GetDefaultServerPort();//�˿ں� 2�ֽ�
    *(u8*)(temp_buff + 14) = g_CSystem.m_OffTime;//�޵����Զ��ر�ʱ�� 1�ֽ�
    *(u8*)(temp_buff + 15) = g_CGPRS.gprs_strength();//�ź�ǿ�� 1�ֽ�
    *(u16*)(temp_buff + 16) = g_CSystem.m_MaxPower;//��ͨ������� 2�ֽ�    
	g_CGPRS.GetCCID(tempid);
    memcpy(temp_buff + 18, tempid, 20);//CCID 20�ֽ�
    //2019_11_14�¼�Э��
    *(u16*)(temp_buff + 38) = g_CSystem.m_MinPower;//��ͨ����С���� 2�ֽ�
    u8 gprs_type = (g_CSystem.Gprs_ModeType == SIM800C)?1:0;
    *(u8*)(temp_buff + 40) = gprs_type;//�豸�������� 0:4G 1:2G 3:NB-IOT    1�ֽ�
    *(u16*)(temp_buff + 41) = g_CSystem.m_PerUnit.PerUse;//��λʱ��    2�ֽ�
    *(u16*)(temp_buff + 43) = g_CSystem.m_PerUnit.PerMoney;//��λ���   2�ֽ�
    SendFrame(45, temp_buff, 0);    
}

void CUdpProtocol::Set_MaxPower(u8 *pData)//�޸ĵ�ͨ������ʡ���С����ֵ
{
    u8 temp_buff[7];
    memcpy(temp_buff, pData, 6);//������ˮ�š�������  6�ֽ�
    g_CSystem.Max_PowerIndex(*((u16 *)(pData+6)), *((u16 *)(pData+8)));//����ͨ�������С����ֵд��flash
    g_CSystem.m_MaxPower = g_CSystem.GetMaxPower();//2018��ȡ�޸�֮��ĵ�ͨ�������ֵ
    g_CSystem.m_MinPower = g_CSystem.GetMinPower();//2018��ȡ�޸�֮��ĵ�ͨ����С����ֵ           
    u8 crc_max = g_CTool.CRC8Value((u8*)&g_CSystem.m_MaxPower, 2);//�����CRCУ��
    u8 crc_min = g_CTool.CRC8Value((u8*)&g_CSystem.m_MinPower, 2);//��С����CRCУ��
    g_CSystem.CRC_Max_Index(crc_max);
    g_CSystem.CRC_Min_Index(crc_min);
    if((g_CSystem.m_Version == MCU_ADC_VERSION)&&(g_CSystem.m_MinPower != 0))
        g_CSystem.m_MinPower = 40;
    g_CChargingPile.CCHARGINGPILE_MIN_POWER = g_CSystem.m_MinPower;//��С����
    temp_buff[6] = 0;//�޸Ľ����0 �ɹ� 1 ʧ��    
    SendFrame(7, temp_buff, 0);
}
//���߽�������
//���� 1 ��һ�η��������� 0 Ϊ�ط�
u8 CUdpProtocol::OnlineTradeRequest(u8 ch, CON_NOTE *pRecord)
{
	u8 temp_SendBuff[90];
	*(u32 *)(temp_SendBuff) = 0;		  //������ˮ�ţ�4B��
	*(u16 *)(temp_SendBuff + 4) = 0x0416; //������(2B)
	*(u16 *)(temp_SendBuff + 6) = pRecord->Addr;
	*(u32 *)(temp_SendBuff + 8) = pRecord->TimeStamp;
	*(u32 *)(temp_SendBuff + 12) = pRecord->SerialNum;
	*(u8 *)(temp_SendBuff + 16) = pRecord->type;
	*(u32 *)(temp_SendBuff + 17) = pRecord->UserNO;
	*(u32 *)(temp_SendBuff + 21) = pRecord->UseValue;
	*(u32 *)(temp_SendBuff + 25) = pRecord->unit;
	*(u8 *)(temp_SendBuff + 29) = pRecord->channel;
	*(u32 *)(temp_SendBuff + 30) = pRecord->StoreTime;
	u8 len = 0;
	for (u8 i = 0; i < CHARGING_LEN; i++)//����¼һ����48��
	{
		if (pRecord->ChargingRecording[i] != 0xFEFE)
			len++;
		else
			break;
	}
	if (len == CHARGING_LEN)//û���ҵ���������
		len = 0;//�����������и���������
	memcpy(temp_SendBuff + 34, pRecord->ChargingRecording, len);
	SendFrame(34 + len, temp_SendBuff, 2);
//	DBG_Printf("ͨ�� %d ���߽�������\r\n",ch);
	if (onlinetrade_fg[ch] == 0)
	{
		online_channels[ch] = 1;
		SetTimer(10, 30, this); //300ms�ȴ��ظ� 3s��ʱ���ط�������ط�����
		onlinetrade_fg[ch] = 1;
		pNoteStream[ch] = new CON_NOTE;
		memcpy(pNoteStream[ch], pRecord, sizeof(*pRecord));
		return 1;
	}
	return 0;
}
// //���߽���ظ�
void CUdpProtocol::OnlineTradeReturn(u8 *pData)
{
	if (!pData[6]) //�ɹ�
	{

	}
	//u32 TimeStamp = *(u32*)(pData+7);
	u32 SerialNum = *(u32 *)(pData + 11);
        u8 i = 0;
	for (i = 0; i < g_CSystem.m_MaxChl; i++)
	{
		if (pNoteStream[i]==NULL)
			continue;
		if (pNoteStream[i]->SerialNum == SerialNum)
        {
            onlinetrade_fg[i] = 0; //����ɹ�
			break;
		}
	}
}

//���߽�������2
//���� 1 ��һ�η��������� 0 Ϊ�ط�
//�޸Ĺ�������Ϊu16��������Ϊ0x0417
u8 CUdpProtocol::OnlineTradeRequest2(u8 ch, CON_NOTE *pRecord)
{
	u8 temp_SendBuff[140];
	*(u32 *)(temp_SendBuff) = 0;		  //������ˮ�ţ�4B��
	*(u16 *)(temp_SendBuff + 4) = 0x0417; //������(2B)
	*(u16 *)(temp_SendBuff + 6) = pRecord->Addr;
	*(u32 *)(temp_SendBuff + 8) = pRecord->TimeStamp;
	*(u32 *)(temp_SendBuff + 12) = pRecord->SerialNum;
	*(u8 *)(temp_SendBuff + 16) = pRecord->type;
	*(u32 *)(temp_SendBuff + 17) = pRecord->UserNO;
	*(u32 *)(temp_SendBuff + 21) = pRecord->UseValue;
	*(u32 *)(temp_SendBuff + 25) = pRecord->unit;
	*(u8 *)(temp_SendBuff + 29) = pRecord->channel;
	*(u32 *)(temp_SendBuff + 30) = pRecord->StoreTime;
    *(u8 *)(temp_SendBuff + 34) = pRecord->TradeReason;//����ԭ��1B��
	u8 len = 0;
	for (u8 i = 0; i < CHARGING_LEN; i++)//����¼һ����48��
	{
		if (pRecord->ChargingRecording[i] != 0xFEFE)
			len++;
		else
			break;
	}
	if (len == CHARGING_LEN)//û���ҵ���������
		len = 0;//�����������и���������
	memcpy(temp_SendBuff + 35, pRecord->ChargingRecording, len*2);
	SendFrame(35 + len*2, temp_SendBuff, 2);
//	DBG_Printf("ͨ�� %d ���߽�������\r\n",ch);
	if (onlinetrade_fg[ch] == 0)
	{
		online_channels[ch] = 1;
		SetTimer(10, 30, this); //300ms�ȴ��ظ� 3s��ʱ���ط�������ط�����
		onlinetrade_fg[ch] = 1;
		pNoteStream[ch] = new CON_NOTE;
		memcpy(pNoteStream[ch], pRecord, sizeof(*pRecord));
		return 1;
	}
	return 0;
}

//���͵�����Ϣ
void CUdpProtocol::send_powoff_inf(void)
{
    u8 my_len = 10;//���ݳ��� ���ĳ��Ⱥ���Ҫ�޸�
	u8 temp_SendBuff[20]={0};//���ݻ�����
    
	*(u32*)(temp_SendBuff) = 0;//������ˮ�ţ�4B��
	*(u16*)(temp_SendBuff+4) = 0x1122;//������(2B)
    *(u8*)(temp_SendBuff+6) = 0xAA;//(1B)//������
    *(u8*)(temp_SendBuff+7) = 0xBB;//(1B)
    *(u8*)(temp_SendBuff+8) = 0xCC;//(1B)
    *(u8*)(temp_SendBuff+9) = 0xDD;//(1B)
    
    memset(m_SendFrame.Data, 0, SND_MAX_LEN	); //��մ��ڻ���������
    m_SendFrame.Len = my_len + 18 + 6;
	//֡ͷ
	m_SendFrame.Data[0] = 0x49;
	m_SendFrame.Data[1] = 0x8a;
	m_SendFrame.Data[2] = 0x5e;
	m_SendFrame.Data[3] = 0xb4;
	//֡����(D-MAC(8)�������(2)��������MAC(8))
	*(u16*)(m_SendFrame.Data+4) = my_len + 18;
	//D-MAC
	memcpy(m_SendFrame.Data+6, g_CSystem.m_DMAC, 8);
	//�����//��������
    *(u16*)(m_SendFrame.Data+14) = ++m_SendRandomNum;
	//������
	memcpy(m_SendFrame.Data+16, temp_SendBuff, my_len);
	//�������������ˮ��
    *(u32*)(m_SendFrame.Data+16) = ++m_CommandNum;
    m_CommandNumSave = m_CommandNum;
	//HMAC(֡���ȡ�D-MAC���������������)
	g_CTool.HMAC(my_len+12, m_SendFrame.Data+4, m_SendFrame.Data+my_len+16);	
	powoff_ComSend();
}

//���Ͳ�����Ϣ
void CUdpProtocol::Send_TestInfo(void)
{
    char tempccid[20]={0};
	u8 temp_SendBuff[90]={0};
	*(u32*)(temp_SendBuff) = 0;//������ˮ�ţ�4B��
	*(u16*)(temp_SendBuff+4) = 0x0412;//������(2B)
    *(u8*)(temp_SendBuff+6) = g_CGPRS.Strengh;//�ź�ǿ��(1B)
    if(g_CGPRS.GetModelType() == AIR_720)
    {
        *(u8*)(temp_SendBuff+7) = 2;//4Gģ���ͺ�(1B)
        *(u16*)(temp_SendBuff+8) = (u16)g_CGPRS.UP_Version();//4Gģ��汾��(2B)
    }
    else if(g_CGPRS.GetModelType() == NEO_N720)
    {
        *(u8*)(temp_SendBuff+7) = 1;//4Gģ���ͺ�(1B)
        *(u16*)(temp_SendBuff+8) = 0;//4Gģ��汾��(2B)
    }
    else
    {
        *(u8*)(temp_SendBuff+7) = 3;//4Gģ���ͺ�(1B)
        *(u16*)(temp_SendBuff+8) = 0;//4Gģ��汾��(2B)
    }
    g_CGPRS.GetCCID(tempccid);
    memcpy(temp_SendBuff+10, tempccid, 20);//CCID 20�ֽ�
    memcpy(temp_SendBuff+30, (u8*)g_COverallTest.Send_Test_Pow, 2*20);//��ͨ������ֵ20·(2B*20)
    *(u16*)(temp_SendBuff+70) = g_COverallTest.Send_Test_Leakage;//©��ֵ(2B)
    memcpy(temp_SendBuff+72, g_COverallTest.Send_Test_Card, 4);//���Կ�����(4B)
    *(u32*)(temp_SendBuff+76) = g_COverallTest.Send_Test_KEY;//�������� bit 1:���� 0:δ��(4B)
    *(u16*)(temp_SendBuff+80) = g_COverallTest.Send_Test_TotalPow;//�ܹ���(2B)
    *(u32*)(temp_SendBuff+82) = g_IUsart.m_Meter_E_total;//������ֵ(4B)

	SendFrame(86, temp_SendBuff, 2);
}

//������Ϣ����
void CUdpProtocol::TestInfo_Reply(u8 *pData)
{
    if (!pData[6])//�ɹ�
    {
        g_COverallTest.TestInfo_ReplyOK();
    }
}

//�����쳣��Ϣ
void CUdpProtocol::Send_abnormal_inf(ABNORM_NOTE *pData)
{
    u8 temp_SendBuff[30];
 //   u8 temp_type = 0;
	*(u32 *)(temp_SendBuff) = 0;		  //������ˮ�ţ�4B��
	*(u16 *)(temp_SendBuff + 4) = 0x0415; //������(2B)
    *(u32 *)(temp_SendBuff + 6) = pData->TimeStamp;//ʱ���(4B)
    *(u32 *)(temp_SendBuff + 10) = pData->SerialNum;//�쳣��Ϣ��ˮ��(4B)
    *(u8 *)(temp_SendBuff + 14) = pData->Type;//�쳣���ͣ�1B��
    g_IAbnormalInf.AbnormalType = pData->Type;
    *(u8 *)(temp_SendBuff + 15) = pData->Length;//�쳣��Ϣ�������ݳ���(1B)
    memcpy(temp_SendBuff+16,pData->Attach,pData->Length);//������������
    if(g_IAbnormalInf.AbnormalType == A_RELAYERR)
        g_IAbnormalInf.RelayErrCh = *(u32*)pData->Attach;
    SendFrame(16+pData->Length, temp_SendBuff, 2);
    u8 m_type =  pData->Type - 1;
    if (sendabnor_fg[m_type] == 0)
	{
        sendabnor_fg[m_type] = 1;
		abnor_sendtype[m_type] = 1;
		SetTimer(16, 30, this); //300ms�ȴ��ظ� 3s��ʱ���ط�������ط�����
		pAbnorNote[m_type] = new ABNORM_NOTE;
		memcpy(pAbnorNote[m_type], pData, sizeof(*pData));
	}
}
//�쳣��Ϣ�ظ�
void CUdpProtocol::Abnormal_reply(u8 *pData)
{
	u32 SerialNum = *(u32 *)(pData + 10);
	for (u8 i = 0; i < ABNORMAL_NUM; i++)
	{
		if (pAbnorNote[i]==NULL)
			continue;
		if (pAbnorNote[i]->SerialNum == SerialNum)
		{
			sendabnor_fg[i] = 0; 
            if(g_IAbnormalInf.AbnormalType == A_RELAYERR)
                g_IAbnormalInf.RelayErrOk = g_IAbnormalInf.RelayErrCh;
            Dprintf(BLUE, "abnormal", "�쳣�յ��ظ�\r\n");
			break;
		}
	}    
}

//��ȡλ����Ϣ
void CUdpProtocol::Get_Position(u8 *pDa)
{
    if((*(char*)g_CGPRS.m_Lac==NULL) && (*(char*)g_CGPRS.m_Id==NULL))//û�ж�������
        return;
    u8 temp_buff[40] = {0};	
    char temp_lac[20] = {'\0'};
    u8 lac_len = 0;
    *(u32 *)(temp_buff) = 0;		  //������ˮ�ţ�4B��
	*(u16 *)(temp_buff + 4) = 0x040d; //������(2B)
    *(u8 *)(temp_buff + 6) = g_CGPRS.m_cimi;//��Ӫ������(1B)
    sprintf(temp_lac,"%s,%s",g_CGPRS.m_Lac,g_CGPRS.m_Id);
    lac_len = strlen(temp_lac);
    memcpy(temp_buff + 7, (u8 *)temp_lac, lac_len);
    SendFrame(7+lac_len, temp_buff, 0);
}