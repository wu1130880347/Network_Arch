#ifndef __CTOOL_H__
#define __CTOOL_H__
#include "CSuperNetInterface.h"
typedef uint8_t const uc8;
typedef uint8_t  uint8;
typedef int8_t  s8;
class CTool
{	
	static uc8 CRC8_TAB[256];
public:
	u8 HMAC_Key[16];
	
	uint8 bcd_to_hex(uint8 b);
	uint8 hex_to_bcd(uint8 h);
	void BCDchargeHEX(uint8 *tBuf, uint8 *sBuf);
	void ChangeFormat(u8 *p, u8 len);//将数组中的字节顺序颠倒
	void LChangeToC(u32 ulData, u8 *pData);//将long型数转换为字符型数组
	s8 EqualityJudge(u8 * pData1, u8 * pData2);//长度为3字节的数据大小判断
	u8 LRCAccount(u8 *pData, u8 ucLen);//计算LRC
	u8 JudgeMealOrderValidity(u8 ucOrder);//判断餐次合法性(手动餐次模式)
	s8 GetOrder(u8 *aTmpA, u8 *aTmpB);//比较数组大小
	u8 GetMealOrder(u8 *aTmp);//取得餐次(自动餐次模式)
	u8 SortMealMsgvalid(u8 *aTmp);//人员类别餐次信息合法判断
	char ValueValidityCheck(u8 *pData);//值段合法性检测
	void Int_BigtoLittle(u8* p);
	void Int_LittletoBig(u8* p);
	u32 Revbit(u32 uidata);
	void Rest_CRC32(void);
	u32  Calculate_CRC32(u8* buff,u32 BuffLength);
	u8 CRC8Value(u8 * ucPtr, u8 ucLen);//CRC8校验码计算
	s8 JudgeDate(u8 *pDate1, u8 *pDate2);
	uint8 ConsumeNoteLRC(uint8 *pData);
	
	//日期相差天数的判定
	int leapyear(int year);
	int CompareDate(u8* date1, u8* date2);
	int diff(u8* date1, u8* date2);
	int CountDateDiff(u8* date1, u8* date2);
	
	void HMAC(u32 len, u8* pData, u8 len_mac, u8* pHmac);//计算HMAC
};
#endif//__CTOOL_H__

