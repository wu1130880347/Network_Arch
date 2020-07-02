#include "IUarts.h"
#include "FreeRTOS.h"					//FreeRTOS使用
#include "task.h"
#include "semphr.h"
//debug info output
#if DBGUART
extern "C"
{
#define DBG_USE 0

#if DBG_USE
    //是否打开该文件内的调试LOG
    static const char EN_LOG = YELLOW;
    //LOG输出文件标记
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
    //GPIO端口设置
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    USART_DeInit(USART2);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);  //使能GPIOA时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE); //使能USART2时钟

    //串口1对应引脚复用映射
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2);  //GPIOA2用为USAT2
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2); //GPIOA3用为USAR2
    //USART1端口配置
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3; //GPIOA9与GPIOA10
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;            //复用功能
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;      //速度50MHz
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;          //推挽复用输出
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;            //上拉
    GPIO_Init(GPIOA, &GPIO_InitStructure);                  //初始化PA2，PA3

    //USART1 初始化设置
    USART_InitStructure.USART_BaudRate = t_para->baud_rate;                         //波特率设置
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;                     //字长为8位数据格式
    USART_InitStructure.USART_StopBits = USART_StopBits_1;                          //一个停止位
    USART_InitStructure.USART_Parity = USART_Parity_No;                             //无奇偶校验位
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; //无硬件数据流控制
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;                 //收发模式
    USART_Init(USART2, &USART_InitStructure);                                       //初始化串口1

    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE); //开启接收中断,RXNE读寄存器非空
    USART_ITConfig(USART2, USART_IT_IDLE, ENABLE); //使能接收一帧数据中断
    USART_ITConfig(USART2, USART_IT_TC, ENABLE);   //开启发送中断,TC发送完成
    USART_Cmd(USART2, ENABLE); //使能串口1

    //USART_ClearFlag(USART2, USART_FLAG_TC | USART_FLAG_IDLE | USART_FLAG_RXNE);

    //USART_ITConfig(USART2, USART_IT_RXNE | USART_IT_TC | USART_IT_IDLE, ENABLE); //开启相关中断

    //Usart1 NVIC 配置
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;         //串口1中断通道
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2; //抢占优先级3
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;        //子优先级3
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;           //IRQ通道使能
    NVIC_Init(&NVIC_InitStructure);                           //根据指定的参数初始化VIC寄存器
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
            if (USART_GetITStatus(USART2, USART_IT_RXNE)) //接收一字节中断
            {
                USART_ClearFlag(USART2, USART_FLAG_RXNE);
                U2_RnRece.Data[U2_HaveReceLen++] = USART_ReceiveData(USART2);
                if (U2_HaveReceLen > U2_max_rxbuf) //接收大于理论长度之后抛掉
                    U2_HaveReceLen = 0;
            }
            else if (USART_GetITStatus(USART2, USART_IT_IDLE) != RESET) //接收一帧中断
            {
                USART_ReceiveData(USART2); //清除IDLE空闲标志位
                USART_ClearFlag(USART2, USART_FLAG_IDLE);
                U2_ReceFinsh = 0;
                extern SemaphoreHandle_t BinarySemaphore_rxd;	//二值信号量句柄
                xSemaphoreGiveFromISR(BinarySemaphore_rxd, &xHigherPriorityTaskWoken); //释放二值信号量
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);                      //如果需要的话进行一次任务切换
            }
            else if (USART_GetITStatus(USART2, USART_IT_TC)) //发送中断
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