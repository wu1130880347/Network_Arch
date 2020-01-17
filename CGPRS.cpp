#include "driver.h"
#include "Stm_init.h"
#include "ctype.h"
#define UDPCONNECT//udp连接

CGPRS *CGPRS::pthis = NULL;
char CGPRS::m_CCID[CCIDLEN] ={0};
u8 CGPRS::ModelType = AIR_720;//上电默认是4G 模块
u8 CGPRS::ModeSwitchLock = 0;//通讯方式未锁
__IO BOOL bTimerFirstArrive = TRUE;	//是否是第一次进入定时器中断

u8 GPRS_Tick = 0;//连续60s 收不到数据查GPRS 状态
u8 UP_Timer = 0;//定时检测是否升级到最新版本
CGPRS::CGPRS()
{
	Step = 0;
	pthis = this;
	IncState = CARRY_OUT;
	Strengh = 0;
	GPRSState = CONNECTING;
	ComRequest = FALSE;
	CIPSENDIndex = 0;
	RebootIndex = 0;
	SysInfoIndex = 0;
	ModelOnIndex = 0;
	CIPSHUTIndex = 0;
	CSTTIndex = 0;
	CSQIndex = 0;
	NetReadIndex = 0;
	StepMaxIndex = 0;
	ShutDownIndex = 0;
	POWERIndex = 0; 
	CIPSTATUSIndex = 0;
	CIPSTARTIndex = 0;
	ErrorState = 0;
	memset(m_CCID, 0, sizeof(m_CCID)/sizeof(char));
	memset(m_Lac, '\0', sizeof(m_Lac));//区域号定位
    memset(m_Id, '\0', sizeof(m_Id));
    
	firstOn = 0;
    UP_Start_flag=0;
    Progress = 0;//升级进度
    UP_Finsh = 0;
    g_COverallTest.Comm_Sucess_flag = 0; 
    g_COverallTest.StartSendTestInfo = 0;
}

CGPRS::~CGPRS()
{
	
}
/*
函数名：    Send_KEY_AT()
函数功能：   发送升级KEY命令
入口参数：   无
出口参数：   1：成功 0：失败
*/
u8 CGPRS::Send_KEY_AT(void)
{
    if( CarryBlock(UPGRADE_KEY, 1) ==0 )//0成功 1失败
        return 1;
    else
        return 0;
}
/*
函数名：    Send_UP_AT()
函数功能：   发送升级命令
入口参数：   无
出口参数：   1：成功 0：失败
*/
u8 CGPRS::Send_UP_AT(void)
{
    if( CarryBlock(UPGRADE, 1) ==0 )//0成功 1失败   
        return 1;
    else 
        return 0;
}

/*
函数名：    Send_Ver_AT()
函数功能：   发送查询版本命令
入口参数：   无
出口参数：   1：成功 0：失败
*/
u8 CGPRS::Send_Ver_AT(void)
{
    if( CarryBlock(VERSION, 1) ==0 )//0成功 1失败   
        return 1;
    else 
        return 0;
}

/*******************************************************************************
 函数名：   UP_Reset
 功能描述：4G下载完升级文件后需重启安装
 入口参数：无
 出口参数：无
********************************************************************************/
void CGPRS::UP_Reset(void)
{
    KillTimer(this);
    GPRSState = ALREADYCONNECTED;
     g_CSystem.FeedDog();
    AIR_720_4GPWR_L;//5s
    g_CSystem.delay_nms(2000); 
     g_CSystem.FeedDog();
     g_CSystem.delay_nms(2000);
     g_CSystem.FeedDog();
     g_CSystem.delay_nms(1000);
     g_CSystem.FeedDog();
    AIR_720_4GPWR_H;//2s
    g_CSystem.delay_nms(2000); 
    g_CSystem.FeedDog();
    GPRS_MODEL_ON_L;//1.25s
    g_CSystem.delay_nms(1250); 
    g_CSystem.FeedDog();
    GPRS_MODEL_ON_H;//10s
    g_CSystem.delay_nms(2000); 
    g_CSystem.FeedDog(); 
    g_CSystem.delay_nms(2000);
    g_CSystem.FeedDog();
    g_CSystem.delay_nms(2000);
    g_CSystem.FeedDog();
    g_CSystem.delay_nms(2000);
    g_CSystem.FeedDog();
    g_CSystem.delay_nms(2000);
    g_CSystem.FeedDog();
}
/*******************************************************************************
 函数名：	Init_Reset
 功能描述：重新初始化gprs各个成员变量
 入口参数：无
 出口参数：无
********************************************************************************/
void CGPRS::Init_Reset(void)
{
	KillTimer(this);
	Step = 0;
	pthis = this;
	IncState = CARRY_OUT;
	Strengh = 0;
	GPRSState = CONNECTING;
    UP_Start_flag=0;
    Progress = 0;//升级进度   
    UP_Finsh = 0;
    UP_Timer = 0;
    
    ErrorState = 0;
	ComRequest = FALSE;
	ComSnd.HaveSndLen = 0;//缓冲未使用
    SetTimer(11, 1, this);
    SetTimer(12, 100, this);//定时检查有没有错误
    g_COverallTest.Comm_Sucess_flag = 0;
    g_COverallTest.StartSendTestInfo = 0;
    memset(m_Lac, '\0',sizeof(m_Lac));
    memset(m_Id, '\0',sizeof(m_Id));
	u8 i = 0;
	POWERIndex = i;
	Instruction[i].Inc = "POWERON";  //电源控制
	Instruction[i].TimeOut = 0;
	Instruction[i].RunNum = 0;
	Instruction[i].Retry = 0;
	Instruction[i].CarryFunc = &PowerOn_CarryCB;
	Instruction[i].WaitFunc = &PowerOn_WaitCB;
	i++;
	ModelOnIndex = i;
	Instruction[i].Inc = "MODELON";  //启动控制
	Instruction[i].TimeOut = 0;
	Instruction[i].RunNum = 0;
	Instruction[i].Retry = 0;
	Instruction[i].CarryFunc = &ModelOn_CarryCB;
	Instruction[i].WaitFunc =&ModelOn_WaitCB;
	i++;
	if (ModelType == NEO_N720)
	{
		Instruction[i].Inc = "AT\r\n";  //波特率同步
		Instruction[i].TimeOut = 100;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &AT_CB;
		i++;
		
		Instruction[i].Inc = "ATE0\r\n";  //关闭回显
		Instruction[i].TimeOut = 10;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &OK_CB;
		i++;

		Instruction[i].Inc = "AT+CGMM\r\n";  //查询模块型号
		Instruction[i].TimeOut = 10;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &MODEL_CB;
		i++;
		
		Instruction[i].Inc = "AT+CCID\r\n";  //获取CCID编号
		Instruction[i].TimeOut = 100;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 2;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CCID_CB;
		i++;
		
		Instruction[i].Inc = "AT+CPIN?\r\n";  //检查sim卡是否在位
		Instruction[i].TimeOut = 500;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CPIN_CB;
		i++;
		
		CSQIndex = i;
		Instruction[i].Inc = "AT+CSQ\r\n";  //读取信号强度
		Instruction[i].TimeOut = 100;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 40;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CSQ_CB;
		i++;

		Instruction[i].Inc = "AT+CREG?\r\n";  //检查注册状态
		Instruction[i].TimeOut = 100;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 40;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CREG_CB;
		i++;

		Instruction[i].Inc = "AT+CGATT?\r\n";  //检查注册状态
		Instruction[i].TimeOut = 100;//10s超时
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 10;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CGATT_CB;
		i++;
		//执行完cgatt则表明模块注册网络成功

		Instruction[i].Inc = "AT$MYNETURC=1\r\n";  //开启主动上报
		Instruction[i].TimeOut = 100;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 1;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &OK_CB;
		i++;
		
		CSTTIndex = i;
		Instruction[i].Inc = "AT$MYNETCON=0,APN,CMIOT\r\n";  //设置apn
		Instruction[i].TimeOut = 10;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 1;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CSTT_CB;
		i++;

		Instruction[i].Inc = "AT$MYNETACT=0,1\r\n";  //激活网络链接
		Instruction[i].TimeOut = 4000;//40s一次激活3次
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &NETACT_CB;
		i++;

		Instruction[i].Inc = "AT$MYNETCLOSE=0\r\n"; //关闭链路
		Instruction[i].TimeOut = 100;//4
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 1;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &NETCLOSE_CB;
		i++;
		CIPSTARTIndex = i;
		//获取服务器的ip 端口
//		u8 temp_ip[4];
//		u16 temp_port;
//		g_CSystem.GetUsingServerIP(temp_ip);
//		temp_port = g_CSystem.GetUsingServerPort();
		char buff[50];
		if (firstOn == 0)
			sprintf(buff, "AT$MYNETSRV=0,0,2,0,\"%d.%d.%d.%d:%d\"\r\n", 120, 27, 138, 107, 8887);
		else
		{
			u8 temp_ip[4];
			u16 temp_port;
			g_CSystem.GetUsingServerIP(temp_ip);
			temp_port = g_CSystem.GetUsingServerPort();
			sprintf(buff, "AT$MYNETSRV=0,0,2,0,\"%d.%d.%d.%d:%d\"\r\n", temp_ip[0], temp_ip[1], temp_ip[2], temp_ip[3], temp_port);
		}
		Instruction[i].Inc.assign(buff); //建立udp链接
		Instruction[i].TimeOut = 1000;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CIPSTART_CB;
		i++;
    
         //CIMI码,区分运营商
        Instruction[i].Inc = "AT+CIMI\r\n";
		Instruction[i].TimeOut = 150;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CIMI_CB;
		i++;

        //获取区域代码
        Instruction[i].Inc = "AT$MYLACID\r\n";
		Instruction[i].TimeOut = 150;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &LACID_CB;
		i++;
        
		Instruction[i].Inc = "AT$MYNETOPEN=0\r\n";  //打开链路
		Instruction[i].TimeOut = 150;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &NETOPEN_CB;
		i++;
		
		Instruction[i].Inc = "";  //空的指令块用来结束配置流程
		Instruction[i].TimeOut = 0;//5s
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 0;
		Instruction[i].CarryFunc = NULL;
		Instruction[i].WaitFunc = NULL;
		i++; 
		
		//下面的指令不属于配置流程里面的指令
		//独立指令
		CIPSENDIndex = i;//标记下标
		Instruction[i].Inc = "AT$MYNETWRITE=0,";//发送数据指令  
		Instruction[i].TimeOut =	 200;// 如果失败的话645s之后模块会主动上报close
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 0;
		Instruction[i].CarryFunc = &CIPSEND_CB1;
		Instruction[i].WaitFunc = &CIPSEND_CB2;
		i++; 

		//读取的最大字段是2048
		NetReadIndex = i;
		Instruction[i].Inc = "AT$MYNETREAD=0,1024\r\n";  //读取已经收到的数据
		Instruction[i].TimeOut = 30;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 8;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &NETREAD_CB;
		i++;
		
		RebootIndex = i;//标记下标
		Instruction[i].Inc = "Reboot";//发送数据指令  
		Instruction[i].TimeOut = 10;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 0;
		Instruction[i].CarryFunc = &Reboot_CB1;
		Instruction[i].WaitFunc = &Reboot_CB2;
		i++; 

		SysInfoIndex = i;//查询网络制式
		Instruction[i].Inc = "AT$MYSYSINFO";
		Instruction[i].TimeOut = 10;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 0;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc =&Mysysinfo_CB;
		i++;
		
		ShutDownIndex = i;//关机
		Instruction[i].Inc = "AT$MYPOWEROFF\r\n";
		//Instruction[i].Inc = "PowerOff";//原
		Instruction[i].TimeOut = 10;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 0;
		//Instruction[i].CarryFunc = &PowerOff_CarryCB;//原
        Instruction[i].CarryFunc = &InsSend_CB;
		//Instruction[i].WaitFunc = &PowerOff_WaitCB;//原
        Instruction[i].WaitFunc = &PowerSoftOff_WaitCB;
		i++;
	}
	else if(ModelType == SIM800C)
	{
		// 2G 模块的配置
		Instruction[i].Inc = "ATE0\r\n";  //关闭回显
		Instruction[i].TimeOut = 10;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &OK_CB;
		i++;
		
		Instruction[i].Inc = "AT+CGMM\r\n";  //查询模块型号
		Instruction[i].TimeOut = 10;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &MODEL_CB;
		i++;
		
		Instruction[i].Inc = "AT+CPIN?\r\n";  //检查sim卡是否在位
		Instruction[i].TimeOut = 500;//5s
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CPIN_CB;
		i++;
		CSQIndex = i;
		Instruction[i].Inc = "AT+CSQ\r\n";  //读取信号强度
		Instruction[i].TimeOut = 80;//800 ms
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 0;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CSQ_CB;
		i++;

		Instruction[i].Inc = "AT+CREG?\r\n";  //检查注册状态
		Instruction[i].TimeOut = 10;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 20;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CREG_CB;
		i++;

		Instruction[i].Inc = "AT+CGATT?\r\n";  //检查注册状态
		Instruction[i].TimeOut = 1000;//10s超时
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 40;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CGATT_CB;
		i++;

		Instruction[i].Inc = "AT+CCID\r\n";  //获取CCID编号
		Instruction[i].TimeOut = 200;// 2s超时
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 1;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CCID_CB;
		i++;
		
		CSTTIndex = i;
		Instruction[i].Inc = "AT+CSTT=\"CMIOT\"\r\n";  //设置apn
		Instruction[i].TimeOut = 10;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 1;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CSTT_CB;
		i++;

		Instruction[i].Inc = "AT+CIICR\r\n";  //建立无线链路
		Instruction[i].TimeOut = 8500;//85s 超时
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 1;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &OK_CB;
		i++;

		Instruction[i].Inc = "AT+CIFSR\r\n";  //获取本机ip
		Instruction[i].TimeOut = 10;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 1;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CIFSR_CB;
		i++;

		//固定udp 源端口,即pos机自身端口
		Instruction[i].Inc = "AT+CLPORT=\"UDP\",\"8888\"\r\n";  
		Instruction[i].TimeOut = 10;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 1;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &OK_CB;
		i++;

		CIPSTARTIndex = i;
		//获取服务器的ip 端口
//		u8 temp_ip[4];
//		u16 temp_port;
//		g_CSystem.GetUsingServerIP(temp_ip);
//		temp_port = g_CSystem.GetUsingServerPort();
		char buff[50];
		if (firstOn == 0)
            sprintf(buff, "AT+CIPSTART=\"UDP\",\"%d.%d.%d.%d\",\"%d\"\r\n", 120, 27, 138, 107, 8887);
        else
		{
			u8 temp_ip[4];
			u16 temp_port;
			g_CSystem.GetUsingServerIP(temp_ip);
			temp_port = g_CSystem.GetUsingServerPort();
            sprintf(buff, "AT+CIPSTART=\"UDP\",\"%d.%d.%d.%d\",\"%d\"\r\n", temp_ip[0], temp_ip[1], temp_ip[2], temp_ip[3], temp_port);
		}
		Instruction[i].Inc.assign(buff); //建立udp链接
		Instruction[i].TimeOut = 16000;// 160s 单链接
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 1;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CIPSTART2G_CB;
		i++;

		Instruction[i].Inc = "AT+CIPHEAD=1\r\n";  //接受到的数据加上前缀+IPD
		Instruction[i].TimeOut = 10;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 1;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &NETOPEN_CB; //与4G 的处理方式相同
		i++;

		Instruction[i].Inc = "";  //空的指令块用来结束配置流程
		Instruction[i].TimeOut = 0;//5s
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 0;
		Instruction[i].CarryFunc = NULL;
		Instruction[i].WaitFunc = NULL;
		i++; 
		
		//下面的指令不属于配置流程里面的指令
		//独立指令
		CIPSENDIndex = i;//标记下标
		Instruction[i].Inc = "AT+CIPSEND=";//发送数据指令  
		Instruction[i].TimeOut = 300;// 如果失败的话645s之后模块会主动上报close
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &CIPSEND_CB1;
		Instruction[i].WaitFunc = &CIPSEND_CB2;
		i++; 

		RebootIndex = i;//标记下标
		Instruction[i].Inc = "Reboot";//发送数据指令  
		Instruction[i].TimeOut = 10;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 0;
		Instruction[i].CarryFunc = &Reboot_CB1;
		Instruction[i].WaitFunc = &Reboot_CB2;
		i++; 

		CIPSHUTIndex = i;//标记下标
		Instruction[i].Inc = "AT+CIPSHUT\r\n";//关闭链接  
		Instruction[i].TimeOut = 800; 
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 0;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CIPSHUT_CB; //17
		i++; 

		CIPSTATUSIndex = i;//标记下标
		Instruction[i].Inc = "AT+CIPSTATUS\r\n";//关闭链接  
		Instruction[i].TimeOut = 100; 
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 0;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CIPSTATUS_CB;
		i++; 	
	}
    else if(ModelType == AIR_720)
    {
		Instruction[i].Inc = "AT\r";  //波特率同步
		Instruction[i].TimeOut = 100;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 30;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &AT_CB;
		i++;              
        
        Instruction[i].Inc = "ATE0\r";  //关闭回显
		Instruction[i].TimeOut = 10;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &OK_CB;
		i++;
		
        Instruction[i].Inc = "AT+UPGRADE=\"AUTO\", 0\r";  //关闭自动升级
		Instruction[i].TimeOut = 10;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &OK_FIX;
		i++;
        
		Instruction[i].Inc = "AT+CGMM\r";  //查询模块型号
		Instruction[i].TimeOut = 10;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &MODEL_CB;
		i++;
		
		Instruction[i].Inc = "AT+CPIN?\r";  //检查sim卡是否在位
		Instruction[i].TimeOut = 500;//5s
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CPIN_CB;
		i++;
        
		CSQIndex = i;
		Instruction[i].Inc = "AT+CSQ\r";  //读取信号强度
		Instruction[i].TimeOut = 120;//800 ms
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CSQ_CB;
		i++;

        Instruction[i].Inc = "AT+CIMI\r";//运营商类型
		Instruction[i].TimeOut = 100;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CIMI_CB;
		i++;
        
        Instruction[i].Inc = "AT+CREG=2\r";//返回LOC和IC
		Instruction[i].TimeOut = 100;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &OK_CB;
		i++;
        
		Instruction[i].Inc = "AT+CREG?\r";  //检查注册状态
		Instruction[i].TimeOut = 100;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 60;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CREG_CB;
		i++;

		Instruction[i].Inc = "AT+CGATT?\r";  //检查GPRS附着状态
		Instruction[i].TimeOut = 1000;//10s超时
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 40;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CGATT_CB;
		i++;
              
		Instruction[i].Inc = "AT+ICCID\r";  //获取CCID编号
		Instruction[i].TimeOut = 400;// 2s超时
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 4;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CCID_CB;
		i++;
        
		CSTTIndex = i;
		Instruction[i].Inc = "AT+CSTT=\"CMIOT\"\r";  //设置apn
		Instruction[i].TimeOut = 10;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 10;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CSTT_CB;
		i++;

		Instruction[i].Inc = "AT+CIICR\r";  //建立无线链路
		Instruction[i].TimeOut = 8500;//85s 超时
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 2;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &OK_CB;
		i++;

		Instruction[i].Inc = "AT+CIFSR\r";  //获取本机ip
		Instruction[i].TimeOut = 600;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 1;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CIFSR_CB;
		i++;

		CIPSTARTIndex = i;
		//获取服务器的ip 端口
//		u8 temp_ip[4];
//		u16 temp_port;
//		g_CSystem.GetUsingServerIP(temp_ip);
//		temp_port = g_CSystem.GetUsingServerPort();
		char buff[50];
		if (firstOn == 0)
            sprintf(buff, "AT+CIPSTART=\"UDP\",\"%d.%d.%d.%d\",\"%d\"\r", 120, 27, 138, 107, 8887);			
		else
		{
			u8 temp_ip[4];
			u16 temp_port;
			g_CSystem.GetUsingServerIP(temp_ip);
			temp_port = g_CSystem.GetUsingServerPort();
            sprintf(buff, "AT+CIPSTART=\"UDP\",\"%d.%d.%d.%d\",\"%d\"\r", temp_ip[0], temp_ip[1], temp_ip[2], temp_ip[3], temp_port);
			//sprintf(buff, "AT+CIPSTART=\"UDP\",\"%d.%d.%d.%d\",\"%d\"\r", 36, 7, 87, 100, 6345);
		}
		Instruction[i].Inc.assign(buff); //建立udp链接
		Instruction[i].TimeOut = 16000;// 160s 单链接
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 1;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CIPSTART2G_CB;
		i++;
             
		Instruction[i].Inc = "AT+CIPHEAD=1\r";  //接受到的数据加上前缀+IPD
		Instruction[i].TimeOut = 10;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 1;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &NETOPEN_CB; //与4G 的处理方式相同
		i++;
      
		Instruction[i].Inc = "";  //空的指令块用来结束配置流程
		Instruction[i].TimeOut = 0;//5s
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 0;
		Instruction[i].CarryFunc = NULL;
		Instruction[i].WaitFunc = NULL;
		i++; 
		
		//下面的指令不属于配置流程里面的指令
		//独立指令
		CIPSENDIndex = i;//标记下标
		Instruction[i].Inc = "AT+CIPSEND=";//发送数据指令  //AT+CIPSEND=
		Instruction[i].TimeOut = 300;// 如果失败的话645s之后模块会主动上报close
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &CIPSEND_CB1;
		Instruction[i].WaitFunc = &CIPSEND_CB2;
		i++; 

		RebootIndex = i;//标记下标
		Instruction[i].Inc = "Reboot";//发送数据指令  
		Instruction[i].TimeOut = 10;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 0;
		Instruction[i].CarryFunc = &Reboot_CB1;
		Instruction[i].WaitFunc = &Reboot_CB2;
		i++; 

		CIPSHUTIndex = i;//标记下标
		Instruction[i].Inc = "AT+CIPSHUT\r";//关闭链接  
		Instruction[i].TimeOut = 800; 
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 0;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CIPSHUT_CB; //17
		i++; 

		CIPSTATUSIndex = i;//标记下标
		Instruction[i].Inc = "AT+CIPSTATUS\r";//链接  
		Instruction[i].TimeOut = 100; 
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 0;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CIPSTATUS_CB;
		i++;
        //远程升级Air720模块
        UPGRADE_KEY = i;
        Instruction[i].Inc = "AT+UPGRADE=\"KEY\",\"2BOF3tmn1dNeytiuWk6u9vi2UOwoG3md\"\r" ;// 配置远程升级KEY 
        //Instruction[i].Inc = "AT+UPGRADE=\"KEY\",\"njgWpfxCHvxaV6XiD47OaMwGiSF4rOeq\"\r" ;        
        Instruction[i].TimeOut = 10; 
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 0;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &OK_CB;
		i++;
        
        UPGRADE = i;
        Instruction[i].Inc = "AT+UPGRADE\r";// 执行升级
		Instruction[i].TimeOut = 10; 
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 0;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &UPGRADE_CB;
		i++;
        
        VERSION = i;
        Instruction[i].Inc = "AT+VER\r";// 查看版本号
        //Instruction[i].Inc = "ATI\r";// 查看版本号
        //Instruction[i].Inc = "AT+UPGRADE?\r";// 升级错误状态
		Instruction[i].TimeOut = 10; 
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 0;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &VER_CB;
		i++;
    }
#ifdef EC20F_TRACE //用于移植跟踪修改EC20F
	else if(ModelType == EC20F)
	{
		Instruction[i].Inc = "AT\r\n";  //波特率同步  最长48s
		Instruction[i].TimeOut = 200;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 24;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &AT_CB;
		i++;
		Instruction[i].Inc = "ATE0\r\n"; //关闭回显
		Instruction[i].TimeOut = 50;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &OK_CB;
		i++;
		Instruction[i].Inc = "AT+CGMM\r\n"; //查询模块型号
		Instruction[i].TimeOut = 50;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &MODEL_CB;
		i++;
		Instruction[i].Inc = "AT+CPIN?\r\n"; //检查sim卡是否在位
		Instruction[i].TimeOut = 200;  //5s
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 5;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CPIN_CB;
		i++;
        CSQIndex = i;
		Instruction[i].Inc = "AT+CSQ\r\n"; //读取信号强度
		Instruction[i].TimeOut = 50; //300 ms
		Instruction[i].RunNum = 3;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CSQ_CB;
		i++;
		Instruction[i].Inc = "AT+CREG?\r\n"; //检查CS服务状态 90s
		Instruction[i].TimeOut = 200;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 45;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CREG_CB;
		i++;
        
        Instruction[i].Inc = "AT+CGREG=2\r\n";
		Instruction[i].TimeOut = 200;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &OK_CB;
		i++;
        
		Instruction[i].Inc = "AT+CGREG?\r\n"; //检查PS服务状态 60s
		Instruction[i].TimeOut = 200;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 30;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CGREG_CB;
		i++;
        
        Instruction[i].Inc = "AT+CIMI\r\n"; //获取运营商类型
		Instruction[i].TimeOut = 200;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 2;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CIMI_CB;
		i++;
        
		Instruction[i].Inc = "AT+CGATT?\r\n"; //检查GPRS附着状态
		Instruction[i].TimeOut = 500;//最长140s
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 28;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CGATT_CB;
		i++;
		Instruction[i].Inc = "AT+QCCID\r\n"; //获取CCID编号
		Instruction[i].TimeOut = 100;// 1s超时
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 4;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CCID_CB;
		i++;
		//Instruction[i].Inc = "AT+QICSGP=1,1,\"UNINET\",\"\",\"\",1\r"; //配置SGP
		Instruction[i].Inc = "AT+QICSGP=1\r\n"; //配置SGP //默认CMNET
		Instruction[i].TimeOut = 100;	  // 1s超时
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 4;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &OK_CB;
		i++;
		Instruction[i].Inc = "AT+QIDEACT=1\r\n"; //配置DEACT
		Instruction[i].TimeOut = 200;		 // 1s超时
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 20;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &OK_CB;
		i++;
		Instruction[i].Inc = "AT+QIACT=1\r\n"; //配置ACT
		Instruction[i].TimeOut = 200;		  // 1s超时
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 60;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &OK_CB;
		i++;
		CIPSTARTIndex = i;
		//获取服务器的ip 端口
		u8 temp_ip[4];
		u16 temp_port = 0;
		g_CSystem.GetUsingServerIP(temp_ip);
		temp_port = g_CSystem.GetUsingServerPort();
		char buff[50];
		if (firstOn == 0)
			sprintf(buff, "AT+QIOPEN=1,0,\"UDP\",\"%d.%d.%d.%d\",%d,0,1\r\n", 120, 27, 138, 107, 8887);
		else
		{
			u8 temp_ip[4];
			g_CSystem.GetUsingServerIP(temp_ip);
			temp_port = g_CSystem.GetUsingServerPort();
			sprintf(buff, "AT+QIOPEN=1,0,\"UDP\",\"%d.%d.%d.%d\",%d,0,1\r\n", temp_ip[0], temp_ip[1], temp_ip[2], temp_ip[3], temp_port);
		}
		Instruction[i].Inc.assign(buff);		 //建立udp链接
		Instruction[i].TimeOut = 12000;		  // 120s超时
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 1;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &OK_CB;
		i++;
		// Instruction[i].Inc = "AT+QISEND=0,5\r"; //发送数据指令  //AT+CIPSEND=
		// Instruction[i].TimeOut = 300;	  // 如果失败的话645s之后模块会主动上报close
		// Instruction[i].RunNum = 0;
		// Instruction[i].Retry = 3;
		// Instruction[i].CarryFunc = &InsSend_CB;
		// Instruction[i].WaitFunc = &OK_FIX;
		// i++;
		// Instruction[i].Inc = "test\r"; //发送数据指令  //AT+CIPSEND=
		// Instruction[i].TimeOut = 300;		  // 如果失败的话645s之后模块会主动上报close
		// Instruction[i].RunNum = 0;
		// Instruction[i].Retry = 3;
		// Instruction[i].CarryFunc = &InsSend_CB;
		// Instruction[i].WaitFunc = &OK_CB;
		// i++;
		// Instruction[i].Inc = "AT+QISEND=0,0\r"; //发送数据指令  //AT+CIPSEND=
		// Instruction[i].TimeOut = 300;  // 如果失败的话645s之后模块会主动上报close
		// Instruction[i].RunNum = 0;
		// Instruction[i].Retry = 3;
		// Instruction[i].CarryFunc = &InsSend_CB;
		// Instruction[i].WaitFunc = &OK_CB;
		// i++;
		// Instruction[i].Inc = "test\r\n"; //发送数据指令  //AT+CIPSEND=
		// Instruction[i].TimeOut = 300;  // 如果失败的话645s之后模块会主动上报close
		// Instruction[i].RunNum = 0;
		// Instruction[i].Retry = 3;
		// Instruction[i].CarryFunc = &TEST_REBOOT;
		// Instruction[i].WaitFunc = &OK_FIX;
		// i++;
		Instruction[i].Inc = "AT+QISTATE=1,0\r\n"; //获取联网状态
		Instruction[i].TimeOut = 100;			 // 1s超时
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 2;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &NETOPEN_CB;
		i++;
		// Instruction[i].Inc = "AT+QIDEACT=1\r"; //关闭ACT
		// Instruction[i].TimeOut = 200;		   // 2s超时
		// Instruction[i].RunNum = 0;
		// Instruction[i].Retry = 2;
		// Instruction[i].CarryFunc = &InsSend_CB;
		// Instruction[i].WaitFunc = &OK_CB;
		// i++;
		// Instruction[i].Inc = "AT+QICLOSE=0\r"; //关闭连接
		// Instruction[i].TimeOut = 200;			 // 2s超时
		// Instruction[i].RunNum = 0;
		// Instruction[i].Retry = 2;
		// Instruction[i].CarryFunc = &InsSend_CB;
		// Instruction[i].WaitFunc = &QICLOSE_CB;
		// i++;
		Instruction[i].Inc = "";	//空的指令块用来结束配置流程
		Instruction[i].TimeOut = 0; //5s
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 0;
		Instruction[i].CarryFunc = NULL;
		Instruction[i].WaitFunc = NULL;
		i++;
		//下面的指令不属于配置流程里面的指令
		//独立指令
		CIPSENDIndex = i;					//标记下标
		Instruction[i].Inc = "AT+QISEND=0,"; //发送数据指令  //AT+CIPSEND=
		Instruction[i].TimeOut = 30;		// 如果失败的话645s之后模块会主动上报close
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 0;
		Instruction[i].CarryFunc = &CIPSEND_CB1;
		Instruction[i].WaitFunc = &CIPSEND_CB2;
		i++;
	}
#endif    
	//将其它指令块赋初值
	StepMaxIndex = i;
	for (;i < MAXINSNUM;i++)
	{
		Instruction[i].Inc = "";  
		Instruction[i].TimeOut = 0;//5s
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 0;
		Instruction[i].CarryFunc = NULL;
		Instruction[i].WaitFunc = NULL;
	}
}

/*******************************************************************************
 函数名：TIM5_CONFIG
 功能描述：TIM5初始化
 入口参数：无
 出口参数：无
********************************************************************************/
void CGPRS::Timer2Config(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN ;//TIM2时钟使能
	TIM2->ARR=100;
	TIM2->PSC=SystemCoreClock/10000 - 1;//预分频器
	
	//这两个东东要同时设置才可以使用中断
	TIM2->DIER|=1<<0;//允许更新中断
	TIM2->DIER|=1<<6;//允许触发中断
	
	TIM2->CR1|=0x01;//使能定时器2
	
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

/*******************************************************************************
 函 数 名： CGPRSConfig
 功能描述： UART1引脚配置 td1410使能配置 gprs使能配置 打开gprs 检测sim
 入口参数： 无

 出口参数： 0: ok; -1:开启gprs失败; -2：sim卡不在位
********************************************************************************/
void CGPRS::Init(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;
    
    RCC->APB1ENR |= RCC_APB1ENR_UART5EN ;//UART5
	GPIOC->CRH &= 0xFFF0FFFF;//PC12(发送/输出)
	GPIOD->CRL &= 0xFFFFF0FF;//PD2(接收/输入)
	GPIOC->CRH |= GPIO_CRH_MODE12|GPIO_CRH_CNF12_1;//UART5 :PC12->TX; 50MHZ,AF_PP;
	GPIOD->CRL |= GPIO_CRL_CNF2_0;//UART5 : PD2->RX;

	UART5->CR1 = 0;
	UART5->CR1 = 0 << 12 | 1 << 6 | 1 << 5 | 1 << 3 | 1 << 2;
	//0xEA6->9600, 0x0753->19200, 0x0138->115200, 0x27->921600(串口1左边的数要差2倍左右，要重算0x0271->115200)
	UART5->BRR = 0x0138;//115200
	UART5->CR2 = 0 << 14 | 0 << 12 | 0 << 11;
	UART5->CR3 = 0;
	UART5->CR1 |= 1 << 13;//开启中断

	NVIC_InitStructure.NVIC_IRQChannel = UART5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	// 4G 与2G电路板上的pwr引脚不同?
	GPIOD->CRL &= 0XFFFFFFF0;
	GPIOD->CRL |= GPIO_CRL_MODE0;	//通用推挽输出

	GPIOE->CRH &= 0XFFFF0FFF;
	GPIOE->CRH |= GPIO_CRH_MODE11;	//通用推挽输出

	GPIOE->CRH &= 0XFFFFF0FF;
	GPIOE->CRH |= GPIO_CRH_MODE10;	//通用推挽输出
	Timer2Config();	//字节超时中断
	NEO_N720_4GNORST;
    if(g_CSystem.Air_Upflag == 0)
        Init_Reset();//初始化各变量的值
    else if(g_CSystem.Air_Upflag == 1)
        g_CGPRS.UP_Reset();
	g_CPowerCheck->set_ledstate(LED_FREQ_2);//正在联网
}

/*******************************************************************************
 函 数 名： gprs_strength
 功能描述： 获取信号强度
 入口参数： 无
 出口参数： 信号强度0-31 数值越大信号越强 -1:读取错误
********************************************************************************/
int CGPRS::gprs_strength(void)
{
    SetTimer(13,500,this);
	return Strengh;
}

/*******************************************************************************
 函 数 名： UP_Version
 功能描述： 获取4G版本
 入口参数： 无
 出口参数： 版本号XXX -1:读取错误
********************************************************************************/
int CGPRS::UP_Version(void)
{
	return Air_Ver;
}

/*******************************************************************************
 函 数 名： UP_Progress
 功能描述： 获取升级进度
 入口参数： 无
 出口参数： 升级进度 0-100 -1:读取错误
********************************************************************************/
int CGPRS::UP_Progress(void)
{
	return Progress;
}

/*******************************************************************************
 函 数 名： TimerEvent
 功能描述： Timer0:建立连接重试定时器 Timer1:发送数据超时定时器 Timer2:检测gprs接收数据
 入口参数： ulTimeNum 定时器编号
 出口参数： 无
********************************************************************************/
void CGPRS::TimerEvent(u32 ulTimeNum)
{
	switch(ulTimeNum)
	{
		case 0:
		{	
			break;
		}
		case 1:
		{
			break;
		}
		case 3:
		{	//发送注册或心跳
			//GPRS通讯已开启
			if (m_LoginFlag == 0x02 || m_LoginFlag == 0x04)
			{
				//过期或者端口错误
				KillTimer(this, 3);
				break;
			}
			if (m_SendFlag)
			{
				m_SendFlag = 0;
				break;
			}
			//端口切换(需置于发送心跳或注册请求前)
			g_CSystem.m_uiPortChangedCounter--;
			if (g_CSystem.m_uiPortChangedCounter == 0)
			{
				//需要切换端口
				g_CSystem.m_uiPortChangedCounter = 720;
				Reconnect();
				break;
			}
			if (m_LoginFlag & 0x80)
			{	//注册成功发送心跳
			  	if(g_CSystem.m_State & 0x20)
			  		CancelTradeReq();//有撤销交易的先发撤销交易0011
				else
                {
					SendHeartBeat();                     
                }
			}
			else
			{	
				switch(firstOn)
				{
					case 0:
					{
						firstOn = 1;
						SendCCID((u8*)m_CCID);
						break;
					}
					case 1:
					{
						firstOn = 2;
						KillTimer(this, 3);//关闭注册或心跳请求
						Reboot();//重启模块
						break;
					}
					case 2:
					{
						SendRegister();
						break;
					}
					default:
						break;
				}
			}
			break;
		}
		case 4:
		{
            IDisplay m_IDisplay;
            KillTimer(this, 4);
            m_IDisplay.StandBy(10);
			break;
		}
		case 5:
		{
            if(g_CGPRS.GetModelType() == AIR_720 && g_CGPRS.UP_Start_flag == 0 && g_CSystem.Start_flag==1)
            {
                g_CGPRS.Send_Ver_AT();//查询版本
                IBeep m_IBeep;
                m_IBeep.OperateSuccess();//长蜂鸣 
                SetTimer(6,200,this);
            }            
            break;
		}
		case 6:
		{
            KillTimer(this,6);
            int ver = g_CGPRS.UP_Version();           
            if(ver < UPDATE_VER)
            {               
                //g_CSystem.Start_flag = 0;//清空上电自动升级标志//先不清，防止一次没有升级成功
                g_CGPRS.Send_KEY_AT();
                SetTimer(8,200,this);
                IBeep m_IBeep;
                m_IBeep.OperateSuccess();//长蜂鸣 
            } 
            else
            {
                g_CSystem.Start_flag = 0;//清空上电自动升级标志
                UP_Timer = 0;
            }
            KillTimer(this,5);                
           break;             
		}
		case 7:
		{
			if (GPRSState != CONNECTING) //在连接阶段不进行判断
			{
				GPRS_Tick++;
				if (GPRS_Tick >= 60)
				{
					GPRS_Tick = 0;
					//热启动
					ErrorState = ERROR_NORESPOND;
				}
			}
			if (CUdpProtocol::u8ServerUpdate == 1)//如果超时
			{
				CUdpProtocol::u8ServerUpdate = 2;
				g_CGPRS.Reboot();
			}
			else if (CUdpProtocol::u8ServerUpdate == 3)
			{
				CUdpProtocol::u8ServerUpdate = 0;
				
				u8 temp_ip[4];
				u16 temp_port;
				g_CSystem.GetUsingServerIP(temp_ip);// 获取当前使用的服务器IP
				temp_port = g_CSystem.GetUsingServerPort();// 获取当前使用的服务器端口
				g_CSystem.ModifyServerIP(temp_ip);// 将获取到的IP更新为默认IP
				g_CSystem.ModifyPort(temp_port);// 将获取到的端口更新为默认端口
			}
			break;
		}
		case 8:
		{
            KillTimer(this,8);
            g_CGPRS.UP_Start_flag = 1;
            g_CGPRS.Send_UP_AT(); 
            IBeep m_IBeep;
            m_IBeep.OperateSuccess();//长蜂鸣
			break;
		}
		case 9:
		{
			PowerOff_WaitCB();//
			KillTimer(this, 9);	//
			break;
		}
		case 10:
		{
			CUdpProtocol::TimerEvent(10);
			break;
		}
		case 11:
		{
			//计时器
			static int Dida = 0;
			if (Dida < 60000)//600s
			{
				Dida++;
			}
			else 
			{
				Dida = 0;
			}
            if (Instruction[Step].Inc != "" && Step < StepMaxIndex )
            {
                if ( IncState == CARRY_OUT)
                {
                    //执行态回调函数
                    IncState = WAIT_RESPOND;
                    Instruction[Step].RunNum++;//执行次数加1
                    Dida = 0;//开始计时
                    if (Instruction[Step].CarryFunc(&Instruction[Step].Inc) == SENDFAIL)
                    {
                        //指令执行失败或者数据未发送成功
                        if (Instruction[Step].RunNum <= Instruction[Step].Retry)
                        {
                            //进行重试
                            IncState = CARRY_OUT;
							if((Step == CIPSENDIndex)&&(ModelType == EC20F))
							{
								ComSend();
							}
							SetTimer(11, 100, this);// 1s之后重新发送
                        }
                        else
                        {
                            //重试的次数超出限定次数
                            IncState = CARRY_OUT;
                            Instruction[Step].RunNum = 0;
                            if( (Step == UPGRADE_KEY || Step == UPGRADE ||Step == VERSION)&& ModelType == AIR_720)
                            {
                                KillTimer(this,11);
                                Step = StepMaxIndex;//释放块 
                            }
                            else
                                ExceptionHandle(&ErrorState);
                        }	
                    }

                }
                else
                {
                    u8 ret = 0;
                    ret = Instruction[Step].WaitFunc();
                    if (ret == RCVWAITING)
                    {
                        //指令还没有返回信息
                        if  (Dida < Instruction[Step].TimeOut)
                        {
                            ;//指令没有超时继续等待
                        }
                        else
                        {
                            //指令等待超时了判断是否重试
                            if (Instruction[Step].RunNum <= Instruction[Step].Retry)
                            {
                                //进行重试
                                IncState = CARRY_OUT;
                                SetTimer(11, 100, this);// 1s之后重新发送
                            }
                            else
                            {
                                //重试的次数超出限定次数
                                IncState = CARRY_OUT;
                                Instruction[Step].RunNum = 0;
                                if((Step == UPGRADE_KEY || Step == UPGRADE ||Step == VERSION)&&ModelType == AIR_720)
                                {
                                    KillTimer(this,11);
                                    Step = StepMaxIndex;//释放块                                   
                                }
                                else
                                    ExceptionHandle(&ErrorState);
                            }								
                        }
					}
					else if (ret == RCVRIGHT)
					{
						//指令收到了合法的返回
						IncState = CARRY_OUT;
						Instruction[Step].RunNum = 0;
						if (Step < MAXINSNUM && GPRSState == CONNECTING)
						{
							//连接未建立说明是配置指令
							//配置指令需要接着执行下一块
							Step++;	
						}
						else
						{
							//如果在连接状况好的情况下
							//执行的指令都是执行单块
							//不继续执行下一块
							Step = StepMaxIndex;//没有指令在占用块了
							KillTimer(this, 11);
						}
					}
					else
					{
						//指令收到了非法的返回
						if (Instruction[Step].RunNum <= Instruction[Step].Retry)
						{
							//进行重试
							IncState = CARRY_OUT;
							SetTimer(11, 100, this);// 1s之后重新发送
						}
						else
						{
							//重试的次数超出限定次数
							IncState = CARRY_OUT;
							Instruction[Step].RunNum = 0;
                             if((Step == UPGRADE_KEY || Step == UPGRADE ||Step == VERSION)&& ModelType == AIR_720)
                            {
                                KillTimer(this,11);
                                Step = StepMaxIndex;//释放块 
                            }
                            else
                                ExceptionHandle(&ErrorState);
                        }	
                        
                    }
                }
            }
            else
            {
                //没有指令可以执行了
                Step = StepMaxIndex;//释放块
                KillTimer(this, 11);
            }
            break;
		}
		case 12:
		{   
            if(g_CSystem.Air_Upflag != 1 && UP_Start_flag != 1)
                ExceptionHandle_Unsolicite(&ErrorState); //定时检测GPRS 有没有出错
			break;
		}
		case 13:
		{
            if(g_CSystem.Start_flag == 1)
            {
                if( ++UP_Timer > 80)//8秒加一次,11分钟检测
                {
                    SetTimer(5,1500,this);
                    UP_Timer = 0;
                }                
            }
			CarryBlock(CSQIndex, 1);
            KillTimer(this,13);
			break;
		}
		case 14:
		{
			//只有N720模块情况下，次代码才有效
			if (ModelType == NEO_N720)
				CarryBlock(NetReadIndex, 1);
			else if (ModelType == SIM800C)
				CarryBlock(CIPSTATUSIndex, 1);
			else if (ModelType == AIR_720)
				CarryBlock(CIPSTATUSIndex, 1);
			break;
		}
		case 15:
		{
			KillTimer(this, 15);
			CarryBlock(CIPSTARTIndex-1, 1);//从关闭连接开始
			GPRSState = CONNECTING;
            break;
		}
        case 16:
        {
            CUdpProtocol::TimerEvent(16);
            break;
        }
		default:
			break;
	}
}

/**************************************************************************
函 数 名：RCV_DataProcess
功能描述：接收完一帧数据后的处理
入口参数：无
出口参数：无
**************************************************************************/
void CGPRS::RCV_DataProcess(void)
{
	//此处辨别串口收到的是报文，还是异常，还是at
	if (GPRSState == ALREADYCONNECTED || g_CSystem.Air_Upflag == 1)
	{
		if (ModelType == NEO_N720)
		{
			//只有在连接后才有可能收到报文
			u8 TempBuff[RCV_MAX] = {0};
			char Mark[]="$MYURCREAD:";//如果有该子串则为报文数据
			char *pMsg = NULL;
			memcpy(TempBuff, ComRcv.Data, ComRcv.Len);
			pMsg = strstr((char*)TempBuff, Mark);
			if (pMsg != NULL)
			{
				//将数据递交给上层将com数据清空
				memset(ComRcv.Data, 0, ComRcv.Len); //读取完毕清空串口缓冲区内容
				ComRcv.Len = 0;
				ComRequest= FALSE;//AT指令则不响应
				if (CarryBlock(NetReadIndex, 1) != 0)//模块主动去读取收到的数据
				{
					//提取指令失败 这是因为（ok 与MYURCREAD 粘接，仍在处于判断发送返回ok过程中）
					Step = StepMaxIndex;
					CarryBlock(NetReadIndex, 1);//强制提取
				}
			}
			else 
			{
				//如果有主动上报的异常在这里处理
				if (UnsolicitedHandle(pMsg) == 0)
				{
					ComRequest= TRUE;//当作at返回进行响应
				}
			}	
		}
		else if (ModelType == SIM800C)
		{
			//只有在连接后才有可能收到报文
			u8 TempBuff[RCV_MAX] = {0};
			char Mark[]="+IPD";//如果有该子串则为报文数据
			char *pMsg = NULL;
			memcpy(TempBuff, ComRcv.Data, ComRcv.Len);
			pMsg = strstr((char*)TempBuff, Mark);
			if (pMsg != NULL)
			{
				//报文数据获取报文长度
				char *pStart = NULL;
				char *pEnd = NULL;
				char LenChar[5] = {0};//rcv_max 最大160
				u16 LenHex = 0;
				pStart = strchr(pMsg, ',');
				pEnd = strchr(pMsg, ':');
				if ((pEnd-pStart-1) < 4)//数据长度3位数
				{
					memcpy(LenChar, pStart+1, pEnd-pStart-1);//获取数据长度
					LenHex = atoi(LenChar);
					if (LenHex < RCV_MAX_LEN)
					{
						memcpy(m_ReceiveFrame.Data, pEnd+1, LenHex);//偏移修改
						m_ReceiveFrame.Len = LenHex;
						m_Request = TRUE;//通知上层进行处理
						ComRequest= FALSE;//AT指令则不响应
					}
				}
				//将数据递交给上层将com数据清空
				memset(ComRcv.Data, 0, ComRcv.Len); //读取完毕清空串口缓冲区内容
				ComRcv.Len = 0;
			}
			else
			{
				//如果有主动上报的异常在这里处理
				if (UnsolicitedHandle(pMsg) == 0)
				{
					ComRequest= TRUE;//当作at返回进行响应
				}
			}	
		}
		else if (ModelType == AIR_720)
		{
			//只有在连接后才有可能收到报文
			u8 TempBuff[RCV_MAX] = {0};
			char Mark[]= "+IPD";//如果有该子串则为报文数据
            char Install[] = "+UPGRADEDL";//如果有该子串则为升级进度
            char Header[] = "+UPGRADE";//如果有该子串则为升级进度
            char New_Header[] = "+UPGRADEIND";//如果有该子串则为升级进度
            char New_UP_error[] = "+UPGRADEIND: -";//
            //char UP_error[] = "+CME ERROR:";//升级错误
            char UP_Ready[] = "RDY";//升级完成后模块自启完成
			char *pMsg = NULL;
            char *UP_pMsg = NULL;
            char *New_UP_pMsg = NULL;
//            char *UP_E_pMsg = NULL;
            char *NEW_UP_E = NULL;
            char *UP_RDY = NULL;
            char *INSTALL = NULL;
			memcpy(TempBuff, ComRcv.Data, ComRcv.Len);
			if( (pMsg = strstr((char*)TempBuff, Mark))!=NULL )
               ;
            else if( (NEW_UP_E = strstr((char*)TempBuff,New_UP_error))!= NULL)
                ; 
            else if( (INSTALL = strstr((char*)TempBuff,Install))!= NULL)
                ; 
            else if( (New_UP_pMsg = strstr((char*)TempBuff,New_Header))!=NULL)
                ;
            else if( (UP_pMsg = strstr((char*)TempBuff,Header))!=NULL)
                ;            
            //else if( (UP_E_pMsg = strstr((char*)TempBuff,UP_error))!=NULL)
                //;  
            else if( (UP_RDY = strstr((char*)TempBuff,UP_Ready))!=NULL)
                ;
			if (pMsg != NULL)
			{
				//报文数据获取报文长度
				char *pStart = NULL;
				char *pEnd = NULL;
				char LenChar[5] = {0};//rcv_max 最大160
				u16 LenHex = 0;
				pStart = strchr(pMsg, ',');
				pEnd = strchr(pMsg, ':');
				if ((pEnd-pStart-1) < 4)//数据长度3位数
				{
					memcpy(LenChar, pStart+1, pEnd-pStart-1);//获取数据长度
					LenHex = atoi(LenChar);
					if (LenHex < RCV_MAX_LEN)
					{
						memcpy(m_ReceiveFrame.Data, pEnd+1, LenHex);//偏移修改
						m_ReceiveFrame.Len = LenHex;
						m_Request = TRUE;//通知上层进行处理
						ComRequest= FALSE;//AT指令则不响应
					}
				}
                if(g_CSystem.Air_Upflag == 1)
                {
                    UP_Start_flag = 0;
                    g_CSystem.WriteUpflag(0);//写升级标志
                    g_CSystem.Air_Upflag = 0;
                }
				//将数据递交给上层将com数据清空
				memset(ComRcv.Data, 0, ComRcv.Len); //读取完毕清空串口缓冲区内容
				ComRcv.Len = 0;
			}
            else if(NEW_UP_E != NULL)
            {
                if(UP_Start_flag==1)
                {
                    IDisplay temp_IDisplay;               
                    temp_IDisplay.Dis_ErrCode(10, ERR_Air720_UP);
                    UP_Start_flag = 0;
                    ComRequest= TRUE; 
                    if(g_CSystem.Start_flag == 1)
                    {
                        SetTimer(5, 1500, this);
                        UP_Timer = 0;
                    }                      
                    SetTimer(4, 300, this);//错误码显示5秒
                }
                memset(ComRcv.Data, 0, ComRcv.Len); //读取完毕清空串口缓冲区内容
                ComRcv.Len = 0;
            }
            else if(INSTALL != NULL && g_CSystem.Air_Upflag==1)
            {
                IBeep m_IBeep;
                m_IBeep.OperateSuccess();//长蜂鸣 
                char *pStart = NULL;
				char *pEnd = NULL;
                char LenChar[5] = {'\0'};
                u16 progress = 0;
                pStart = strchr(INSTALL, ':');
                pEnd = strrchr(INSTALL,'0');                               
                if(pEnd!=NULL && pStart!=NULL && pEnd - pStart < 5)
                {
                    memcpy(LenChar, pStart+2, pEnd-pStart-1);//获取升级进度
                    progress = atoi(LenChar);
                    Progress = progress;    
                    m_IBeep.OperateSuccess();//长蜂鸣
                    if(Progress == 100)//升级完成
                    {                     
                      m_IBeep.OperateSuccess();//长蜂鸣
                      g_CSystem.Air_Upflag = 0;
                      g_CSystem.WriteUpflag(0);//写升级标志
                      UP_Start_flag = 0;
                      Reboot();  
                    }
                }
                ComRequest= TRUE;
                memset(ComRcv.Data, 0, ComRcv.Len); //读取完毕清空串口缓冲区内容
				ComRcv.Len = 0;
            }
            else if(New_UP_pMsg != NULL)
            {
                //升级进度
                char *pStart = NULL;
				char *pEnd = NULL;
                char LenChar[5] = {'\0'};
                u16 progress = 0;
                pStart = strchr(New_UP_pMsg, ':');
                pEnd = strrchr(New_UP_pMsg,'0');                               
                if(pEnd!=NULL && pStart!=NULL && pEnd - pStart < 5)
                {
                    IBeep m_IBeep;
                    memcpy(LenChar, pStart+2, pEnd-pStart-1);//获取升级进度
                    progress = atoi(LenChar);
                    Progress = progress;    
                    m_IBeep.OperateSuccess();//长蜂鸣
                    if(Progress == 100)//升级完成
                    {                     
                      m_IBeep.OperateSuccess();//长蜂鸣
                      UP_Finsh = 1; 
                      g_CSystem.Air_Upflag = 1;
                      g_CSystem.WriteUpflag(1);//写升级标志
                      UP_Start_flag = 0;
                      UP_Reset();   
                    }
                }
                ComRequest= TRUE;
                memset(ComRcv.Data, 0, ComRcv.Len); //读取完毕清空串口缓冲区内容
				ComRcv.Len = 0;
            }
            else if(UP_pMsg != NULL)
            {
                //升级进度
                char *pStart = NULL;
				char *pEnd = NULL;
                char LenChar[5] = {'\0'};
                u16 progress = 0;
                pStart = strchr(UP_pMsg, ':');
                pEnd = strrchr(UP_pMsg,'0');
                if(pEnd!=NULL && pStart!=NULL && pEnd - pStart < 5)
                {
                    memcpy(LenChar, pStart+2, pEnd-pStart-1);//获取升级进度
                    progress = atoi(LenChar);
                    Progress = progress;     
                    IBeep m_IBeep;
                    m_IBeep.OperateSuccess();//长蜂鸣
                    if(Progress == 100)//升级完成
                    {                                              
                        m_IBeep.OperateSuccess();//长蜂鸣                                                
                        UP_Finsh = 1;
                        g_CSystem.Air_Upflag = 1;
                        g_CSystem.WriteUpflag(1);//写升级标志
                        UP_Start_flag = 0;
                        UP_Reset();
                    }
                }
                ComRequest= TRUE;
                memset(ComRcv.Data, 0, ComRcv.Len); //读取完毕清空串口缓冲区内容
				ComRcv.Len = 0;
            }            
            /*else if(UP_E_pMsg != NULL )//升级错误
            {
                IDisplay temp_IDisplay;
                temp_IDisplay.Dis_ErrCode(10, ERR_Air720_UP);
                UP_Start_flag = 0;           
                ComRequest= TRUE;
                memset(ComRcv.Data, 0, ComRcv.Len); //读取完毕清空串口缓冲区内容
				ComRcv.Len = 0;
                //SetTimer(4, 300, this);//错误码显示5秒
            }*/
            else if( UP_RDY != NULL)
            {
                memset(ComRcv.Data, 0, ComRcv.Len); //读取完毕清空串口缓冲区内容               
				ComRcv.Len = 0;
                if(g_CSystem.Air_Upflag == 1)
                {
                    UP_Start_flag = 0;
                    g_CSystem.WriteUpflag(0);//写升级标志
                    g_CSystem.Air_Upflag = 0;
                    Reboot();
                }
            }
			else
			{
				//如果有主动上报的异常在这里处理
				if (UnsolicitedHandle(pMsg) == 0)
				{
					ComRequest= TRUE;//当作at返回进行响应
				}
                if( g_CSystem.Air_Upflag == 1)
                {
                    memset(ComRcv.Data, 0, ComRcv.Len); //读取完毕清空串口缓冲区内容               
                    ComRcv.Len = 0;
                }
			}
		}
#ifdef EC20F_TRACE //用于移植跟踪修改EC20F
		else if (ModelType == EC20F)
		{
			//只有在连接后才有可能收到报文
			u8 TempBuff[RCV_MAX] = {0};
			char Mark[] = "+QIURC:"; //如果有该子串则为报文数据
			char *pMsg = NULL;
			memcpy(TempBuff, ComRcv.Data, ComRcv.Len);
			pMsg = strstr((char *)TempBuff, Mark);
			if (pMsg != NULL)
			{
				//报文数据获取报文长度
				char *pStart = NULL;
				char *pEnd = NULL;
				char LenChar[5] = {0}; //rcv_max 最大160
				u16 LenHex = 0;
				pStart = strchr(pMsg, ',')+2;
				pEnd = strchr(pMsg, '\r');
//                Dprintf(FALSE,"ERROR:","step 0:%d\r\n",pEnd - pStart - 1); 
				if ((pEnd - pStart - 1) < 4 && (pEnd - pStart - 1)>=0) //数据长度3位数
				{
					memcpy(LenChar, pStart + 1, pEnd - pStart - 1); //获取数据长度
					LenHex = atoi(LenChar);
					if (LenHex < RCV_MAX_LEN)
					{
						memcpy(m_ReceiveFrame.Data, pEnd + 2, LenHex); //偏移修改
						m_ReceiveFrame.Len = LenHex;
						m_Request = TRUE;   //通知上层进行处理
						ComRequest = FALSE; //AT指令则不响应
					}
				}
				//将数据递交给上层将com数据清空
				memset(ComRcv.Data, 0, ComRcv.Len); //读取完毕清空串口缓冲区内容
				ComRcv.Len = 0;
			}
			else
			{
				//如果有主动上报的异常在这里处理
				if (UnsolicitedHandle(pMsg) == 0)
				{
					ComRequest= TRUE;//当作at返回进行响应
				}
			}
        }
#endif        
	}
	else
	{
		ComRequest = TRUE;//未连接的时候所有指令当作at返回
	}
}

/**************************************************************************
函 数 名：SetTimeout_Ms
功能描述：收取信息超时时间设定
入口参数：超时时间 最大值1864ms
出口参数：无
**************************************************************************/
void CGPRS::SetTimeout_Ms(u16 nms)
{
	//systick用的是系统时钟的1/8
	//系统时钟是72M，则systick是9M
	//则1s可计数9M，1us计数9次load 是24bit计数器
	//load 最大值是0xff ffff 则最大可以计数0xff ffff/9 = 1864 135us
	SysTick->LOAD=nms*9000; //时间加载
	SysTick->VAL=0x00;        //清空计数器
	SysTick->CTRL=0x01 ;      //开始倒数
}

/**************************************************************************
函 数 名：IfTimeout
功能描述：是否达到超时时间
入口参数：无
出口参数：无
**************************************************************************/
u8 CGPRS::IfTimeout(void)
{
	u32 temp;
	temp=SysTick->CTRL;
	if (temp&0x01&&!(temp&(1<<16)))
	{
		return 1;//计数未完成
	}
	else
	{
		//CTRL bit16 为1表示计数完成
		SysTick->CTRL=0x00;       //关闭计数器
		SysTick->VAL =0X00;       //清空计数器
		return 0;
	}
}


void CGPRS::ResetHeartBeatTimer(void)
{
	SetTimer(3, 1000, this);//心跳/注册帧定时器(20秒的一半)
}

//虚函数，GPRS继承的此函数为空
void CGPRS::TimeoutCountReset(void)
{
	
}

void CGPRS::ComSend(void)
{
	SendData(m_SendFrame.Len, m_SendFrame.Data);
}

void CGPRS::powoff_ComSend(void)
{
    powoff_SendData(m_SendFrame.Len, m_SendFrame.Data);
}

//直接发送已经封好包的数据
u8 CGPRS::SendData(u16 len, u8 *pData)
{
	//if (m_LoginFlag == 0xff)
		//return 4; //注册失败不发送
	if (GPRSState == ALREADYCONNECTED)
	{	
		if (CarryBlock(CIPSENDIndex, 8))//非模块通信指令可抢占
		{         
			//块被占用没法发送
			return 4;
		}
		std::ostringstream s;
#ifdef EC20F_TRACE //用于移植跟踪修改EC20F
		if(ModelType == EC20F)
		{
			s <<"\r\n"<< Instruction[CIPSENDIndex].Inc<< len<<"\r\n";
		}
		else
		{
			s <<"\r\n"<< Instruction[CIPSENDIndex].Inc<< len<<"\r";
		}
#endif        
//		s <<"\r\n"<< Instruction[CIPSENDIndex].Inc<< len<<"\r";
		std::string Inc(s.str());
		if (Write(&Inc) == 0)
		{
			while(IFComBuffEmpty())//等待cipsend=x 指令发送完成
			{;}
			if (SetComBuff(pData, len) == 0)
			{
				return 0;
			}
			else
			{
				return 1; 
			}
		}
		else
		{
			return 2;
		}
	}
	else
	{
		return 3;
	}
}

//掉电发送数据
u8 CGPRS::powoff_SendData(u16 len, u8 *pData)
{
    if (GPRSState == ALREADYCONNECTED)
    {      
        KillTimer(this);//关闭GPRS的定时器任务
        Step = CIPSENDIndex;//下标指向发送命令
        memset(ComRcv.Data, 0, ComRcv.Len); //清空串口缓冲区内容
        std::ostringstream s;
        if(ModelType == EC20F)
		{
			s <<"\r\n"<< Instruction[CIPSENDIndex].Inc<< len<<"\r\n";
		}
        else
        {
            s <<"\r\n"<< Instruction[CIPSENDIndex].Inc<< len<<"\r";
        }
		std::string Inc(s.str());
        ComSnd.HaveSndLen = 0;
        if (Write(&Inc) == 0)
		{
			while(IFComBuffEmpty())//等待cipsend=x 指令发送完成
			{;}
            memset(ComSnd.Data, 0, SND_MAX); //清空串口发送缓冲区内容
			ComSnd.Len = len;
            memcpy(ComSnd.Data, pData, len);//拷贝发送数据到串口发送缓存
            g_CSystem.delay_nms(5);//延时等待模块的">"
			ComBuffOut();//发送
            Step = StepMaxIndex;//没有指令在占用块了;
            return 0;
		}
        else
        {
            return 2;
        }
    }
    else
	{
		return 3;
	}
}

BOOL CGPRS::ReceiveCheck(RECEIVE_FRAME *frame)
{
	BOOL ret = CUdpProtocol::ReceiveCheck(frame);
	if (ret)
	{
		GPRS_Tick = 0;
	}
	return ret;
}

extern "C"
{
	/**************************************************************************
	函 数 名：USART3_IRQHandler
	功能描述：串行中断服务程序
	入口参数：无
	出口参数：无
	**************************************************************************/
	void UART5_IRQHandler(void)
	{
		while(1)
		{
			if (UART5->SR & (1 << 5))	//接收中断
			{
				UART5->SR &= ~(1 << 5);
				if (0 == g_CGPRS.ComRcv.Len)
					TIM2->CR1|=0x01;//使能定时器2
				else
					TIM2->CNT = 0;//重装初值
				if (g_CGPRS.ComRcv.Len < RCV_MAX)//有最大值
				{
					g_CGPRS.ComRcv.Data[g_CGPRS.ComRcv.Len++] = UART5->DR;
				}
				else
				{
					TIM2->CNT = 0;//重装初值
				}
			}
			else if (UART5->SR & (1 << 6))	//发送中断
			{
				UART5->SR &= ~(1 << 6);
				if (g_CGPRS.ComSnd.HaveSndLen < g_CGPRS.ComSnd.Len)
				{
					UART5->DR = g_CGPRS.ComSnd.Data[g_CGPRS.ComSnd.HaveSndLen++];
				}
				else
				{
					g_CGPRS.ComSnd.HaveSndLen= 0;
				}
			}
			else
			{	//此时为异常(注：还不能解决全部无限中断的情况)//ksh_test
				//UART5->SR = 0x80;
				//UART5->DR &= (u16)0x01ff;
				break;
			}
		}

	}

	void TIM2_IRQHandler(void)
	{
		if (TIM2->SR & 0x0001)//溢出中断
		{	
			TIM2->CR1 &= 0xfffe;//禁能定时器2
			//字节超时
			if(FALSE == bTimerFirstArrive)
			{
				g_CGPRS.RCV_DataProcess();
				TIM2->CNT = 0;//重装初值
			}
			else
			{
				bTimerFirstArrive = FALSE;//第一次上电进的中断不是超时而是自动进入的
				TIM2->CNT = 0;//重装初值
			}
		}
		TIM2->SR &= ~(1 << 0);//清除中断标志位
	}
	
}
u8 CGPRS::Write(string *str)
{
	u16 templen = 0;
	templen = str->length();
	if (templen < SND_MAX)
	{	
		if (IFComBuffEmpty() == 0)
		{
			ComSnd.Len = templen;
			strcpy((char*)ComSnd.Data, str->c_str());
			ComBuffOut();
			return 0;
		}
		else
		{
			return 2;
		}
	}
	else
	{
		return 1;//长度不合法
	}
}

//将串口缓冲区的数据发送出去
void CGPRS::ComBuffOut(void)
{
	ComSnd.HaveSndLen = 1;
	UART5->DR = ComSnd.Data[0];	
}

//0 串口缓冲没有被占用可以发送 1正在使用
u8 CGPRS::IFComBuffEmpty(void)
{
	if (ComSnd.HaveSndLen == 0)
		return 0;
	else
		return 1;	
}

u8 CGPRS::SetComBuff(u8* pData, u16 Len)
{
	if (Len < SND_MAX && pData != NULL)
	{
		ComSnd.Len = Len;
		memcpy(ComSnd.Data, pData, Len);
		return 0;
	}
	else
	{
		return 1;
	}
}

void CGPRS::Read(string *str)
{
	ComRequest = FALSE;
	str->assign((char*)ComRcv.Data, 0, ComRcv.Len);
	memset(ComRcv.Data, 0, ComRcv.Len); //读取完毕清空串口缓冲区内容
	ComRcv.Len = 0;
}

void CGPRS::Read(char *strchar)
{
	ComRequest = FALSE;
	memcpy(strchar,ComRcv.Data, ComRcv.Len);
	memset(ComRcv.Data, 0, ComRcv.Len); //读取完毕清空串口缓冲区内容
	ComRcv.Len = 0;
}

void CGPRS::Reboot(void)
{
    if(g_CGPRS.UP_Start_flag == 0)
        Init_Reset();
}

void CGPRS::ExceptionHandle(u8 *ErrorCode)
{
	//在块执行错误后都会进入此函数
	//表明块执行完成?
	Step = StepMaxIndex;
	switch(*ErrorCode)
	{
		case ERROR_AT:
		case ERROR_OK:
		case ERROR_CCID:
		case ERROR_MODEL:
		{
			//同步波特率失败 或者是ccid失败 则认为是2G 模块
			if(ModeSwitchLock==0)   //如果模式锁未锁，若读取CCID不成功，则切换尝试
	        {
#ifdef EC20F_TRACE //用于移植跟踪修改EC20F
				if(ModelType == AIR_720)
	                ModelType = EC20F;
				else if (ModelType == EC20F)
					ModelType = NEO_N720;
				else if(ModelType == NEO_N720)
	                ModelType = SIM800C;
				else
					ModelType = AIR_720;
#endif
	        }
			Reboot();
			break;
		}
		case ERROR_ATE0: //回显 
		case ERROR_CPIN: //pin码设定 
		case ERROR_CREG://注册状态	
		case ERROR_CGATT://附着状态
			m_GprsWrong = ERROR_MODLE; //之前的错误都说明模块或卡有问题//触发服务器ip切换
		case ERROR_CSTT://设置apn
		case ERROR_CIICR://建立链路	
		case ERROR_CIFSR	://获取ip
		case ERROR_CIPSHUT: //关闭链接
		case ERROR_CIPHEAD://报文加头
		case ERROR_NORMALSEND://指令发送
		case ERROR_CIPSTART://打开链接
		{
			Reboot();
		}
		case ERROR_CIPSEND://发送数据
		case ERROR_NOARROW://收不到"<"暂不处理
		case ERROR_CSQ://信号强度暂且不处理
        break;
		default:
			break;
	}
	*ErrorCode = 0;
}

//模块主动上报的错误响应
//中断会检测到模块主动上报的错误，但是中断
//中不能调用settimer之类的操作因此需要轮询
void CGPRS::ExceptionHandle_Unsolicite(u8 *ErrorCode)
{
	if (*ErrorCode == 0)
	{
		return;
	}
	switch(*ErrorCode)
	{
		case ERROR_DEACT://上报断线
		case ERROR_NORESPOND:
		{
			//Model_Reconnect();
			//*ErrorCode = 0;
			Reboot();
			*ErrorCode = 0;
			break;
		}
		default:
			break;
	}
}

u8 CGPRS::UnsolicitedHandle(char* pData)
{
	if (ModelType == NEO_N720)
	{	
		char Mark[]="$MYURCCLOSE";//连接断开了
		if (strstr(pData, Mark) != NULL)	
		{
			ErrorState = ERROR_DEACT;
			return 1;
		}
	}
	else if (ModelType == SIM800C)
	{
		char Mark[]="+PDP DEACT";//连接断开了
		if (strstr(pData, Mark) != NULL)	
		{
			ErrorState = ERROR_DEACT;
			return 1;
		}
	}
	else if (ModelType == AIR_720)
	{
		char Mark[]="+PDP DEACT";//连接断开了
		if (strstr(pData, Mark) != NULL)	
		{
			ErrorState = ERROR_DEACT;
			return 1;
		}
	}
	return 0;//没有已知异常
}

//立马执行某个命令块
//StepNum :块号 StartTime启动时间 OverTime 超时
u8 CGPRS::CarryBlock(u8 StepNum, u8 StartTime)
{
	if (Step == StepMaxIndex)
	{
		//块没有被占用
		Step = StepNum;
		SetTimer(11, StartTime, this);
		IncState = CARRY_OUT;
		return 0;
	}
	else
	{
		return 1;
	}
	
}
//抢占式执行pre = 1
//用于心跳/参数回复
//可以抢占定时器11的发送数据
/*u8 CGPRS::CarryBlock(u8 StepNum, u8 StartTime, u8 Pre)
{	
	if (Pre == 1)
	{
		Step = StepNum; //抢占式
		SetTimer(11, StartTime, this);
		IncState = CARRY_OUT;
		return 0;	
	}
}*/
u8 CGPRS::PowerOn_CarryCB(string *Inc)
{
	if (ModelType == NEO_N720)
		NEO_N720_4GPWR_L; //模块断电
	else if (ModelType == SIM800C)
		SIM800C_2GPWR_L;
	else if (ModelType == AIR_720)
		AIR_720_4GPWR_L;
#ifdef EC20F_TRACE //用于移植跟踪修改EC20F
	else if (ModelType == EC20F)
		EC20F_4GPWR_L;
#endif
	pthis->SetTimer(11, 500, pthis);
	return SENDOK;
}
////////
u8 CGPRS::PowerSoftOff_WaitCB(void)
{
	pthis->SetTimer(9, 1000, pthis);
	return SENDOK;
}

u8 CGPRS::PowerOn_WaitCB(void)
{
	if (ModelType == NEO_N720)
		NEO_N720_4GPWR_H;//模块上电
	else if (ModelType == SIM800C)
		SIM800C_2GPWR_H;
	else if (ModelType == AIR_720)
		AIR_720_4GPWR_H;
#ifdef EC20F_TRACE //用于移植跟踪修改EC20F
	else if (ModelType == EC20F)
		EC20F_4GPWR_H;
#endif
	pthis->SetTimer(11, 200, pthis);
	return RCVRIGHT;
}

u8 CGPRS::PowerOff_CarryCB(string *Inc)
{
	if (ModelType == NEO_N720)
		GPRS_MODEL_ON_L;  //发起关机
	else if (ModelType == SIM800C)
		GPRS_MODEL_ON_H;
	else if (ModelType == AIR_720)
		GPRS_MODEL_ON_L;
	pthis->SetTimer(11, 100, pthis);
	return SENDOK;
}

u8 CGPRS::PowerOff_WaitCB(void)
{
	if (ModelType == NEO_N720)
		GPRS_MODEL_ON_H;
	else if (ModelType == SIM800C)
		GPRS_MODEL_ON_L;
	else if (ModelType == AIR_720)
		GPRS_MODEL_ON_H;
	g_CSystem.delay_nms(10);
	if (ModelType == NEO_N720)
		NEO_N720_4GPWR_L; //模块断电
	else if (ModelType == SIM800C)
		SIM800C_2GPWR_L;
	else if (ModelType == AIR_720)
		AIR_720_4GPWR_L;
	pthis->KillTimer(pthis);
	pthis->Init_Reset();
	return RCVRIGHT;
}

 u8 CGPRS::ModelOn_CarryCB(string *Inc)
{
	if (ModelType == NEO_N720)
    {
		GPRS_MODEL_ON_L;//发送启动信号
        pthis->SetTimer(11, 150, pthis);//启动信号最少持续200	  
    }
	else if(ModelType == SIM800C)
    {
		GPRS_MODEL_ON_H;// 2G 的电路设计不同
        pthis->SetTimer(11, 150, pthis);//启动信号最少持续200	  
    }
	else if(ModelType == AIR_720)
    {
		GPRS_MODEL_ON_L;
        pthis->SetTimer(11, 300, pthis);//启动信号1~1.5s//2018 125->300
    }
#ifdef EC20F_TRACE //用于移植跟踪修改EC20F
	else if (ModelType == EC20F)
	{
		GPRS_MODEL_ON_L;
		pthis->SetTimer(11, 125, pthis); //启动信号1~1.5s
	}
#endif
	return SENDOK;
}
 
u8 CGPRS::ModelOn_WaitCB(void)
{
	if (ModelType == NEO_N720)
		GPRS_MODEL_ON_H;//结束启动信号
	else if(ModelType == SIM800C)
		GPRS_MODEL_ON_L;
	else if(ModelType == AIR_720)
		GPRS_MODEL_ON_H;
#ifdef EC20F_TRACE //用于移植跟踪修改EC20F
	else if (ModelType == EC20F)
	{
		GPRS_MODEL_ON_H;//加快启动速度
		pthis->SetTimer(11, 100, pthis); //模块启动10s后再发指令
		return RCVRIGHT;
	}
#endif
	pthis->SetTimer(11, 1800, pthis);//模块启动18s后再发指令
	return RCVRIGHT;
}

u8 CGPRS::InsSend_CB(string *Inc)
{
	if (pthis->Write(Inc) == 0)
	{
		pthis->SetTimer(11, 1, pthis);//每10ms检查是否有数据返回
		return SENDOK;
	}
	else
	{
		pthis->ErrorState = ERROR_NORMALSEND;
		return SENDFAIL;//发送失败
	}

}
#ifdef EC20F_TRACE
u8 CGPRS::TEST_REBOOT(string *Inc)
{
	static u8 fg = 1;
	if (fg)
	{
		fg = 0;
		//pthis->Reboot();
	}
	return SENDOK;
}
#endif

u8 CGPRS::OK_FIX(void)//2019强制放返回ok
{
    return RCVRIGHT;
}

u8 CGPRS::OK_CB(void)
{
	string ComTempBuf = "";
	string Result= "OK";
	//这种函数是非阻塞调用
	if (pthis->ComRequest == TRUE)
	{
		pthis->Read(&ComTempBuf);
		if(ComTempBuf.find(Result) != string::npos)
		{//结果判定成功
			return RCVRIGHT;
		}
		else
		{
			pthis->ErrorState = ERROR_OK;
			return RCVWRONG;
		}
	}
	else
	{
		//还没有收到任何数据
		pthis->ErrorState = ERROR_OK;
		return RCVWAITING;
	}
}
#ifdef EC20F_TRACE
u8 CGPRS::QICLOSE_CB(void)
{
	string ComTempBuf = "";
	string Result = "OK";
	//这种函数是非阻塞调用
	if (pthis->ComRequest == TRUE)
	{
		pthis->Read(&ComTempBuf);
		if (ComTempBuf.find(Result) != string::npos)
		{ //结果判定成功
			
			return RCVRIGHT;
		}
		else
		{
			pthis->ErrorState = ERROR_OK;
			return RCVWRONG;
		}
	}
	else
	{
		//还没有收到任何数据
		pthis->ErrorState = ERROR_OK;
		return RCVWAITING;
	}
}
#endif
u8 CGPRS::UPGRADE_CB(void)
{
	string ComTempBuf = "";
	string Result= "OK";
    string ERR = "+CME ERROR";
	//这种函数是非阻塞调用
	if (pthis->ComRequest == TRUE)
	{
		pthis->Read(&ComTempBuf);
		if(ComTempBuf.find(Result) != string::npos)
		{//结果判定成功
			return RCVRIGHT;
		}
        else if(ComTempBuf.find(ERR) != string::npos)
        {
            return RCVRIGHT;
        }
		else
		{
			pthis->ErrorState = ERROR_OK;
			return RCVWRONG;
		}
	}
	else
	{
		//还没有收到任何数据
		pthis->ErrorState = ERROR_OK;
		return RCVWAITING;
	}
}

u8 CGPRS::AT_CB(void)
{
	string ComTempBuf = "";
	string Result= "OK";
	//这种函数是非阻塞调用
//	if (pthis->ComRequest == TRUE)
//	{
		pthis->Read(&ComTempBuf);
		if(ComTempBuf.find(Result) != string::npos)
		{//结果判定成功
			return RCVRIGHT;//2
		}
		else
		{
			//pthis->ErrorState = ERROR_AT;
			return RCVWRONG;//3
		}
//	}
/*	else
	{
		//还没有收到任何数据
		pthis->ErrorState = ERROR_AT;
		return RCVWAITING;
	}*/
}

u8 CGPRS::CPIN_CB(void)
{
	string ComTempBuf = "";
	string Result= "+CPIN: READY";
    string Result2= "+CPIN:READY";
	//这种函数是非阻塞调用
	if (pthis->ComRequest == TRUE)
	{
		pthis->Read(&ComTempBuf);
		if(ComTempBuf.find(Result) != string::npos || ComTempBuf.find(Result2) != string::npos)  
		{//结果判定成功
            if(g_CSystem.Air_Upflag == 1)//2018
            {
                g_CGPRS.UP_Start_flag = 0;
                g_CSystem.WriteUpflag(0);//写升级标志
                g_CSystem.Air_Upflag = 0;
            }
			return RCVRIGHT;
		}
		else
		{
			//此处返回的信息可能有多种
			pthis->ErrorState = ERROR_CPIN;
			return RCVWRONG;
		}
	}
	else
	{
		//还没有收到任何数据
		pthis->ErrorState = ERROR_CPIN;
		return RCVWAITING;
	}
}

u8 CGPRS::CSQ_CB(void)
{
	string ComTempBuf = "";
	string Result= "+CSQ:";
	//这种函数是非阻塞调用
	if (pthis->ComRequest == TRUE)
	{
		pthis->Read(&ComTempBuf);
		if(ComTempBuf.find(Result) != string::npos)
		{	
			//结果判定成功
			//返回帧的格式
			//+CSQ: 29,0
			int IndexStart = ComTempBuf.find("CSQ");
			IndexStart += 5;//去掉空格移动到信号强度字段上
			int IndexEnd = ComTempBuf.find(",",IndexStart);
			if ((IndexEnd-IndexStart) >= 3)
			{
				//信号强度字段最大是99，如果超过3个
				//字符则说明提取有问题
				pthis->ErrorState = ERROR_CSQ;
				return RCVWRONG;
			}
			else
			{
				//找到信号强度的相关字段
				string CSQ = ComTempBuf.substr(IndexStart, IndexEnd-IndexStart);
				//将信号强度转换类型
				pthis->Strengh = strtol((&CSQ)->c_str(), NULL, 10);
				if (pthis->Strengh >31)
				{
					//最信号强度是31 
					//比这个大的会出现99，这是没有连上网
					pthis->Strengh = 0;
				}
				return RCVRIGHT;
			}
		}
		else
		{
			//此处返回的信息可能有多种
			pthis->ErrorState = ERROR_CSQ;
			return RCVWRONG;
		}
	}
	else
	{
		//还没有收到任何数据
		pthis->ErrorState = ERROR_CSQ;
		return RCVWAITING;
	}
}

u8 CGPRS::VER_CB(void)
{
    char TempBuff[1024] = {0};
	char Result[] = "AirM2M_720";
    char *ver = NULL;
	//这种函数是非阻塞调用
	if (pthis->ComRequest == TRUE)
	{
		pthis->Read(TempBuff);
        ver = strstr((char *)TempBuff, Result);
		if( ver != NULL )
		{	
			//结果判定成功
			//返回帧的格式
            char *pStart = NULL;
            char *pEnd = NULL;
            char LenChar[10] = {'\0'};//rcv_max 最大160
            u16 version = 0;
			pStart = strchr(ver, 'V');
			pEnd = strchr(ver, 'L');
            if (pEnd - pStart < 7 && pEnd-pStart > 0)//合法长度
            {
                //找到信号强度的相关字段
                memcpy(LenChar, pStart+1, pEnd-pStart-2);//获取数据长度
                //将信号强度转换类型
                version = atoi(LenChar);	
                pthis->Air_Ver = version;
                return RCVRIGHT;    
            }
            else
            {
                //此处返回的信息可能有多种
                return RCVWRONG;
            }            
		}
        else
        {
            //此处返回的信息可能有多种
            //可以细化返回的错误类型
            return RCVWRONG;
        }
    }
	else
	{
		//还没有收到任何数据
		pthis->ErrorState = ERROR_CSQ;
		return RCVWAITING;
	}
}

u8 CGPRS::CREG_CB(void)
{
	string ComTempBuf = "";
	string Result= "+CREG: 0,1";
	string Result2="+CREG: 0,5";
	string Result3="+CREG: 1,1";
    string Result4="+CREG: 2,1";//设置返回LAC和IC时，2,1
	//这种函数是非阻塞调用
	if (pthis->ComRequest == TRUE)
	{
		pthis->Read(&ComTempBuf);
		if(ComTempBuf.find(Result) != string::npos || ComTempBuf.find(Result2) != string::npos|| \
          ComTempBuf.find(Result3) != string::npos || ComTempBuf.find(Result4) != string::npos)
		{//结果判定成功
			ModeSwitchLock = 1;       //如果注册成功，则锁定模式切换锁
			if(g_CSystem.Gprs_ModeType != ModelType)
			 {
			 	g_CSystem.WriteModelType(ModelType);//把模块型号写入flash
			 }
             if(ModelType == AIR_720)
            {
                string temp_id = ComTempBuf.substr(14,4);
                strncpy((char*)pthis->m_Lac,temp_id.c_str(),4);
                
                temp_id = ComTempBuf.substr(21,7);
                strncpy((char*)pthis->m_Id,temp_id.c_str(),7);
            }
			return RCVRIGHT;
		}
		else
		{
			//此处返回的信息可能有多种
			//可以细化返回的错误类型
			pthis->ErrorState = ERROR_CREG;
			return RCVWRONG;
		}
	}
	else
	{
		//还没有收到任何数据
		pthis->ErrorState = ERROR_CREG;
		return RCVWAITING;
	}
}

#ifdef EC20F_TRACE
u8 CGPRS::CGREG_CB(void)
{
	string ComTempBuf = "";
	string Result = "+CGREG: 0,1";
	string Result2 = "+CGREG: 0,5";
	string Result3 = "+CGREG: 1,1";
    string Result4 = "+CGREG: 2,1";//ec20获取区域号时返回2,1
	//这种函数是非阻塞调用
	if (pthis->ComRequest == TRUE)
	{
		pthis->Read(&ComTempBuf);
		if (ComTempBuf.find(Result) != string::npos || ComTempBuf.find(Result2) != string::npos \
          || ComTempBuf.find(Result3) != string::npos || ComTempBuf.find(Result4) != string::npos)
		{						//结果判定成功
			ModeSwitchLock = 1; //如果注册成功，则锁定模式切换锁
            
            string temp_id = ComTempBuf.substr(15,4);
            strncpy((char*)pthis->m_Lac,temp_id.c_str(),4);
            
            temp_id = ComTempBuf.substr(22,7);
            strncpy((char*)pthis->m_Id,temp_id.c_str(),7); 
			return RCVRIGHT;
		}
		else
		{
			//此处返回的信息可能有多种
			//可以细化返回的错误类型
			pthis->ErrorState = ERROR_CREG;
			return RCVWRONG;
		}
	}
	else
	{
		//还没有收到任何数据
		pthis->ErrorState = ERROR_CREG;
		return RCVWAITING;
	}
}
#endif
u8 CGPRS::CGATT_CB(void)
{
	string ComTempBuf = "";
	string Result= "+CGATT: 1";
	//这种函数是非阻塞调用
	if (pthis->ComRequest == TRUE)
	{
		pthis->Read(&ComTempBuf);
		if(ComTempBuf.find(Result) != string::npos)
		{//结果判定成功
			return RCVRIGHT;
		}
		else
		{
			//此处返回的信息可能有多种
			//可以细化返回的错误类型
			pthis->ErrorState = ERROR_CGATT;
			return RCVWRONG;
		}
	}
	else
	{
		//还没有收到任何数据
		pthis->ErrorState = ERROR_CGATT;
		return RCVWAITING;
	}
}

u8 CGPRS::CCID_CB(void)
{
	string ComTempBuf = "";
	string Result= "OK";
	//这种函数是非阻塞调用
	if (pthis->ComRequest == TRUE)
	{
		pthis->Read(&ComTempBuf);
		if(ComTempBuf.find(Result) != string::npos)
		{
			if (ModelType == NEO_N720)
			{
				int IndexEnd;
				int IndexStart;
				//结果判定成功
				//ccid:格式\r\n8.....9\r\n\r\nOK\r\n
				IndexStart = ComTempBuf.find("CCID");
				IndexStart += 6;//指向CCID 字段
				IndexEnd = ComTempBuf.find("\r",IndexStart);
				if (IndexEnd-IndexStart == 20)
				{	  
					//长度进行校验
					string tempid = ComTempBuf.substr(IndexStart, IndexEnd-IndexStart);
					strncpy(m_CCID,tempid.c_str(),20);
					//m_CCID 中的字母统一用大写
					for (int i = 0; i<sizeof(m_CCID);i++)
					{
						m_CCID[i] = toupper(m_CCID[i]);
					}
					if (pthis->CompareICCID() == 0)
					{//已上报iccid
						pthis->firstOn = 2;
						
						u8 temp_ip[4];
						u16 temp_port;
						char buff[50];
						g_CSystem.GetUsingServerIP(temp_ip);
						temp_port = g_CSystem.GetUsingServerPort();
						sprintf(buff, "AT$MYNETSRV=0,0,2,0,\"%d.%d.%d.%d:%d\"\r\n", temp_ip[0], temp_ip[1], temp_ip[2], temp_ip[3], temp_port);
						pthis->Instruction[pthis->CIPSTARTIndex].Inc.assign(buff); //建立udp链接
					}
					return RCVRIGHT;
				}
				else
				{
					pthis->ErrorState = ERROR_CCID;
					return RCVWRONG;
				}
			}
			else if(ModelType == SIM800C)
			{
				//结果判定成功
				//ccid:格式\r\n8.....9\r\n\r\nOK\r\n
				int IndexStart = ComTempBuf.find("\n");
				IndexStart += 1;//指向CCID 字段
				int IndexEnd = ComTempBuf.find("\r",IndexStart);
				if (IndexEnd-IndexStart == 20)
				{	//长度进行校验
					//memcpy(m_CCID,c_str(ComTempBuf+IndexStart), 20);
					string tempid = ComTempBuf.substr(IndexStart, IndexEnd-IndexStart);
					strncpy(m_CCID,tempid.c_str(),20);
					//m_CCID 中的字母统一用大写
					for (int i = 0; i<sizeof(m_CCID);i++)
					{
						m_CCID[i] = toupper(m_CCID[i]);
					}
					if (pthis->CompareICCID() == 0)
					{
						pthis->firstOn = 2;
						
						u8 temp_ip[4];
						u16 temp_port;
						char buff[50];
						g_CSystem.GetUsingServerIP(temp_ip);
						temp_port = g_CSystem.GetUsingServerPort();
						sprintf(buff, "AT+CIPSTART=\"UDP\",\"%d.%d.%d.%d\",\"%d\"\r\n", temp_ip[0], temp_ip[1], temp_ip[2], temp_ip[3], temp_port);
						pthis->Instruction[pthis->CIPSTARTIndex].Inc.assign(buff); //建立udp链接
					}
					return RCVRIGHT;
				}
				else
				{
					pthis->ErrorState = ERROR_CCID;
					return RCVWRONG;
				}
			}
			else if(ModelType == AIR_720)
			{
				//结果判定成功
				//ccid:格式\r\n8.....9\r\n\r\nOK\r\n
				int IndexStart = ComTempBuf.find("\n");
				IndexStart += 9;//指向CCID 字段
				int IndexEnd = ComTempBuf.find("\r",IndexStart);
				if (IndexEnd-IndexStart == 20)
				{	//长度进行校验
					//memcpy(m_CCID,c_str(ComTempBuf+IndexStart), 20);
					string tempid = ComTempBuf.substr(IndexStart, IndexEnd-IndexStart);
					strncpy(m_CCID,tempid.c_str(),20);
					//m_CCID 中的字母统一用大写
					for (int i = 0; i<sizeof(m_CCID);i++)
					{
						m_CCID[i] = toupper(m_CCID[i]);
					}
					if (pthis->CompareICCID() == 0)
					{
						pthis->firstOn = 2;
						
						u8 temp_ip[4];
						u16 temp_port;
						char buff[50];
						g_CSystem.GetUsingServerIP(temp_ip);
						temp_port = g_CSystem.GetUsingServerPort();
						sprintf(buff, "AT+CIPSTART=\"UDP\",\"%d.%d.%d.%d\",\"%d\"\r\n", temp_ip[0], temp_ip[1], temp_ip[2], temp_ip[3], temp_port);
						pthis->Instruction[pthis->CIPSTARTIndex].Inc.assign(buff); //建立udp链接
					}
					return RCVRIGHT;
				}
				else
				{
					pthis->ErrorState = ERROR_CCID;
					return RCVWRONG;
				}
			}
#ifdef EC20F_TRACE //用于移植跟踪修改EC20F
			else if (ModelType == EC20F)
			{
				int IndexEnd;
				int IndexStart;
				//结果判定成功
				//ccid:格式\r\n8.....9\r\n\r\nOK\r\n
				IndexStart = ComTempBuf.find("QCCID");
				IndexStart += 7; //指向CCID 字段
				IndexEnd = ComTempBuf.find("\r", IndexStart);
				if (IndexEnd - IndexStart == 20)
				{
					//长度进行校验
					string tempid = ComTempBuf.substr(IndexStart, IndexEnd - IndexStart);
					strncpy(m_CCID, tempid.c_str(), 20);
					//m_CCID 中的字母统一用大写
					for (int i = 0; i < sizeof(m_CCID); i++)
					{
						m_CCID[i] = toupper(m_CCID[i]);
					}
					if (pthis->CompareICCID() == 0 || g_CSystem.head_err_type&0x01)
					{ //已上报iccid 或 flash错误
						pthis->firstOn = 2;

						u8 temp_ip[4];
						u16 temp_port;
						char buff[50];
						g_CSystem.GetUsingServerIP(temp_ip);
						temp_port = g_CSystem.GetUsingServerPort();
						sprintf(buff, "AT+QIOPEN=1,0,\"UDP\",\"%d.%d.%d.%d\",%d,0,1\r\n", temp_ip[0], temp_ip[1], temp_ip[2], temp_ip[3], temp_port);
						pthis->Instruction[pthis->CIPSTARTIndex].Inc.assign(buff); //建立udp链接
					}
					return RCVRIGHT;
				}
				else
				{
					pthis->ErrorState = ERROR_CCID;
					return RCVWRONG;
				}
			}
#endif
		}
		else
		{
			//此处返回的信息可能有多种
			//可以细化返回的错误类型
			pthis->ErrorState = ERROR_CCID;
			return RCVWRONG;
		}
	}
	else
	{
		//还没有收到任何数据
		pthis->ErrorState = ERROR_CCID;
		return RCVWAITING;
	}
        return RCVWAITING;
}

u8 CGPRS::CSTT_CB(void)
{
	string ComTempBuf = "";
	string Result= "OK";
	//这种函数是非阻塞调用
	if (pthis->ComRequest == TRUE)
	{ 
		pthis->Read(&ComTempBuf);
		if(ComTempBuf.find(Result) != string::npos)
		{
			//结果判定成功
			return RCVRIGHT;
		}
		else
		{
			//此处返回的信息可能有多种
			//可以细化返回的错误类型
			pthis->ErrorState = ERROR_CSTT;
			return RCVWRONG;
		}
	}
	else
	{
		//还没有收到任何数据
		pthis->ErrorState = ERROR_CSTT;
		return RCVWAITING;
	}
}

u8 CGPRS::NETACT_CB(void)
{
	string ComTempBuf = "";
	string Result= "."; // ip 的点
	//这种函数是非阻塞调用
	if (pthis->ComRequest == TRUE)
	{
		pthis->Read(&ComTempBuf);
		if(ComTempBuf.find(Result) != string::npos)
		{
			//结果判定成功
			//将本机ip存储起来
			return RCVRIGHT;
		}
		else
		{
			//此处返回的信息可能有多种
			//可以细化返回的错误类型
			pthis->ErrorState = ERROR_CIFSR;
			return RCVWRONG;
		}
	}
	else
	{
		//还没有收到任何数据
		pthis->ErrorState = ERROR_CIFSR;
		return RCVWAITING;
	}
}

u8 CGPRS::NETCLOSE_CB(void)
{
	string ComTempBuf = "";
	//string Result= "."; // ip 的点
	//这种函数是非阻塞调用
	if (pthis->ComRequest == TRUE)
	{
		//收到东西即可，不需要判断内容
		return RCVRIGHT;
	}
	else
	{
		//还没有收到任何数据
		pthis->ErrorState = ERROR_CIFSR;
		return RCVWAITING;
	}
}
u8 CGPRS::CIFSR_CB(void)
{
	string ComTempBuf = "";
	string Result= "."; // ip 的点
	//这种函数是非阻塞调用
	if (pthis->ComRequest == TRUE)
	{
		pthis->Read(&ComTempBuf);
		if(ComTempBuf.find(Result) != string::npos)
		{
			//结果判定成功
			//将本机ip存储起来
			return RCVRIGHT;
		}
		else
		{
			//此处返回的信息可能有多种
			//可以细化返回的错误类型
			pthis->ErrorState = ERROR_CIFSR;
			return RCVWRONG;
		}
	}
	else
	{
		//还没有收到任何数据
		pthis->ErrorState = ERROR_CIFSR;
		return RCVWAITING;
	}
}

u8 CGPRS::CIPSTART2G_CB(void)
{
	string ComTempBuf = "";
	string Result= "CONNECT OK";
	//这种函数是非阻塞调用
	if (pthis->ComRequest == TRUE)
	{
		pthis->Read(&ComTempBuf);
		if(ComTempBuf.find(Result) != string::npos)
		{
			//结果判定成功
			//将本机ip存储起来
			return RCVRIGHT;
		}
		else
		{
			//此处返回的信息可能有多种
			//可以细化返回的错误类型
			pthis->ErrorState = ERROR_CIPSTART;
			return RCVWRONG;
		}
	}
	else
	{
		//还没有收到任何数据
		pthis->ErrorState = ERROR_CIPSTART;
		return RCVWAITING;
	}
}

u8 CGPRS::CIPSTART_CB(void)
{
	string ComTempBuf = "";
	string Result= "OK";
	//这种函数是非阻塞调用
	if (pthis->ComRequest == TRUE)
	{
		pthis->Read(&ComTempBuf);
		if(ComTempBuf.find(Result) != string::npos)
		{
			//结果判定成功
			//将本机ip存储起来
			return RCVRIGHT;
		}
		else
		{
			//此处返回的信息可能有多种
			//可以细化返回的错误类型
			pthis->ErrorState = ERROR_CIPSTART;
			return RCVWRONG;
		}
	}
	else
	{
		//还没有收到任何数据
		pthis->ErrorState = ERROR_CIPSTART;
		return RCVWAITING;
	}
}
u8 CGPRS::NETOPEN_CB(void)
{
	string ComTempBuf = "";
	string Result= "OK";
	//这种函数是非阻塞调用
	if (pthis->ComRequest == TRUE)
	{
		pthis->Read(&ComTempBuf);
		if(ComTempBuf.find(Result) != string::npos)
		{
			//结果判定成功
			pthis->GPRSState = ALREADYCONNECTED;//执行这个命令成功后会相当于   gprs 完全配置完成
			pthis->SetTimer(3, 1000, pthis);//配置完成后开启心跳
			pthis->SetTimer(7, 100, pthis);//心跳返回检测定时器
			pthis->SetTimer(13, 800, pthis);//信号强度定时器
            pthis->SetTimer(5, 1500, pthis);//开机检测升级//2019
            if(g_CGPRS.firstOn & 0x02)
            {
                g_COverallTest.StartSendTestInfo = 1;
            }
			g_CPowerCheck->set_ledstate((pthis->m_LoginFlag&0x80)?LED_FREQ_0_1:LED_FREQ_1);//联网成功，等待配对
			g_CSystem.m_State &= ~0x08;//GPRS连接成功
            g_COverallTest.Comm_Sucess_flag = 1; 
			return RCVRIGHT;
		}
		else
		{
			//此处返回的信息可能有多种
			//可以细化返回的错误类型
			pthis->ErrorState = ERROR_CIPHEAD;
			return RCVWRONG;
		}
	}
	else
	{
		//还没有收到任何数据
		pthis->ErrorState = ERROR_CIPHEAD;
		return RCVWAITING;
	}
}

//CIMI码
u8 CGPRS::CIMI_CB(void)
{
    string ComTempBuf = "";
    string Result = "OK";
    if(pthis->ComRequest == TRUE)
    {
        pthis->Read(&ComTempBuf);  
        if(ComTempBuf.find(Result) != string::npos)
        {
            int IndexStart = ComTempBuf.find("460");
            if(IndexStart == -1)
                return RCVWAITING;     
            int IndexEnd = ComTempBuf.find("\r",IndexStart);          
            if(IndexEnd-IndexStart == 15)
            {
                IndexStart += 3;
                string cimi_str = ComTempBuf.substr(IndexStart,2);
                pthis->m_cimi = strtol((&cimi_str)->c_str(), NULL,10);
                return RCVRIGHT;
            }
            else 
                return RCVRIGHT;//也返回成功
        }
        else
        {
            return RCVWRONG;
        }
    }
    else
	{
		return RCVWAITING;
	}
}

//获取LAC和ID码
u8 CGPRS::LACID_CB(void)
{
	string ComTempBuf = "";
    string Result= "MYLACID";
    //这种函数是非阻塞调用
    if (pthis->ComRequest == TRUE)
    { 
        pthis->Read(&ComTempBuf);
        if(ComTempBuf.find(Result) != string::npos)
        {
            //结果判定成功
            //lac: 1~f fff  (16bits)
            //ID:(2G)1~65535 (4G)1~f fff fff
            string tempid = ComTempBuf.substr(12, 4);
            strncpy((char*)pthis->m_Lac,tempid.c_str(),4);

            tempid = ComTempBuf.substr(17, 7);
            strncpy((char*)pthis->m_Id,tempid.c_str(),7);
            return RCVRIGHT;
        }
        else
        {
            return RCVWRONG;
        }
    }
    else
    {
        //还没有收到任何数据
        return RCVWAITING;
    }
}

u8 CGPRS::NETREAD_CB(void)
{
	char ComTempBuf[1024] = "";
	char Result[]= "$MYNETREAD"; 
	char *pMsg = NULL;
	u8 ret = 0;
	//这种函数是非阻塞调用
	if (pthis->ComRequest == TRUE)
	{
		pthis->Read(ComTempBuf);
		pMsg = strstr(ComTempBuf, Result);
		if(pMsg != NULL)
		{
			//结果判定成功
			//报文数据获取报文长度
			char *pStart = NULL;
			char *pEnd = NULL;
			char LenChar[5] = {0};
			u16 LenHex = 0;
			pStart = strchr(pMsg, ',');
			pEnd = strchr(pMsg, '\r');
			if (pEnd - pStart < 5 && pEnd-pStart > 0)//合法长度
			{
				memcpy(LenChar, pStart+1, pEnd-pStart-1);//获取数据长度
				LenHex = atoi(LenChar);
				if (LenHex ==0)
				{
					//KillTimer(pthis, 11); //一直读到缓冲区无内容
					//g_CGPRS.m_Request = TRUE;
					//ret = RCVRIGHT;
					KillTimer(pthis,14);
					ret = RCVRIGHT;
				}
				else
				{
					memcpy(g_CGPRS.m_ReceiveFrame.Data, pEnd+2, LenHex);//偏移修改
					g_CGPRS.m_ReceiveFrame.Len = LenHex;
					KillTimer(pthis, 11); 
					g_CGPRS.m_Request = TRUE;
					ret = RCVRIGHT;
					//缓冲区可能还有内容
					SetTimer(14, 10, pthis);
					//g_CGPRS.m_Request = TRUE;//通知上层进行处理
					//ret = RCVWRONG;//为了避免缓冲区仍有内容让指令进行重试
					//把缓冲区的内容读完
				}
			}
			else
			{
				//收到的东西不合法
				ret = RCVWRONG;//
			}	
		}
		else
		{
			//此处返回的信息可能有多种
			//可以细化返回的错误类型
			pthis->ErrorState = ERROR_CIPSEND;
			ret = RCVWRONG;
		}
	}
	else
	{
		//还没有收到任何数据
		pthis->ErrorState = ERROR_CIPSEND;
		ret = RCVWAITING;
	}
	return ret;
}

u8 CGPRS::CIPSEND_CB1(string* Inc)
{
	if (ModelType == NEO_N720)
	{
		string ComTempBuf = "";
		string Result= "MYNETWRITE:";
		if (pthis->ComRequest == TRUE)
		{
			pthis->Read(&ComTempBuf);
			if(ComTempBuf.find(Result) != string::npos)
			{
				//可以进行发送
				if (pthis->IFComBuffEmpty() == 0)
				{
					pthis->ComBuffOut();
					pthis->SetTimer(11, 1, pthis);//每10ms检测一次是否有返回
					return SENDOK;
				}
				else 
				{
					return SENDFAIL;
				}
			}
			else
			{
				//此处返回的信息可能有多种
				//可以细化返回的错误类
				pthis->ErrorState = ERROR_CIPSEND;
				return SENDFAIL;
			}
		}
		else
		{
			//没有收到">"认为发送失败
			pthis->ErrorState = ERROR_NOARROW;
			return SENDFAIL;
		}
	}
	else if (ModelType == SIM800C)
	{
		string ComTempBuf = "";
		string Result= ">";
		if (pthis->ComRequest == TRUE)
		{
			pthis->Read(&ComTempBuf);
			if(ComTempBuf.find(Result) != string::npos)
			{
				//可以进行发送
				if (pthis->IFComBuffEmpty() == 0)
				{
					pthis->ComBuffOut();
					pthis->SetTimer(11, 1, pthis);//每10ms检测一次是否有返回
					return SENDOK;
				}
				else 
				{
					return SENDFAIL;
				}
			}
			else
			{
				//此处返回的信息可能有多种
				//可以细化返回的错误类
				pthis->ErrorState = ERROR_CIPSEND;
				return SENDFAIL;
			}
		}
		else
		{
			//没有收到">"认为发送失败
			pthis->ErrorState = ERROR_CIPSEND;
			return SENDFAIL;
		}
	}
	else if (ModelType == AIR_720)
	{
		string ComTempBuf = "";
		string Result= ">";
		if (pthis->ComRequest == TRUE)
		{
			pthis->Read(&ComTempBuf);
			if(ComTempBuf.find(Result) != string::npos)
			{
				//可以进行发送
				if (pthis->IFComBuffEmpty() == 0)
				{
					pthis->ComBuffOut();
					pthis->SetTimer(11, 1, pthis);//每10ms检测一次是否有返回
					return SENDOK;
				}
				else 
				{
					return SENDFAIL;
				}
			}
			else
			{
				//此处返回的信息可能有多种
				//可以细化返回的错误类
				pthis->ErrorState = ERROR_CIPSEND;
				return SENDFAIL;
			}
		}
		else
		{
			//没有收到">"认为发送失败
			pthis->ErrorState = ERROR_CIPSEND;
			return SENDFAIL;
		}
	}
#ifdef EC20F_TRACE //用于移植跟踪修改EC20F
	else if (ModelType == EC20F)
	{
		string ComTempBuf = "";
		string Result = ">";
		if (pthis->ComRequest == TRUE)
		{
			pthis->Read(&ComTempBuf);
			if (ComTempBuf.find(Result) != string::npos)
			{
				//可以进行发送
				if (pthis->IFComBuffEmpty() == 0)
				{
					pthis->ComBuffOut();
					pthis->SetTimer(11, 1, pthis); //每10ms检测一次是否有返回
					return SENDOK;
				}
				else
				{
					return SENDFAIL;
				}
			}
			else
			{
				//此处返回的信息可能有多种
				//可以细化返回的错误类
				pthis->ErrorState = ERROR_CIPSEND;
				return SENDFAIL;
			}
		}
		else
		{
			//没有收到">"认为发送失败
			pthis->ErrorState = ERROR_CIPSEND;
			return SENDFAIL;
		}
	}
#endif
        return SENDFAIL;
}

u8 CGPRS::CIPSEND_CB2(void)
{
	if (ModelType == NEO_N720)
	{
		string ComTempBuf = "";
		string Result= "OK"; 
		string Result2= "MYURCREAD";
		u8 ret = 0;
		//这种函数是非阻塞调用
		if (pthis->ComRequest == TRUE)
		{
			pthis->Read(&ComTempBuf);
			if(ComTempBuf.find(Result) != string::npos)
			{
				//结果判定成功
				KillTimer(pthis, 11); //独立指令执行完毕之后不再走流程要注意
				ret = RCVRIGHT;
			}
			else
			{
				//此处返回的信息可能有多种
				//可以细化返回的错误类型
				pthis->ErrorState = ERROR_CIPSEND;
				ret = RCVWRONG;
			}
		}
		else
		{
			//还没有收到任何数据
			pthis->ErrorState = ERROR_CIPSEND;
			ret = RCVWAITING;
		}
		return ret;
	}
	else if (ModelType == SIM800C)
	{
		string ComTempBuf = "";
		string Result= "SEND OK"; 
		u8 ret = 0;
		//这种函数是非阻塞调用
		if (pthis->ComRequest == TRUE)
		{
			pthis->Read(&ComTempBuf);
			if(ComTempBuf.find(Result) != string::npos)
			{
				//结果判定成功
				KillTimer(pthis, 11); //独立指令执行完毕之后不再走流程要注意
				ret = RCVRIGHT;
			}
			else
			{
				//此处返回的信息可能有多种
				//可以细化返回的错误类型
				pthis->ErrorState = ERROR_CIPSEND;
				ret = RCVWRONG;
			}
		}
		else
		{
			//还没有收到任何数据
			pthis->ErrorState = ERROR_CIPSEND;
			ret = RCVWAITING;
		}
		return ret;
	}
	else if (ModelType == AIR_720)
	{
		string ComTempBuf = "";
		string Result= "SEND OK"; 
//        string Result= "DATA ACCEPT";//快发模式返回值
		u8 ret = 0;
		//这种函数是非阻塞调用
		if (pthis->ComRequest == TRUE)
		{
			pthis->Read(&ComTempBuf);
			if(ComTempBuf.find(Result) != string::npos)
			{
				//结果判定成功
				KillTimer(pthis, 11); //独立指令执行完毕之后不再走流程要注意
				ret = RCVRIGHT;
			}
			else
			{
				//此处返回的信息可能有多种
				//可以细化返回的错误类型
				pthis->ErrorState = ERROR_CIPSEND;
				ret = RCVWRONG;
			}
		}
		else
		{
			//还没有收到任何数据
			pthis->ErrorState = ERROR_CIPSEND;
			ret = RCVWAITING;
		}
		return ret;
	}
#ifdef EC20F_TRACE //用于移植跟踪修改EC20F
	else if (ModelType == EC20F)
	{
		string ComTempBuf = "";
		string Result = "SEND OK";
		u8 ret = 0;
		//这种函数是非阻塞调用
		if (pthis->ComRequest == TRUE)
		{
			pthis->Read(&ComTempBuf);
			if (ComTempBuf.find(Result) != string::npos)
			{
				//结果判定成功
				KillTimer(pthis, 11); //独立指令执行完毕之后不再走流程要注意
				ret = RCVRIGHT;
			}
			else
			{
				//此处返回的信息可能有多种
				//可以细化返回的错误类型
				pthis->ErrorState = ERROR_CIPSEND;
				ret = RCVWRONG;
			}
		}
		else
		{
			//还没有收到任何数据
			pthis->ErrorState = ERROR_CIPSEND;
			ret = RCVWAITING;
		}
		return ret;
	}
#endif
	return SENDFAIL;
}

u8 CGPRS::Reboot_CB1(string* Inc)
{
	GPRS_MODEL_ON_H; 
	pthis->SetTimer(11, 150, pthis);
	return SENDOK;
}

u8 CGPRS::Reboot_CB2(void)
{
	GPRS_MODEL_ON_L; 
	pthis->SetTimer(11, 200, pthis);
	pthis->Step = pthis->ModelOnIndex-1;//下次执行启动块
	return RCVRIGHT;
}

u8 CGPRS::Mysysinfo_CB(void)
{
	string ComTempBuf = "";
	string Result= "MYSYSINFO";
	//这种函数是非阻塞调用
	if (pthis->ComRequest == TRUE)
	{
		pthis->Read(&ComTempBuf);
		if(ComTempBuf.find(Result) != string::npos)
		{ 
			return RCVRIGHT;
		}
		else
		{
			//此处返回的信息可能有多种
			//可以细化返回的错误类型
			pthis->ErrorState = ERROR_MYSYSINFO;
			return RCVWRONG;
		}
	}
	else
	{
		//还没有收到任何数据
		pthis->ErrorState = ERROR_MYSYSINFO;
		return RCVWAITING;
	}
}
u8 CGPRS::CIPSHUT_CB(void)
{
	string ComTempBuf = "";
	string Result= "SHUT OK";
	//这种函数是非阻塞调用
	if (pthis->ComRequest == TRUE)
	{
		pthis->Read(&ComTempBuf);
		if(ComTempBuf.find(Result) != string::npos)
		{
			//结果判定成功
			SetTimer(11, 1, pthis);
			pthis->Step = pthis->CSTTIndex-1; //从设置apn 重新开始链接
			return RCVRIGHT;
		}
		else
		{
			//此处返回的信息可能有多种
			//可以细化返回的错误类型
			pthis->ErrorState = ERROR_CIPSHUT;
			return RCVWRONG;
		}
	}
	else
	{
		//还没有收到任何数据
		pthis->ErrorState = ERROR_CIPSHUT;
		return RCVWAITING;
	}
}

u8 CGPRS::CIPSTATUS_CB(void)
{
	string ComTempBuf = "";
	string Result= "CONNECT OK";
	//这种函数是非阻塞调用
	if (pthis->ComRequest == TRUE)
	{
		pthis->Read(&ComTempBuf);
		if(ComTempBuf.find(Result) != string::npos)
		{
			//结果判定成功
			return RCVRIGHT;
		}
		else
		{
			//此处返回的信息可能有多种
			//可以细化返回的错误类型
			pthis->ErrorState = ERROR_CIPSTATUS;
			return RCVWRONG;
		}
	}
	else
	{
		//还没有收到任何数据
		pthis->ErrorState = ERROR_CIPSTATUS;
		return RCVWAITING;
	}
}

u8 CGPRS::MODEL_CB(void) //型号识别，GPRS根据型号回调确定当前使用的是那个模块
{
	string ComTempBuf = "";
	string n720_Result= "N720";
	string sim800c_Result= "SIM800C";
	string air720_Old_Result= "Nezha_MIFI";
    string air720_Result= "Air720";
#ifdef EC20F_TRACE //用于移植跟踪修改EC20F
	string ec20f_Result = "EC20F";
#endif
	if(ModelType==NEO_N720)
	{
		
		//这种函数是非阻塞调用
		if (pthis->ComRequest == TRUE)
		{
			pthis->Read(&ComTempBuf);
			if(ComTempBuf.find(n720_Result) != string::npos)
			{//结果判定成功
				return RCVRIGHT;
			}
			else
			{
				pthis->ErrorState = ERROR_MODEL;
				return RCVWRONG;
			}
		}
		else
		{
			//还没有收到任何数据
			pthis->ErrorState = ERROR_MODEL;
			return RCVWAITING;
		}
	}
	else if(ModelType==SIM800C)
	{
		//这种函数是非阻塞调用
		if (pthis->ComRequest == TRUE)
		{
			pthis->Read(&ComTempBuf);
			if(ComTempBuf.find(sim800c_Result) != string::npos)
			{//结果判定成功
				return RCVRIGHT;
			}
			else
			{
				pthis->ErrorState = ERROR_MODEL;
				return RCVWRONG;
			}
		}
		else
		{
			//还没有收到任何数据
			pthis->ErrorState = ERROR_MODEL;
			return RCVWAITING;
		}
	}
	else if(ModelType==AIR_720)
	{
		//这种函数是非阻塞调用
		if (pthis->ComRequest == TRUE)
		{
			pthis->Read(&ComTempBuf);
			if(ComTempBuf.find(air720_Result) != string::npos)
			{//结果判定成功
				return RCVRIGHT;
			}
            else if(ComTempBuf.find(air720_Old_Result) != string::npos)
            {
                return RCVRIGHT;
            }
			else
			{
				pthis->ErrorState = ERROR_MODEL;
				return RCVWRONG;
			}
		}
		else
		{
			//还没有收到任何数据
			pthis->ErrorState = ERROR_MODEL;
			return RCVWAITING;
		}
	}
#ifdef EC20F_TRACE //用于移植跟踪修改EC20F
	else if (ModelType == EC20F)
	{
		//这种函数是非阻塞调用
		if (pthis->ComRequest == TRUE)
		{
			pthis->Read(&ComTempBuf);
			if (ComTempBuf.find(ec20f_Result) != string::npos)
			{ //结果判定成功
				return RCVRIGHT;
			}
			else
			{
				pthis->ErrorState = ERROR_MODEL;
				return RCVWRONG;
			}
		}
		else
		{
			//还没有收到任何数据
			pthis->ErrorState = ERROR_MODEL;
			return RCVWAITING;
		}
	}
#endif
    return RCVWAITING;
}
//手持机关机的时候需要将GPRS关闭
void CGPRS::CloseGPRS(void)
{
	CarryBlock(ShutDownIndex, 1);
}

//上层调用获取CCID
//传入参数:pCCID  将CCID[0-21]存出
//返回值:0 成功 1 失败
u8  CGPRS::GetCCID(char *pCCID)
{	
	if (strlen(m_CCID) == 0)
	{
		return 1;//空串说明还未获取到或者获取失败
	} 
	else
	{
		memcpy(pCCID, m_CCID, CCIDLEN);
		return 0;
	}
}

void CGPRS::Reconnect(void)
{
	SetTimer(15, 1, this);
}

u8 CGPRS::CompareICCID(void)
{
	char current_iccid_buff[CCIDLEN] = {0};
	char older_iccid_buff[CCIDLEN]={0};
	
	u8 ret = 1;//初始状态:此次开机前后ICCID不一致
	
	if(0 == GetCCID(current_iccid_buff))//获取ICCID
	{	
		if(0 == GetOlderICCIDFromFlash(older_iccid_buff))//获取存储于flash中的此次开机前使用的ICCID
		{
			if(0 == memcmp(current_iccid_buff, older_iccid_buff, CCIDLEN))//比较两者是否一致
			{
				ret = 0;//如果一致，返回值设置为0
			}
		}
	}
	return ret;													
}

u8 CGPRS::GetOlderICCIDFromFlash(char *pICCID)
{
	u8 temp_buff[SYSTEM_EFFECTIVE_SIZE];

	g_CSST_Flash.flash_read(SYSTEM_SET_START_ADDR, temp_buff, SYSTEM_EFFECTIVE_SIZE);
	memcpy(pICCID, temp_buff + SYSTEM_ICCIDBACKUP_ADDR, 20);

	return 0;
}

void CGPRS::WriteICCIDToFlash(void)
{
	char iccid_buff[CCIDLEN] = {0};

	if(0 == GetCCID(iccid_buff))// 获取当前ICCID
	{	
		u8 temp_buff[SYSTEM_EFFECTIVE_SIZE];

		g_CSST_Flash.flash_read(SYSTEM_SET_START_ADDR, temp_buff, SYSTEM_EFFECTIVE_SIZE);
		memcpy(temp_buff + SYSTEM_ICCIDBACKUP_ADDR, iccid_buff, CCIDLEN);
		g_CSST_Flash.SST_Memory4kErase(SYSTEM_SET_START_ADDR / SST_A_PAGE_CAPA);
		g_CSST_Flash.flash_write(SYSTEM_SET_START_ADDR, temp_buff, SYSTEM_EFFECTIVE_SIZE);
	}
}
u8 CGPRS::GetModelType(void)
{
	if ((g_CSystem.m_Version == 4) || (g_CSystem.m_new_rn8209c_version == 1)) //带硬件版本ADC检测
		return 1;
	if(ModelType == SIM800C)
    {
        GPIO_InitTypeDef GPIO_InitStructure;
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;//硬件上没有上拉
        GPIO_Init(GPIOE, &GPIO_InitStructure);
        GPIO_SetBits(GPIOE, GPIO_Pin_11);//默认置高  
        if(GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_11))
        {
             GPIOE->CRH &= 0XFFFF0FFF;
             GPIOE->CRH |= GPIO_CRH_MODE11;
             GPIOE->BSRR |=(1<<11);
             return 1;
        }
        else
        {
            GPIOE->CRH &= 0XFFFF0FFF;
            GPIOE->CRH |= GPIO_CRH_MODE11;
            GPIOE->BSRR |=(1<<11);
            return 0;
        }
    }
    else if(ModelType == AIR_720)
        return 2;
    else
        return 1;
}

void CGPRS::send_testinf_time(void)
{
    Send_TestInfo();//发送整机检测信息
}

void CGPRS::SetModelType(u8 mode)
{
    ModelType = mode;
}

void CGPRS::send_poweroff(void)
{
    send_powoff_inf();    
}