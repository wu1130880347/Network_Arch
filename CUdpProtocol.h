#ifndef __C_UDP_PROTOCOL_H__
#define __C_UDP_PROTOCOL_H__
#include "head.h"
#include "CSuperTimer.h"
#include "INoteStream.h"
#include "IAbnormalInf.h"

#define RCV_MAX_LEN		512         //����������󻺴�
#define SND_MAX_LEN		512         //����������󻺴�
#define RCV_MAX_LEN_TEMP	(RCV_MAX_LEN+50)

#pragma pack(1)
typedef struct
{
	u16 Len;	//���ݳ���
	u8 Data[RCV_MAX_LEN];
}RECEIVE_FRAME;
#pragma pack() 

#pragma pack(1)
typedef struct
{
	u16 Len;	//���ݳ���
	u8 Data[SND_MAX_LEN];
}SEND_FRAME;
#pragma pack()

class CUdpProtocol:public CSuperTimer
{
public:
    u8 m_Request;                           //�Ƿ���յ��������� 0:������1:������
    RECEIVE_FRAME       m_ReceiveFrame;     //�������ݻ���
    SEND_FRAME          m_SendFrame;        //�������ݻ���
    u16 m_SendRandomNum;                    //��������ʱ�������
    u16 m_ReceiveRandomNum;                 //���յ��������
    u32 m_CommandNum;                       //��������ʱ��������ˮ��
    u32 m_CommandNumSave;                   //�����������͵�������ˮ��
	CUdpProtocol();
    
	virtual void ComSend(void);
	virtual void TimerEvent(u32 ulTimeNum);
    virtual BOOL ReceiveCheck(RECEIVE_FRAME *frame);

	void Agreement(void);
	void SendFrame(u16 len, u8 *pData, u8 mode);
	void ReSend(void);                      //�ط���������
	
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
	void OperatorCardReq(u8 *pCardNO);//��Ӫ�̿���������
	void OperatorCardReply(u8 *pData);//��Ӫ�̿�����ظ�
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
    u8 OnlineTradeRequest2(u8 ch, CON_NOTE *pRecord); //���߽�������2
    void SetServerList(u8 *pData);
    void SendCCID(u8 *CCID);
    void GetICCIDResponse(u8 *pData); //��ȡICCID
    void SendICCIDResponse(void);     //�ϱ�ICCID�󣬷������Ļظ�
    void Get_Meter_E_totalDispose(u8 *pData);
    void Get_Meter_E_totalReply(void);
    void Get_Parameter(u8 *pData);              //��ȡ�豸����
    void Set_MaxPower(u8 *pData);               //���õ�ͨ������
    void send_powoff_inf(void);                 //���͵�������
    virtual void powoff_ComSend(void);          //���緢��
    void Send_TestInfo(void);                   //���Ͳ�����Ϣ
    void TestInfo_Reply(u8 *pdata);             //������Ϣ����
    void Send_abnormal_inf(ABNORM_NOTE *pData); //�����쳣��Ϣ
    void Abnormal_reply(u8 *pData);             //�쳣��Ϣ�ظ�
    void Get_Position(u8 *pData);               //��ȡλ����Ϣ
};
#endif
