
#include "twoeight_logic.h"


namespace game_twoeight
{
	//扑克数据
	static const uint8 s_szCardListData[CARD_COUNT] = {
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,	// 1筒-9筒，白板
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
	};

	//////////////////////////////////////////////////////////////////////////

	// 值大小比较
	// nextCardValue>firstCardValue  返回1
	// nextCardValue<firstCardValue  返回-1
	// nextCardValue==firstCardValue 返回0
	int CompareCardValue(uint8 firstCardValue, uint8 nextCardValue) {
		if (firstCardValue < nextCardValue)
			return 1;
		if (firstCardValue > nextCardValue)
			return -1;
		return 0;
	}

	// 混乱牌
	void TwoeightLogic::RandCardList(uint8 cbTableCardArray[MAX_SEAT_INDEX][SINGLE_CARD_NUM]) {
		//混乱准备
		uint8 cbCardData[CARD_COUNT];
		memcpy(cbCardData, s_szCardListData, sizeof(s_szCardListData));

		uint8 cbCardBuffer[CARD_COUNT] = { 0 };

		//混乱扑克
		uint8 cbRandCount = 0, cbPosition = 0, cbBufferCount = MAX_SEAT_INDEX * SINGLE_CARD_NUM;
		do {
			cbPosition = g_RandGen.RandUInt() % (CARD_COUNT - cbRandCount);
			cbCardBuffer[cbRandCount++] = cbCardData[cbPosition];
			cbCardData[cbPosition] = cbCardData[CARD_COUNT - cbRandCount];
		} while (cbRandCount < cbBufferCount);

		memcpy(cbTableCardArray, cbCardBuffer, cbBufferCount);
		return;
	}

	//获取牌型
	int TwoeightLogic::GetCardType(const uint8 cbCardData[SINGLE_CARD_NUM]) {
		if (cbCardData[0] == cbCardData[1]) {
			if (cbCardData[0] % 10 == 0)
				return emCardTypeWhiteLeopard;
			return emCardTypeLeopard;
		}
		if ((cbCardData[0] == 2 && cbCardData[1] == 8) || (cbCardData[0] == 8 && cbCardData[1] == 2))
			return emCardTypeTwoeight;
		if (GetCardPoint(cbCardData) >= 8)
			return emCardTypePoint8;
		return emCardTypePoint;
	}



	// 大小比较
	int TwoeightLogic::CompareCard(const uint8 cbFirstCardData[SINGLE_CARD_NUM], const uint8 cbNextCardData[SINGLE_CARD_NUM], int &multiple) {
		// 获取牌型
		int cbFirstCardType = GetCardType(cbFirstCardData);
		int cbNextCardType = GetCardType(cbNextCardData);

		if (cbFirstCardType > cbNextCardType) {
			multiple = cbFirstCardType;
			return -1;
		}
		if (cbFirstCardType < cbNextCardType) {
			multiple = cbNextCardType;
			return 1;
		}
		if (cbFirstCardType == emCardTypeWhiteLeopard || cbFirstCardType == emCardTypeTwoeight)
			return 0;

		multiple = cbFirstCardType;
		if (cbFirstCardType == emCardTypeLeopard)
			return CompareCardValue(cbFirstCardData[0], cbNextCardData[0]);
		// 获取牌组点数,再比较
		return CompareCardValue(GetCardPoint(cbFirstCardData), GetCardPoint(cbNextCardData));
	}
}