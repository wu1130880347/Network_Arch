#include <string.h>
#include "Hmac_sha1.h"
#include "stm32f4xx.h"

/*******************************************************************************
 �� �� ����	bcd_to_hex
 ����������	��bcd��ת����16������
 ��ڲ�����	b:bcd����
 ���ڲ�����	��
 �� �� ֵ:	16������
********************************************************************************/
uint8 CTool::bcd_to_hex(uint8 b)
{
	uint8 temp;

	temp  = b / 16 * 10;
	temp += b % 16;
	return temp;
}

/*******************************************************************************
 �� �� ����	hex_to_bcd
 ����������	��16������ת����bcd��
 ��ڲ�����	h:16������
 ���ڲ�����	��
 �� �� ֵ:	bcd��
********************************************************************************/
uint8 CTool::hex_to_bcd(uint8 h)
{
	uint8 temp;

	temp  = ((h / 10)) << 4;
	temp += h % 10;
	return temp;
}

/**************************************************************************
����ԭ�ͣ�OTHER_BCDchargeHEX
������������BCDת��ΪHEX
��ڲ�������
���ڲ�����
**************************************************************************/
void CTool::BCDchargeHEX(uint8 *tBuf, uint8 *sBuf)
{
	uint8 i;
	for(i=0; i<6; i++)
		tBuf[i] = (sBuf[i]>>4)*10+(sBuf[i]&0x0f);
}

/*******************************************************************************
����ԭ�ͣ�ChangeFormat
�����������������е��ֽ�˳��ߵ�
��ڲ�����p:ָ�� len:��Ҫת���ĳ���(0<len<5)
���ڲ�������
********************************************************************************/
void CTool::ChangeFormat(u8 *p, u8 len)
{
	u8 i, temp[4];

	for (i = 0; i < len; i++)
	{
		temp[len - 1 - i] = p[i];
	}

	memcpy(p, temp, len);
}

/**************************************************************************
����ԭ�ͣ�OTHER_LChangeToC
������������long����ת��Ϊ�ַ�������
��ڲ�����ulData=>long������pData=>��ŵ�ַ
���ڲ�������
**************************************************************************/
void CTool::LChangeToC(u32 ulData, u8 *pData)
{
	memcpy(pData, (u8 *)&ulData, 4);
}

/**************************************************************************
 ����ԭ�ͣ�EqualityJudge
 ��������������Ϊ3�ֽڵ����ݴ�С�ж�
 ��ڲ�����pData1->����1, pData2->����2
 ���ڲ�����-1 => pData1С��pData2; 0=>pData1����pData2, 1=>pData1����pData2
**************************************************************************/
s8 CTool::EqualityJudge(u8 * pData1, u8 * pData2)
{
	u8 i;

	for(i = 0; i < 3; i++)
	{
		if(pData1[i] < pData2[i])
			return -1;
		if(pData1[i] > pData2[i])
			return 1;
	}
	return 0;
}

/**************************************************************************
����ԭ�ͣ�LRCAccount
��������������LRC
��ڲ�����pData->0:����ָ��, ucLen->���ݳ���
���ڲ���������LRC
ע��:	  �����峤�Ȳ���Ϊ0
**************************************************************************/
u8 CTool::LRCAccount(u8 *pData, u8 ucLen)
{
	u8 i;
	u8 ucLRC = 0;

	for(i = 0; i < ucLen; i++)
		ucLRC += pData[i];
	return ucLRC;
}

/********************************************************************************************
����ԭ�ͣ�GetOrder
����������
��ڲ�������
���ڲ�����-1=>aTmpA<aTmpB;��0=>aTmpA==aTmpB;��1=>aTmpA>aTmpB
*********************************************************************************************/
s8 CTool::GetOrder(u8 *aTmpA, u8 *aTmpB)
{
	u8 i;
	for(i=0; i<2; i++)
	{
		if(aTmpA[i] < aTmpB[i])
			return -1;
		if(aTmpA[i] > aTmpB[i])
			return 1;
	}
	return 0;
}

/**************************************************************************
����ԭ�ͣ�OTHER_SortMealMsgvalid
������������Ա���ʹ���Ϣ�Ϸ��ж�
��ڲ���������Ϊȫff,�򲻺Ϸ�
���ڲ�����0=>�Ϸ� 1=>���Ϸ�
**************************************************************************/
u8 CTool::SortMealMsgvalid(u8 *aTmp)
{
	u8 i;
	for(i=0; i<4; i++)
	{
		if(aTmp[i] != 0xff)
			break;
	}
	return (i==4);
}


/**************************************************************************
����ԭ�ͣ�ValueValidityCheck
����������ֵ�κϷ��Լ��
��ڲ�����pData: ���ݻ���
���ڲ�����1=>��Ч��0=>��Ч
**************************************************************************/
char CTool::ValueValidityCheck(u8 *pData)
{
	u8 ucLrc = 0;
	ucLrc = LRCAccount(pData, 15);
	if(ucLrc != pData[15])
		return 1;
	else
		return 0;
}


/**************************************************************************
����ԭ�ͣ�Int_BigtoLittle��
����������int �� ��С��ת������
��ڲ�������
���ڲ�������
**************************************************************************/
void CTool::Int_BigtoLittle(u8* p)
{
	u8 temp[4];
	u8 i;
	for(i=0;i<4;i++)
	{
		temp[3-i] = p[i];
	}
	memcpy(p,temp,4);
}

/**************************************************************************
����ԭ�ͣ�Int_LittletoBig��
����������int �� С�˵����ת������
��ڲ�������
���ڲ�������
**************************************************************************/
void CTool::Int_LittletoBig(u8* p)
{
	u8 temp[4];
	u8 i;
	for(i=0;i<4;i++)
	{
		temp[3-i] = p[i];
	}
	memcpy(p,temp,4);
}



/*******************************************************************************
 �� �� ����	Revbit
 ����������	λ������
 ��ڲ�����	Ҫ���������������
 ���ڲ�����	������
********************************************************************************/
u32 CTool::Revbit(u32 uidata)
{
	asm("rbit r0,r0");
	return uidata;
}

/*******************************************************************************
 �� �� ����	Rest_CRC32
 ����������	��λCRC�Ĵ���
 ��ڲ�����	��
 ���ڲ�����	��
********************************************************************************/
void CTool::Rest_CRC32(void)
{
	CRC->CR = 1;
}
//����ط�ͬʱ����CRC32��Ҫ�����м�ֵ
/*******************************************************************************
 �� �� ����	Calculate_CRC32
 ����������	�������������CRC32У��
 ��ڲ�����	Ҫ�����ֵ���ֽڳ��� ����ֻ�������ֵ���������4�ı���
 ���ڲ�����	������
********************************************************************************/
u32  CTool::Calculate_CRC32(u8* buff,u32 BuffLength)
{	
	u32* temp;
	
	u32 Result;
	
	BuffLength >>= 2;
	
	temp = (u32*)buff;
	
	for(u32 index = 0;index < BuffLength;index++)
	{
		CRC->DR = Revbit(temp[index]);
	}
	
	Result = (Revbit(CRC->DR)^0xFFFFFFFF);
	
	return Result;
}

//CRC8У���	X^8 + X^2 + X^1 + 1
uc8 CTool::CRC8_TAB[256] = {
    0x00,0x07,0x0E,0x09,0x1C,0x1B,0x12,0x15,0x38,0x3F,0x36,0x31,0x24,0x23,0x2A,
    0x2D,0x70,0x77,0x7E,0x79,0x6C,0x6B,0x62,0x65,0x48,0x4F,0x46,0x41,0x54,0x53,
    0x5A,0x5D,0xE0,0xE7,0xEE,0xE9,0xFC,0xFB,0xF2,0xF5,0xD8,0xDF,0xD6,0xD1,0xC4,
    0xC3,0xCA,0xCD,0x90,0x97,0x9E,0x99,0x8C,0x8B,0x82,0x85,0xA8,0xAF,0xA6,0xA1,
    0xB4,0xB3,0xBA,0xBD,0xC7,0xC0,0xC9,0xCE,0xDB,0xDC,0xD5,0xD2,0xFF,0xF8,0xF1,
    0xF6,0xE3,0xE4,0xED,0xEA,0xB7,0xB0,0xB9,0xBE,0xAB,0xAC,0xA5,0xA2,0x8F,0x88,
    0x81,0x86,0x93,0x94,0x9D,0x9A,0x27,0x20,0x29,0x2E,0x3B,0x3C,0x35,0x32,0x1F,
    0x18,0x11,0x16,0x03,0x04,0x0D,0x0A,0x57,0x50,0x59,0x5E,0x4B,0x4C,0x45,0x42,
    0x6F,0x68,0x61,0x66,0x73,0x74,0x7D,0x7A,0x89,0x8E,0x87,0x80,0x95,0x92,0x9B,
    0x9C,0xB1,0xB6,0xBF,0xB8,0xAD,0xAA,0xA3,0xA4,0xF9,0xFE,0xF7,0xF0,0xE5,0xE2,
    0xEB,0xEC,0xC1,0xC6,0xCF,0xC8,0xDD,0xDA,0xD3,0xD4,0x69,0x6E,0x67,0x60,0x75,
    0x72,0x7B,0x7C,0x51,0x56,0x5F,0x58,0x4D,0x4A,0x43,0x44,0x19,0x1E,0x17,0x10,
    0x05,0x02,0x0B,0x0C,0x21,0x26,0x2F,0x28,0x3D,0x3A,0x33,0x34,0x4E,0x49,0x40,
    0x47,0x52,0x55,0x5C,0x5B,0x76,0x71,0x78,0x7F,0x6A,0x6D,0x64,0x63,0x3E,0x39,
    0x30,0x37,0x22,0x25,0x2C,0x2B,0x06,0x01,0x08,0x0F,0x1A,0x1D,0x14,0x13,0xAE,
    0xA9,0xA0,0xA7,0xB2,0xB5,0xBC,0xBB,0x96,0x91,0x98,0x9F,0x8A,0x8D,0x84,0x83,
    0xDE,0xD9,0xD0,0xD7,0xC2,0xC5,0xCC,0xCB,0xE6,0xE1,0xE8,0xEF,0xFA,0xFD,0xF4,
    0xF3
};

/**************************************************************************
 �� �� ����UART_CRC8Value
 ����������CRC8У�������
 ��ڲ�����ucPtr->����ָ��, ucLen->���ݳ���
 ���ڲ�����CRC8У����
**************************************************************************/
u8 CTool::CRC8Value(u8 * ucPtr, u8 ucLen)//CRC8У�������
{
	u8 ucIndex;		//CRC8У��������
	u8 ucCRC8 = 0;	//CRC8�ֽڳ�ʼ��
	
	//����CRC8λУ��
	while (ucLen--)
	{
		ucIndex = ucCRC8 ^ (*ucPtr++);
		ucCRC8 = CTool::CRC8_TAB[ucIndex];
	}
	
	//����CRC8У������
	return (~ucCRC8);
}


/**************************************************************************
 ����ԭ�ͣ�JudgeDate
 �������������ڴ�С�ж�
 ��ڲ�����pDate1->����1��ַ, pDate2->����2��ַ
 ���ڲ�����-1->pDate1С��pDate2, 0->pDate1����pDate2, 1->pDate1����pDate2
**************************************************************************/
s8 CTool::JudgeDate(u8 *pDate1, u8 *pDate2)//���ڴ�С�ж�
{
	/*u8 i;
	u8 aTmp[3];

	memcpy(aTmp, pDate1, 3);
	aTmp[1] = (((aTmp[1]&0x0f)/10)<<4)+((aTmp[1]&0x0f)%10);
	for(i = 0; i < 3; i++)
	{
		if(aTmp[i] < pDate2[i])
			return -1;
		if(aTmp[i] > pDate2[i])
			return 1;
	}
	return 0;*/
	u8 i;
	for(i = 0; i < 3; i++)
	{
		if(pDate1[i] < pDate2[i])
			return -1;
		if(pDate1[i] > pDate2[i])
			return 1;
	}
	return 0;
}

/**************************************************************************
����ԭ�ͣ�ConsumeNoteLRC
��������������LRC��PC100-C 26�ֽڼ�¼ר�ã�
��ڲ�����pData->����ָ��
���ڲ�����ucRet->LRCֵ
**************************************************************************/
uint8 CTool::ConsumeNoteLRC(uint8 *pData)
{
	uint8 i;
	uint8 ucRet = 0;

	for(i = 0; i < 24; i++)
	{
		ucRet += pData[i];
	}
	return ucRet;
}



//�����ж�:1=>����;0=>ƽ��
int CTool::leapyear(int year)
{
	if((year%4==0 && year%100!=0) || year%400==0)
		return 1;
	else
		return 0;
}


//�ڶ�������С����-1,���򷵻�0
//����Ϊu8����������յ�16����ֵ
int CTool::CompareDate(u8* date1, u8* date2)
{//0:��,1:��,2:��
	if(date1[0] == date2[0])//�������
	{
		if(date1[1] > date2[1])
		{
			return -1;
		}
		else if(date1[1] == date2[1])//�������
		{
			if(date1[2] > date2[2])
			{
				return -1;
			}
		}
	}
	else if(date1[0] > date2[0])
	{
		return -1;
	}
	return 0;
}

//����Ϊu8����������յ�16����ֵ
int CTool::diff(u8* date1, u8* date2)
{
	int i;
	int diff = 0;
	const int month[13]={0,31,28,31,30,31,30,31,31,30,31,30,31};
	if (date1[0] == date2[0])
	{
		if (date1[1] == date2[1])
		{
			diff = date2[2] - date1[2];
		}
		else
		{
			for (i = date1[1]+1; i < date2[1]; i++)
			{
				diff += month[i];
			}
			diff += month[date1[1]] - date1[2] + date2[2];
			if (leapyear(date1[0]))
				if ((date1[1] <= 2) && (date2[1] > 2))
					diff++;
		}
	}
	else
	{
		for (i = date1[0]+1; i < date2[0]; i++)
		{
			if (leapyear(i))
				diff += 366;
			else
				diff += 365;
		}
		for (i = date1[1]+1; i <= 12; i++)    //date1������ĩ������
		{
			diff += month[i];
		}
		diff += month[date1[1]] - date1[2];
		if (date1[1] <= 2)
			if (leapyear(date1[0]))
				diff++;
		for (i = 1; i < date2[1]; i++)     //date2�������������
		{
			diff += month[i];
		}
		diff += date2[2];
		if (date1[1] > 2)
			if (leapyear(date2[0]))
				diff++;
	}
	return diff;
}


//�����������ڵĲ�ֵ
//����Ϊu8����������յ�BCD��
//��һ������С:�����������ڵĲ�ֵ;
//�ڶ�������С:���ش���
//date1:��������ѵ�����
//date2:pos������
int CTool::CountDateDiff(u8* date1, u8* date2)
{
	u8 temp_date1[3];
	u8 temp_date2[3];
	memcpy(temp_date1, date1, 3);
	memcpy(temp_date2, date2, 3);
	for (int i = 0; i < 3; i++)
	{	//BCD��ת��
		temp_date1[i] = bcd_to_hex(temp_date1[i]);
		temp_date2[i] = bcd_to_hex(temp_date2[i]);
	}
	if (CompareDate(temp_date1, temp_date2) == -1)
		return -1;//pos������С,����,��ֹ����

	int days = diff(temp_date1, temp_date2);
	return days;
}

//HMAC��Կ
//ksh_test
//uc8 CTool::HMAC_Key[16] = {0x21, 0x29, 0x00, 0xf0, 0xa1, 0x81, 0x40, 0x28, 0x00, 0xf0, 0x9b, 0x81, 0xa2, 0xe1, 0x9d, 0xf8};//ʯάȫ
//uc8 CTool::HMAC_Key[16] = {0x20, 0x28, 0x00, 0xf0, 0xa1, 0x81, 0x40, 0x28, 0x00, 0xf0, 0x9b, 0x81, 0xa2, 0xe1, 0x9d, 0xf8};//ͨ��

//����HMAC
//��ڲ���:lenҪ���ܵ����ݳ��ȣ�pDataҪ���ܵ����ݣ�len_macҪ��ȡ��У�鳤��(�20�ֽ�)
//���ڲ���:pHmac���ܽ��(8�ֽ�)
void CTool::HMAC(u32 len, u8* pData, u8 len_mac, u8* pHmac)
{
	u8 temp_Hmac[20];//���ܽ��Ϊ20�ֽڣ�ֻ�����е�8�ֽ�
	
	hmac_sha1((u8*)HMAC_Key, 16, pData, len, temp_Hmac);
	if (len_mac > 20)
		len_mac = 20;
	memcpy(pHmac, temp_Hmac, len_mac);
}




