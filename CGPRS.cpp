#include "driver.h"
#include "Stm_init.h"
#include "ctype.h"
#define UDPCONNECT//udp����

CGPRS *CGPRS::pthis = NULL;
char CGPRS::m_CCID[CCIDLEN] ={0};
u8 CGPRS::ModelType = AIR_720;//�ϵ�Ĭ����4G ģ��
u8 CGPRS::ModeSwitchLock = 0;//ͨѶ��ʽδ��
__IO BOOL bTimerFirstArrive = TRUE;	//�Ƿ��ǵ�һ�ν��붨ʱ���ж�

u8 GPRS_Tick = 0;//����60s �ղ������ݲ�GPRS ״̬
u8 UP_Timer = 0;//��ʱ����Ƿ����������°汾
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
	memset(m_Lac, '\0', sizeof(m_Lac));//����Ŷ�λ
    memset(m_Id, '\0', sizeof(m_Id));
    
	firstOn = 0;
    UP_Start_flag=0;
    Progress = 0;//��������
    UP_Finsh = 0;
    g_COverallTest.Comm_Sucess_flag = 0; 
    g_COverallTest.StartSendTestInfo = 0;
}

CGPRS::~CGPRS()
{
	
}
/*
��������    Send_KEY_AT()
�������ܣ�   ��������KEY����
��ڲ�����   ��
���ڲ�����   1���ɹ� 0��ʧ��
*/
u8 CGPRS::Send_KEY_AT(void)
{
    if( CarryBlock(UPGRADE_KEY, 1) ==0 )//0�ɹ� 1ʧ��
        return 1;
    else
        return 0;
}
/*
��������    Send_UP_AT()
�������ܣ�   ������������
��ڲ�����   ��
���ڲ�����   1���ɹ� 0��ʧ��
*/
u8 CGPRS::Send_UP_AT(void)
{
    if( CarryBlock(UPGRADE, 1) ==0 )//0�ɹ� 1ʧ��   
        return 1;
    else 
        return 0;
}

/*
��������    Send_Ver_AT()
�������ܣ�   ���Ͳ�ѯ�汾����
��ڲ�����   ��
���ڲ�����   1���ɹ� 0��ʧ��
*/
u8 CGPRS::Send_Ver_AT(void)
{
    if( CarryBlock(VERSION, 1) ==0 )//0�ɹ� 1ʧ��   
        return 1;
    else 
        return 0;
}

/*******************************************************************************
 ��������   UP_Reset
 ����������4G�����������ļ�����������װ
 ��ڲ�������
 ���ڲ�������
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
 ��������	Init_Reset
 �������������³�ʼ��gprs������Ա����
 ��ڲ�������
 ���ڲ�������
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
    Progress = 0;//��������   
    UP_Finsh = 0;
    UP_Timer = 0;
    
    ErrorState = 0;
	ComRequest = FALSE;
	ComSnd.HaveSndLen = 0;//����δʹ��
    SetTimer(11, 1, this);
    SetTimer(12, 100, this);//��ʱ�����û�д���
    g_COverallTest.Comm_Sucess_flag = 0;
    g_COverallTest.StartSendTestInfo = 0;
    memset(m_Lac, '\0',sizeof(m_Lac));
    memset(m_Id, '\0',sizeof(m_Id));
	u8 i = 0;
	POWERIndex = i;
	Instruction[i].Inc = "POWERON";  //��Դ����
	Instruction[i].TimeOut = 0;
	Instruction[i].RunNum = 0;
	Instruction[i].Retry = 0;
	Instruction[i].CarryFunc = &PowerOn_CarryCB;
	Instruction[i].WaitFunc = &PowerOn_WaitCB;
	i++;
	ModelOnIndex = i;
	Instruction[i].Inc = "MODELON";  //��������
	Instruction[i].TimeOut = 0;
	Instruction[i].RunNum = 0;
	Instruction[i].Retry = 0;
	Instruction[i].CarryFunc = &ModelOn_CarryCB;
	Instruction[i].WaitFunc =&ModelOn_WaitCB;
	i++;
	if (ModelType == NEO_N720)
	{
		Instruction[i].Inc = "AT\r\n";  //������ͬ��
		Instruction[i].TimeOut = 100;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &AT_CB;
		i++;
		
		Instruction[i].Inc = "ATE0\r\n";  //�رջ���
		Instruction[i].TimeOut = 10;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &OK_CB;
		i++;

		Instruction[i].Inc = "AT+CGMM\r\n";  //��ѯģ���ͺ�
		Instruction[i].TimeOut = 10;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &MODEL_CB;
		i++;
		
		Instruction[i].Inc = "AT+CCID\r\n";  //��ȡCCID���
		Instruction[i].TimeOut = 100;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 2;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CCID_CB;
		i++;
		
		Instruction[i].Inc = "AT+CPIN?\r\n";  //���sim���Ƿ���λ
		Instruction[i].TimeOut = 500;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CPIN_CB;
		i++;
		
		CSQIndex = i;
		Instruction[i].Inc = "AT+CSQ\r\n";  //��ȡ�ź�ǿ��
		Instruction[i].TimeOut = 100;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 40;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CSQ_CB;
		i++;

		Instruction[i].Inc = "AT+CREG?\r\n";  //���ע��״̬
		Instruction[i].TimeOut = 100;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 40;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CREG_CB;
		i++;

		Instruction[i].Inc = "AT+CGATT?\r\n";  //���ע��״̬
		Instruction[i].TimeOut = 100;//10s��ʱ
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 10;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CGATT_CB;
		i++;
		//ִ����cgatt�����ģ��ע������ɹ�

		Instruction[i].Inc = "AT$MYNETURC=1\r\n";  //���������ϱ�
		Instruction[i].TimeOut = 100;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 1;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &OK_CB;
		i++;
		
		CSTTIndex = i;
		Instruction[i].Inc = "AT$MYNETCON=0,APN,CMIOT\r\n";  //����apn
		Instruction[i].TimeOut = 10;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 1;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CSTT_CB;
		i++;

		Instruction[i].Inc = "AT$MYNETACT=0,1\r\n";  //������������
		Instruction[i].TimeOut = 4000;//40sһ�μ���3��
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &NETACT_CB;
		i++;

		Instruction[i].Inc = "AT$MYNETCLOSE=0\r\n"; //�ر���·
		Instruction[i].TimeOut = 100;//4
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 1;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &NETCLOSE_CB;
		i++;
		CIPSTARTIndex = i;
		//��ȡ��������ip �˿�
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
		Instruction[i].Inc.assign(buff); //����udp����
		Instruction[i].TimeOut = 1000;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CIPSTART_CB;
		i++;
    
         //CIMI��,������Ӫ��
        Instruction[i].Inc = "AT+CIMI\r\n";
		Instruction[i].TimeOut = 150;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CIMI_CB;
		i++;

        //��ȡ�������
        Instruction[i].Inc = "AT$MYLACID\r\n";
		Instruction[i].TimeOut = 150;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &LACID_CB;
		i++;
        
		Instruction[i].Inc = "AT$MYNETOPEN=0\r\n";  //����·
		Instruction[i].TimeOut = 150;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &NETOPEN_CB;
		i++;
		
		Instruction[i].Inc = "";  //�յ�ָ�������������������
		Instruction[i].TimeOut = 0;//5s
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 0;
		Instruction[i].CarryFunc = NULL;
		Instruction[i].WaitFunc = NULL;
		i++; 
		
		//�����ָ������������������ָ��
		//����ָ��
		CIPSENDIndex = i;//����±�
		Instruction[i].Inc = "AT$MYNETWRITE=0,";//��������ָ��  
		Instruction[i].TimeOut =	 200;// ���ʧ�ܵĻ�645s֮��ģ��������ϱ�close
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 0;
		Instruction[i].CarryFunc = &CIPSEND_CB1;
		Instruction[i].WaitFunc = &CIPSEND_CB2;
		i++; 

		//��ȡ������ֶ���2048
		NetReadIndex = i;
		Instruction[i].Inc = "AT$MYNETREAD=0,1024\r\n";  //��ȡ�Ѿ��յ�������
		Instruction[i].TimeOut = 30;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 8;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &NETREAD_CB;
		i++;
		
		RebootIndex = i;//����±�
		Instruction[i].Inc = "Reboot";//��������ָ��  
		Instruction[i].TimeOut = 10;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 0;
		Instruction[i].CarryFunc = &Reboot_CB1;
		Instruction[i].WaitFunc = &Reboot_CB2;
		i++; 

		SysInfoIndex = i;//��ѯ������ʽ
		Instruction[i].Inc = "AT$MYSYSINFO";
		Instruction[i].TimeOut = 10;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 0;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc =&Mysysinfo_CB;
		i++;
		
		ShutDownIndex = i;//�ػ�
		Instruction[i].Inc = "AT$MYPOWEROFF\r\n";
		//Instruction[i].Inc = "PowerOff";//ԭ
		Instruction[i].TimeOut = 10;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 0;
		//Instruction[i].CarryFunc = &PowerOff_CarryCB;//ԭ
        Instruction[i].CarryFunc = &InsSend_CB;
		//Instruction[i].WaitFunc = &PowerOff_WaitCB;//ԭ
        Instruction[i].WaitFunc = &PowerSoftOff_WaitCB;
		i++;
	}
	else if(ModelType == SIM800C)
	{
		// 2G ģ�������
		Instruction[i].Inc = "ATE0\r\n";  //�رջ���
		Instruction[i].TimeOut = 10;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &OK_CB;
		i++;
		
		Instruction[i].Inc = "AT+CGMM\r\n";  //��ѯģ���ͺ�
		Instruction[i].TimeOut = 10;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &MODEL_CB;
		i++;
		
		Instruction[i].Inc = "AT+CPIN?\r\n";  //���sim���Ƿ���λ
		Instruction[i].TimeOut = 500;//5s
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CPIN_CB;
		i++;
		CSQIndex = i;
		Instruction[i].Inc = "AT+CSQ\r\n";  //��ȡ�ź�ǿ��
		Instruction[i].TimeOut = 80;//800 ms
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 0;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CSQ_CB;
		i++;

		Instruction[i].Inc = "AT+CREG?\r\n";  //���ע��״̬
		Instruction[i].TimeOut = 10;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 20;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CREG_CB;
		i++;

		Instruction[i].Inc = "AT+CGATT?\r\n";  //���ע��״̬
		Instruction[i].TimeOut = 1000;//10s��ʱ
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 40;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CGATT_CB;
		i++;

		Instruction[i].Inc = "AT+CCID\r\n";  //��ȡCCID���
		Instruction[i].TimeOut = 200;// 2s��ʱ
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 1;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CCID_CB;
		i++;
		
		CSTTIndex = i;
		Instruction[i].Inc = "AT+CSTT=\"CMIOT\"\r\n";  //����apn
		Instruction[i].TimeOut = 10;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 1;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CSTT_CB;
		i++;

		Instruction[i].Inc = "AT+CIICR\r\n";  //����������·
		Instruction[i].TimeOut = 8500;//85s ��ʱ
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 1;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &OK_CB;
		i++;

		Instruction[i].Inc = "AT+CIFSR\r\n";  //��ȡ����ip
		Instruction[i].TimeOut = 10;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 1;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CIFSR_CB;
		i++;

		//�̶�udp Դ�˿�,��pos������˿�
		Instruction[i].Inc = "AT+CLPORT=\"UDP\",\"8888\"\r\n";  
		Instruction[i].TimeOut = 10;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 1;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &OK_CB;
		i++;

		CIPSTARTIndex = i;
		//��ȡ��������ip �˿�
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
		Instruction[i].Inc.assign(buff); //����udp����
		Instruction[i].TimeOut = 16000;// 160s ������
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 1;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CIPSTART2G_CB;
		i++;

		Instruction[i].Inc = "AT+CIPHEAD=1\r\n";  //���ܵ������ݼ���ǰ׺+IPD
		Instruction[i].TimeOut = 10;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 1;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &NETOPEN_CB; //��4G �Ĵ���ʽ��ͬ
		i++;

		Instruction[i].Inc = "";  //�յ�ָ�������������������
		Instruction[i].TimeOut = 0;//5s
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 0;
		Instruction[i].CarryFunc = NULL;
		Instruction[i].WaitFunc = NULL;
		i++; 
		
		//�����ָ������������������ָ��
		//����ָ��
		CIPSENDIndex = i;//����±�
		Instruction[i].Inc = "AT+CIPSEND=";//��������ָ��  
		Instruction[i].TimeOut = 300;// ���ʧ�ܵĻ�645s֮��ģ��������ϱ�close
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &CIPSEND_CB1;
		Instruction[i].WaitFunc = &CIPSEND_CB2;
		i++; 

		RebootIndex = i;//����±�
		Instruction[i].Inc = "Reboot";//��������ָ��  
		Instruction[i].TimeOut = 10;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 0;
		Instruction[i].CarryFunc = &Reboot_CB1;
		Instruction[i].WaitFunc = &Reboot_CB2;
		i++; 

		CIPSHUTIndex = i;//����±�
		Instruction[i].Inc = "AT+CIPSHUT\r\n";//�ر�����  
		Instruction[i].TimeOut = 800; 
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 0;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CIPSHUT_CB; //17
		i++; 

		CIPSTATUSIndex = i;//����±�
		Instruction[i].Inc = "AT+CIPSTATUS\r\n";//�ر�����  
		Instruction[i].TimeOut = 100; 
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 0;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CIPSTATUS_CB;
		i++; 	
	}
    else if(ModelType == AIR_720)
    {
		Instruction[i].Inc = "AT\r";  //������ͬ��
		Instruction[i].TimeOut = 100;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 30;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &AT_CB;
		i++;              
        
        Instruction[i].Inc = "ATE0\r";  //�رջ���
		Instruction[i].TimeOut = 10;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &OK_CB;
		i++;
		
        Instruction[i].Inc = "AT+UPGRADE=\"AUTO\", 0\r";  //�ر��Զ�����
		Instruction[i].TimeOut = 10;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &OK_FIX;
		i++;
        
		Instruction[i].Inc = "AT+CGMM\r";  //��ѯģ���ͺ�
		Instruction[i].TimeOut = 10;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &MODEL_CB;
		i++;
		
		Instruction[i].Inc = "AT+CPIN?\r";  //���sim���Ƿ���λ
		Instruction[i].TimeOut = 500;//5s
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CPIN_CB;
		i++;
        
		CSQIndex = i;
		Instruction[i].Inc = "AT+CSQ\r";  //��ȡ�ź�ǿ��
		Instruction[i].TimeOut = 120;//800 ms
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CSQ_CB;
		i++;

        Instruction[i].Inc = "AT+CIMI\r";//��Ӫ������
		Instruction[i].TimeOut = 100;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CIMI_CB;
		i++;
        
        Instruction[i].Inc = "AT+CREG=2\r";//����LOC��IC
		Instruction[i].TimeOut = 100;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &OK_CB;
		i++;
        
		Instruction[i].Inc = "AT+CREG?\r";  //���ע��״̬
		Instruction[i].TimeOut = 100;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 60;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CREG_CB;
		i++;

		Instruction[i].Inc = "AT+CGATT?\r";  //���GPRS����״̬
		Instruction[i].TimeOut = 1000;//10s��ʱ
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 40;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CGATT_CB;
		i++;
              
		Instruction[i].Inc = "AT+ICCID\r";  //��ȡCCID���
		Instruction[i].TimeOut = 400;// 2s��ʱ
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 4;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CCID_CB;
		i++;
        
		CSTTIndex = i;
		Instruction[i].Inc = "AT+CSTT=\"CMIOT\"\r";  //����apn
		Instruction[i].TimeOut = 10;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 10;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CSTT_CB;
		i++;

		Instruction[i].Inc = "AT+CIICR\r";  //����������·
		Instruction[i].TimeOut = 8500;//85s ��ʱ
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 2;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &OK_CB;
		i++;

		Instruction[i].Inc = "AT+CIFSR\r";  //��ȡ����ip
		Instruction[i].TimeOut = 600;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 1;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CIFSR_CB;
		i++;

		CIPSTARTIndex = i;
		//��ȡ��������ip �˿�
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
		Instruction[i].Inc.assign(buff); //����udp����
		Instruction[i].TimeOut = 16000;// 160s ������
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 1;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CIPSTART2G_CB;
		i++;
             
		Instruction[i].Inc = "AT+CIPHEAD=1\r";  //���ܵ������ݼ���ǰ׺+IPD
		Instruction[i].TimeOut = 10;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 1;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &NETOPEN_CB; //��4G �Ĵ���ʽ��ͬ
		i++;
      
		Instruction[i].Inc = "";  //�յ�ָ�������������������
		Instruction[i].TimeOut = 0;//5s
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 0;
		Instruction[i].CarryFunc = NULL;
		Instruction[i].WaitFunc = NULL;
		i++; 
		
		//�����ָ������������������ָ��
		//����ָ��
		CIPSENDIndex = i;//����±�
		Instruction[i].Inc = "AT+CIPSEND=";//��������ָ��  //AT+CIPSEND=
		Instruction[i].TimeOut = 300;// ���ʧ�ܵĻ�645s֮��ģ��������ϱ�close
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &CIPSEND_CB1;
		Instruction[i].WaitFunc = &CIPSEND_CB2;
		i++; 

		RebootIndex = i;//����±�
		Instruction[i].Inc = "Reboot";//��������ָ��  
		Instruction[i].TimeOut = 10;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 0;
		Instruction[i].CarryFunc = &Reboot_CB1;
		Instruction[i].WaitFunc = &Reboot_CB2;
		i++; 

		CIPSHUTIndex = i;//����±�
		Instruction[i].Inc = "AT+CIPSHUT\r";//�ر�����  
		Instruction[i].TimeOut = 800; 
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 0;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CIPSHUT_CB; //17
		i++; 

		CIPSTATUSIndex = i;//����±�
		Instruction[i].Inc = "AT+CIPSTATUS\r";//����  
		Instruction[i].TimeOut = 100; 
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 0;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CIPSTATUS_CB;
		i++;
        //Զ������Air720ģ��
        UPGRADE_KEY = i;
        Instruction[i].Inc = "AT+UPGRADE=\"KEY\",\"2BOF3tmn1dNeytiuWk6u9vi2UOwoG3md\"\r" ;// ����Զ������KEY 
        //Instruction[i].Inc = "AT+UPGRADE=\"KEY\",\"njgWpfxCHvxaV6XiD47OaMwGiSF4rOeq\"\r" ;        
        Instruction[i].TimeOut = 10; 
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 0;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &OK_CB;
		i++;
        
        UPGRADE = i;
        Instruction[i].Inc = "AT+UPGRADE\r";// ִ������
		Instruction[i].TimeOut = 10; 
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 0;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &UPGRADE_CB;
		i++;
        
        VERSION = i;
        Instruction[i].Inc = "AT+VER\r";// �鿴�汾��
        //Instruction[i].Inc = "ATI\r";// �鿴�汾��
        //Instruction[i].Inc = "AT+UPGRADE?\r";// ��������״̬
		Instruction[i].TimeOut = 10; 
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 0;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &VER_CB;
		i++;
    }
#ifdef EC20F_TRACE //������ֲ�����޸�EC20F
	else if(ModelType == EC20F)
	{
		Instruction[i].Inc = "AT\r\n";  //������ͬ��  �48s
		Instruction[i].TimeOut = 200;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 24;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &AT_CB;
		i++;
		Instruction[i].Inc = "ATE0\r\n"; //�رջ���
		Instruction[i].TimeOut = 50;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &OK_CB;
		i++;
		Instruction[i].Inc = "AT+CGMM\r\n"; //��ѯģ���ͺ�
		Instruction[i].TimeOut = 50;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &MODEL_CB;
		i++;
		Instruction[i].Inc = "AT+CPIN?\r\n"; //���sim���Ƿ���λ
		Instruction[i].TimeOut = 200;  //5s
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 5;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CPIN_CB;
		i++;
        CSQIndex = i;
		Instruction[i].Inc = "AT+CSQ\r\n"; //��ȡ�ź�ǿ��
		Instruction[i].TimeOut = 50; //300 ms
		Instruction[i].RunNum = 3;
		Instruction[i].Retry = 3;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CSQ_CB;
		i++;
		Instruction[i].Inc = "AT+CREG?\r\n"; //���CS����״̬ 90s
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
        
		Instruction[i].Inc = "AT+CGREG?\r\n"; //���PS����״̬ 60s
		Instruction[i].TimeOut = 200;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 30;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CGREG_CB;
		i++;
        
        Instruction[i].Inc = "AT+CIMI\r\n"; //��ȡ��Ӫ������
		Instruction[i].TimeOut = 200;
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 2;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CIMI_CB;
		i++;
        
		Instruction[i].Inc = "AT+CGATT?\r\n"; //���GPRS����״̬
		Instruction[i].TimeOut = 500;//�140s
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 28;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CGATT_CB;
		i++;
		Instruction[i].Inc = "AT+QCCID\r\n"; //��ȡCCID���
		Instruction[i].TimeOut = 100;// 1s��ʱ
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 4;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &CCID_CB;
		i++;
		//Instruction[i].Inc = "AT+QICSGP=1,1,\"UNINET\",\"\",\"\",1\r"; //����SGP
		Instruction[i].Inc = "AT+QICSGP=1\r\n"; //����SGP //Ĭ��CMNET
		Instruction[i].TimeOut = 100;	  // 1s��ʱ
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 4;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &OK_CB;
		i++;
		Instruction[i].Inc = "AT+QIDEACT=1\r\n"; //����DEACT
		Instruction[i].TimeOut = 200;		 // 1s��ʱ
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 20;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &OK_CB;
		i++;
		Instruction[i].Inc = "AT+QIACT=1\r\n"; //����ACT
		Instruction[i].TimeOut = 200;		  // 1s��ʱ
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 60;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &OK_CB;
		i++;
		CIPSTARTIndex = i;
		//��ȡ��������ip �˿�
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
		Instruction[i].Inc.assign(buff);		 //����udp����
		Instruction[i].TimeOut = 12000;		  // 120s��ʱ
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 1;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &OK_CB;
		i++;
		// Instruction[i].Inc = "AT+QISEND=0,5\r"; //��������ָ��  //AT+CIPSEND=
		// Instruction[i].TimeOut = 300;	  // ���ʧ�ܵĻ�645s֮��ģ��������ϱ�close
		// Instruction[i].RunNum = 0;
		// Instruction[i].Retry = 3;
		// Instruction[i].CarryFunc = &InsSend_CB;
		// Instruction[i].WaitFunc = &OK_FIX;
		// i++;
		// Instruction[i].Inc = "test\r"; //��������ָ��  //AT+CIPSEND=
		// Instruction[i].TimeOut = 300;		  // ���ʧ�ܵĻ�645s֮��ģ��������ϱ�close
		// Instruction[i].RunNum = 0;
		// Instruction[i].Retry = 3;
		// Instruction[i].CarryFunc = &InsSend_CB;
		// Instruction[i].WaitFunc = &OK_CB;
		// i++;
		// Instruction[i].Inc = "AT+QISEND=0,0\r"; //��������ָ��  //AT+CIPSEND=
		// Instruction[i].TimeOut = 300;  // ���ʧ�ܵĻ�645s֮��ģ��������ϱ�close
		// Instruction[i].RunNum = 0;
		// Instruction[i].Retry = 3;
		// Instruction[i].CarryFunc = &InsSend_CB;
		// Instruction[i].WaitFunc = &OK_CB;
		// i++;
		// Instruction[i].Inc = "test\r\n"; //��������ָ��  //AT+CIPSEND=
		// Instruction[i].TimeOut = 300;  // ���ʧ�ܵĻ�645s֮��ģ��������ϱ�close
		// Instruction[i].RunNum = 0;
		// Instruction[i].Retry = 3;
		// Instruction[i].CarryFunc = &TEST_REBOOT;
		// Instruction[i].WaitFunc = &OK_FIX;
		// i++;
		Instruction[i].Inc = "AT+QISTATE=1,0\r\n"; //��ȡ����״̬
		Instruction[i].TimeOut = 100;			 // 1s��ʱ
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 2;
		Instruction[i].CarryFunc = &InsSend_CB;
		Instruction[i].WaitFunc = &NETOPEN_CB;
		i++;
		// Instruction[i].Inc = "AT+QIDEACT=1\r"; //�ر�ACT
		// Instruction[i].TimeOut = 200;		   // 2s��ʱ
		// Instruction[i].RunNum = 0;
		// Instruction[i].Retry = 2;
		// Instruction[i].CarryFunc = &InsSend_CB;
		// Instruction[i].WaitFunc = &OK_CB;
		// i++;
		// Instruction[i].Inc = "AT+QICLOSE=0\r"; //�ر�����
		// Instruction[i].TimeOut = 200;			 // 2s��ʱ
		// Instruction[i].RunNum = 0;
		// Instruction[i].Retry = 2;
		// Instruction[i].CarryFunc = &InsSend_CB;
		// Instruction[i].WaitFunc = &QICLOSE_CB;
		// i++;
		Instruction[i].Inc = "";	//�յ�ָ�������������������
		Instruction[i].TimeOut = 0; //5s
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 0;
		Instruction[i].CarryFunc = NULL;
		Instruction[i].WaitFunc = NULL;
		i++;
		//�����ָ������������������ָ��
		//����ָ��
		CIPSENDIndex = i;					//����±�
		Instruction[i].Inc = "AT+QISEND=0,"; //��������ָ��  //AT+CIPSEND=
		Instruction[i].TimeOut = 30;		// ���ʧ�ܵĻ�645s֮��ģ��������ϱ�close
		Instruction[i].RunNum = 0;
		Instruction[i].Retry = 0;
		Instruction[i].CarryFunc = &CIPSEND_CB1;
		Instruction[i].WaitFunc = &CIPSEND_CB2;
		i++;
	}
#endif    
	//������ָ��鸳��ֵ
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
 ��������TIM5_CONFIG
 ����������TIM5��ʼ��
 ��ڲ�������
 ���ڲ�������
********************************************************************************/
void CGPRS::Timer2Config(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN ;//TIM2ʱ��ʹ��
	TIM2->ARR=100;
	TIM2->PSC=SystemCoreClock/10000 - 1;//Ԥ��Ƶ��
	
	//����������Ҫͬʱ���òſ���ʹ���ж�
	TIM2->DIER|=1<<0;//��������ж�
	TIM2->DIER|=1<<6;//�������ж�
	
	TIM2->CR1|=0x01;//ʹ�ܶ�ʱ��2
	
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

/*******************************************************************************
 �� �� ���� CGPRSConfig
 ���������� UART1�������� td1410ʹ������ gprsʹ������ ��gprs ���sim
 ��ڲ����� ��

 ���ڲ����� 0: ok; -1:����gprsʧ��; -2��sim������λ
********************************************************************************/
void CGPRS::Init(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;
    
    RCC->APB1ENR |= RCC_APB1ENR_UART5EN ;//UART5
	GPIOC->CRH &= 0xFFF0FFFF;//PC12(����/���)
	GPIOD->CRL &= 0xFFFFF0FF;//PD2(����/����)
	GPIOC->CRH |= GPIO_CRH_MODE12|GPIO_CRH_CNF12_1;//UART5 :PC12->TX; 50MHZ,AF_PP;
	GPIOD->CRL |= GPIO_CRL_CNF2_0;//UART5 : PD2->RX;

	UART5->CR1 = 0;
	UART5->CR1 = 0 << 12 | 1 << 6 | 1 << 5 | 1 << 3 | 1 << 2;
	//0xEA6->9600, 0x0753->19200, 0x0138->115200, 0x27->921600(����1��ߵ���Ҫ��2�����ң�Ҫ����0x0271->115200)
	UART5->BRR = 0x0138;//115200
	UART5->CR2 = 0 << 14 | 0 << 12 | 0 << 11;
	UART5->CR3 = 0;
	UART5->CR1 |= 1 << 13;//�����ж�

	NVIC_InitStructure.NVIC_IRQChannel = UART5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	// 4G ��2G��·���ϵ�pwr���Ų�ͬ?
	GPIOD->CRL &= 0XFFFFFFF0;
	GPIOD->CRL |= GPIO_CRL_MODE0;	//ͨ���������

	GPIOE->CRH &= 0XFFFF0FFF;
	GPIOE->CRH |= GPIO_CRH_MODE11;	//ͨ���������

	GPIOE->CRH &= 0XFFFFF0FF;
	GPIOE->CRH |= GPIO_CRH_MODE10;	//ͨ���������
	Timer2Config();	//�ֽڳ�ʱ�ж�
	NEO_N720_4GNORST;
    if(g_CSystem.Air_Upflag == 0)
        Init_Reset();//��ʼ����������ֵ
    else if(g_CSystem.Air_Upflag == 1)
        g_CGPRS.UP_Reset();
	g_CPowerCheck->set_ledstate(LED_FREQ_2);//��������
}

/*******************************************************************************
 �� �� ���� gprs_strength
 ���������� ��ȡ�ź�ǿ��
 ��ڲ����� ��
 ���ڲ����� �ź�ǿ��0-31 ��ֵԽ���ź�Խǿ -1:��ȡ����
********************************************************************************/
int CGPRS::gprs_strength(void)
{
    SetTimer(13,500,this);
	return Strengh;
}

/*******************************************************************************
 �� �� ���� UP_Version
 ���������� ��ȡ4G�汾
 ��ڲ����� ��
 ���ڲ����� �汾��XXX -1:��ȡ����
********************************************************************************/
int CGPRS::UP_Version(void)
{
	return Air_Ver;
}

/*******************************************************************************
 �� �� ���� UP_Progress
 ���������� ��ȡ��������
 ��ڲ����� ��
 ���ڲ����� �������� 0-100 -1:��ȡ����
********************************************************************************/
int CGPRS::UP_Progress(void)
{
	return Progress;
}

/*******************************************************************************
 �� �� ���� TimerEvent
 ���������� Timer0:�����������Զ�ʱ�� Timer1:�������ݳ�ʱ��ʱ�� Timer2:���gprs��������
 ��ڲ����� ulTimeNum ��ʱ�����
 ���ڲ����� ��
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
		{	//����ע�������
			//GPRSͨѶ�ѿ���
			if (m_LoginFlag == 0x02 || m_LoginFlag == 0x04)
			{
				//���ڻ��߶˿ڴ���
				KillTimer(this, 3);
				break;
			}
			if (m_SendFlag)
			{
				m_SendFlag = 0;
				break;
			}
			//�˿��л�(�����ڷ���������ע������ǰ)
			g_CSystem.m_uiPortChangedCounter--;
			if (g_CSystem.m_uiPortChangedCounter == 0)
			{
				//��Ҫ�л��˿�
				g_CSystem.m_uiPortChangedCounter = 720;
				Reconnect();
				break;
			}
			if (m_LoginFlag & 0x80)
			{	//ע��ɹ���������
			  	if(g_CSystem.m_State & 0x20)
			  		CancelTradeReq();//�г������׵��ȷ���������0011
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
						KillTimer(this, 3);//�ر�ע�����������
						Reboot();//����ģ��
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
                g_CGPRS.Send_Ver_AT();//��ѯ�汾
                IBeep m_IBeep;
                m_IBeep.OperateSuccess();//������ 
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
                //g_CSystem.Start_flag = 0;//����ϵ��Զ�������־//�Ȳ��壬��ֹһ��û�������ɹ�
                g_CGPRS.Send_KEY_AT();
                SetTimer(8,200,this);
                IBeep m_IBeep;
                m_IBeep.OperateSuccess();//������ 
            } 
            else
            {
                g_CSystem.Start_flag = 0;//����ϵ��Զ�������־
                UP_Timer = 0;
            }
            KillTimer(this,5);                
           break;             
		}
		case 7:
		{
			if (GPRSState != CONNECTING) //�����ӽ׶β������ж�
			{
				GPRS_Tick++;
				if (GPRS_Tick >= 60)
				{
					GPRS_Tick = 0;
					//������
					ErrorState = ERROR_NORESPOND;
				}
			}
			if (CUdpProtocol::u8ServerUpdate == 1)//�����ʱ
			{
				CUdpProtocol::u8ServerUpdate = 2;
				g_CGPRS.Reboot();
			}
			else if (CUdpProtocol::u8ServerUpdate == 3)
			{
				CUdpProtocol::u8ServerUpdate = 0;
				
				u8 temp_ip[4];
				u16 temp_port;
				g_CSystem.GetUsingServerIP(temp_ip);// ��ȡ��ǰʹ�õķ�����IP
				temp_port = g_CSystem.GetUsingServerPort();// ��ȡ��ǰʹ�õķ������˿�
				g_CSystem.ModifyServerIP(temp_ip);// ����ȡ����IP����ΪĬ��IP
				g_CSystem.ModifyPort(temp_port);// ����ȡ���Ķ˿ڸ���ΪĬ�϶˿�
			}
			break;
		}
		case 8:
		{
            KillTimer(this,8);
            g_CGPRS.UP_Start_flag = 1;
            g_CGPRS.Send_UP_AT(); 
            IBeep m_IBeep;
            m_IBeep.OperateSuccess();//������
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
			//��ʱ��
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
                    //ִ��̬�ص�����
                    IncState = WAIT_RESPOND;
                    Instruction[Step].RunNum++;//ִ�д�����1
                    Dida = 0;//��ʼ��ʱ
                    if (Instruction[Step].CarryFunc(&Instruction[Step].Inc) == SENDFAIL)
                    {
                        //ָ��ִ��ʧ�ܻ�������δ���ͳɹ�
                        if (Instruction[Step].RunNum <= Instruction[Step].Retry)
                        {
                            //��������
                            IncState = CARRY_OUT;
							if((Step == CIPSENDIndex)&&(ModelType == EC20F))
							{
								ComSend();
							}
							SetTimer(11, 100, this);// 1s֮�����·���
                        }
                        else
                        {
                            //���ԵĴ��������޶�����
                            IncState = CARRY_OUT;
                            Instruction[Step].RunNum = 0;
                            if( (Step == UPGRADE_KEY || Step == UPGRADE ||Step == VERSION)&& ModelType == AIR_720)
                            {
                                KillTimer(this,11);
                                Step = StepMaxIndex;//�ͷſ� 
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
                        //ָ�û�з�����Ϣ
                        if  (Dida < Instruction[Step].TimeOut)
                        {
                            ;//ָ��û�г�ʱ�����ȴ�
                        }
                        else
                        {
                            //ָ��ȴ���ʱ���ж��Ƿ�����
                            if (Instruction[Step].RunNum <= Instruction[Step].Retry)
                            {
                                //��������
                                IncState = CARRY_OUT;
                                SetTimer(11, 100, this);// 1s֮�����·���
                            }
                            else
                            {
                                //���ԵĴ��������޶�����
                                IncState = CARRY_OUT;
                                Instruction[Step].RunNum = 0;
                                if((Step == UPGRADE_KEY || Step == UPGRADE ||Step == VERSION)&&ModelType == AIR_720)
                                {
                                    KillTimer(this,11);
                                    Step = StepMaxIndex;//�ͷſ�                                   
                                }
                                else
                                    ExceptionHandle(&ErrorState);
                            }								
                        }
					}
					else if (ret == RCVRIGHT)
					{
						//ָ���յ��˺Ϸ��ķ���
						IncState = CARRY_OUT;
						Instruction[Step].RunNum = 0;
						if (Step < MAXINSNUM && GPRSState == CONNECTING)
						{
							//����δ����˵��������ָ��
							//����ָ����Ҫ����ִ����һ��
							Step++;	
						}
						else
						{
							//���������״���õ������
							//ִ�е�ָ���ִ�е���
							//������ִ����һ��
							Step = StepMaxIndex;//û��ָ����ռ�ÿ���
							KillTimer(this, 11);
						}
					}
					else
					{
						//ָ���յ��˷Ƿ��ķ���
						if (Instruction[Step].RunNum <= Instruction[Step].Retry)
						{
							//��������
							IncState = CARRY_OUT;
							SetTimer(11, 100, this);// 1s֮�����·���
						}
						else
						{
							//���ԵĴ��������޶�����
							IncState = CARRY_OUT;
							Instruction[Step].RunNum = 0;
                             if((Step == UPGRADE_KEY || Step == UPGRADE ||Step == VERSION)&& ModelType == AIR_720)
                            {
                                KillTimer(this,11);
                                Step = StepMaxIndex;//�ͷſ� 
                            }
                            else
                                ExceptionHandle(&ErrorState);
                        }	
                        
                    }
                }
            }
            else
            {
                //û��ָ�����ִ����
                Step = StepMaxIndex;//�ͷſ�
                KillTimer(this, 11);
            }
            break;
		}
		case 12:
		{   
            if(g_CSystem.Air_Upflag != 1 && UP_Start_flag != 1)
                ExceptionHandle_Unsolicite(&ErrorState); //��ʱ���GPRS ��û�г���
			break;
		}
		case 13:
		{
            if(g_CSystem.Start_flag == 1)
            {
                if( ++UP_Timer > 80)//8���һ��,11���Ӽ��
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
			//ֻ��N720ģ������£��δ������Ч
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
			CarryBlock(CIPSTARTIndex-1, 1);//�ӹر����ӿ�ʼ
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
�� �� ����RCV_DataProcess
����������������һ֡���ݺ�Ĵ���
��ڲ�������
���ڲ�������
**************************************************************************/
void CGPRS::RCV_DataProcess(void)
{
	//�˴���𴮿��յ����Ǳ��ģ������쳣������at
	if (GPRSState == ALREADYCONNECTED || g_CSystem.Air_Upflag == 1)
	{
		if (ModelType == NEO_N720)
		{
			//ֻ�������Ӻ���п����յ�����
			u8 TempBuff[RCV_MAX] = {0};
			char Mark[]="$MYURCREAD:";//����и��Ӵ���Ϊ��������
			char *pMsg = NULL;
			memcpy(TempBuff, ComRcv.Data, ComRcv.Len);
			pMsg = strstr((char*)TempBuff, Mark);
			if (pMsg != NULL)
			{
				//�����ݵݽ����ϲ㽫com�������
				memset(ComRcv.Data, 0, ComRcv.Len); //��ȡ�����մ��ڻ���������
				ComRcv.Len = 0;
				ComRequest= FALSE;//ATָ������Ӧ
				if (CarryBlock(NetReadIndex, 1) != 0)//ģ������ȥ��ȡ�յ�������
				{
					//��ȡָ��ʧ�� ������Ϊ��ok ��MYURCREAD ճ�ӣ����ڴ����жϷ��ͷ���ok�����У�
					Step = StepMaxIndex;
					CarryBlock(NetReadIndex, 1);//ǿ����ȡ
				}
			}
			else 
			{
				//����������ϱ����쳣�����ﴦ��
				if (UnsolicitedHandle(pMsg) == 0)
				{
					ComRequest= TRUE;//����at���ؽ�����Ӧ
				}
			}	
		}
		else if (ModelType == SIM800C)
		{
			//ֻ�������Ӻ���п����յ�����
			u8 TempBuff[RCV_MAX] = {0};
			char Mark[]="+IPD";//����и��Ӵ���Ϊ��������
			char *pMsg = NULL;
			memcpy(TempBuff, ComRcv.Data, ComRcv.Len);
			pMsg = strstr((char*)TempBuff, Mark);
			if (pMsg != NULL)
			{
				//�������ݻ�ȡ���ĳ���
				char *pStart = NULL;
				char *pEnd = NULL;
				char LenChar[5] = {0};//rcv_max ���160
				u16 LenHex = 0;
				pStart = strchr(pMsg, ',');
				pEnd = strchr(pMsg, ':');
				if ((pEnd-pStart-1) < 4)//���ݳ���3λ��
				{
					memcpy(LenChar, pStart+1, pEnd-pStart-1);//��ȡ���ݳ���
					LenHex = atoi(LenChar);
					if (LenHex < RCV_MAX_LEN)
					{
						memcpy(m_ReceiveFrame.Data, pEnd+1, LenHex);//ƫ���޸�
						m_ReceiveFrame.Len = LenHex;
						m_Request = TRUE;//֪ͨ�ϲ���д���
						ComRequest= FALSE;//ATָ������Ӧ
					}
				}
				//�����ݵݽ����ϲ㽫com�������
				memset(ComRcv.Data, 0, ComRcv.Len); //��ȡ�����մ��ڻ���������
				ComRcv.Len = 0;
			}
			else
			{
				//����������ϱ����쳣�����ﴦ��
				if (UnsolicitedHandle(pMsg) == 0)
				{
					ComRequest= TRUE;//����at���ؽ�����Ӧ
				}
			}	
		}
		else if (ModelType == AIR_720)
		{
			//ֻ�������Ӻ���п����յ�����
			u8 TempBuff[RCV_MAX] = {0};
			char Mark[]= "+IPD";//����и��Ӵ���Ϊ��������
            char Install[] = "+UPGRADEDL";//����и��Ӵ���Ϊ��������
            char Header[] = "+UPGRADE";//����и��Ӵ���Ϊ��������
            char New_Header[] = "+UPGRADEIND";//����и��Ӵ���Ϊ��������
            char New_UP_error[] = "+UPGRADEIND: -";//
            //char UP_error[] = "+CME ERROR:";//��������
            char UP_Ready[] = "RDY";//������ɺ�ģ���������
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
				//�������ݻ�ȡ���ĳ���
				char *pStart = NULL;
				char *pEnd = NULL;
				char LenChar[5] = {0};//rcv_max ���160
				u16 LenHex = 0;
				pStart = strchr(pMsg, ',');
				pEnd = strchr(pMsg, ':');
				if ((pEnd-pStart-1) < 4)//���ݳ���3λ��
				{
					memcpy(LenChar, pStart+1, pEnd-pStart-1);//��ȡ���ݳ���
					LenHex = atoi(LenChar);
					if (LenHex < RCV_MAX_LEN)
					{
						memcpy(m_ReceiveFrame.Data, pEnd+1, LenHex);//ƫ���޸�
						m_ReceiveFrame.Len = LenHex;
						m_Request = TRUE;//֪ͨ�ϲ���д���
						ComRequest= FALSE;//ATָ������Ӧ
					}
				}
                if(g_CSystem.Air_Upflag == 1)
                {
                    UP_Start_flag = 0;
                    g_CSystem.WriteUpflag(0);//д������־
                    g_CSystem.Air_Upflag = 0;
                }
				//�����ݵݽ����ϲ㽫com�������
				memset(ComRcv.Data, 0, ComRcv.Len); //��ȡ�����մ��ڻ���������
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
                    SetTimer(4, 300, this);//��������ʾ5��
                }
                memset(ComRcv.Data, 0, ComRcv.Len); //��ȡ�����մ��ڻ���������
                ComRcv.Len = 0;
            }
            else if(INSTALL != NULL && g_CSystem.Air_Upflag==1)
            {
                IBeep m_IBeep;
                m_IBeep.OperateSuccess();//������ 
                char *pStart = NULL;
				char *pEnd = NULL;
                char LenChar[5] = {'\0'};
                u16 progress = 0;
                pStart = strchr(INSTALL, ':');
                pEnd = strrchr(INSTALL,'0');                               
                if(pEnd!=NULL && pStart!=NULL && pEnd - pStart < 5)
                {
                    memcpy(LenChar, pStart+2, pEnd-pStart-1);//��ȡ��������
                    progress = atoi(LenChar);
                    Progress = progress;    
                    m_IBeep.OperateSuccess();//������
                    if(Progress == 100)//�������
                    {                     
                      m_IBeep.OperateSuccess();//������
                      g_CSystem.Air_Upflag = 0;
                      g_CSystem.WriteUpflag(0);//д������־
                      UP_Start_flag = 0;
                      Reboot();  
                    }
                }
                ComRequest= TRUE;
                memset(ComRcv.Data, 0, ComRcv.Len); //��ȡ�����մ��ڻ���������
				ComRcv.Len = 0;
            }
            else if(New_UP_pMsg != NULL)
            {
                //��������
                char *pStart = NULL;
				char *pEnd = NULL;
                char LenChar[5] = {'\0'};
                u16 progress = 0;
                pStart = strchr(New_UP_pMsg, ':');
                pEnd = strrchr(New_UP_pMsg,'0');                               
                if(pEnd!=NULL && pStart!=NULL && pEnd - pStart < 5)
                {
                    IBeep m_IBeep;
                    memcpy(LenChar, pStart+2, pEnd-pStart-1);//��ȡ��������
                    progress = atoi(LenChar);
                    Progress = progress;    
                    m_IBeep.OperateSuccess();//������
                    if(Progress == 100)//�������
                    {                     
                      m_IBeep.OperateSuccess();//������
                      UP_Finsh = 1; 
                      g_CSystem.Air_Upflag = 1;
                      g_CSystem.WriteUpflag(1);//д������־
                      UP_Start_flag = 0;
                      UP_Reset();   
                    }
                }
                ComRequest= TRUE;
                memset(ComRcv.Data, 0, ComRcv.Len); //��ȡ�����մ��ڻ���������
				ComRcv.Len = 0;
            }
            else if(UP_pMsg != NULL)
            {
                //��������
                char *pStart = NULL;
				char *pEnd = NULL;
                char LenChar[5] = {'\0'};
                u16 progress = 0;
                pStart = strchr(UP_pMsg, ':');
                pEnd = strrchr(UP_pMsg,'0');
                if(pEnd!=NULL && pStart!=NULL && pEnd - pStart < 5)
                {
                    memcpy(LenChar, pStart+2, pEnd-pStart-1);//��ȡ��������
                    progress = atoi(LenChar);
                    Progress = progress;     
                    IBeep m_IBeep;
                    m_IBeep.OperateSuccess();//������
                    if(Progress == 100)//�������
                    {                                              
                        m_IBeep.OperateSuccess();//������                                                
                        UP_Finsh = 1;
                        g_CSystem.Air_Upflag = 1;
                        g_CSystem.WriteUpflag(1);//д������־
                        UP_Start_flag = 0;
                        UP_Reset();
                    }
                }
                ComRequest= TRUE;
                memset(ComRcv.Data, 0, ComRcv.Len); //��ȡ�����մ��ڻ���������
				ComRcv.Len = 0;
            }            
            /*else if(UP_E_pMsg != NULL )//��������
            {
                IDisplay temp_IDisplay;
                temp_IDisplay.Dis_ErrCode(10, ERR_Air720_UP);
                UP_Start_flag = 0;           
                ComRequest= TRUE;
                memset(ComRcv.Data, 0, ComRcv.Len); //��ȡ�����մ��ڻ���������
				ComRcv.Len = 0;
                //SetTimer(4, 300, this);//��������ʾ5��
            }*/
            else if( UP_RDY != NULL)
            {
                memset(ComRcv.Data, 0, ComRcv.Len); //��ȡ�����մ��ڻ���������               
				ComRcv.Len = 0;
                if(g_CSystem.Air_Upflag == 1)
                {
                    UP_Start_flag = 0;
                    g_CSystem.WriteUpflag(0);//д������־
                    g_CSystem.Air_Upflag = 0;
                    Reboot();
                }
            }
			else
			{
				//����������ϱ����쳣�����ﴦ��
				if (UnsolicitedHandle(pMsg) == 0)
				{
					ComRequest= TRUE;//����at���ؽ�����Ӧ
				}
                if( g_CSystem.Air_Upflag == 1)
                {
                    memset(ComRcv.Data, 0, ComRcv.Len); //��ȡ�����մ��ڻ���������               
                    ComRcv.Len = 0;
                }
			}
		}
#ifdef EC20F_TRACE //������ֲ�����޸�EC20F
		else if (ModelType == EC20F)
		{
			//ֻ�������Ӻ���п����յ�����
			u8 TempBuff[RCV_MAX] = {0};
			char Mark[] = "+QIURC:"; //����и��Ӵ���Ϊ��������
			char *pMsg = NULL;
			memcpy(TempBuff, ComRcv.Data, ComRcv.Len);
			pMsg = strstr((char *)TempBuff, Mark);
			if (pMsg != NULL)
			{
				//�������ݻ�ȡ���ĳ���
				char *pStart = NULL;
				char *pEnd = NULL;
				char LenChar[5] = {0}; //rcv_max ���160
				u16 LenHex = 0;
				pStart = strchr(pMsg, ',')+2;
				pEnd = strchr(pMsg, '\r');
//                Dprintf(FALSE,"ERROR:","step 0:%d\r\n",pEnd - pStart - 1); 
				if ((pEnd - pStart - 1) < 4 && (pEnd - pStart - 1)>=0) //���ݳ���3λ��
				{
					memcpy(LenChar, pStart + 1, pEnd - pStart - 1); //��ȡ���ݳ���
					LenHex = atoi(LenChar);
					if (LenHex < RCV_MAX_LEN)
					{
						memcpy(m_ReceiveFrame.Data, pEnd + 2, LenHex); //ƫ���޸�
						m_ReceiveFrame.Len = LenHex;
						m_Request = TRUE;   //֪ͨ�ϲ���д���
						ComRequest = FALSE; //ATָ������Ӧ
					}
				}
				//�����ݵݽ����ϲ㽫com�������
				memset(ComRcv.Data, 0, ComRcv.Len); //��ȡ�����մ��ڻ���������
				ComRcv.Len = 0;
			}
			else
			{
				//����������ϱ����쳣�����ﴦ��
				if (UnsolicitedHandle(pMsg) == 0)
				{
					ComRequest= TRUE;//����at���ؽ�����Ӧ
				}
			}
        }
#endif        
	}
	else
	{
		ComRequest = TRUE;//δ���ӵ�ʱ������ָ���at����
	}
}

/**************************************************************************
�� �� ����SetTimeout_Ms
������������ȡ��Ϣ��ʱʱ���趨
��ڲ�������ʱʱ�� ���ֵ1864ms
���ڲ�������
**************************************************************************/
void CGPRS::SetTimeout_Ms(u16 nms)
{
	//systick�õ���ϵͳʱ�ӵ�1/8
	//ϵͳʱ����72M����systick��9M
	//��1s�ɼ���9M��1us����9��load ��24bit������
	//load ���ֵ��0xff ffff �������Լ���0xff ffff/9 = 1864 135us
	SysTick->LOAD=nms*9000; //ʱ�����
	SysTick->VAL=0x00;        //��ռ�����
	SysTick->CTRL=0x01 ;      //��ʼ����
}

/**************************************************************************
�� �� ����IfTimeout
�����������Ƿ�ﵽ��ʱʱ��
��ڲ�������
���ڲ�������
**************************************************************************/
u8 CGPRS::IfTimeout(void)
{
	u32 temp;
	temp=SysTick->CTRL;
	if (temp&0x01&&!(temp&(1<<16)))
	{
		return 1;//����δ���
	}
	else
	{
		//CTRL bit16 Ϊ1��ʾ�������
		SysTick->CTRL=0x00;       //�رռ�����
		SysTick->VAL =0X00;       //��ռ�����
		return 0;
	}
}


void CGPRS::ResetHeartBeatTimer(void)
{
	SetTimer(3, 1000, this);//����/ע��֡��ʱ��(20���һ��)
}

//�麯����GPRS�̳еĴ˺���Ϊ��
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

//ֱ�ӷ����Ѿ���ð�������
u8 CGPRS::SendData(u16 len, u8 *pData)
{
	//if (m_LoginFlag == 0xff)
		//return 4; //ע��ʧ�ܲ�����
	if (GPRSState == ALREADYCONNECTED)
	{	
		if (CarryBlock(CIPSENDIndex, 8))//��ģ��ͨ��ָ�����ռ
		{         
			//�鱻ռ��û������
			return 4;
		}
		std::ostringstream s;
#ifdef EC20F_TRACE //������ֲ�����޸�EC20F
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
			while(IFComBuffEmpty())//�ȴ�cipsend=x ָ������
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

//���緢������
u8 CGPRS::powoff_SendData(u16 len, u8 *pData)
{
    if (GPRSState == ALREADYCONNECTED)
    {      
        KillTimer(this);//�ر�GPRS�Ķ�ʱ������
        Step = CIPSENDIndex;//�±�ָ��������
        memset(ComRcv.Data, 0, ComRcv.Len); //��մ��ڻ���������
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
			while(IFComBuffEmpty())//�ȴ�cipsend=x ָ������
			{;}
            memset(ComSnd.Data, 0, SND_MAX); //��մ��ڷ��ͻ���������
			ComSnd.Len = len;
            memcpy(ComSnd.Data, pData, len);//�����������ݵ����ڷ��ͻ���
            g_CSystem.delay_nms(5);//��ʱ�ȴ�ģ���">"
			ComBuffOut();//����
            Step = StepMaxIndex;//û��ָ����ռ�ÿ���;
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
	�� �� ����USART3_IRQHandler
	���������������жϷ������
	��ڲ�������
	���ڲ�������
	**************************************************************************/
	void UART5_IRQHandler(void)
	{
		while(1)
		{
			if (UART5->SR & (1 << 5))	//�����ж�
			{
				UART5->SR &= ~(1 << 5);
				if (0 == g_CGPRS.ComRcv.Len)
					TIM2->CR1|=0x01;//ʹ�ܶ�ʱ��2
				else
					TIM2->CNT = 0;//��װ��ֵ
				if (g_CGPRS.ComRcv.Len < RCV_MAX)//�����ֵ
				{
					g_CGPRS.ComRcv.Data[g_CGPRS.ComRcv.Len++] = UART5->DR;
				}
				else
				{
					TIM2->CNT = 0;//��װ��ֵ
				}
			}
			else if (UART5->SR & (1 << 6))	//�����ж�
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
			{	//��ʱΪ�쳣(ע�������ܽ��ȫ�������жϵ����)//ksh_test
				//UART5->SR = 0x80;
				//UART5->DR &= (u16)0x01ff;
				break;
			}
		}

	}

	void TIM2_IRQHandler(void)
	{
		if (TIM2->SR & 0x0001)//����ж�
		{	
			TIM2->CR1 &= 0xfffe;//���ܶ�ʱ��2
			//�ֽڳ�ʱ
			if(FALSE == bTimerFirstArrive)
			{
				g_CGPRS.RCV_DataProcess();
				TIM2->CNT = 0;//��װ��ֵ
			}
			else
			{
				bTimerFirstArrive = FALSE;//��һ���ϵ�����жϲ��ǳ�ʱ�����Զ������
				TIM2->CNT = 0;//��װ��ֵ
			}
		}
		TIM2->SR &= ~(1 << 0);//����жϱ�־λ
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
		return 1;//���Ȳ��Ϸ�
	}
}

//�����ڻ����������ݷ��ͳ�ȥ
void CGPRS::ComBuffOut(void)
{
	ComSnd.HaveSndLen = 1;
	UART5->DR = ComSnd.Data[0];	
}

//0 ���ڻ���û�б�ռ�ÿ��Է��� 1����ʹ��
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
	memset(ComRcv.Data, 0, ComRcv.Len); //��ȡ�����մ��ڻ���������
	ComRcv.Len = 0;
}

void CGPRS::Read(char *strchar)
{
	ComRequest = FALSE;
	memcpy(strchar,ComRcv.Data, ComRcv.Len);
	memset(ComRcv.Data, 0, ComRcv.Len); //��ȡ�����մ��ڻ���������
	ComRcv.Len = 0;
}

void CGPRS::Reboot(void)
{
    if(g_CGPRS.UP_Start_flag == 0)
        Init_Reset();
}

void CGPRS::ExceptionHandle(u8 *ErrorCode)
{
	//�ڿ�ִ�д���󶼻����˺���
	//������ִ�����?
	Step = StepMaxIndex;
	switch(*ErrorCode)
	{
		case ERROR_AT:
		case ERROR_OK:
		case ERROR_CCID:
		case ERROR_MODEL:
		{
			//ͬ��������ʧ�� ������ccidʧ�� ����Ϊ��2G ģ��
			if(ModeSwitchLock==0)   //���ģʽ��δ��������ȡCCID���ɹ������л�����
	        {
#ifdef EC20F_TRACE //������ֲ�����޸�EC20F
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
		case ERROR_ATE0: //���� 
		case ERROR_CPIN: //pin���趨 
		case ERROR_CREG://ע��״̬	
		case ERROR_CGATT://����״̬
			m_GprsWrong = ERROR_MODLE; //֮ǰ�Ĵ���˵��ģ���������//����������ip�л�
		case ERROR_CSTT://����apn
		case ERROR_CIICR://������·	
		case ERROR_CIFSR	://��ȡip
		case ERROR_CIPSHUT: //�ر�����
		case ERROR_CIPHEAD://���ļ�ͷ
		case ERROR_NORMALSEND://ָ���
		case ERROR_CIPSTART://������
		{
			Reboot();
		}
		case ERROR_CIPSEND://��������
		case ERROR_NOARROW://�ղ���"<"�ݲ�����
		case ERROR_CSQ://�ź�ǿ�����Ҳ�����
        break;
		default:
			break;
	}
	*ErrorCode = 0;
}

//ģ�������ϱ��Ĵ�����Ӧ
//�жϻ��⵽ģ�������ϱ��Ĵ��󣬵����ж�
//�в��ܵ���settimer֮��Ĳ��������Ҫ��ѯ
void CGPRS::ExceptionHandle_Unsolicite(u8 *ErrorCode)
{
	if (*ErrorCode == 0)
	{
		return;
	}
	switch(*ErrorCode)
	{
		case ERROR_DEACT://�ϱ�����
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
		char Mark[]="$MYURCCLOSE";//���ӶϿ���
		if (strstr(pData, Mark) != NULL)	
		{
			ErrorState = ERROR_DEACT;
			return 1;
		}
	}
	else if (ModelType == SIM800C)
	{
		char Mark[]="+PDP DEACT";//���ӶϿ���
		if (strstr(pData, Mark) != NULL)	
		{
			ErrorState = ERROR_DEACT;
			return 1;
		}
	}
	else if (ModelType == AIR_720)
	{
		char Mark[]="+PDP DEACT";//���ӶϿ���
		if (strstr(pData, Mark) != NULL)	
		{
			ErrorState = ERROR_DEACT;
			return 1;
		}
	}
	return 0;//û����֪�쳣
}

//����ִ��ĳ�������
//StepNum :��� StartTime����ʱ�� OverTime ��ʱ
u8 CGPRS::CarryBlock(u8 StepNum, u8 StartTime)
{
	if (Step == StepMaxIndex)
	{
		//��û�б�ռ��
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
//��ռʽִ��pre = 1
//��������/�����ظ�
//������ռ��ʱ��11�ķ�������
/*u8 CGPRS::CarryBlock(u8 StepNum, u8 StartTime, u8 Pre)
{	
	if (Pre == 1)
	{
		Step = StepNum; //��ռʽ
		SetTimer(11, StartTime, this);
		IncState = CARRY_OUT;
		return 0;	
	}
}*/
u8 CGPRS::PowerOn_CarryCB(string *Inc)
{
	if (ModelType == NEO_N720)
		NEO_N720_4GPWR_L; //ģ��ϵ�
	else if (ModelType == SIM800C)
		SIM800C_2GPWR_L;
	else if (ModelType == AIR_720)
		AIR_720_4GPWR_L;
#ifdef EC20F_TRACE //������ֲ�����޸�EC20F
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
		NEO_N720_4GPWR_H;//ģ���ϵ�
	else if (ModelType == SIM800C)
		SIM800C_2GPWR_H;
	else if (ModelType == AIR_720)
		AIR_720_4GPWR_H;
#ifdef EC20F_TRACE //������ֲ�����޸�EC20F
	else if (ModelType == EC20F)
		EC20F_4GPWR_H;
#endif
	pthis->SetTimer(11, 200, pthis);
	return RCVRIGHT;
}

u8 CGPRS::PowerOff_CarryCB(string *Inc)
{
	if (ModelType == NEO_N720)
		GPRS_MODEL_ON_L;  //����ػ�
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
		NEO_N720_4GPWR_L; //ģ��ϵ�
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
		GPRS_MODEL_ON_L;//���������ź�
        pthis->SetTimer(11, 150, pthis);//�����ź����ٳ���200	  
    }
	else if(ModelType == SIM800C)
    {
		GPRS_MODEL_ON_H;// 2G �ĵ�·��Ʋ�ͬ
        pthis->SetTimer(11, 150, pthis);//�����ź����ٳ���200	  
    }
	else if(ModelType == AIR_720)
    {
		GPRS_MODEL_ON_L;
        pthis->SetTimer(11, 300, pthis);//�����ź�1~1.5s//2018 125->300
    }
#ifdef EC20F_TRACE //������ֲ�����޸�EC20F
	else if (ModelType == EC20F)
	{
		GPRS_MODEL_ON_L;
		pthis->SetTimer(11, 125, pthis); //�����ź�1~1.5s
	}
#endif
	return SENDOK;
}
 
u8 CGPRS::ModelOn_WaitCB(void)
{
	if (ModelType == NEO_N720)
		GPRS_MODEL_ON_H;//���������ź�
	else if(ModelType == SIM800C)
		GPRS_MODEL_ON_L;
	else if(ModelType == AIR_720)
		GPRS_MODEL_ON_H;
#ifdef EC20F_TRACE //������ֲ�����޸�EC20F
	else if (ModelType == EC20F)
	{
		GPRS_MODEL_ON_H;//�ӿ������ٶ�
		pthis->SetTimer(11, 100, pthis); //ģ������10s���ٷ�ָ��
		return RCVRIGHT;
	}
#endif
	pthis->SetTimer(11, 1800, pthis);//ģ������18s���ٷ�ָ��
	return RCVRIGHT;
}

u8 CGPRS::InsSend_CB(string *Inc)
{
	if (pthis->Write(Inc) == 0)
	{
		pthis->SetTimer(11, 1, pthis);//ÿ10ms����Ƿ������ݷ���
		return SENDOK;
	}
	else
	{
		pthis->ErrorState = ERROR_NORMALSEND;
		return SENDFAIL;//����ʧ��
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

u8 CGPRS::OK_FIX(void)//2019ǿ�Ʒŷ���ok
{
    return RCVRIGHT;
}

u8 CGPRS::OK_CB(void)
{
	string ComTempBuf = "";
	string Result= "OK";
	//���ֺ����Ƿ���������
	if (pthis->ComRequest == TRUE)
	{
		pthis->Read(&ComTempBuf);
		if(ComTempBuf.find(Result) != string::npos)
		{//����ж��ɹ�
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
		//��û���յ��κ�����
		pthis->ErrorState = ERROR_OK;
		return RCVWAITING;
	}
}
#ifdef EC20F_TRACE
u8 CGPRS::QICLOSE_CB(void)
{
	string ComTempBuf = "";
	string Result = "OK";
	//���ֺ����Ƿ���������
	if (pthis->ComRequest == TRUE)
	{
		pthis->Read(&ComTempBuf);
		if (ComTempBuf.find(Result) != string::npos)
		{ //����ж��ɹ�
			
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
		//��û���յ��κ�����
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
	//���ֺ����Ƿ���������
	if (pthis->ComRequest == TRUE)
	{
		pthis->Read(&ComTempBuf);
		if(ComTempBuf.find(Result) != string::npos)
		{//����ж��ɹ�
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
		//��û���յ��κ�����
		pthis->ErrorState = ERROR_OK;
		return RCVWAITING;
	}
}

u8 CGPRS::AT_CB(void)
{
	string ComTempBuf = "";
	string Result= "OK";
	//���ֺ����Ƿ���������
//	if (pthis->ComRequest == TRUE)
//	{
		pthis->Read(&ComTempBuf);
		if(ComTempBuf.find(Result) != string::npos)
		{//����ж��ɹ�
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
		//��û���յ��κ�����
		pthis->ErrorState = ERROR_AT;
		return RCVWAITING;
	}*/
}

u8 CGPRS::CPIN_CB(void)
{
	string ComTempBuf = "";
	string Result= "+CPIN: READY";
    string Result2= "+CPIN:READY";
	//���ֺ����Ƿ���������
	if (pthis->ComRequest == TRUE)
	{
		pthis->Read(&ComTempBuf);
		if(ComTempBuf.find(Result) != string::npos || ComTempBuf.find(Result2) != string::npos)  
		{//����ж��ɹ�
            if(g_CSystem.Air_Upflag == 1)//2018
            {
                g_CGPRS.UP_Start_flag = 0;
                g_CSystem.WriteUpflag(0);//д������־
                g_CSystem.Air_Upflag = 0;
            }
			return RCVRIGHT;
		}
		else
		{
			//�˴����ص���Ϣ�����ж���
			pthis->ErrorState = ERROR_CPIN;
			return RCVWRONG;
		}
	}
	else
	{
		//��û���յ��κ�����
		pthis->ErrorState = ERROR_CPIN;
		return RCVWAITING;
	}
}

u8 CGPRS::CSQ_CB(void)
{
	string ComTempBuf = "";
	string Result= "+CSQ:";
	//���ֺ����Ƿ���������
	if (pthis->ComRequest == TRUE)
	{
		pthis->Read(&ComTempBuf);
		if(ComTempBuf.find(Result) != string::npos)
		{	
			//����ж��ɹ�
			//����֡�ĸ�ʽ
			//+CSQ: 29,0
			int IndexStart = ComTempBuf.find("CSQ");
			IndexStart += 5;//ȥ���ո��ƶ����ź�ǿ���ֶ���
			int IndexEnd = ComTempBuf.find(",",IndexStart);
			if ((IndexEnd-IndexStart) >= 3)
			{
				//�ź�ǿ���ֶ������99���������3��
				//�ַ���˵����ȡ������
				pthis->ErrorState = ERROR_CSQ;
				return RCVWRONG;
			}
			else
			{
				//�ҵ��ź�ǿ�ȵ�����ֶ�
				string CSQ = ComTempBuf.substr(IndexStart, IndexEnd-IndexStart);
				//���ź�ǿ��ת������
				pthis->Strengh = strtol((&CSQ)->c_str(), NULL, 10);
				if (pthis->Strengh >31)
				{
					//���ź�ǿ����31 
					//�������Ļ����99������û��������
					pthis->Strengh = 0;
				}
				return RCVRIGHT;
			}
		}
		else
		{
			//�˴����ص���Ϣ�����ж���
			pthis->ErrorState = ERROR_CSQ;
			return RCVWRONG;
		}
	}
	else
	{
		//��û���յ��κ�����
		pthis->ErrorState = ERROR_CSQ;
		return RCVWAITING;
	}
}

u8 CGPRS::VER_CB(void)
{
    char TempBuff[1024] = {0};
	char Result[] = "AirM2M_720";
    char *ver = NULL;
	//���ֺ����Ƿ���������
	if (pthis->ComRequest == TRUE)
	{
		pthis->Read(TempBuff);
        ver = strstr((char *)TempBuff, Result);
		if( ver != NULL )
		{	
			//����ж��ɹ�
			//����֡�ĸ�ʽ
            char *pStart = NULL;
            char *pEnd = NULL;
            char LenChar[10] = {'\0'};//rcv_max ���160
            u16 version = 0;
			pStart = strchr(ver, 'V');
			pEnd = strchr(ver, 'L');
            if (pEnd - pStart < 7 && pEnd-pStart > 0)//�Ϸ�����
            {
                //�ҵ��ź�ǿ�ȵ�����ֶ�
                memcpy(LenChar, pStart+1, pEnd-pStart-2);//��ȡ���ݳ���
                //���ź�ǿ��ת������
                version = atoi(LenChar);	
                pthis->Air_Ver = version;
                return RCVRIGHT;    
            }
            else
            {
                //�˴����ص���Ϣ�����ж���
                return RCVWRONG;
            }            
		}
        else
        {
            //�˴����ص���Ϣ�����ж���
            //����ϸ�����صĴ�������
            return RCVWRONG;
        }
    }
	else
	{
		//��û���յ��κ�����
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
    string Result4="+CREG: 2,1";//���÷���LAC��ICʱ��2,1
	//���ֺ����Ƿ���������
	if (pthis->ComRequest == TRUE)
	{
		pthis->Read(&ComTempBuf);
		if(ComTempBuf.find(Result) != string::npos || ComTempBuf.find(Result2) != string::npos|| \
          ComTempBuf.find(Result3) != string::npos || ComTempBuf.find(Result4) != string::npos)
		{//����ж��ɹ�
			ModeSwitchLock = 1;       //���ע��ɹ���������ģʽ�л���
			if(g_CSystem.Gprs_ModeType != ModelType)
			 {
			 	g_CSystem.WriteModelType(ModelType);//��ģ���ͺ�д��flash
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
			//�˴����ص���Ϣ�����ж���
			//����ϸ�����صĴ�������
			pthis->ErrorState = ERROR_CREG;
			return RCVWRONG;
		}
	}
	else
	{
		//��û���յ��κ�����
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
    string Result4 = "+CGREG: 2,1";//ec20��ȡ�����ʱ����2,1
	//���ֺ����Ƿ���������
	if (pthis->ComRequest == TRUE)
	{
		pthis->Read(&ComTempBuf);
		if (ComTempBuf.find(Result) != string::npos || ComTempBuf.find(Result2) != string::npos \
          || ComTempBuf.find(Result3) != string::npos || ComTempBuf.find(Result4) != string::npos)
		{						//����ж��ɹ�
			ModeSwitchLock = 1; //���ע��ɹ���������ģʽ�л���
            
            string temp_id = ComTempBuf.substr(15,4);
            strncpy((char*)pthis->m_Lac,temp_id.c_str(),4);
            
            temp_id = ComTempBuf.substr(22,7);
            strncpy((char*)pthis->m_Id,temp_id.c_str(),7); 
			return RCVRIGHT;
		}
		else
		{
			//�˴����ص���Ϣ�����ж���
			//����ϸ�����صĴ�������
			pthis->ErrorState = ERROR_CREG;
			return RCVWRONG;
		}
	}
	else
	{
		//��û���յ��κ�����
		pthis->ErrorState = ERROR_CREG;
		return RCVWAITING;
	}
}
#endif
u8 CGPRS::CGATT_CB(void)
{
	string ComTempBuf = "";
	string Result= "+CGATT: 1";
	//���ֺ����Ƿ���������
	if (pthis->ComRequest == TRUE)
	{
		pthis->Read(&ComTempBuf);
		if(ComTempBuf.find(Result) != string::npos)
		{//����ж��ɹ�
			return RCVRIGHT;
		}
		else
		{
			//�˴����ص���Ϣ�����ж���
			//����ϸ�����صĴ�������
			pthis->ErrorState = ERROR_CGATT;
			return RCVWRONG;
		}
	}
	else
	{
		//��û���յ��κ�����
		pthis->ErrorState = ERROR_CGATT;
		return RCVWAITING;
	}
}

u8 CGPRS::CCID_CB(void)
{
	string ComTempBuf = "";
	string Result= "OK";
	//���ֺ����Ƿ���������
	if (pthis->ComRequest == TRUE)
	{
		pthis->Read(&ComTempBuf);
		if(ComTempBuf.find(Result) != string::npos)
		{
			if (ModelType == NEO_N720)
			{
				int IndexEnd;
				int IndexStart;
				//����ж��ɹ�
				//ccid:��ʽ\r\n8.....9\r\n\r\nOK\r\n
				IndexStart = ComTempBuf.find("CCID");
				IndexStart += 6;//ָ��CCID �ֶ�
				IndexEnd = ComTempBuf.find("\r",IndexStart);
				if (IndexEnd-IndexStart == 20)
				{	  
					//���Ƚ���У��
					string tempid = ComTempBuf.substr(IndexStart, IndexEnd-IndexStart);
					strncpy(m_CCID,tempid.c_str(),20);
					//m_CCID �е���ĸͳһ�ô�д
					for (int i = 0; i<sizeof(m_CCID);i++)
					{
						m_CCID[i] = toupper(m_CCID[i]);
					}
					if (pthis->CompareICCID() == 0)
					{//���ϱ�iccid
						pthis->firstOn = 2;
						
						u8 temp_ip[4];
						u16 temp_port;
						char buff[50];
						g_CSystem.GetUsingServerIP(temp_ip);
						temp_port = g_CSystem.GetUsingServerPort();
						sprintf(buff, "AT$MYNETSRV=0,0,2,0,\"%d.%d.%d.%d:%d\"\r\n", temp_ip[0], temp_ip[1], temp_ip[2], temp_ip[3], temp_port);
						pthis->Instruction[pthis->CIPSTARTIndex].Inc.assign(buff); //����udp����
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
				//����ж��ɹ�
				//ccid:��ʽ\r\n8.....9\r\n\r\nOK\r\n
				int IndexStart = ComTempBuf.find("\n");
				IndexStart += 1;//ָ��CCID �ֶ�
				int IndexEnd = ComTempBuf.find("\r",IndexStart);
				if (IndexEnd-IndexStart == 20)
				{	//���Ƚ���У��
					//memcpy(m_CCID,c_str(ComTempBuf+IndexStart), 20);
					string tempid = ComTempBuf.substr(IndexStart, IndexEnd-IndexStart);
					strncpy(m_CCID,tempid.c_str(),20);
					//m_CCID �е���ĸͳһ�ô�д
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
						pthis->Instruction[pthis->CIPSTARTIndex].Inc.assign(buff); //����udp����
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
				//����ж��ɹ�
				//ccid:��ʽ\r\n8.....9\r\n\r\nOK\r\n
				int IndexStart = ComTempBuf.find("\n");
				IndexStart += 9;//ָ��CCID �ֶ�
				int IndexEnd = ComTempBuf.find("\r",IndexStart);
				if (IndexEnd-IndexStart == 20)
				{	//���Ƚ���У��
					//memcpy(m_CCID,c_str(ComTempBuf+IndexStart), 20);
					string tempid = ComTempBuf.substr(IndexStart, IndexEnd-IndexStart);
					strncpy(m_CCID,tempid.c_str(),20);
					//m_CCID �е���ĸͳһ�ô�д
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
						pthis->Instruction[pthis->CIPSTARTIndex].Inc.assign(buff); //����udp����
					}
					return RCVRIGHT;
				}
				else
				{
					pthis->ErrorState = ERROR_CCID;
					return RCVWRONG;
				}
			}
#ifdef EC20F_TRACE //������ֲ�����޸�EC20F
			else if (ModelType == EC20F)
			{
				int IndexEnd;
				int IndexStart;
				//����ж��ɹ�
				//ccid:��ʽ\r\n8.....9\r\n\r\nOK\r\n
				IndexStart = ComTempBuf.find("QCCID");
				IndexStart += 7; //ָ��CCID �ֶ�
				IndexEnd = ComTempBuf.find("\r", IndexStart);
				if (IndexEnd - IndexStart == 20)
				{
					//���Ƚ���У��
					string tempid = ComTempBuf.substr(IndexStart, IndexEnd - IndexStart);
					strncpy(m_CCID, tempid.c_str(), 20);
					//m_CCID �е���ĸͳһ�ô�д
					for (int i = 0; i < sizeof(m_CCID); i++)
					{
						m_CCID[i] = toupper(m_CCID[i]);
					}
					if (pthis->CompareICCID() == 0 || g_CSystem.head_err_type&0x01)
					{ //���ϱ�iccid �� flash����
						pthis->firstOn = 2;

						u8 temp_ip[4];
						u16 temp_port;
						char buff[50];
						g_CSystem.GetUsingServerIP(temp_ip);
						temp_port = g_CSystem.GetUsingServerPort();
						sprintf(buff, "AT+QIOPEN=1,0,\"UDP\",\"%d.%d.%d.%d\",%d,0,1\r\n", temp_ip[0], temp_ip[1], temp_ip[2], temp_ip[3], temp_port);
						pthis->Instruction[pthis->CIPSTARTIndex].Inc.assign(buff); //����udp����
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
			//�˴����ص���Ϣ�����ж���
			//����ϸ�����صĴ�������
			pthis->ErrorState = ERROR_CCID;
			return RCVWRONG;
		}
	}
	else
	{
		//��û���յ��κ�����
		pthis->ErrorState = ERROR_CCID;
		return RCVWAITING;
	}
        return RCVWAITING;
}

u8 CGPRS::CSTT_CB(void)
{
	string ComTempBuf = "";
	string Result= "OK";
	//���ֺ����Ƿ���������
	if (pthis->ComRequest == TRUE)
	{ 
		pthis->Read(&ComTempBuf);
		if(ComTempBuf.find(Result) != string::npos)
		{
			//����ж��ɹ�
			return RCVRIGHT;
		}
		else
		{
			//�˴����ص���Ϣ�����ж���
			//����ϸ�����صĴ�������
			pthis->ErrorState = ERROR_CSTT;
			return RCVWRONG;
		}
	}
	else
	{
		//��û���յ��κ�����
		pthis->ErrorState = ERROR_CSTT;
		return RCVWAITING;
	}
}

u8 CGPRS::NETACT_CB(void)
{
	string ComTempBuf = "";
	string Result= "."; // ip �ĵ�
	//���ֺ����Ƿ���������
	if (pthis->ComRequest == TRUE)
	{
		pthis->Read(&ComTempBuf);
		if(ComTempBuf.find(Result) != string::npos)
		{
			//����ж��ɹ�
			//������ip�洢����
			return RCVRIGHT;
		}
		else
		{
			//�˴����ص���Ϣ�����ж���
			//����ϸ�����صĴ�������
			pthis->ErrorState = ERROR_CIFSR;
			return RCVWRONG;
		}
	}
	else
	{
		//��û���յ��κ�����
		pthis->ErrorState = ERROR_CIFSR;
		return RCVWAITING;
	}
}

u8 CGPRS::NETCLOSE_CB(void)
{
	string ComTempBuf = "";
	//string Result= "."; // ip �ĵ�
	//���ֺ����Ƿ���������
	if (pthis->ComRequest == TRUE)
	{
		//�յ��������ɣ�����Ҫ�ж�����
		return RCVRIGHT;
	}
	else
	{
		//��û���յ��κ�����
		pthis->ErrorState = ERROR_CIFSR;
		return RCVWAITING;
	}
}
u8 CGPRS::CIFSR_CB(void)
{
	string ComTempBuf = "";
	string Result= "."; // ip �ĵ�
	//���ֺ����Ƿ���������
	if (pthis->ComRequest == TRUE)
	{
		pthis->Read(&ComTempBuf);
		if(ComTempBuf.find(Result) != string::npos)
		{
			//����ж��ɹ�
			//������ip�洢����
			return RCVRIGHT;
		}
		else
		{
			//�˴����ص���Ϣ�����ж���
			//����ϸ�����صĴ�������
			pthis->ErrorState = ERROR_CIFSR;
			return RCVWRONG;
		}
	}
	else
	{
		//��û���յ��κ�����
		pthis->ErrorState = ERROR_CIFSR;
		return RCVWAITING;
	}
}

u8 CGPRS::CIPSTART2G_CB(void)
{
	string ComTempBuf = "";
	string Result= "CONNECT OK";
	//���ֺ����Ƿ���������
	if (pthis->ComRequest == TRUE)
	{
		pthis->Read(&ComTempBuf);
		if(ComTempBuf.find(Result) != string::npos)
		{
			//����ж��ɹ�
			//������ip�洢����
			return RCVRIGHT;
		}
		else
		{
			//�˴����ص���Ϣ�����ж���
			//����ϸ�����صĴ�������
			pthis->ErrorState = ERROR_CIPSTART;
			return RCVWRONG;
		}
	}
	else
	{
		//��û���յ��κ�����
		pthis->ErrorState = ERROR_CIPSTART;
		return RCVWAITING;
	}
}

u8 CGPRS::CIPSTART_CB(void)
{
	string ComTempBuf = "";
	string Result= "OK";
	//���ֺ����Ƿ���������
	if (pthis->ComRequest == TRUE)
	{
		pthis->Read(&ComTempBuf);
		if(ComTempBuf.find(Result) != string::npos)
		{
			//����ж��ɹ�
			//������ip�洢����
			return RCVRIGHT;
		}
		else
		{
			//�˴����ص���Ϣ�����ж���
			//����ϸ�����صĴ�������
			pthis->ErrorState = ERROR_CIPSTART;
			return RCVWRONG;
		}
	}
	else
	{
		//��û���յ��κ�����
		pthis->ErrorState = ERROR_CIPSTART;
		return RCVWAITING;
	}
}
u8 CGPRS::NETOPEN_CB(void)
{
	string ComTempBuf = "";
	string Result= "OK";
	//���ֺ����Ƿ���������
	if (pthis->ComRequest == TRUE)
	{
		pthis->Read(&ComTempBuf);
		if(ComTempBuf.find(Result) != string::npos)
		{
			//����ж��ɹ�
			pthis->GPRSState = ALREADYCONNECTED;//ִ���������ɹ�����൱��   gprs ��ȫ�������
			pthis->SetTimer(3, 1000, pthis);//������ɺ�������
			pthis->SetTimer(7, 100, pthis);//�������ؼ�ⶨʱ��
			pthis->SetTimer(13, 800, pthis);//�ź�ǿ�ȶ�ʱ��
            pthis->SetTimer(5, 1500, pthis);//�����������//2019
            if(g_CGPRS.firstOn & 0x02)
            {
                g_COverallTest.StartSendTestInfo = 1;
            }
			g_CPowerCheck->set_ledstate((pthis->m_LoginFlag&0x80)?LED_FREQ_0_1:LED_FREQ_1);//�����ɹ����ȴ����
			g_CSystem.m_State &= ~0x08;//GPRS���ӳɹ�
            g_COverallTest.Comm_Sucess_flag = 1; 
			return RCVRIGHT;
		}
		else
		{
			//�˴����ص���Ϣ�����ж���
			//����ϸ�����صĴ�������
			pthis->ErrorState = ERROR_CIPHEAD;
			return RCVWRONG;
		}
	}
	else
	{
		//��û���յ��κ�����
		pthis->ErrorState = ERROR_CIPHEAD;
		return RCVWAITING;
	}
}

//CIMI��
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
                return RCVRIGHT;//Ҳ���سɹ�
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

//��ȡLAC��ID��
u8 CGPRS::LACID_CB(void)
{
	string ComTempBuf = "";
    string Result= "MYLACID";
    //���ֺ����Ƿ���������
    if (pthis->ComRequest == TRUE)
    { 
        pthis->Read(&ComTempBuf);
        if(ComTempBuf.find(Result) != string::npos)
        {
            //����ж��ɹ�
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
        //��û���յ��κ�����
        return RCVWAITING;
    }
}

u8 CGPRS::NETREAD_CB(void)
{
	char ComTempBuf[1024] = "";
	char Result[]= "$MYNETREAD"; 
	char *pMsg = NULL;
	u8 ret = 0;
	//���ֺ����Ƿ���������
	if (pthis->ComRequest == TRUE)
	{
		pthis->Read(ComTempBuf);
		pMsg = strstr(ComTempBuf, Result);
		if(pMsg != NULL)
		{
			//����ж��ɹ�
			//�������ݻ�ȡ���ĳ���
			char *pStart = NULL;
			char *pEnd = NULL;
			char LenChar[5] = {0};
			u16 LenHex = 0;
			pStart = strchr(pMsg, ',');
			pEnd = strchr(pMsg, '\r');
			if (pEnd - pStart < 5 && pEnd-pStart > 0)//�Ϸ�����
			{
				memcpy(LenChar, pStart+1, pEnd-pStart-1);//��ȡ���ݳ���
				LenHex = atoi(LenChar);
				if (LenHex ==0)
				{
					//KillTimer(pthis, 11); //һֱ����������������
					//g_CGPRS.m_Request = TRUE;
					//ret = RCVRIGHT;
					KillTimer(pthis,14);
					ret = RCVRIGHT;
				}
				else
				{
					memcpy(g_CGPRS.m_ReceiveFrame.Data, pEnd+2, LenHex);//ƫ���޸�
					g_CGPRS.m_ReceiveFrame.Len = LenHex;
					KillTimer(pthis, 11); 
					g_CGPRS.m_Request = TRUE;
					ret = RCVRIGHT;
					//���������ܻ�������
					SetTimer(14, 10, pthis);
					//g_CGPRS.m_Request = TRUE;//֪ͨ�ϲ���д���
					//ret = RCVWRONG;//Ϊ�˱��⻺��������������ָ���������
					//�ѻ����������ݶ���
				}
			}
			else
			{
				//�յ��Ķ������Ϸ�
				ret = RCVWRONG;//
			}	
		}
		else
		{
			//�˴����ص���Ϣ�����ж���
			//����ϸ�����صĴ�������
			pthis->ErrorState = ERROR_CIPSEND;
			ret = RCVWRONG;
		}
	}
	else
	{
		//��û���յ��κ�����
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
				//���Խ��з���
				if (pthis->IFComBuffEmpty() == 0)
				{
					pthis->ComBuffOut();
					pthis->SetTimer(11, 1, pthis);//ÿ10ms���һ���Ƿ��з���
					return SENDOK;
				}
				else 
				{
					return SENDFAIL;
				}
			}
			else
			{
				//�˴����ص���Ϣ�����ж���
				//����ϸ�����صĴ�����
				pthis->ErrorState = ERROR_CIPSEND;
				return SENDFAIL;
			}
		}
		else
		{
			//û���յ�">"��Ϊ����ʧ��
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
				//���Խ��з���
				if (pthis->IFComBuffEmpty() == 0)
				{
					pthis->ComBuffOut();
					pthis->SetTimer(11, 1, pthis);//ÿ10ms���һ���Ƿ��з���
					return SENDOK;
				}
				else 
				{
					return SENDFAIL;
				}
			}
			else
			{
				//�˴����ص���Ϣ�����ж���
				//����ϸ�����صĴ�����
				pthis->ErrorState = ERROR_CIPSEND;
				return SENDFAIL;
			}
		}
		else
		{
			//û���յ�">"��Ϊ����ʧ��
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
				//���Խ��з���
				if (pthis->IFComBuffEmpty() == 0)
				{
					pthis->ComBuffOut();
					pthis->SetTimer(11, 1, pthis);//ÿ10ms���һ���Ƿ��з���
					return SENDOK;
				}
				else 
				{
					return SENDFAIL;
				}
			}
			else
			{
				//�˴����ص���Ϣ�����ж���
				//����ϸ�����صĴ�����
				pthis->ErrorState = ERROR_CIPSEND;
				return SENDFAIL;
			}
		}
		else
		{
			//û���յ�">"��Ϊ����ʧ��
			pthis->ErrorState = ERROR_CIPSEND;
			return SENDFAIL;
		}
	}
#ifdef EC20F_TRACE //������ֲ�����޸�EC20F
	else if (ModelType == EC20F)
	{
		string ComTempBuf = "";
		string Result = ">";
		if (pthis->ComRequest == TRUE)
		{
			pthis->Read(&ComTempBuf);
			if (ComTempBuf.find(Result) != string::npos)
			{
				//���Խ��з���
				if (pthis->IFComBuffEmpty() == 0)
				{
					pthis->ComBuffOut();
					pthis->SetTimer(11, 1, pthis); //ÿ10ms���һ���Ƿ��з���
					return SENDOK;
				}
				else
				{
					return SENDFAIL;
				}
			}
			else
			{
				//�˴����ص���Ϣ�����ж���
				//����ϸ�����صĴ�����
				pthis->ErrorState = ERROR_CIPSEND;
				return SENDFAIL;
			}
		}
		else
		{
			//û���յ�">"��Ϊ����ʧ��
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
		//���ֺ����Ƿ���������
		if (pthis->ComRequest == TRUE)
		{
			pthis->Read(&ComTempBuf);
			if(ComTempBuf.find(Result) != string::npos)
			{
				//����ж��ɹ�
				KillTimer(pthis, 11); //����ָ��ִ�����֮����������Ҫע��
				ret = RCVRIGHT;
			}
			else
			{
				//�˴����ص���Ϣ�����ж���
				//����ϸ�����صĴ�������
				pthis->ErrorState = ERROR_CIPSEND;
				ret = RCVWRONG;
			}
		}
		else
		{
			//��û���յ��κ�����
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
		//���ֺ����Ƿ���������
		if (pthis->ComRequest == TRUE)
		{
			pthis->Read(&ComTempBuf);
			if(ComTempBuf.find(Result) != string::npos)
			{
				//����ж��ɹ�
				KillTimer(pthis, 11); //����ָ��ִ�����֮����������Ҫע��
				ret = RCVRIGHT;
			}
			else
			{
				//�˴����ص���Ϣ�����ж���
				//����ϸ�����صĴ�������
				pthis->ErrorState = ERROR_CIPSEND;
				ret = RCVWRONG;
			}
		}
		else
		{
			//��û���յ��κ�����
			pthis->ErrorState = ERROR_CIPSEND;
			ret = RCVWAITING;
		}
		return ret;
	}
	else if (ModelType == AIR_720)
	{
		string ComTempBuf = "";
		string Result= "SEND OK"; 
//        string Result= "DATA ACCEPT";//�췢ģʽ����ֵ
		u8 ret = 0;
		//���ֺ����Ƿ���������
		if (pthis->ComRequest == TRUE)
		{
			pthis->Read(&ComTempBuf);
			if(ComTempBuf.find(Result) != string::npos)
			{
				//����ж��ɹ�
				KillTimer(pthis, 11); //����ָ��ִ�����֮����������Ҫע��
				ret = RCVRIGHT;
			}
			else
			{
				//�˴����ص���Ϣ�����ж���
				//����ϸ�����صĴ�������
				pthis->ErrorState = ERROR_CIPSEND;
				ret = RCVWRONG;
			}
		}
		else
		{
			//��û���յ��κ�����
			pthis->ErrorState = ERROR_CIPSEND;
			ret = RCVWAITING;
		}
		return ret;
	}
#ifdef EC20F_TRACE //������ֲ�����޸�EC20F
	else if (ModelType == EC20F)
	{
		string ComTempBuf = "";
		string Result = "SEND OK";
		u8 ret = 0;
		//���ֺ����Ƿ���������
		if (pthis->ComRequest == TRUE)
		{
			pthis->Read(&ComTempBuf);
			if (ComTempBuf.find(Result) != string::npos)
			{
				//����ж��ɹ�
				KillTimer(pthis, 11); //����ָ��ִ�����֮����������Ҫע��
				ret = RCVRIGHT;
			}
			else
			{
				//�˴����ص���Ϣ�����ж���
				//����ϸ�����صĴ�������
				pthis->ErrorState = ERROR_CIPSEND;
				ret = RCVWRONG;
			}
		}
		else
		{
			//��û���յ��κ�����
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
	pthis->Step = pthis->ModelOnIndex-1;//�´�ִ��������
	return RCVRIGHT;
}

u8 CGPRS::Mysysinfo_CB(void)
{
	string ComTempBuf = "";
	string Result= "MYSYSINFO";
	//���ֺ����Ƿ���������
	if (pthis->ComRequest == TRUE)
	{
		pthis->Read(&ComTempBuf);
		if(ComTempBuf.find(Result) != string::npos)
		{ 
			return RCVRIGHT;
		}
		else
		{
			//�˴����ص���Ϣ�����ж���
			//����ϸ�����صĴ�������
			pthis->ErrorState = ERROR_MYSYSINFO;
			return RCVWRONG;
		}
	}
	else
	{
		//��û���յ��κ�����
		pthis->ErrorState = ERROR_MYSYSINFO;
		return RCVWAITING;
	}
}
u8 CGPRS::CIPSHUT_CB(void)
{
	string ComTempBuf = "";
	string Result= "SHUT OK";
	//���ֺ����Ƿ���������
	if (pthis->ComRequest == TRUE)
	{
		pthis->Read(&ComTempBuf);
		if(ComTempBuf.find(Result) != string::npos)
		{
			//����ж��ɹ�
			SetTimer(11, 1, pthis);
			pthis->Step = pthis->CSTTIndex-1; //������apn ���¿�ʼ����
			return RCVRIGHT;
		}
		else
		{
			//�˴����ص���Ϣ�����ж���
			//����ϸ�����صĴ�������
			pthis->ErrorState = ERROR_CIPSHUT;
			return RCVWRONG;
		}
	}
	else
	{
		//��û���յ��κ�����
		pthis->ErrorState = ERROR_CIPSHUT;
		return RCVWAITING;
	}
}

u8 CGPRS::CIPSTATUS_CB(void)
{
	string ComTempBuf = "";
	string Result= "CONNECT OK";
	//���ֺ����Ƿ���������
	if (pthis->ComRequest == TRUE)
	{
		pthis->Read(&ComTempBuf);
		if(ComTempBuf.find(Result) != string::npos)
		{
			//����ж��ɹ�
			return RCVRIGHT;
		}
		else
		{
			//�˴����ص���Ϣ�����ж���
			//����ϸ�����صĴ�������
			pthis->ErrorState = ERROR_CIPSTATUS;
			return RCVWRONG;
		}
	}
	else
	{
		//��û���յ��κ�����
		pthis->ErrorState = ERROR_CIPSTATUS;
		return RCVWAITING;
	}
}

u8 CGPRS::MODEL_CB(void) //�ͺ�ʶ��GPRS�����ͺŻص�ȷ����ǰʹ�õ����Ǹ�ģ��
{
	string ComTempBuf = "";
	string n720_Result= "N720";
	string sim800c_Result= "SIM800C";
	string air720_Old_Result= "Nezha_MIFI";
    string air720_Result= "Air720";
#ifdef EC20F_TRACE //������ֲ�����޸�EC20F
	string ec20f_Result = "EC20F";
#endif
	if(ModelType==NEO_N720)
	{
		
		//���ֺ����Ƿ���������
		if (pthis->ComRequest == TRUE)
		{
			pthis->Read(&ComTempBuf);
			if(ComTempBuf.find(n720_Result) != string::npos)
			{//����ж��ɹ�
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
			//��û���յ��κ�����
			pthis->ErrorState = ERROR_MODEL;
			return RCVWAITING;
		}
	}
	else if(ModelType==SIM800C)
	{
		//���ֺ����Ƿ���������
		if (pthis->ComRequest == TRUE)
		{
			pthis->Read(&ComTempBuf);
			if(ComTempBuf.find(sim800c_Result) != string::npos)
			{//����ж��ɹ�
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
			//��û���յ��κ�����
			pthis->ErrorState = ERROR_MODEL;
			return RCVWAITING;
		}
	}
	else if(ModelType==AIR_720)
	{
		//���ֺ����Ƿ���������
		if (pthis->ComRequest == TRUE)
		{
			pthis->Read(&ComTempBuf);
			if(ComTempBuf.find(air720_Result) != string::npos)
			{//����ж��ɹ�
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
			//��û���յ��κ�����
			pthis->ErrorState = ERROR_MODEL;
			return RCVWAITING;
		}
	}
#ifdef EC20F_TRACE //������ֲ�����޸�EC20F
	else if (ModelType == EC20F)
	{
		//���ֺ����Ƿ���������
		if (pthis->ComRequest == TRUE)
		{
			pthis->Read(&ComTempBuf);
			if (ComTempBuf.find(ec20f_Result) != string::npos)
			{ //����ж��ɹ�
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
			//��û���յ��κ�����
			pthis->ErrorState = ERROR_MODEL;
			return RCVWAITING;
		}
	}
#endif
    return RCVWAITING;
}
//�ֳֻ��ػ���ʱ����Ҫ��GPRS�ر�
void CGPRS::CloseGPRS(void)
{
	CarryBlock(ShutDownIndex, 1);
}

//�ϲ���û�ȡCCID
//�������:pCCID  ��CCID[0-21]���
//����ֵ:0 �ɹ� 1 ʧ��
u8  CGPRS::GetCCID(char *pCCID)
{	
	if (strlen(m_CCID) == 0)
	{
		return 1;//�մ�˵����δ��ȡ�����߻�ȡʧ��
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
	
	u8 ret = 1;//��ʼ״̬:�˴ο���ǰ��ICCID��һ��
	
	if(0 == GetCCID(current_iccid_buff))//��ȡICCID
	{	
		if(0 == GetOlderICCIDFromFlash(older_iccid_buff))//��ȡ�洢��flash�еĴ˴ο���ǰʹ�õ�ICCID
		{
			if(0 == memcmp(current_iccid_buff, older_iccid_buff, CCIDLEN))//�Ƚ������Ƿ�һ��
			{
				ret = 0;//���һ�£�����ֵ����Ϊ0
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

	if(0 == GetCCID(iccid_buff))// ��ȡ��ǰICCID
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
	if ((g_CSystem.m_Version == 4) || (g_CSystem.m_new_rn8209c_version == 1)) //��Ӳ���汾ADC���
		return 1;
	if(ModelType == SIM800C)
    {
        GPIO_InitTypeDef GPIO_InitStructure;
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;//Ӳ����û������
        GPIO_Init(GPIOE, &GPIO_InitStructure);
        GPIO_SetBits(GPIOE, GPIO_Pin_11);//Ĭ���ø�  
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
    Send_TestInfo();//�������������Ϣ
}

void CGPRS::SetModelType(u8 mode)
{
    ModelType = mode;
}

void CGPRS::send_poweroff(void)
{
    send_powoff_inf();    
}