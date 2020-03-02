#include "CUdpProtocol.h"
#include "CNetManagement.h"
#include "driver.h"

void CUdpProtocol::Init(CNetManagement *pthis,void *para)
{
    mp_this = pthis;
    m_dat_package.p_sendFrame= &m_SendFrame;
    m_dat_package.p_receFrame = &m_ReceiveFrame;
}
void CUdpProtocol::ComSend(void) 
{
    mp_this->ComSend();
}
NetPackageStruct_t *CUdpProtocol::Ret_ReceBuf(void)
{
    return &m_dat_package;
}
NetPackageStruct_t *CUdpProtocol::Ret_SendBuf(void)
{
    return &m_dat_package;
}
BOOL CUdpProtocol::ReceiveCheck(RECEIVE_FRAME *frame)
{
    u8 temp_buff[4] = {0xab, 0x12, 0xcd, 0x34};
    u8 temp_HMAC[8];

    //请求判定
    if (m_Request == 0)
        return FALSE;
    //复制数据
    frame->Len = m_ReceiveFrame.Len;
    memcpy(frame->Data, m_ReceiveFrame.Data, m_ReceiveFrame.Len);
    m_Request = 0;

    //帧头判定
    if (memcmp(frame->Data, temp_buff, 4))
        return FALSE;
    //长度判定
    if (*(u16 *)(frame->Data + 4) != (frame->Len - 6))
    { //粘包处理(只留第一个包)
        frame->Len = *(u16 *)(frame->Data + 4) + 6;
    }
    //长度判定
    if ((frame->Len == 0) || (frame->Len > rx_buf_max))
        return FALSE;
    //DMAC判定
    if (memcmp(frame->Data + 6, 0x00000000ul, 8))
        return FALSE;
    //保存接收随机数
    m_ReceiveRandomNum = *(u16 *)(frame->Data + 14);
    //HMAC判定
    g_CTool.HMAC((*(u16 *)(frame->Data + 4)) - 6, frame->Data + 4, 8, temp_HMAC);
    if (memcmp(frame->Data + (*(u16 *)(frame->Data + 4)) - 2, temp_HMAC, 8))
        return FALSE;

    return TRUE;
}

void CUdpProtocol::Agreement(void)
{
    if (ReceiveCheck(&m_ReceiveFrame))
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