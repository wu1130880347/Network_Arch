#ifndef __CGPRS_H__
#define __CGPRS_H__

#include "stm32f10x.h"
#include "CUdpProtocol.h"
#include "string"
#include "stdlib.h"
#include "sstream"
#define std

#define EC20F_TRACE //用于移植跟踪修改EC20F
//#ifdef EC20F_TRACE

//GPRS模块 reset team_on 引脚电平控制
#define NEO_N720_4GPWR_H	GPIOD->BSRR |= (1<<0)
#define NEO_N720_4GPWR_L	GPIOD->BRR |= (1<<0) //PE11 高电平三极管导通，pwr控制引脚拉低

#define AIR_720_4GPWR_H		GPIOD->BSRR |= (1<<0)
#define AIR_720_4GPWR_L		GPIOD->BRR |= (1<<0)

#ifdef EC20F_TRACE
#define EC20F_4GPWR_H 	GPIOD->BSRR |= (1 << 0)
#define EC20F_4GPWR_L 	GPIOD->BRR |= (1 << 0)
#endif

#define SIM800C_2GPWR_H	 	GPIOC->BRR |= (1<<3)
#define SIM800C_2GPWR_L	 	GPIOC->BSRR |= (1<<3) //PE11 高电平三极管导通，pwr控制引脚拉低

#define NEO_N720_4GNORST	GPIOE->BRR |= (1<<11) //与2G公用一个引脚但是作用不同

#define AIR_720_4GNORST		GPIOE->BRR |= (1<<11)

#define GPRS_MODEL_ON_H		GPIOE->BSRR |= (1<<10)
#define GPRS_MODEL_ON_L		GPIOE->BRR |= (1<<10)

//通信模块的种类
#define NEO_N720	0
#define SIM800C		1
#define AIR_720		2
#ifdef EC20F_TRACE
#define EC20F       3
#endif
#define GPRS_MODE_NUM  3 //GPRS支持数量

//自动升级版本限制
#define UPDATE_VER  666//当前最新版本666

//sim800
//指令状态
#define CARRY_OUT 	  	1	//执行
#define WAIT_RESPOND 	2	//等待
 //回调函数结构
typedef u8 (*InsCarryCB)(string* Inc); 
typedef u8 (*InsWaitCB)(void);
//指令结构体
typedef struct
{
	string Inc; //指令内容
	u16 TimeOut;//指令超时 超时倍率 10ms
	u8 RunNum;//指令已经执行的次数
	u8 Retry;//指令重试次数
	InsCarryCB CarryFunc;//执行态回调函数
	InsWaitCB WaitFunc;//等待态回调函数
}INSTRUCTION_STRUCT;

//指令响应的宏
#define RCVWAITING 1
#define RCVRIGHT 2
#define RCVWRONG 3

//指令发送的宏
#define SENDOK 1
#define SENDFAIL 2

//最大指令条数
#define MAXINSNUM 30

//串口发送接收所用的结构体
#define RCV_MAX 600
#define SND_MAX 160
#pragma pack(1)
typedef struct
{
	u16 Len;	//数据长度
	u8 Data[RCV_MAX];
}RCV_FRAME;
#pragma pack()

#pragma pack(1)
typedef struct
{
	u16 Len;	//数据长度
	u8 Data[SND_MAX];
	u16 HaveSndLen;
}SND_FRAME;
#pragma pack()

#define CONNECTING 				1
#define ALREADYCONNECTED 		2

//执行错误码
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

#define CCIDLEN 20//CCID 20个数字

#define ERROR_MODLE 1
#define ERROR_REGIST 2

class CGPRS:public CUdpProtocol
{
private:    
	void SetTimeout_Ms(u16 nms);
	u8 IfTimeout(void);
	//用来存储不同的指令
	INSTRUCTION_STRUCT Instruction[MAXINSNUM];
	int Step;//标记AT指令执行的步骤
	static CGPRS *pthis;//用于静态回调函数调用非静态变量/函数
	u8 IncState;//指令状态CARRY_OUT 执行 WAIT_RESPOND 等待	
    u8 Progress;//升级进度
    u16 Air_Ver;//版本号
	//标记指令下标的变量
	u8 CIPSENDIndex;//cipsend指令的下标
	u8 RebootIndex;//重启块指令下标
	u8 SysInfoIndex;//查询网络制式
	u8 ModelOnIndex;//启动块指令下标
	u8 CIPSHUTIndex;
	u8 CSTTIndex;
	u8 CSQIndex;
	u8 NetReadIndex;
	u8 StepMaxIndex;//标记最大块序号
	u8 ShutDownIndex;
	u8 POWERIndex;// 电源控制下标
	u8 CIPSTATUSIndex;//关闭下标链接
	u8 CIPSTARTIndex;//连接服务器ip端口下标
    //2018.11.21添加Air720模块升级固件
    u8 UPGRADE_KEY;//配置远程升级KEY
    u8 UPGRADE;//执行升级
    u8 VERSION;//版本号
#ifdef EC20F_TRACE
	u8 powerSUC; //加快启动速度
#endif
	u8 ErrorState;//标记gprs出错位置
	static char m_CCID[CCIDLEN];//存储20位的CCID
	u8 firstOn;//第一次上电判断0 :第一次上电 1.已经上过电了
	static u8 ModelType;//通信模块的种类，2G 或者4G
	static u8 ModeSwitchLock;    //模式切换锁 0未锁；1已锁定通讯方式（2G/4G）
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
    static u8 VER_CB(void);//版本
    static u8 UPGRADE_CB(void);//升级指令
    static u8 CIMI_CB(void);//运营商名称
    static u8 LACID_CB(void);//区域代码
	void ExceptionHandle(u8 *ErrorCode);
	u8 UnsolicitedHandle(char* pData);
	void ExceptionHandle_Unsolicite(u8 *ErrorCode);	

	//底层功能性
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
    u8 Strengh;//信号强度
    u8 UP_Start_flag;
    u8 UP_Finsh;
    u8 CarryBlock(u8 StepNum, u8 StartTime);
    
    u8 GPRSState;//GPRS连接情况 CONNECTING 正在连接 ALREADYCONNECTED 已经连接成功
	u8 m_GprsWrong;//ERRORMODLE 模块问题  ERROR_REGIST 多次注册失败
    u8 m_cimi;//运营商类型
    u8 m_Lac[5];//区域号 空余一个字符'\0'
    u8 m_Id[8];//小区号 空余一个字符 '\0'
	CGPRS();
	~CGPRS();
	virtual void Init(void);
	void RCV_DataProcess(void);
	int gprs_strength(void);
    int UP_Progress(void);//升级进度查询
    int UP_Version(void);//版本号查询
	void Timer2Config(void);
	virtual void ResetHeartBeatTimer(void);
	virtual void TimeoutCountReset(void);
	virtual void ComSend(void);
	virtual void TimerEvent(u32 ulTimeNum);
	u8 SendData(u16 len, u8 *pData);
	void Init_Reset(void);
    void UP_Reset(void);//升级之后重启
	virtual BOOL ReceiveCheck(RECEIVE_FRAME *frame);
	void CloseGPRS(void);
	u8  GetCCID(char *pCCID);
	void Reconnect(void);

	u8 CompareICCID(void);
	u8 GetOlderICCIDFromFlash(char *pICCID);
	void WriteICCIDToFlash(void);
	u8 GetModelType(void);
	RCV_FRAME ComRcv;//底 层串口数据区域
	SND_FRAME ComSnd;
	BOOL ComRequest;
    void SetModelType(u8 mode);
    u8 powoff_SendData(u16 len, u8 *pData);//掉电发送
    virtual void powoff_ComSend(void);//掉电发送
    void send_poweroff(void);
    void send_testinf_time(void);//发送检测信息
};
#endif //__CGPRS_H__
