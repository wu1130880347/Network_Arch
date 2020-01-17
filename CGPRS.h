#ifndef __CGPRS_H__
#define __CGPRS_H__

#include "stm32f10x.h"
#include "CUdpProtocol.h"
#include "string"
#include "stdlib.h"
#include "sstream"
#define std

#define EC20F_TRACE //������ֲ�����޸�EC20F
//#ifdef EC20F_TRACE

//GPRSģ�� reset team_on ���ŵ�ƽ����
#define NEO_N720_4GPWR_H	GPIOD->BSRR |= (1<<0)
#define NEO_N720_4GPWR_L	GPIOD->BRR |= (1<<0) //PE11 �ߵ�ƽ�����ܵ�ͨ��pwr������������

#define AIR_720_4GPWR_H		GPIOD->BSRR |= (1<<0)
#define AIR_720_4GPWR_L		GPIOD->BRR |= (1<<0)

#ifdef EC20F_TRACE
#define EC20F_4GPWR_H 	GPIOD->BSRR |= (1 << 0)
#define EC20F_4GPWR_L 	GPIOD->BRR |= (1 << 0)
#endif

#define SIM800C_2GPWR_H	 	GPIOC->BRR |= (1<<3)
#define SIM800C_2GPWR_L	 	GPIOC->BSRR |= (1<<3) //PE11 �ߵ�ƽ�����ܵ�ͨ��pwr������������

#define NEO_N720_4GNORST	GPIOE->BRR |= (1<<11) //��2G����һ�����ŵ������ò�ͬ

#define AIR_720_4GNORST		GPIOE->BRR |= (1<<11)

#define GPRS_MODEL_ON_H		GPIOE->BSRR |= (1<<10)
#define GPRS_MODEL_ON_L		GPIOE->BRR |= (1<<10)

//ͨ��ģ�������
#define NEO_N720	0
#define SIM800C		1
#define AIR_720		2
#ifdef EC20F_TRACE
#define EC20F       3
#endif
#define GPRS_MODE_NUM  3 //GPRS֧������

//�Զ������汾����
#define UPDATE_VER  666//��ǰ���°汾666

//sim800
//ָ��״̬
#define CARRY_OUT 	  	1	//ִ��
#define WAIT_RESPOND 	2	//�ȴ�
 //�ص������ṹ
typedef u8 (*InsCarryCB)(string* Inc); 
typedef u8 (*InsWaitCB)(void);
//ָ��ṹ��
typedef struct
{
	string Inc; //ָ������
	u16 TimeOut;//ָ�ʱ ��ʱ���� 10ms
	u8 RunNum;//ָ���Ѿ�ִ�еĴ���
	u8 Retry;//ָ�����Դ���
	InsCarryCB CarryFunc;//ִ��̬�ص�����
	InsWaitCB WaitFunc;//�ȴ�̬�ص�����
}INSTRUCTION_STRUCT;

//ָ����Ӧ�ĺ�
#define RCVWAITING 1
#define RCVRIGHT 2
#define RCVWRONG 3

//ָ��͵ĺ�
#define SENDOK 1
#define SENDFAIL 2

//���ָ������
#define MAXINSNUM 30

//���ڷ��ͽ������õĽṹ��
#define RCV_MAX 600
#define SND_MAX 160
#pragma pack(1)
typedef struct
{
	u16 Len;	//���ݳ���
	u8 Data[RCV_MAX];
}RCV_FRAME;
#pragma pack()

#pragma pack(1)
typedef struct
{
	u16 Len;	//���ݳ���
	u8 Data[SND_MAX];
	u16 HaveSndLen;
}SND_FRAME;
#pragma pack()

#define CONNECTING 				1
#define ALREADYCONNECTED 		2

//ִ�д�����
#define ERROR_AT				0
#define ERROR_ATE0			 	1
#define ERROR_CPIN 				2
#define ERROR_CSQ				3
#define ERROR_CREG				4
#define ERROR_CGATT				5
#define ERROR_CCID				6
#define ERROR_CSTT				7
#define ERROR_CIICR 			8
#define ERROR_CIFSR				9
#define ERROR_CIPSHUT 			10
#define ERROR_CIPSTART 			11
#define ERROR_CIPSEND 			12
#define ERROR_CIPHEAD 			13
#define ERROR_NORMALSEND 		14
#define ERROR_DEACT 			15
#define ERROR_NORESPOND 		16
#define ERROR_NOARROW 			17
#define ERROR_OK 				18
#define ERROR_MYSYSINFO 		19
#define ERROR_CIPSTATUS			20
#define ERROR_MODEL				21

#define CCIDLEN 20//CCID 20������

#define ERROR_MODLE 1
#define ERROR_REGIST 2

class CGPRS:public CUdpProtocol
{
private:    
	void SetTimeout_Ms(u16 nms);
	u8 IfTimeout(void);
	//�����洢��ͬ��ָ��
	INSTRUCTION_STRUCT Instruction[MAXINSNUM];
	int Step;//���ATָ��ִ�еĲ���
	static CGPRS *pthis;//���ھ�̬�ص��������÷Ǿ�̬����/����
	u8 IncState;//ָ��״̬CARRY_OUT ִ�� WAIT_RESPOND �ȴ�	
    u8 Progress;//��������
    u16 Air_Ver;//�汾��
	//���ָ���±�ı���
	u8 CIPSENDIndex;//cipsendָ����±�
	u8 RebootIndex;//������ָ���±�
	u8 SysInfoIndex;//��ѯ������ʽ
	u8 ModelOnIndex;//������ָ���±�
	u8 CIPSHUTIndex;
	u8 CSTTIndex;
	u8 CSQIndex;
	u8 NetReadIndex;
	u8 StepMaxIndex;//����������
	u8 ShutDownIndex;
	u8 POWERIndex;// ��Դ�����±�
	u8 CIPSTATUSIndex;//�ر��±�����
	u8 CIPSTARTIndex;//���ӷ�����ip�˿��±�
    //2018.11.21���Air720ģ�������̼�
    u8 UPGRADE_KEY;//����Զ������KEY
    u8 UPGRADE;//ִ������
    u8 VERSION;//�汾��
#ifdef EC20F_TRACE
	u8 powerSUC; //�ӿ������ٶ�
#endif
	u8 ErrorState;//���gprs����λ��
	static char m_CCID[CCIDLEN];//�洢20λ��CCID
	u8 firstOn;//��һ���ϵ��ж�0 :��һ���ϵ� 1.�Ѿ��Ϲ�����
	static u8 ModelType;//ͨ��ģ������࣬2G ����4G
	static u8 ModeSwitchLock;    //ģʽ�л��� 0δ����1������ͨѶ��ʽ��2G/4G��
	void Reboot(void);
	static u8 PowerOn_CarryCB(string *Inc);
	static u8  PowerOn_WaitCB(void);
	static u8 ModelOn_CarryCB(string *Inc);
	static u8 ModelOn_WaitCB(void);
	static u8 InsSend_CB(string *Inc);
    static u8 OK_FIX(void);
	static u8 OK_CB(void);
#ifdef EC20F_TRACE
	static u8 TEST_REBOOT(string *Inc);
	static u8 QICLOSE_CB(void);
	static u8 CGREG_CB(void);
#endif
	static u8 AT_CB(void);
	static u8 CPIN_CB(void);
	static u8 CSQ_CB(void);
	static u8 CREG_CB(void);
	static u8 CGATT_CB(void);
	static u8 CCID_CB(void);
	static u8 CSTT_CB(void);
	static u8 PowerSoftOff_WaitCB(void);//
	static u8 NETACT_CB(void);
    static u8 CIPSTART_CB(void);
	static u8 NETCLOSE_CB(void);
	static u8 NETOPEN_CB(void);
	static u8 CIPSEND_CB1(string* Inc);
	static u8 CIPSEND_CB2(void);
	static u8 Reboot_CB1(string* Inc);
	static u8 Reboot_CB2(void);
	static u8 CIPSHUT_CB(void);
	static u8 NETREAD_CB(void);
	static u8 Mysysinfo_CB(void);
	static u8 PowerOff_CarryCB(string *Inc);
	static u8 PowerOff_WaitCB(void);
	static u8 CIFSR_CB(void);
	static u8 CIPSTART2G_CB(void);
	static u8 CIPSTATUS_CB(void);
	static u8 MODEL_CB(void);
    static u8 VER_CB(void);//�汾
    static u8 UPGRADE_CB(void);//����ָ��
    static u8 CIMI_CB(void);//��Ӫ������
    static u8 LACID_CB(void);//�������
	void ExceptionHandle(u8 *ErrorCode);
	u8 UnsolicitedHandle(char* pData);
	void ExceptionHandle_Unsolicite(u8 *ErrorCode);	

	//�ײ㹦����
	void ComBuffOut(void);
	u8 IFComBuffEmpty(void);
	u8 SetComBuff(u8* pData, u16 Len);
	u8 Write(string *str);
	void Read(string *str);
	void Read(char *strchar);
	
public:
    u8 Send_KEY_AT(void);
    u8 Send_UP_AT(void);
    u8 Send_Ver_AT(void);
    u8 Strengh;//�ź�ǿ��
    u8 UP_Start_flag;
    u8 UP_Finsh;
    u8 CarryBlock(u8 StepNum, u8 StartTime);
    
    u8 GPRSState;//GPRS������� CONNECTING �������� ALREADYCONNECTED �Ѿ����ӳɹ�
	u8 m_GprsWrong;//ERRORMODLE ģ������  ERROR_REGIST ���ע��ʧ��
    u8 m_cimi;//��Ӫ������
    u8 m_Lac[5];//����� ����һ���ַ�'\0'
    u8 m_Id[8];//С���� ����һ���ַ� '\0'
	CGPRS();
	~CGPRS();
	virtual void Init(void);
	void RCV_DataProcess(void);
	int gprs_strength(void);
    int UP_Progress(void);//�������Ȳ�ѯ
    int UP_Version(void);//�汾�Ų�ѯ
	void Timer2Config(void);
	virtual void ResetHeartBeatTimer(void);
	virtual void TimeoutCountReset(void);
	virtual void ComSend(void);
	virtual void TimerEvent(u32 ulTimeNum);
	u8 SendData(u16 len, u8 *pData);
	void Init_Reset(void);
    void UP_Reset(void);//����֮������
	virtual BOOL ReceiveCheck(RECEIVE_FRAME *frame);
	void CloseGPRS(void);
	u8  GetCCID(char *pCCID);
	void Reconnect(void);

	u8 CompareICCID(void);
	u8 GetOlderICCIDFromFlash(char *pICCID);
	void WriteICCIDToFlash(void);
	u8 GetModelType(void);
	RCV_FRAME ComRcv;//�� �㴮����������
	SND_FRAME ComSnd;
	BOOL ComRequest;
    void SetModelType(u8 mode);
    u8 powoff_SendData(u16 len, u8 *pData);//���緢��
    virtual void powoff_ComSend(void);//���緢��
    void send_poweroff(void);
    void send_testinf_time(void);//���ͼ����Ϣ
};
#endif //__CGPRS_H__
