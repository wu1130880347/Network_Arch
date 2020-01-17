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

u16 CUdpProtocol::m_SendRandomNum;		//主动发送时的随机数
u16 CUdpProtocol::m_ReceiveRandomNum;	//接收到的随机数
u32 CUdpProtocol::m_CommandNum;		//主动发送时的命令流水号
u32 CUdpProtocol::m_CommandNumSave;	//保存主动发送的命令流水号
__no_init UPGRADE_LIST structUpgrade @ (0x20008D00);//升级序列(4bytes)
u8 CUdpProtocol::u8ServerUpdate = 0;//服务器IP更新:0不更新；1需要重连；2重连中；3更新IP到flash

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
//发送数据(入口参数之传数据域)
//mode 0:回复发送，1:主动发送心跳(命令流水号固定0，上层已赋值)，2:主动发送其他指令(命令流水号在此处赋值)
void CUdpProtocol::SendFrame(u16 len, u8 *pData, u8 mode)
{
	/*u8 temp_buff[150];
	//帧头
	temp_buff[0] = 0x49;
	temp_buff[1] = 0x8a;
	temp_buff[2] = 0x5e;
	temp_buff[3] = 0xb4;
	//帧长度(D-MAC(8)、随机数(2)、数据域、MAC(8))
	*(u16*)(temp_buff+4) = len + 18;
	//D-MAC
	memcpy(temp_buff+6, g_CSystem.m_DMAC, 8);
	//随机数
	if (mode)//主动发送
		*(u16*)(temp_buff+14) = ++m_SendRandomNum;
	else	//回复发送
		*(u16*)(temp_buff+14) = m_ReceiveRandomNum;
	//数据域
	memcpy(temp_buff+16, pData, len);
	//数据域的命令流水号
	if (mode == 2){
		*(u32*)(temp_buff+16) = ++m_CommandNum;
		m_CommandNumSave = m_CommandNum;
	}
	//HMAC(帧长度、D-MAC、随机数、数据域)
	g_CTool.HMAC(len+12, temp_buff+4, temp_buff+len+16);

	//置刚发送过的标志
	m_SendFlag = 1; 
	
	ComSend();*/

	//if (m_LoginFlag == 0xff)
		//return;//注册失败，不再对服务器发送数据
	//发送总长度
	m_SendFrame.Len = len + 18 + 6;
	if (m_SendFrame.Len > SND_MAX_LEN)
		return;
	//帧头
	m_SendFrame.Data[0] = 0x49;
	m_SendFrame.Data[1] = 0x8a;
	m_SendFrame.Data[2] = 0x5e;
	m_SendFrame.Data[3] = 0xb4;
	//帧长度(D-MAC(8)、随机数(2)、数据域、MAC(8))
	*(u16*)(m_SendFrame.Data+4) = len + 18;
	//D-MAC
	memcpy(m_SendFrame.Data+6, g_CSystem.m_DMAC, 8);
	//随机数
	if (mode)//主动发送
		*(u16*)(m_SendFrame.Data+14) = ++m_SendRandomNum;
	else	//回复发送
		*(u16*)(m_SendFrame.Data+14) = m_ReceiveRandomNum;
	//数据域
	memcpy(m_SendFrame.Data+16, pData, len);
	//数据域的命令流水号
	if (mode == 2)
	{
		*(u32*)(m_SendFrame.Data+16) = ++m_CommandNum;
		m_CommandNumSave = m_CommandNum;
	}
	//HMAC(帧长度、D-MAC、随机数、数据域)
	g_CTool.HMAC(len+12, m_SendFrame.Data+4, m_SendFrame.Data+len+16);
	
	//置刚发送过的标志
	m_SendFlag = 1;
	
	ComSend();
	//重发缓冲处理
	m_ReSndLen = len+24;
	if (m_ReSndLen > SND_MAX_LEN)
		m_ReSndLen = SND_MAX_LEN;
	memcpy(m_ReSndBuf, m_SendFrame.Data, m_ReSndLen);
}

//重发缓冲数据
void CUdpProtocol::ReSend(void)
{
	//置刚发送过的标志
	m_SendFlag = 1;
	
	ComSend();
}

//接收数据校验
//pData缓冲
BOOL CUdpProtocol::ReceiveCheck(RECEIVE_FRAME *frame)
{
	u8 temp_buff[4] = {0x49, 0x8a, 0x5e, 0xb4};
	u8 temp_HMAC[8];
	
	//请求判定
	if (m_Request == FALSE)
		return FALSE;
	frame->Len = m_ReceiveFrame.Len;
	memcpy(frame->Data, m_ReceiveFrame.Data, m_ReceiveFrame.Len);
	m_Request = 0;
	
	//帧头判定
	if (memcmp(frame->Data, temp_buff, 4))
		return FALSE;
	//长度判定
	if (*(u16*)(frame->Data+4) != (frame->Len-6))
	{	//粘包处理(只留第一个包)
		frame->Len = *(u16*)(frame->Data+4)+6;
	}
	//长度判定
	if ((frame->Len == 0) || (frame->Len > RCV_MAX_LEN))
		return FALSE;
	//DMAC判定
	if (memcmp(frame->Data+6, g_CSystem.m_DMAC, 8))
		return FALSE;
	//保存接收随机数
	m_ReceiveRandomNum = *(u16*)(frame->Data+14);
	//HMAC判定
	g_CTool.HMAC((*(u16*)(frame->Data+4))-6, frame->Data+4, temp_HMAC);
	if (memcmp(frame->Data+(*(u16*)(frame->Data+4))-2, temp_HMAC, 8))
		return FALSE;
	return TRUE;
	//复制下层上报的数据
	/*frame->Len = m_ReceiveFrame.Len;
	memcpy(frame->Data, m_ReceiveFrame.Data, m_ReceiveFrame.Len);
	m_Request = FALSE;
	
	//长度判定
	if ((frame.len == 0) || (frame.len > RCV_MAX_LEN))
	{
		return FALSE;
	}
	//粘包情况去除心跳回复
	while ((*(u16*)(frame.rcvdata+4)) < (frame.len-6))
	{
		if ((*(u32*)(frame.rcvdata+16)) == 0)
		{	//心跳回复(长度固定31字节)
			frame.len -= 31;
			memcpy(temp_receive_buff, frame.rcvdata+31, frame.len);
			memcpy(frame.rcvdata, temp_receive_buff, frame.len);
		}
		else	//其他回复
			break;
	}
	
	//复制数据
	memcpy(temp_receive_buff, frame.rcvdata, RCV_MAX_LEN);
	m_Request = FALSE;
	
	//帧头判定
	if (memcmp(temp_receive_buff, temp_buff, 4))
		return FALSE;
	//长度判定
	if ((*(u16*)(temp_receive_buff+4)) > RCV_MAX_LEN)
		return FALSE;
	//DMAC判定
	if (memcmp(temp_receive_buff+6, g_CSystem.m_DMAC, 8))
		return FALSE;
	//保存接收随机数
	m_ReceiveRandomNum = *(u16*)(temp_receive_buff+14);
	//HMAC判定
	g_CTool.HMAC((*(u16*)(temp_receive_buff+4))-6, temp_receive_buff+4, temp_HMAC);
	if(0)//((memcmp(temp_receive_buff+(*(u16*)(temp_receive_buff+4))-2, temp_HMAC, 8)))
		return FALSE;
	
	//数据复制到后续要使用的缓冲
	memcpy(pData, temp_receive_buff+16, (*(u16*)(temp_receive_buff+4))-18);
	return TRUE;*/
	
}

//协议解析
void CUdpProtocol::Agreement(void)
{
	RECEIVE_FRAME temp_frame;
	if (!ReceiveCheck(&temp_frame))
		return;//没有收到有效数据
		
	g_CSystem.m_uiPortChangedCounter = 5;//通讯重置为5
	g_CSystem.CommunitTimerReset();
	//接收一帧处理
	switch(*(u16*)(temp_frame.Data+20))
	{
	case 0x0001://心跳回复
		HeartBeatReply(temp_frame.Data+16);
		break;
	case 0x0002://注册回复
		RegisterReply(temp_frame.Data+16);
		break;
	case 0x0003://获取设备状态
		GetPosState(temp_frame.Data+16);
		break;
	case 0x0004://设置设备编号 2016-5-19 修改添加
		SetNodeAddr(temp_frame.Data+16);
		break;
	case 0x0005://修改模块参数
		SetPosPara(temp_frame.Data+16);
		break;
	case 0x0008://余额查询
		BalanceQueryReply(temp_frame.Data+16);	
		break;
	case 0x000A://清空交易记录
		EmptyNote(temp_frame.Data+16);
		break;
	case 0x000D://下发文件信息
		DownloadFileName(temp_frame.Data+16);
		break;
	case 0x000E://下发文件数据
		DownloadFileData(temp_frame.Data+16);
		break;
	case 0x000F://远程重启电桩
		LongRangeReset(temp_frame.Data+16);
		break;
	case 0x0011:
		CancelTradeReply(temp_frame.Data+16); //2016-5-22 
		break;
	case 0x0016://采集交易记录
		ReadNote(temp_frame.Data+16);
		break;
	case 0x0017://清空语音
		EmptyVoice(temp_frame.Data+16);
		break;
	case 0x0024: //获取ICCID
	  	GetICCIDResponse(temp_frame.Data+16);
		break;
	case 0x0025://ICCID上报成功后的处理
		SendICCIDResponse();
		break;
	case 0x0200://更新服务器列表2017.07.21
		SetServerList(temp_frame.Data+16);
		break;
	case 0x0201://设置通讯加密密钥
		SetComKey(temp_frame.Data+16);
		break;
	case 0x0202://设置D-MAC
		SetDMAC(temp_frame.Data+16);
		break;
	case 0x0204://运营商刷卡回复
	  	OperatorCardReply(temp_frame.Data+16);
		break;
	case 0x0401://修改充电模式(要重启生效)
		SetTradeMode(temp_frame.Data+16);
		break;
	case 0x0402://修改单位
		SetTradeUnit(temp_frame.Data+16);
		break;
	case 0x0403://修改无电量自动关闭时间
		SetTradeOffTime(temp_frame.Data+16);
		break;
	case 0x0404://清空硬币数量
		ClearCoin(temp_frame.Data+16);
		break;
	case 0x040a://充电桩向服务器请求刷卡扣费回复
		CardTradeReply(temp_frame.Data+16);
		break;
	case 0x0406://微信开启充电
		StartCharge(temp_frame.Data+16);
		break;
	case 0x0407://微信结束充电
		StopCharge(temp_frame.Data+16);
		break;
	case 0x0408://读硬币数量
	  	UpLoadCoins(temp_frame.Data+16);
		break;
	case 0x0409://修改单位费率
		SetTradePerUnit(temp_frame.Data+16);
		break;
	case 0x40C:
		Get_Meter_E_totalDispose(temp_frame.Data+16);
		break;
    case 0x040d://获取位置信息
        Get_Position(temp_frame.Data+16);
        break;
    case 0x0410://修改通道功率最大值、最小值2018        
        Set_MaxPower(temp_frame.Data+16);
        break;
	case 0x0411: //获取设备参数信息2018
		Get_Parameter(temp_frame.Data + 16);
		break;
    case 0x0412://测试信息回复
        TestInfo_Reply(temp_frame.Data+16);
        break;
    case 0x0415://异常信息上报回复
        Abnormal_reply(temp_frame.Data+16);
        break;
	case 0x0416: //在线结算回复
		OnlineTradeReturn(temp_frame.Data + 16);
		break;
    case 0x0417: //在线结算2回复
        OnlineTradeReturn(temp_frame.Data + 16);
        break;
	default:
		break;
	}
}

//发送心跳0001
//2016-5-22改为变化长度
void CUdpProtocol::SendHeartBeat(void)
{
	u8 i,j,cnt = 0,len = 9;//帧固定长度    
	
	//先计算返回帧变化长度
	for(i=0;i<g_CSystem.m_MaxChl;i++)//2016-5-30 修改
	{
		if(g_CChargingPile.m_SwitchState[i] == POWER_ON)
			cnt++;
	}
	if(cnt)
	  len += cnt*2;

	u8 temp_SendBuff[29];
	*(u32*)(temp_SendBuff) = 0;//命令流水号
	*(u16*)(temp_SendBuff+4) = 0x0001;//功能码
	temp_SendBuff[6] = g_CSystem.m_State;
	if(cnt)
	{
		u8 Index = 0;
		for(j=0;j<g_CSystem.m_MaxChl;j++)
		{
			i = g_CSystem.m_MaxChl- j - 1;//通道状态位的高位对应数据低位//修改2路桩不上传状态的bug
			if(g_CChargingPile.m_SwitchState[i] == POWER_ON)
			{
				*(u16*)(temp_SendBuff+7) |= (0x0001<<i);
				if (g_CSystem.m_Mode == 0)//大车增加脉冲2016-5-27
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


//心跳回复
void CUdpProtocol::HeartBeatReply(u8 *pData)
{
	if (pData[6])//设备未注册
		m_LoginFlag = 0;//修改注册状态
    else
    {
        if(g_CSystem.head_err_type)
        {//发送硬件异常信息            
            u8 tmp_buff = (g_CSystem.head_err_type & 0x01)?2:(g_CSystem.head_err_type & 0x02)?1:3;
            //发送异常信息
            g_IAbnormalInf.CreateAbnormal(A_HARDERR,&tmp_buff);
            g_CSystem.head_err_type = 0;
        }
    }
}
//上电发送CCID
void CUdpProtocol::SendCCID(u8* CCID)
{
	u8 temp_SendBuff[30];
	u8 temp_ip[4];
	g_CSystem.GetDefaultServerIP(temp_ip);
	*(u32*)(temp_SendBuff) = 0;//命令流水号
	*(u16*)(temp_SendBuff+4) = 0x0025;//功能码
	memcpy(temp_SendBuff+6, temp_ip, 4);
	memcpy(temp_SendBuff+10, CCID, 20);
	SendFrame(30, temp_SendBuff, 2);		
}

//发送注册
void CUdpProtocol::SendRegister(void)
{
	//u8 temp_SendBuff[25];
	u8 temp_SendBuff[29];
	*(u32*)(temp_SendBuff) = 0;//命令流水号
	*(u16*)(temp_SendBuff+4) = 0x0002;//功能码
	*(u16*)(temp_SendBuff+6) = g_CSystem.m_unNodeAddr;//设备编号
	temp_SendBuff[8] = EQUIPMENT_TYPE_H;//大类型(类别):交流桩
	temp_SendBuff[9] = EQUIPMENT_TYPE_L;//小类型(型号):阳光单桩
	temp_SendBuff[10] = VERSION_NUMBER_H;//主版本号
	temp_SendBuff[11] = VERSION_NUMBER_L;//次版本号
	temp_SendBuff[12] = RELEASE_DATE_YEAR;//发布日期(年)
	temp_SendBuff[13] = RELEASE_DATE_MONTH;//发布日期(月)
	temp_SendBuff[14] = RELEASE_DATE_DAY;//发布日期(日)
	temp_SendBuff[15] = 0;//最高位为定制标识位:1为定制版本，0为通用版本
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

	temp_SendBuff[28] = 0x01;//控制字，最低位为1，表示本帧需要服务器的回复

	SendFrame(29, temp_SendBuff, 2);
}

//注册回复
void CUdpProtocol::RegisterReply(u8 *pData)
{
	if (pData[6])//注册失败
	{
		if(pData[6] == 2 || pData[6] == 4)//如果服务器回复设备过期或端口不匹配
		{
			m_LoginFlag = 0xff;//修改注册状态，只有重启才能再发心跳	
		}
	}
	else
	{
		m_LoginFlag = 0x80;//修改注册状态
		g_CPowerCheck->set_ledstate(LED_FREQ_0_1);//配对成功，10s一闪        
        g_CGPRS.SendHeartBeat();//发送心跳修改云端状态，加快设备启动
		if (u8ServerUpdate == 2)
			u8ServerUpdate = 3;
        u8 *temp = 0; 
        Get_Position(temp);//上报位置
	}
}

//获取设备状态
void CUdpProtocol::GetPosState(u8 *pData)
{
	//u8 temp_SendBuff[23];
	u8 temp_SendBuff[26];
	*(u32*)(temp_SendBuff) = 0;//命令流水号
	*(u16*)(temp_SendBuff+4) = 0x0003;//功能码
	*(u16*)(temp_SendBuff+6) = g_CSystem.m_unNodeAddr;//设备编号
	temp_SendBuff[8] = EQUIPMENT_TYPE_H;//大类型(类别):交流桩
	temp_SendBuff[9] = EQUIPMENT_TYPE_L;//小类型(型号):阳光单桩
	temp_SendBuff[10] = VERSION_NUMBER_H;//主版本号
	temp_SendBuff[11] = VERSION_NUMBER_L;//次版本号
	temp_SendBuff[12] = RELEASE_DATE_YEAR;//发布日期(年)
	temp_SendBuff[13] = RELEASE_DATE_MONTH;//发布日期(月)
	temp_SendBuff[14] = RELEASE_DATE_DAY;//发布日期(日)
	temp_SendBuff[15] = 0;//最高位为定制标识位:1为定制版本，0为通用版本
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


//修改设备编号 2016-5-19
void CUdpProtocol::SetNodeAddr(u8 *pData)
{
	u8 temp_buff[7];
	memcpy(temp_buff, pData, 6);//命令流水号、功能码
	temp_buff[6] = 0;//执行成功
	SendFrame(7, temp_buff, 0);
	
	g_CSystem.ModifyNodeAddr(*((u16 *)(pData+6))); //确认一下 2016-5-19
}

//修改充电模式(要重启)
void CUdpProtocol::SetTradeMode(u8 *pData)
{
	u8 temp_buff[7];
	
	memcpy(temp_buff, pData, 6);//命令流水号、功能码
	g_CSystem.ModifyMode(pData[6]);//0=>2路大车; 1=>10路小车
	
	temp_buff[6] = 0;//执行成功
	SendFrame(7, temp_buff, 0);

	//重启
	(&g_CUdpProtocol)->m_ReStart = 1;
	SetTimer (1, 100, (&g_CUdpProtocol));
}

//修改单位(即时生效)
void CUdpProtocol::SetTradeUnit(u8 *pData)
{
	u8 temp_buff[7];
	
	memcpy(temp_buff, pData, 6);//命令流水号、功能码
	g_CSystem.ModifyUnit(pData[6]);// 1~200
	
	temp_buff[6] = 0;//执行成功
	SendFrame(7, temp_buff, 0);
}

//修改单位费率
void CUdpProtocol::SetTradePerUnit(u8 *pData)
{
	u8 temp_buff[7];
	
	UnitRate temp_perunit;
	temp_perunit.PerUse = *(u32*)(pData+6);
	temp_perunit.PerMoney = *(u32*)(pData+10); //此处不知道可否
	g_CSystem.ModifyPerUnit(temp_perunit);// 1~200
	memcpy(temp_buff, pData, 6);//命令流水号、功能码
	temp_buff[6] = 0;//执行成功
	SendFrame(7, temp_buff, 0);
}

//修改无电量自动关闭时间(即时生效)
void CUdpProtocol::SetTradeOffTime(u8 *pData)
{
	u8 temp_buff[7];
	
	memcpy(temp_buff, pData, 6);//命令流水号、功能码
	g_CSystem.ModifyOffTime(pData[6]);// 3~200
	
	temp_buff[6] = 0;//执行成功
	SendFrame(7, temp_buff, 0);
}

//清空硬币数量
void CUdpProtocol::ClearCoin(u8 *pData)
{
	u8 temp_buff[7];
	
	memcpy(temp_buff, pData, 6);//命令流水号、功能码
	g_CSystem.ClearCoinNum();
	
	temp_buff[6] = 0;//执行成功
	SendFrame(7, temp_buff, 0);
}

//修改模块参数
void CUdpProtocol::SetPosPara(u8 *pData)
{
	u8 temp_buff[7];
	
	memcpy(temp_buff, pData, 6);//命令流水号、功能码
	//其余参数暂时不能修改
	g_CSystem.ModifyServerIP(pData+6);//服务器IP
	g_CSystem.ModifyPort(*(u16*)(pData+10));//服务器端口
	g_CSystem.ModifyCommunicationMode(0);//修改通讯方式(改为IP、port通讯)
	
	//temp_buff[6] = 0;//执行成功
	SendFrame(6, temp_buff, 0);
}

//主动指令:充电桩向服务器请求刷卡扣费
//money单位元  channel 0~9
//如果money的单位是分
void CUdpProtocol::CardTrade(u8 *pCardNO, u32 money,u8 channel)
{
	u8 temp_buff[23];
	
	*(u32*)(temp_buff) = 0;//命令流水号由发送时赋值
	*(u16*)(temp_buff+4) = 0x040a;//功能码
	temp_buff[6] = channel+1;
	memcpy(temp_buff+7, pCardNO, 4);//卡内码
	*(u32*)(temp_buff+11) = g_CSystem.m_TimeStamp;//时间戳
	*(u32*)(temp_buff+15) = IChannels::s_Channel[channel].ShortRecordSn;//交易流水号
	//temp_buff[19] = money;//充电金额
	memcpy(temp_buff+19, (u8*)&money, 4);//充电金额
	
	ICard::s_CardMsg.State = 0xff;
	SendFrame(23, temp_buff, 2);
}

//请求刷卡请求回复
void CUdpProtocol::CardTradeReply(u8 *pData)
{// 包含结果码、用户编号、时间戳、短记录流水号、交易额
	memcpy((u8*)&ICard::s_CardMsg.State, pData+6, 17); 
	if (pData[6] == 22)
		m_LoginFlag = 0x00; 
}

void CUdpProtocol::CancelTradeReq(void)
{
    ;
}

//返回结果：撤消交易请求返回（PC=>DVR） 0011  2015-5-22
void CUdpProtocol::CancelTradeReply(u8 *pData)
{
	;
}

//余额查询
void CUdpProtocol::BalanceQueryRequest(u8 *CardNO)
{
	u8 temp_buff[12];
	
	*(u32*)(temp_buff) = 0;//命令流水号
	*(u16*)(temp_buff+4) = 0x0008;//功能码
	*(u16*)(temp_buff+6) = g_CSystem.m_unNodeAddr;//设备编号
	memcpy(temp_buff+8, CardNO, 4);//卡内码
	ICard::s_CardMsg.State = 0xff;
	SendFrame(12, temp_buff, 2);
}

//余额查询回复
void CUdpProtocol::BalanceQueryReply(u8 *pData)
{
	if (m_CommandNumSave != *(u32*)pData)
		return;
	memcpy((u8*)&ICard::s_CardMsg.State, pData+6, 1);//结果码
	memcpy((u8*)&ICard::s_CardMsg.CardNO, pData+7, 8);//用户信息
}

//运营商配置请求（ DVR=>PC）0204
void CUdpProtocol::OperatorCardReq(u8 *pCardNO)
{
	u8 temp_buff[10];

	*(u32*)(temp_buff) = 0;//命令流水号由发送时赋值
	*(u16*)(temp_buff+4) = 0x0204;//功能码
	memcpy(temp_buff+6, pCardNO, 4);//卡内码
	
	ICard::s_CardMsg.State = 0xff;
	SendFrame(10, temp_buff, 2);
}

//运营商卡请求回复
void CUdpProtocol::OperatorCardReply(u8 *pData)
{
	memcpy((u8*)&ICard::s_CardMsg.State, pData+6, 1); //取到回复结果
}

//微信开启充电 用户编号(4)、插座编号(1)、充电金额(4)
void CUdpProtocol::StartCharge(u8 *pData)
{
    u8 repeat_flag=0;//判重标志
	u8 temp_buff[16];
	u8 len = 7;
	u8 ch = pData[10]-1;
//	INoteStream temp_INoteStream;
	
	memcpy(temp_buff, pData, 6);//命令流水号、功能码
	if (g_CSystem.m_State & 0x02)
		temp_buff[6] = 4;//记录区无效
	if (g_CSystem.m_State & 0x20)
		temp_buff[6] = 5;//有撤消记录未上传

	if (pData[10] !=0 && pData[10] <= g_CSystem.m_MaxChl)
	{
		if ((g_CSystem.m_Switch>>ch) & 0x01)
		{	//拨码控制插座可用
			if (g_CChargingPile.m_SwitchState[ch] == POWER_OFF)
			{
				temp_buff[6] = 0;//可以投币启动 或 刷卡账户启动
				//temp_INoteStream.IncreaseShortRecord();//启动充电流水号增加
				//g_CSystem.m_SendReply[pData[10]].ShortRecordBuff = temp_INoteStream.ShortRecordView();//流水号入缓存
			}
			else if (g_CChargingPile.m_SwitchState[ch] == POWER_ON)
			{	//已经启动，要续费(注:投币启动只支持投币续费)//账户相同才能续费
				if (*(u32*)(pData+6) == IChannels::s_Channel[ch].UserNO)
                {
					temp_buff[6] = 0;
                    repeat_flag = 1;
                }
				else
					temp_buff[6] = 3;//已被别的账户启动
			}
			else
				temp_buff[6] = 3;//正在等待关闭
		}
		else
			temp_buff[6] = 2;//拨码控制不可用
	}
	else
		temp_buff[6] = 1;//插座编号不存在
	
	if (temp_buff[6] == 0)
	{	//可以开启充电
		IChannels::s_Channel[ch].UserNO = *(u32*)(pData+6);//用户编号为0xFFFFFFFF是匿名用户，不退款
        if(repeat_flag == 0)
        {
            if(g_CSystem.m_Mode)
                //IChannels::s_Channel[ch].MaxValue += *(u32*)(pData+11)/100.0*g_CSystem.m_Unit*60;
                IChannels::s_Channel[ch].MaxValue += (*(u32*)(pData+11))/g_CSystem.m_PerUnit.PerMoney*g_CSystem.m_PerUnit.PerUse*60;
            else
                IChannels::s_Channel[ch].MaxValue += *(u32*)(pData+11)/g_CSystem.m_PerUnit.PerMoney*g_CSystem.m_PerUnit.PerUse*10;   
            if (*(pData+15) == 1)
            {
                //不判断充电最小功率
                g_CSystem.m_LowCurrentOff |= 1<<ch;
                g_CSystem.ModifyLowCurrentOff(g_CSystem.m_LowCurrentOff);
                
            }
            else
            {
                g_CSystem.m_LowCurrentOff &= ~(1<<ch);
                g_CSystem.ModifyLowCurrentOff(g_CSystem.m_LowCurrentOff);
            }
            g_CChargingPile.RechargeStart(ch);//打开开关
		}//end if(repeat_falg==0)
		IShortRecord tmp_SR;
		//数据回复
		len += 9;
		*(u32*)(temp_buff+7) = g_CSystem.m_TimeStamp;//时间戳
		*(u32*)(temp_buff+11) = tmp_SR.View();//交易流水号 
		temp_buff[15] = ch+1;	
		if(repeat_flag == 0)
        {
            //存储通道数据
            IChannels::s_Channel[ch].State = CHANNEL_STATE_VALID;
            IChannels::s_Channel[ch].CancelFlag = CHANNEL_CANCELFLAG_FALSE;
            IChannels::s_Channel[ch].ShortRecordSn = tmp_SR.View();
            IChannels::Save(ch, &IChannels::s_Channel[ch]);
            tmp_SR.Add();//变更短记录流水号
        }
	}
	SendFrame(len, temp_buff, 0);
}

//微信结束充电 用户编号(4)、插座编号(1)
void CUdpProtocol::StopCharge(u8 *pData)
{
	u8 temp_buff[8];
	
	memcpy(temp_buff, pData, 6);//命令流水号、功能码
	if ((pData[10]!=0) && (pData[10]<=g_CSystem.m_MaxChl))
	{
		//通道号合法
		u8 ch = pData[10]-1;//0~9
		if (g_CChargingPile.m_SwitchState[ch] == POWER_ON)//2016-5-31去掉模式判断，大小车处理一致
		{
			if (*(u32*)(pData+6) == IChannels::s_Channel[ch].UserNO)
			{	//此处不回复剩余电量，在交易记录中回复
				temp_buff[6] = 0;//关闭成功
				g_CChargingPile.m_SwitchState[ch] = POWER_WAIT_OFF;
                g_CChargingPile.m_ErrorState[ch] = WECHAT_END;//结算原因：微信结束充电
			}
			else
				temp_buff[6] = 2;//不是此用户开启的
		}
		else
			temp_buff[6] = 1;//已经关闭
	}
	else 
		temp_buff[6] = 4;//非法通道号

	temp_buff[7] = pData[10];//增加插座编号
	SendFrame(8, temp_buff, 0);
}

//清空交易记录
void CUdpProtocol::EmptyNote(u8 *pData)
{
	u8 temp_buff[7];
	u8 temp_compare_buff[4] = {0xAA, 0x55, 0xBB, 0x44};
	
	memcpy(temp_buff, pData, 6);//命令流水号//功能码
	if (memcmp(pData+6, temp_compare_buff, 4))//数据序列不合法
	{
		temp_buff[6] = 1;
		SendFrame(7, temp_buff, 0);
	}
	else
	{
		temp_buff[6] = 0;
		SendFrame(7, temp_buff, 0);
		INoteStream temp_INoteStream;
		temp_INoteStream.EmptyNote();//清空交易记录
		g_CSystem.ModifyTimeStamp(*(u32*)(pData+10));//修改时间戳
		IShortRecord tmp_stream;
		tmp_stream.Set(1);
		//更新标志及变量
		g_CSystem.m_State &= ~0x02;
		g_CSystem.m_State &= ~0x04;
        g_IAbnormalInf.Set(1);//复位异常信息流水号
	}
	
}

//交易记录采集
void CUdpProtocol::ReadNote(u8 *pData)
{
	u8 temp_buff[150];
	
    if(g_CSystem.m_Version == RN8209C_VERSION || USE_HW_CSE7761)
    {
        memcpy(temp_buff, pData, 4);//命令流水号
        *(u16* )(temp_buff + 4) = 0x0418;//采集交易记录2
    }
    else
    {
        memcpy(temp_buff, pData, 6);//命令流水号//功能码
    }		
	INoteStream temp_INoteStream;
	temp_INoteStream.GetNote(INoteStream::s_structNotesAddrRead, &temp_INoteStream.m_note);
	if (pData[6] & 0x01)
	{	//停止交易记录采集 不回复
		if (pData[6] & 0x10)
		{	//确认位，校验上传的短交易流水
			if (temp_INoteStream.m_note.SerialNum == *(u32*)(pData+7))
			{	//移动读指针
				temp_INoteStream.NextReadNoteAddr();
			}
		}
		return;
	}
	if (temp_INoteStream.m_note.NoteState == NOTES_STATE_EMPTY)
	{	//无交易记录
		temp_buff[6] = 1;
		SendFrame(7, temp_buff, 0);
		g_CSystem.m_State &= ~0x04;
		return;
	}
	if (pData[6] & 0x10)
	{	//确认位，校验上传的短交易流水
		if (temp_INoteStream.m_note.SerialNum != *(u32*)(pData+7))
		{	//确认的短交易流水错误
			temp_buff[6] = 2;
			SendFrame(7, temp_buff, 0);
			return;
		}
		temp_INoteStream.NextReadNoteAddr();//移动读指针
	}
	temp_INoteStream.GetNote(INoteStream::s_structNotesAddrRead, &temp_INoteStream.m_note);
	if (temp_INoteStream.m_note.NoteState == NOTES_STATE_EMPTY)
	{	//无交易记录
		temp_buff[6] = 1;
		SendFrame(7, temp_buff, 0);
		g_CSystem.m_State &= ~0x04;
		return;
	}
	temp_buff[6] = 0;
    if(g_CSystem.m_Version == RN8209C_VERSION || USE_HW_CSE7761)
    {
        //将交易记录中的多余充电曲线剔除
        //充电曲线0xFE代表的是充电曲线结束
        u8 len = 0;
        for (u8 i = 0; i < CHARGING_LEN; i++)//充电记录一共有48个
        {
                if (temp_INoteStream.m_note.ChargingRecording[i] !=0xFEFE)
                    len++;
                else
                    break;
        }
        if (len == CHARGING_LEN)//没有找到结束符号
            len = 0;//这里面至少有个结束符号
        //第一个括号是减去记录状态字
        //第二个括号是减去充电曲线多余的部分
        //第三个是减去CRC
        // 先把有充电记录的前半部分拷贝，然后再拷贝crc
        u8 Sendlen = (NOTES_LEN_SIZE-1)-(CHARGING_LEN-len)*2-1;
        memcpy(temp_buff+7, (u8*)&temp_INoteStream.m_note.Addr, Sendlen);
        //把crc拷贝过去
        memcpy(temp_buff+7+Sendlen, (u8*)&temp_INoteStream.m_note.Addr+NOTES_LEN_SIZE-1, 1);
        
        SendFrame(7+Sendlen+1, temp_buff, 0);//一是crc
    }
    else
    {
        memcpy(temp_buff+7, (u8*)&temp_INoteStream.m_note.Addr, NOTES_LEN_SIZE-1-1-48-4-1);
        memcpy(temp_buff+7+(NOTES_LEN_SIZE-1-1-48-4-1), (u8*)&temp_INoteStream.m_note.CRC8, 1);
        SendFrame(7+NOTES_LEN_SIZE-1-48-4-1, temp_buff, 0);
    }
}

//清空语音
void CUdpProtocol::EmptyVoice(u8 *pData)
{
	u8 temp_buff[7];
	
	memcpy(temp_buff, pData, 6);//命令流水号、功能码
	if (pData[6] != 0xAA || pData[7] != 0x55 || pData[8] != 0xBB || pData[9] != 0x44)
		temp_buff[6] = 1;//失败(数据序列不符)
	else
	{
		temp_buff[6] = 0;//成功
		IVoice temp_IVoice;
		temp_IVoice.EmptyWavDate();
	}
	SendFrame(7, temp_buff, 0);
}

//下发文件信息
void CUdpProtocol::DownloadFileName(u8 *pData)
{
	u8 temp_buff[7];
	
	memcpy(temp_buff, pData, 6);//命令流水号//功能码
	temp_buff[6] = 0;
	m_FileType = pData[6];
	switch (m_FileType)
	{
		case 0:	//升级文件
			IUpdate temp_IUpdate;
#ifdef UPDATE_CHECK_TRACE
			g_CSystem.g_update_fg = 0;
#endif            
			if (temp_IUpdate.DownloadName(pData+7))
				temp_buff[6] = 1;//错误
			break;
		case 1:	//语音文件
			IVoice temp_IVoice;
			if (temp_IVoice.DownloadWavName(pData+7))
				temp_buff[6] = 1;//错误
			break;
		default:
			temp_buff[6] = 2;//文件类型不存在
			break;
	}
	SendFrame(7, temp_buff, 0);
}

//下发文件数据
void CUdpProtocol::DownloadFileData(u8 *pData)
{
	u8 temp_buff[11];
	
	memcpy(temp_buff, pData, 6);//命令流水号//功能码
	memcpy(temp_buff+7, pData+6, 4);//偏移地址
	temp_buff[6] = 0;
	switch (m_FileType)
	{
		case 0:	//升级文件
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
			{	//升级文件传送完成
				//写升级状态字，重启升级
				structUpgrade.ucListA		= 0xaa;
				structUpgrade.ucAntitoneA	= 0x55;
				structUpgrade.ucListB		= 0xbb;
				structUpgrade.ucAntitoneB	= 0x44;
				
				SendFrame(11, temp_buff, 0);//升级跳转前回复
				(&g_CUdpProtocol)->m_ReStart= 1;
				SetTimer(1, 100, (&g_CUdpProtocol)); // 1s之后跳转
			}
			break;
		}
		case 1:	//语音文件
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

//远程重启电桩
void CUdpProtocol::LongRangeReset(u8 *pData)
{
	u8 temp_buff[7];
	
	memcpy(temp_buff, pData, 6);//命令流水号、功能码
	if (pData[6] != 0xaa || pData[7] != 0x55 || pData[8] != 0xbb || pData[9] != 0x44)
	{
		temp_buff[6] = 1;//失败(数据序列不符)
		SendFrame(7, temp_buff, 0);
		return;
	}
	
	temp_buff[6] = 0;//成功(收到跳转指令)
	SendFrame(7, temp_buff, 0);
	
	(&g_CUdpProtocol)->m_ReStart = 1;
	SetTimer (1, 100, (&g_CUdpProtocol));
}

//修改通讯密钥
void CUdpProtocol::SetComKey(u8 *pData)
{
	u8 temp_buff[7];
	
	memcpy(temp_buff, pData, 6);//命令流水号、功能码
	if (pData[6] != 0xAA || pData[7] != 0x55 || pData[8] != 0xBB || pData[9] != 0x44)
		temp_buff[6] = 1;//失败(数据序列不符)
	else
	{
		temp_buff[6] = 0;//成功
		IGID temp_IGID;
		temp_IGID.Modify_Com_Key(1, pData+10);//修改通讯密钥
	}
	SendFrame(7, temp_buff, 0);
}

//修改DMAC
void CUdpProtocol::SetDMAC(u8 *pData)
{
	u8 temp_buff[7];
	
	memcpy(temp_buff, pData, 6);//命令流水号、功能码
	if (pData[6] != 0xA1 || pData[7] != 0x51 || pData[8] != 0xB1 || pData[9] != 0x41)
		temp_buff[6] = 1;//失败(数据序列不符)
	else
	{
		IGID temp_IGID;
		if (temp_IGID.Modify_DMAC(pData+10) == 0)
			temp_buff[6] = 2;//失败(DMAC不合法)
		else
		{
			temp_buff[6] = 0;//成功
			temp_IGID.Get_DMAC(g_CSystem.m_DMAC);//重新获取新DMAC
		}
	}
	SendFrame(7, temp_buff, 0);
}

//上传硬币数量 2016-5-20
//读硬币数量（PC=>DVR）0408
void CUdpProtocol::UpLoadCoins(u8 *pData)
{
	u8 temp_buff[10];
	
	memcpy(temp_buff, pData, 6);//命令流水号、功能码
	*(u32*)(temp_buff+6) = g_CSystem.m_CoinNumNow;
	SendFrame(10, temp_buff, 0);
}

// 当前注册状态:0未注册；0x80已注册；0xff注册被拒绝
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
				*(u32*)(0xE000ED0C) = 0x05FA0001;//跳转到boot
				//内核复位外设不复位
				while (1);
				
			}
			else if((&g_CUdpProtocol)->m_ReStart == 1)
			{
				(&g_CUdpProtocol)->m_ReStart = 0;
				NVIC_SystemReset();//重启
				//内核与外设都会复位
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
		case 10://等待在线结算成功
		{
			u8 temp_kill_fg = 0;
//			DBG_Printf("等待结算中...\r\n");
			for (u8 i = 0; i < g_CSystem.m_MaxChl; i++)
			{
				if (online_channels[i]) //正常请求结算的通道
				{
					if (onlinetrade_fg[i] == 0) //结算成功
					{
						wait_timeout[i] = 0;
						reSend_request[i] = 0;
						online_channels[i] = 0;
						delete pNoteStream[i];
						pNoteStream[i] = NULL;
					}
					if (++wait_timeout[i] >= 10)//3s超时，重发
					{
						wait_timeout[i] = 0;
						if (++reSend_request[i] >= 3)//3次超时，生成结算记录
						{
							online_channels[i] = 0;
							reSend_request[i] = 0;
							onlinetrade_fg[i] = 0; //超时释放
//							DBG_Printf("结算超时，存入flash\r\n");
							//超时后生成结算记录
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
				if (abnor_sendtype[i]) //正常请求结算的通道
				{
					if (sendabnor_fg[i] == 0) //结算成功
					{
						abnor_waitcount[i] = 0;
						abnor_waittime[i] = 0;
						abnor_sendtype[i] = 0;
						delete pAbnorNote[i];
						pAbnorNote[i] = NULL;
					}
					if (++abnor_waittime[i] > 9)//3s超时，重发
					{
						abnor_waittime[i] = 0;
						if (++abnor_waitcount[i] > 2)//3次超时，生成结算记录
						{
							abnor_sendtype[i] = 0;
							abnor_waitcount[i] = 0;
							sendabnor_fg[i] = 0; //超时释放
							//超时后生成结算记录
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
函数名称：SetServerList
函数功能：接收到服务器发送来的服务器列表信息后，进行处理
入口参数：数据指针
出口参数：无 
编写  人：furongwei
作成日期：2017.08.03
************************************************************/
void CUdpProtocol::SetServerList(u8* pData)
{	
	u8 result = 1;
	u8 temp_buff[61] = {0};
	u8 reply_buff[7] = {0};
	
	memcpy(reply_buff, pData, 6);// 保存回复服务器信息（命令流水号、功能码）														
	temp_buff[0] = pData[10];
	if ((pData[6] == 0xAA) && (pData[7] == 0x55) && (pData[8] == 0xBB) && (pData[9] == 0x44))// 数据序列校验
	{
		if((pData[10] > 0) && (pData[10] <= 10))// 服务器IP条数校验
		{
			u8 i;
			for(i = 0; i < pData[10]; i++)
			{
				if(g_CSystem.LegalCheck(pData + 11 + i * 6) == 0)// 如果合法
				{
					memcpy(((temp_buff+1) + i * 6), (pData + 11 + i * 6), 6);// 复制IP到缓存区
				}
				else// 遇到非法数据，即代表有效数据结束
				{
					break;
				}
			}
			if(i == pData[10])// 检测实际检测到的IP条数与传来的条数是否一致
			{// 如果一致
				if(g_CSystem.ModifyServerIpList(temp_buff,  pData[10]) == 0)// 服务器列表写入flash	
					result = 0;// 成功返回0
			}
		}
	}
	
	reply_buff[6] = result;// 保存回复服务器信息（执行结果）														
	SendFrame(7, reply_buff, 0);// 将设置服务器列表操作的结果，发送给服务器
	g_CSystem.JudgeIfNeedToSwitchIP();	
}

/************************************************************
函数名称：GetICCIDResponse
函数功能：从服务器发送命令获取ICCID
入口参数：命令
出口参数：无
编写  人：furongwei
作成日期：2017.06.12
************************************************************/
void CUdpProtocol::GetICCIDResponse(u8 *pData)
{
	u8 temp_buff[27] = {0};
	char iccid_buff[CCIDLEN] = {0};
	
	memcpy(temp_buff, pData, 6);
	
	g_CGPRS.GetCCID(iccid_buff);
	
	if(iccid_buff)// 如果获取到了ICCID
	{
		temp_buff[6] = 1;// 获取成功：1
		memcpy(temp_buff+7, iccid_buff, CCIDLEN);
	}
	else							
	{
		temp_buff[6] = 0;// 获取失败：0
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
	
	memcpy(m_Cmd_Fuc_Buf, pData, 6);//命令流水号、功能码
	g_IUsart.use_hardOrsoft = g_IUsart.use_hardOrsoft_default;//重新初始化
	g_IUsart.Config(RS485);//将串口配置为485通信
	//可能引起服务器超时
	SetTimer(5, 500, (&g_CUdpProtocol));//需要一段时间延时，以便通过485发送指令读取数值
}

void CUdpProtocol::Get_Meter_E_totalReply(void)
{
	u8 temp_buff[10];
	memcpy(temp_buff, m_Cmd_Fuc_Buf, 6);//命令流水号、功能码
	
	//if(g_IUsart.m_HandShake == 2)
		*(u32*)(temp_buff+6) = g_IUsart.m_Meter_E_total;
	//else
		//*(u32*)(temp_buff+6) = 0;
	SendFrame(10, temp_buff, 0);
	g_IUsart.Config(RS232);//将串口配置回232通信
}

void CUdpProtocol::Get_Parameter(u8 *pData)//获取设备参数
{
    u8 temp_buff[45];
    u8 aTmp[4];
    char tempid[20]={0};
    memcpy(temp_buff, pData, 6);//命令流水号、功能码  6字节
    *(u16*)(temp_buff+6) = g_CSystem.m_unNodeAddr;//设备编号 2字节
    g_CSystem.GetDefaultServerIP(aTmp); 
    memcpy(temp_buff + 8, aTmp, 4);//ip 4字节
    *(u16*)(temp_buff + 12) = g_CSystem.GetDefaultServerPort();//端口号 2字节
    *(u8*)(temp_buff + 14) = g_CSystem.m_OffTime;//无电量自动关闭时间 1字节
    *(u8*)(temp_buff + 15) = g_CGPRS.gprs_strength();//信号强度 1字节
    *(u16*)(temp_buff + 16) = g_CSystem.m_MaxPower;//单通道最大功率 2字节    
	g_CGPRS.GetCCID(tempid);
    memcpy(temp_buff + 18, tempid, 20);//CCID 20字节
    //2019_11_14新加协议
    *(u16*)(temp_buff + 38) = g_CSystem.m_MinPower;//单通道最小功率 2字节
    u8 gprs_type = (g_CSystem.Gprs_ModeType == SIM800C)?1:0;
    *(u8*)(temp_buff + 40) = gprs_type;//设备联网类型 0:4G 1:2G 3:NB-IOT    1字节
    *(u16*)(temp_buff + 41) = g_CSystem.m_PerUnit.PerUse;//单位时间    2字节
    *(u16*)(temp_buff + 43) = g_CSystem.m_PerUnit.PerMoney;//单位金额   2字节
    SendFrame(45, temp_buff, 0);    
}

void CUdpProtocol::Set_MaxPower(u8 *pData)//修改单通道最大功率、最小功率值
{
    u8 temp_buff[7];
    memcpy(temp_buff, pData, 6);//命令流水号、功能码  6字节
    g_CSystem.Max_PowerIndex(*((u16 *)(pData+6)), *((u16 *)(pData+8)));//将单通道最大、最小功率值写入flash
    g_CSystem.m_MaxPower = g_CSystem.GetMaxPower();//2018获取修改之后的单通道最大功率值
    g_CSystem.m_MinPower = g_CSystem.GetMinPower();//2018获取修改之后的单通道最小功率值           
    u8 crc_max = g_CTool.CRC8Value((u8*)&g_CSystem.m_MaxPower, 2);//最大功率CRC校验
    u8 crc_min = g_CTool.CRC8Value((u8*)&g_CSystem.m_MinPower, 2);//最小功率CRC校验
    g_CSystem.CRC_Max_Index(crc_max);
    g_CSystem.CRC_Min_Index(crc_min);
    if((g_CSystem.m_Version == MCU_ADC_VERSION)&&(g_CSystem.m_MinPower != 0))
        g_CSystem.m_MinPower = 40;
    g_CChargingPile.CCHARGINGPILE_MIN_POWER = g_CSystem.m_MinPower;//最小功率
    temp_buff[6] = 0;//修改结果，0 成功 1 失败    
    SendFrame(7, temp_buff, 0);
}
//在线结算请求
//返回 1 第一次发生，返回 0 为重发
u8 CUdpProtocol::OnlineTradeRequest(u8 ch, CON_NOTE *pRecord)
{
	u8 temp_SendBuff[90];
	*(u32 *)(temp_SendBuff) = 0;		  //命令流水号（4B）
	*(u16 *)(temp_SendBuff + 4) = 0x0416; //功能码(2B)
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
	for (u8 i = 0; i < CHARGING_LEN; i++)//充电记录一共有48个
	{
		if (pRecord->ChargingRecording[i] != 0xFEFE)
			len++;
		else
			break;
	}
	if (len == CHARGING_LEN)//没有找到结束符号
		len = 0;//这里面至少有个结束符号
	memcpy(temp_SendBuff + 34, pRecord->ChargingRecording, len);
	SendFrame(34 + len, temp_SendBuff, 2);
//	DBG_Printf("通道 %d 在线结算请求\r\n",ch);
	if (onlinetrade_fg[ch] == 0)
	{
		online_channels[ch] = 1;
		SetTimer(10, 30, this); //300ms等待回复 3s超时则重发，最多重发三次
		onlinetrade_fg[ch] = 1;
		pNoteStream[ch] = new CON_NOTE;
		memcpy(pNoteStream[ch], pRecord, sizeof(*pRecord));
		return 1;
	}
	return 0;
}
// //在线结算回复
void CUdpProtocol::OnlineTradeReturn(u8 *pData)
{
	if (!pData[6]) //成功
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
            onlinetrade_fg[i] = 0; //结算成功
			break;
		}
	}
}

//在线结算请求2
//返回 1 第一次发生，返回 0 为重发
//修改功率曲线为u16，功能码为0x0417
u8 CUdpProtocol::OnlineTradeRequest2(u8 ch, CON_NOTE *pRecord)
{
	u8 temp_SendBuff[140];
	*(u32 *)(temp_SendBuff) = 0;		  //命令流水号（4B）
	*(u16 *)(temp_SendBuff + 4) = 0x0417; //功能码(2B)
	*(u16 *)(temp_SendBuff + 6) = pRecord->Addr;
	*(u32 *)(temp_SendBuff + 8) = pRecord->TimeStamp;
	*(u32 *)(temp_SendBuff + 12) = pRecord->SerialNum;
	*(u8 *)(temp_SendBuff + 16) = pRecord->type;
	*(u32 *)(temp_SendBuff + 17) = pRecord->UserNO;
	*(u32 *)(temp_SendBuff + 21) = pRecord->UseValue;
	*(u32 *)(temp_SendBuff + 25) = pRecord->unit;
	*(u8 *)(temp_SendBuff + 29) = pRecord->channel;
	*(u32 *)(temp_SendBuff + 30) = pRecord->StoreTime;
    *(u8 *)(temp_SendBuff + 34) = pRecord->TradeReason;//结算原因（1B）
	u8 len = 0;
	for (u8 i = 0; i < CHARGING_LEN; i++)//充电记录一共有48个
	{
		if (pRecord->ChargingRecording[i] != 0xFEFE)
			len++;
		else
			break;
	}
	if (len == CHARGING_LEN)//没有找到结束符号
		len = 0;//这里面至少有个结束符号
	memcpy(temp_SendBuff + 35, pRecord->ChargingRecording, len*2);
	SendFrame(35 + len*2, temp_SendBuff, 2);
//	DBG_Printf("通道 %d 在线结算请求\r\n",ch);
	if (onlinetrade_fg[ch] == 0)
	{
		online_channels[ch] = 1;
		SetTimer(10, 30, this); //300ms等待回复 3s超时则重发，最多重发三次
		onlinetrade_fg[ch] = 1;
		pNoteStream[ch] = new CON_NOTE;
		memcpy(pNoteStream[ch], pRecord, sizeof(*pRecord));
		return 1;
	}
	return 0;
}

//发送掉电信息
void CUdpProtocol::send_powoff_inf(void)
{
    u8 my_len = 10;//数据长度 更改长度后需要修改
	u8 temp_SendBuff[20]={0};//数据缓存区
    
	*(u32*)(temp_SendBuff) = 0;//命令流水号（4B）
	*(u16*)(temp_SendBuff+4) = 0x1122;//功能码(2B)
    *(u8*)(temp_SendBuff+6) = 0xAA;//(1B)//数据域
    *(u8*)(temp_SendBuff+7) = 0xBB;//(1B)
    *(u8*)(temp_SendBuff+8) = 0xCC;//(1B)
    *(u8*)(temp_SendBuff+9) = 0xDD;//(1B)
    
    memset(m_SendFrame.Data, 0, SND_MAX_LEN	); //清空串口缓冲区内容
    m_SendFrame.Len = my_len + 18 + 6;
	//帧头
	m_SendFrame.Data[0] = 0x49;
	m_SendFrame.Data[1] = 0x8a;
	m_SendFrame.Data[2] = 0x5e;
	m_SendFrame.Data[3] = 0xb4;
	//帧长度(D-MAC(8)、随机数(2)、数据域、MAC(8))
	*(u16*)(m_SendFrame.Data+4) = my_len + 18;
	//D-MAC
	memcpy(m_SendFrame.Data+6, g_CSystem.m_DMAC, 8);
	//随机数//主动发送
    *(u16*)(m_SendFrame.Data+14) = ++m_SendRandomNum;
	//数据域
	memcpy(m_SendFrame.Data+16, temp_SendBuff, my_len);
	//数据域的命令流水号
    *(u32*)(m_SendFrame.Data+16) = ++m_CommandNum;
    m_CommandNumSave = m_CommandNum;
	//HMAC(帧长度、D-MAC、随机数、数据域)
	g_CTool.HMAC(my_len+12, m_SendFrame.Data+4, m_SendFrame.Data+my_len+16);	
	powoff_ComSend();
}

//发送测试信息
void CUdpProtocol::Send_TestInfo(void)
{
    char tempccid[20]={0};
	u8 temp_SendBuff[90]={0};
	*(u32*)(temp_SendBuff) = 0;//命令流水号（4B）
	*(u16*)(temp_SendBuff+4) = 0x0412;//功能码(2B)
    *(u8*)(temp_SendBuff+6) = g_CGPRS.Strengh;//信号强度(1B)
    if(g_CGPRS.GetModelType() == AIR_720)
    {
        *(u8*)(temp_SendBuff+7) = 2;//4G模块型号(1B)
        *(u16*)(temp_SendBuff+8) = (u16)g_CGPRS.UP_Version();//4G模块版本号(2B)
    }
    else if(g_CGPRS.GetModelType() == NEO_N720)
    {
        *(u8*)(temp_SendBuff+7) = 1;//4G模块型号(1B)
        *(u16*)(temp_SendBuff+8) = 0;//4G模块版本号(2B)
    }
    else
    {
        *(u8*)(temp_SendBuff+7) = 3;//4G模块型号(1B)
        *(u16*)(temp_SendBuff+8) = 0;//4G模块版本号(2B)
    }
    g_CGPRS.GetCCID(tempccid);
    memcpy(temp_SendBuff+10, tempccid, 20);//CCID 20字节
    memcpy(temp_SendBuff+30, (u8*)g_COverallTest.Send_Test_Pow, 2*20);//各通道功率值20路(2B*20)
    *(u16*)(temp_SendBuff+70) = g_COverallTest.Send_Test_Leakage;//漏电值(2B)
    memcpy(temp_SendBuff+72, g_COverallTest.Send_Test_Card, 4);//测试卡内码(4B)
    *(u32*)(temp_SendBuff+76) = g_COverallTest.Send_Test_KEY;//按键测试 bit 1:按过 0:未按(4B)
    *(u16*)(temp_SendBuff+80) = g_COverallTest.Send_Test_TotalPow;//总功率(2B)
    *(u32*)(temp_SendBuff+82) = g_IUsart.m_Meter_E_total;//电量初值(4B)

	SendFrame(86, temp_SendBuff, 2);
}

//测试信息返回
void CUdpProtocol::TestInfo_Reply(u8 *pData)
{
    if (!pData[6])//成功
    {
        g_COverallTest.TestInfo_ReplyOK();
    }
}

//发送异常信息
void CUdpProtocol::Send_abnormal_inf(ABNORM_NOTE *pData)
{
    u8 temp_SendBuff[30];
 //   u8 temp_type = 0;
	*(u32 *)(temp_SendBuff) = 0;		  //命令流水号（4B）
	*(u16 *)(temp_SendBuff + 4) = 0x0415; //功能码(2B)
    *(u32 *)(temp_SendBuff + 6) = pData->TimeStamp;//时间戳(4B)
    *(u32 *)(temp_SendBuff + 10) = pData->SerialNum;//异常信息流水号(4B)
    *(u8 *)(temp_SendBuff + 14) = pData->Type;//异常类型（1B）
    g_IAbnormalInf.AbnormalType = pData->Type;
    *(u8 *)(temp_SendBuff + 15) = pData->Length;//异常信息附件数据长度(1B)
    memcpy(temp_SendBuff+16,pData->Attach,pData->Length);//拷贝附加数据
    if(g_IAbnormalInf.AbnormalType == A_RELAYERR)
        g_IAbnormalInf.RelayErrCh = *(u32*)pData->Attach;
    SendFrame(16+pData->Length, temp_SendBuff, 2);
    u8 m_type =  pData->Type - 1;
    if (sendabnor_fg[m_type] == 0)
	{
        sendabnor_fg[m_type] = 1;
		abnor_sendtype[m_type] = 1;
		SetTimer(16, 30, this); //300ms等待回复 3s超时则重发，最多重发三次
		pAbnorNote[m_type] = new ABNORM_NOTE;
		memcpy(pAbnorNote[m_type], pData, sizeof(*pData));
	}
}
//异常信息回复
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
            Dprintf(BLUE, "abnormal", "异常收到回复\r\n");
			break;
		}
	}    
}

//获取位置信息
void CUdpProtocol::Get_Position(u8 *pDa)
{
    if((*(char*)g_CGPRS.m_Lac==NULL) && (*(char*)g_CGPRS.m_Id==NULL))//没有读到数据
        return;
    u8 temp_buff[40] = {0};	
    char temp_lac[20] = {'\0'};
    u8 lac_len = 0;
    *(u32 *)(temp_buff) = 0;		  //命令流水号（4B）
	*(u16 *)(temp_buff + 4) = 0x040d; //功能码(2B)
    *(u8 *)(temp_buff + 6) = g_CGPRS.m_cimi;//运营商类型(1B)
    sprintf(temp_lac,"%s,%s",g_CGPRS.m_Lac,g_CGPRS.m_Id);
    lac_len = strlen(temp_lac);
    memcpy(temp_buff + 7, (u8 *)temp_lac, lac_len);
    SendFrame(7+lac_len, temp_buff, 0);
}