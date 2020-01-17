#ifndef __C_UDP_PROTOCOL_H__
#define __C_UDP_PROTOCOL_H__
#include "head.h"
#include "CSuperTimer.h"
#include "INoteStream.h"
#include "IAbnormalInf.h"

#define RCV_MAX_LEN		512         //交互接收最大缓存
#define SND_MAX_LEN		512         //交互发送最大缓存
#define RCV_MAX_LEN_TEMP	(RCV_MAX_LEN+50)

#pragma pack(1)
typedef struct
{
	u16 Len;	//数据长度
	u8 Data[RCV_MAX_LEN];
}RECEIVE_FRAME;
#pragma pack() 

#pragma pack(1)
typedef struct
{
	u16 Len;	//数据长度
	u8 Data[SND_MAX_LEN];
}SEND_FRAME;
#pragma pack()

class CUdpProtocol:public CSuperTimer
{
public:
    u8 m_Request;                           //是否接收到数据请求 0:无请求；1:有请求
    RECEIVE_FRAME       m_ReceiveFrame;     //接收数据缓冲
    SEND_FRAME          m_SendFrame;        //发送数据缓冲
    u16 m_SendRandomNum;                    //主动发送时的随机数
    u16 m_ReceiveRandomNum;                 //接收到的随机数
    u32 m_CommandNum;                       //主动发送时的命令流水号
    u32 m_CommandNumSave;                   //保存主动发送的命令流水号
	CUdpProtocol();
    
	virtual void ComSend(void);
	virtual void TimerEvent(u32 ulTimeNum);
    virtual BOOL ReceiveCheck(RECEIVE_FRAME *frame);

	void Agreement(void);
	void SendFrame(u16 len, u8 *pData, u8 mode);
	void ReSend(void);                      //重发缓冲数据
	
	void SendHeartBeat(void);
	void HeartBeatReply(u8 *pData);
	void SendRegister(void);
	void RegisterReply(u8 *pData);
	void GetPosState(u8 *pData);
	void SetNodeAddr(u8 *pData);
	void SetTradeMode(u8 *pData);
	void SetTradeUnit(u8 *pData);
	void SetTradeOffTime(u8 *pData);
	void ClearCoin(u8 *pData);
	void SetPosPara(u8 *pData);
	void CardTrade(u8 *pCardNO, u32 money,u8 channel);
	void CardTradeReply(u8 *pData);
	void BalanceQueryRequest(u8 *CardNO);
	void BalanceQueryReply(u8 *pData);
	void OperatorCardReq(u8 *pCardNO);//运营商卡请求配置
	void OperatorCardReply(u8 *pData);//运营商卡请求回复
	void CancelTradeReq(void);  
	void CancelTradeReply(u8 *pData);
	void UpLoadCoins(u8 *pData);
	void StartCharge(u8 *pData);
	void StopCharge(u8 *pData);
	void EmptyNote(u8 *pData);
	void ReadNote(u8 *pData);
	void EmptyVoice(u8 *pData);
	void DownloadFileName(u8 *pData);
	void DownloadFileData(u8 *pData);
	void LongRangeReset(u8 *pData);
	void SetComKey(u8 *pData);
	void SetDMAC(u8 *pData);
	u8 GetLoginState(void);
	void SetTradePerUnit(u8 *pData);
	void OnlineTradeReturn(u8 *pData);
	u8 OnlineTradeRequest(u8 ch, CON_NOTE *pRecord);
    u8 OnlineTradeRequest2(u8 ch, CON_NOTE *pRecord); //在线结算请求2
    void SetServerList(u8 *pData);
    void SendCCID(u8 *CCID);
    void GetICCIDResponse(u8 *pData); //获取ICCID
    void SendICCIDResponse(void);     //上报ICCID后，服务器的回复
    void Get_Meter_E_totalDispose(u8 *pData);
    void Get_Meter_E_totalReply(void);
    void Get_Parameter(u8 *pData);              //获取设备参数
    void Set_MaxPower(u8 *pData);               //设置单通道功率
    void send_powoff_inf(void);                 //发送掉电数据
    virtual void powoff_ComSend(void);          //掉电发送
    void Send_TestInfo(void);                   //发送测试信息
    void TestInfo_Reply(u8 *pdata);             //测试信息返回
    void Send_abnormal_inf(ABNORM_NOTE *pData); //发送异常信息
    void Abnormal_reply(u8 *pData);             //异常信息回复
    void Get_Position(u8 *pData);               //获取位置信息
};
#endif
