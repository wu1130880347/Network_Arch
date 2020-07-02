#include "IUarts.h"
#include "FreeRTOS.h"					//FreeRTOSʹ��
#include "task.h"
#include "semphr.h"
//debug info output
#if DBGUART
extern "C"
{
#define DBG_USE 0

#if DBG_USE
    //�Ƿ�򿪸��ļ��ڵĵ���LOG
    static const char EN_LOG = YELLOW;
    //LOG����ļ����
    static const char TAG[] = "UartDrv: ";
#else
#define EN_LOG 0
#define TAG ""
#define Dprintf(...)
#endif
}

#endif

static SEND_FRAME U2_RnSend;
static RECEIVE_FRAME U2_RnRece;
static uint8_t U2_ReceFinsh = 0;
static uint8_t U2_SendFinsh = 0;
static uint16_t U2_HaveSenLen = 1;
static uint16_t U2_HaveReceLen = 0;
static uint16_t U2_max_rxbuf = 0;
static uint16_t U2_max_txbuf = 0;


IUarts::IUarts()
{}
IUarts::~IUarts()
{}
Net_Status_t IUarts::Detect(u16 *dev_id)
{
    return NET_OK;
}
Net_Status_t IUarts::Init(void *para)
{
    NetDrvUartStruct_t *t_para = (NetDrvUartStruct_t *)para;
    U2_RnRece.Data = t_para->rx_buffer;
    U2_RnSend.Data = t_para->tx_buffer;
    U2_max_rxbuf = t_para->rx_max;
    U2_max_txbuf = t_para->tx_max;
    //GPIO�˿�����
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    USART_DeInit(USART2);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);  //ʹ��GPIOAʱ��
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE); //ʹ��USART2ʱ��

    //����1��Ӧ���Ÿ���ӳ��
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2);  //GPIOA2��ΪUSAT2
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2); //GPIOA3��ΪUSAR2
    //USART1�˿�����
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3; //GPIOA9��GPIOA10
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;            //���ù���
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;      //�ٶ�50MHz
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;          //���츴�����
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;            //����
    GPIO_Init(GPIOA, &GPIO_InitStructure);                  //��ʼ��PA2��PA3

    //USART1 ��ʼ������
    USART_InitStructure.USART_BaudRate = t_para->baud_rate;                         //����������
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;                     //�ֳ�Ϊ8λ���ݸ�ʽ
    USART_InitStructure.USART_StopBits = USART_StopBits_1;                          //һ��ֹͣλ
    USART_InitStructure.USART_Parity = USART_Parity_No;                             //����żУ��λ
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; //��Ӳ������������
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;                 //�շ�ģʽ
    USART_Init(USART2, &USART_InitStructure);                                       //��ʼ������1

    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE); //���������ж�,RXNE���Ĵ����ǿ�
    USART_ITConfig(USART2, USART_IT_IDLE, ENABLE); //ʹ�ܽ���һ֡�����ж�
    USART_ITConfig(USART2, USART_IT_TC, ENABLE);   //���������ж�,TC�������
    USART_Cmd(USART2, ENABLE); //ʹ�ܴ���1

    //USART_ClearFlag(USART2, USART_FLAG_TC | USART_FLAG_IDLE | USART_FLAG_RXNE);

    //USART_ITConfig(USART2, USART_IT_RXNE | USART_IT_TC | USART_IT_IDLE, ENABLE); //��������ж�

    //Usart1 NVIC ����
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;         //����1�ж�ͨ��
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2; //��ռ���ȼ�3
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;        //�����ȼ�3
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;           //IRQͨ��ʹ��
    NVIC_Init(&NVIC_InitStructure);                           //����ָ���Ĳ�����ʼ��VIC�Ĵ���
    return NET_OK;
}

Net_Status_t IUarts::SendData(NetPackageStruct_t *dat)
{
    if(U2_SendFinsh)
    {
        Dprintf(EN_LOG,TAG,"Uart BUSY : UART2\r\n");
        return NET_BUSY;
    }
    if(dat->p_sendFrame->Len > U2_max_txbuf)
    {
        Dprintf(EN_LOG,TAG,"Uart Send Len Err : UART2\r\n");
        return NET_ERR;
    }
    memcpy(&U2_RnSend,dat->p_sendFrame,sizeof(SEND_FRAME));
    uint16_t t_len = U2_RnSend.Len;
    uint8_t *t_dat = U2_RnSend.Data;
    Dprintf(EN_LOG,TAG,"Uart send data len = %d\r\n",t_len);
    for(uint16_t i = 0;i<t_len;i++)
        Dprintf(EN_LOG,"","%02x ",t_dat[i]);
    Dprintf(EN_LOG,"","\r\n");

    USART_SendData(USART2,U2_RnSend.Data[0]);
    U2_SendFinsh = 1;
    return NET_OK;
}
Net_Status_t IUarts::ReceData(NetPackageStruct_t *dat)
{
    if(!U2_ReceFinsh)
    {
        U2_ReceFinsh = 1;
        U2_RnRece.Len = U2_HaveReceLen;
        U2_HaveReceLen = 0;
        memcpy(dat->p_receFrame,&U2_RnRece,sizeof(RECEIVE_FRAME));

        uint16_t t_len = U2_RnRece.Len;
        uint8_t *t_dat = U2_RnRece.Data;
        Dprintf(PINK, TAG, "Uart Rece data len = %d\r\n", t_len);
        for (uint16_t i = 0; i < t_len; i++)
            Dprintf(PINK, "", "%02x ", t_dat[i]);
        Dprintf(PINK, "", "\r\n");

        return NET_OK;
    }
    return NET_ERR;
}
Net_Status_t IUarts::NetConfig(NetConfigStruct_t *cfg)
{
    return NET_OK;
}
Net_Status_t IUarts::NetTest(NetTestStruct_t *test)
{
    return NET_OK;
}
extern "C"
{
    void USART2_IRQHandler(void)
    {
        BaseType_t xHigherPriorityTaskWoken;
        while (1)
        {
            if (USART_GetITStatus(USART2, USART_IT_RXNE)) //����һ�ֽ��ж�
            {
                USART_ClearFlag(USART2, USART_FLAG_RXNE);
                U2_RnRece.Data[U2_HaveReceLen++] = USART_ReceiveData(USART2);
                if (U2_HaveReceLen > U2_max_rxbuf) //���մ������۳���֮���׵�
                    U2_HaveReceLen = 0;
            }
            else if (USART_GetITStatus(USART2, USART_IT_IDLE) != RESET) //����һ֡�ж�
            {
                USART_ReceiveData(USART2); //���IDLE���б�־λ
                USART_ClearFlag(USART2, USART_FLAG_IDLE);
                U2_ReceFinsh = 0;
                extern SemaphoreHandle_t BinarySemaphore_rxd;	//��ֵ�ź������
                xSemaphoreGiveFromISR(BinarySemaphore_rxd, &xHigherPriorityTaskWoken); //�ͷŶ�ֵ�ź���
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);                      //�����Ҫ�Ļ�����һ�������л�
            }
            else if (USART_GetITStatus(USART2, USART_IT_TC)) //�����ж�
            {
                USART_ClearFlag(USART2, USART_FLAG_TC);
                if (U2_HaveSenLen < U2_RnSend.Len)
                {
                    USART_SendData(USART2, U2_RnSend.Data[U2_HaveSenLen++]);
                }
                else
                {
                    U2_HaveSenLen = 1;
                    U2_SendFinsh = 0;
                }
            }
            else
            {
                break;
            }
        }
    }
}