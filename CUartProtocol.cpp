#include <map>
#include "CUartProtocol.h"
#include "CNetManagement.h"
#include "driver.h"


//debug info output
#if DBGUART
extern "C"
{
#define DBG_USE 1

#if DBG_USE
    //是否打开该文件内的调试LOG
    static const char EN_LOG = GREEN;
    //LOG输出文件标记
    static const char TAG[] = "UartPro: ";
#else
#ifdef DBG_Printf
#undef DBG_Printf
#define DBG_Printf(...)
#else
#endif
#endif
}

#endif

static std::map<void*,CUartProtocol*> m_udpmap;
CUartProtocol::CUartProtocol()
{
    m_SendRandomNum = 0;    //主动发送时的随机数
    m_ReceiveRandomNum = 0; //接收到的随机数
    m_CommandNum = 0;       //主动发送时的命令流水号
    m_CommandNumSave = 0;   //保存主动发送的命令流水号
    rx_buf_max = 0;
    tx_buf_max = 0;
    mp_this = nullptr;
    m_dat_package.p_sendFrame= nullptr;
    m_dat_package.p_receFrame = nullptr;
}
CUartProtocol::~CUartProtocol()
{

}
CUartProtocol *CUartProtocol::get_this(void* t_target)
{
    Dprintf(EN_LOG,TAG,"Cnet get_this = 0x%08x\r\n",t_target);
    static CUartProtocol * p_temp;
    p_temp = m_udpmap[t_target];
    return p_temp;
}

void CUartProtocol::Init(CNetManagement *pthis,void *para)
{
    NetParaType_t *temp = (NetParaType_t*)para;
    Dprintf(EN_LOG,TAG,"Cnet addr = 0x%08x\r\n",(void *)pthis);
    Dprintf(EN_LOG,TAG,"txf_max = %d rxf_max = %d\r\n",temp->t_rdt_num.tx_max,temp->t_rdt_num.rx_max);

    m_SendFrame.Data = temp->t_rdt_num.tx_buf;//初始化基本数据buffer
    m_ReceiveFrame.Data = temp->t_rdt_num.rx_buf;
    rx_buf_max = temp->t_rdt_num.tx_max;
    tx_buf_max = temp->t_rdt_num.rx_max;
    mp_this = pthis;
    m_dat_package.p_sendFrame= &m_SendFrame;
    m_dat_package.p_receFrame = &m_ReceiveFrame;
    m_udpmap[para] = this;//将其类加入图内
}
void CUartProtocol::ComSend(void)
{
    mp_this->ComSend();
}
NetPackageStruct_t *CUartProtocol::Ret_ReceBuf(void)
{
    return &m_dat_package;
}
NetPackageStruct_t *CUartProtocol::Ret_SendBuf(void)
{
    return &m_dat_package;
}
BOOL CUartProtocol::ReceiveCheck(RECEIVE_FRAME *frame)
{
    //请求判定
    if (m_Request == 0)
        return FALSE;
    return TRUE;
}

void CUartProtocol::Agreement(void)
{
    if (ReceiveCheck(&m_ReceiveFrame))
        return;

    switch (*(u16 *)(m_ReceiveFrame.Data + 20))
    {
    case 0x0001: //心跳回复
        //HeartBeatReply(m_ReceiveFrame.Data + 16);
        break;
    // case 0x0002: //注册回复
    //     RegisterReply(m_ReceiveFrame.Data + 16);
    //     break;
    // case 0x0003: //获取设备信息
    //     GetPosState(m_ReceiveFrame.Data + 16);
    //     break;
    // case 0x0004: //修改设备编号
    //     SetAddr(m_ReceiveFrame.Data + 16);
    //     break;
    // case 0x0005: //修改服务器IP及端口
    //     SetServer(m_ReceiveFrame.Data + 16);
    //     break;
    // case 0x0006: //读取设备时间
    //     ReadTime(m_ReceiveFrame.Data + 16);
    //     break;
    // case 0x0007: //修改设备时间
    //     SetTime(m_ReceiveFrame.Data + 16);
    //     break;
    // case 0x0008: //余额查询回复
    //     BalanceQueryReply(m_ReceiveFrame.Data + 16);
    //     break;
    // case 0x0009: //消费请求回复
    //     TradeReply(m_ReceiveFrame.Data + 16);
    //     break;
    // case 0x000A: //清空交易记录
    //     EmptyNote(m_ReceiveFrame.Data + 16);
    //     break;
    // case 0x000B: //重采交易记录
    //     RereadNote(m_ReceiveFrame.Data + 16);
    //     break;
    // case 0x000C: //日收入查询回复
    //     IncomeReply(m_ReceiveFrame.Data + 16);
    //     break;
    // case 0x000D: //下发文件信息
    //     DownloadFileName(m_ReceiveFrame.Data + 16);
    //     break;
    // case 0x000E: //下发文件数据
    //     DownloadFileData(m_ReceiveFrame.Data + 16);
    //     break;
    // case 0x000F: //远程重启
    //     LongRangeReset(m_ReceiveFrame.Data + 16);
    //     break;
    // case 0x0010: //更新时间参数
    //     SetTimePara(m_ReceiveFrame.Data + 16);
    //     break;
    // case 0x0011: //撤消交易请求
    //     CancelReply(m_ReceiveFrame.Data + 16);
    //     break;
    // case 0x0012: //清空商品信息
    //     MenuEmpty(m_ReceiveFrame.Data + 16);
    //     break;
    // case 0x0013: //更新商品信息
    //     MenuSet(m_ReceiveFrame.Data + 16);
    //     break;
    // case 0x0014: //更新商品信息版本号
    //     MenuVersionSet(m_ReceiveFrame.Data + 16);
    //     break;
    // case 0x0015: //更新卡参数
    //     CardPara(m_ReceiveFrame.Data + 16);
    //     break;
    // case 0x0016: //采集交易记录
    //     ReadNote(m_ReceiveFrame.Data + 16);
    //     break;
    // case 0x0017: //清空语音
    //     EmptyVoice(m_ReceiveFrame.Data + 16);
    //     break;
    // case 0x001E: //计时撤消回复
    //     TimeCancelReply(m_ReceiveFrame.Data + 16);
    //     break;
    // case 0x0024: //获取ICCID
    //     GetICCIDResponse(m_ReceiveFrame.Data + 16);
    //     break;
    // case 0x0025: //ICCID上报成功后的处理
    //     SendICCIDResponse();
    //     break;
    // case 0x0200: //更新服务器列表2017.07.21
    //     SetServerList(m_ReceiveFrame.Data + 16);
    //     break;
    // case 0x0201: //修改通讯密钥
    //     SetComKey(m_ReceiveFrame.Data + 16);
    //     break;
    // case 0x0202: //修改DMAC
    //     SetDMAC(m_ReceiveFrame.Data + 16);
    //     break;
    // case 0x0203: //修改二维码是否开启
    //     SetQRencode(m_ReceiveFrame.Data + 16);
    //     break;
    // case 0x0204: //运营商配对卡
    //     PairCardReply(m_ReceiveFrame.Data + 16);
    //     break;
    default:
        break;
    }
}
//发送数据(入口参数只传数据域)
//mode 0:回复发送，1:主动发送 心跳(命令流水号固定0，上层已赋值)，2:主动发送其他指令(命令流水号在此处赋值)
void CUartProtocol::SendFrame(u16 len, u8 *pData, u8 mode)
{
	m_SendFrame.Len = len + 18 + 6;
	if (m_SendFrame.Len > tx_buf_max)
		return;
	//帧头
	m_SendFrame.Data[0] = 0x49;
	m_SendFrame.Data[1] = 0x8a;
	m_SendFrame.Data[2] = 0x5e;
	m_SendFrame.Data[3] = 0xb4;
	//帧长度(D-MAC(8)、随机数(2)、数据域、MAC(8))
	*(u16*)(m_SendFrame.Data+4) = len + 18;
	//D-MAC
    static uint8_t temp_dmac[8] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
	memcpy(m_SendFrame.Data+6, temp_dmac, 8);
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
    //static uint8_t temp[8] = {0,1,2,3,4,5,6,7};
    
    //memcpy(m_SendFrame.Data+len+16, temp, 8);
	//HMAC(帧长度、D-MAC、随机数、数据域)
	g_CTool.HMAC(len+12, m_SendFrame.Data+4, 8, m_SendFrame.Data+len+16);
	ComSend();
}
void CUartProtocol::HeartBeatSend(void)
{
    u8 temp_buff[8];

    *(u32 *)(temp_buff) = 0;                                     //命令流水号
    *(u16 *)(temp_buff + 4) = 0x0001;                            //功能码
    temp_buff[6] = 0x00; //屏蔽GPRS初始化失败
    temp_buff[7] = 0x00;

    SendFrame(8, temp_buff, 1);
}