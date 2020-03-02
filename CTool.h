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
	void ChangeFormat(u8 *p, u8 len);//�������е��ֽ�˳��ߵ�
	void LChangeToC(u32 ulData, u8 *pData);//��long����ת��Ϊ�ַ�������
	s8 EqualityJudge(u8 * pData1, u8 * pData2);//����Ϊ3�ֽڵ����ݴ�С�ж�
	u8 LRCAccount(u8 *pData, u8 ucLen);//����LRC
	u8 JudgeMealOrderValidity(u8 ucOrder);//�жϲʹκϷ���(�ֶ��ʹ�ģʽ)
	s8 GetOrder(u8 *aTmpA, u8 *aTmpB);//�Ƚ������С
	u8 GetMealOrder(u8 *aTmp);//ȡ�òʹ�(�Զ��ʹ�ģʽ)
	u8 SortMealMsgvalid(u8 *aTmp);//��Ա���ʹ���Ϣ�Ϸ��ж�
	char ValueValidityCheck(u8 *pData);//ֵ�κϷ��Լ��
	void Int_BigtoLittle(u8* p);
	void Int_LittletoBig(u8* p);
	u32 Revbit(u32 uidata);
	void Rest_CRC32(void);
	u32  Calculate_CRC32(u8* buff,u32 BuffLength);
	u8 CRC8Value(u8 * ucPtr, u8 ucLen);//CRC8У�������
	s8 JudgeDate(u8 *pDate1, u8 *pDate2);
	uint8 ConsumeNoteLRC(uint8 *pData);
	
	//��������������ж�
	int leapyear(int year);
	int CompareDate(u8* date1, u8* date2);
	int diff(u8* date1, u8* date2);
	int CountDateDiff(u8* date1, u8* date2);
	
	void HMAC(u32 len, u8* pData, u8 len_mac, u8* pHmac);//����HMAC
};
#endif//__CTOOL_H__

