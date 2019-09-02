#ifndef OGLordRobotAI_h__
#define OGLordRobotAI_h__

// yz 2017-7-18
// ɾ���ӿ���̳й�ϵ
// ɾ��virtual

#pragma once
#include "Robot.h"

#include <vector>
#include <map>
#include <set>
//#include <hash_map>
//#include <algorithm>
//#include <numeric>

//#include <iostream>
#include "ddz_interface.h"

/*
enum ROBOT_LEVEL
{
    BASE_LEVEL = 0,
    SIMPLE_LEVEL = 1,
    MIDDLE_LEVEL = 2,
    HARD_LEVEL = 3,
    BEST_LEVEL = 4
};
*/

class OGLordRobotAI
{
public:
	//used for new robot
    LordRobot* robot;
	//LordRobot m_robot;
	int lord_score;

	OGLordRobotAI(void);
	~OGLordRobotAI(void);

    // �յ�������Ϣ
    bool RbtInInitCard(int argSeat,                        // �Լ�����λ��
        std::vector<int> argHandCard        // ���͵�����
        );

    // ������������̼���
    bool RbtInSetLevel(int argLevel);                  // ���������̼���

    // ���������λ��ҵ�����Ϣ
    bool RbtInNtfCardInfo(std::vector<std::vector<int>> argHandCard
        // ������λ��ҵ����� 0,1,2,3��Ԫ�طֱ��ʾ��0,1,2����λ�Լ����Ƶ����ݡ�
        );

    // �յ���ҽз���Ϣ
    bool RbtInCallScore(int argSeat,                       // ��λ��
        int argCallScore                   // �еķ���
        );

    // ��������зֲ���
    bool RbtOutGetCallScore(int &callScore                 // ����ֵ���� 
        );

    // ���������������Ϣ
    bool RbtInSetGrabLord(int argSeat                      // ��λ��
        );

    // �������������
    bool RbtOutGetGrabLord(bool &grabLord                  // TRUE�������� FALSE������
        );

    // �յ�ȷ��������Ϣ
    bool RbtInSetLord(int argLordSeat,       // ������λ
        std::vector<int> argReceiceCard      // �����յ��ĵ���
        );


    // �����ƽ׶Ρ�
    // �յ��������Ϣ
    bool RbtInTakeOutCard(int argSeat,   // ��λ��
        std::vector<int> argCards        // ������
        );

    // ����������Ʋ���
    bool RbtOutGetTakeOutCard(std::vector<int> &vecCards    // ���ص���
        );


    // �����û����ˡ�
    // ���ڵڶ���ʱ���û�����AI �Լ���������
    // ���û�������Ϣ
    bool RbtResetData();

    // ���õ����������λ
    bool RbtInSetSeat(int argMySeat,         // �Լ�����λ
        int argLordSeat                      // ��������λ
        );

    // �������ƺ͵���
    bool RbtInSetCard(std::vector<int> argInitCard,        // �Լ���ʼ������
        std::vector<int> argReceiveCard      // �����յ��ĵ���
        );
    // ���ó��Ƽ�¼
    bool RbtInTakeOutRecord(std::vector<int> argTakeOutSeat,               // ��ʷ����-��λ��¼
        std::vector<std::vector<int>> argTakeOutRecord // ��ʷ����-�Ƽ�¼
        );

	// ��������
	// ���ش�����Ϣ
	bool RbtOutGetLastError(int &errorCode);           // ���ص�ǰ������

    // �������������Ϣ 
    bool RbtInShowCard(int argShowSeat, std::vector<int> argHandCard);

    // ������Ʋ��� 
    bool RbtOutShowCard(bool &showCard);

    // ����ӱ����� 
    bool RbtOutDoubleGame(bool &Double_game);
private:

	void updateAiPosition();
	
	void sortHandMap();

	void sortHandMapLvl();

	void findLowestHand(Hand &hand);

	void findHigherHand(Hand &hand);

	void findHigherHandNotBomb(Hand &hand);

	void findMostCardsHandNotBomb(Hand &hand);

	void takeOutHand(Hand &hand, std::vector<int> &takeOutCards);

	void takeOutLvl0(Hand &hand);

	void takeOutLvl1(Hand &hand);

	void takeOutHighLvl(Hand &hand);

	bool isFree();

	bool isDanger();

	bool isGood(HandsMapSummary &hmSummary, int controlCount, int minOppNum);

	bool isFirstHalf();

	bool isGoodFarmer();

	void findChargeHandFirst(Hand &hand, bool typeCountFirst);

	void lordTakeOutFree(Hand &hand);

	void lordTakeOutFreeFarmerOnly1Card(Hand &hand);

	void lordTakeOutFreeFarmerOnly2Cards(Hand &hand);

	void lordTakeOutFreeNormal(Hand &hand);

	void lordTakeOutHigher(Hand &hand);

	void lordTakeOutHigherFarmerOnly1Card(Hand &hand);

	void lordTakeOutHigherFarmerOnly2Cards(Hand &hand);

	void lordTakeOutHigherNormal(Hand &hand);

	void lordTakeOutHigherRebuild(Hand &hand);

	void farmerTakeOutLordOnly1Card(Hand &hand);

	void farmerTakeOutLordOnly2Cards(Hand &hand);

	void farmerMustTakeOutLordOnly1Card(Hand &hand);

	void farmerMustTakeOutLordCharge(Hand &hand);

	void goodFarmerOverOtherFarmer(Hand &hand);

	void downGoodFarmerTakeOut(Hand &hand);

	void downBadFarmerTakeOut(Hand &hand);

	void upGoodFarmerTakeOut(Hand &hand);

	void upBadFarmerTakeOut(Hand &hand);

	void downFarmerTakeOutUpFarmerOnly1Card(Hand &hand);

	void findBestHigherHandFromMap(Hand &hand);

	void findBestHigherHandFromPoints(Hand &hand, bool force, bool lordOnly1Card);

	void farmerTakeOutWhenLordTakeOutHighSolo(Hand &hand);

	void refindForTrio(Hand &hand);

	bool isDangerLvl();

	bool isGoodLvl(HandsMapSummary &hmSummary, int minOppNum);

	void findKickerLvl(Hand &hand);

	bool containsHand(std::map<HandType, std::vector<Hand>> &allHands, Hand &hand);

	void lordTakeOutFreeLvl(Hand &hand);

	void lordTakeOutFreeFarmerOnly1CardLvl(Hand &hand);

	void lordTakeOutFreeFarmerOnly2CardsLvl(Hand &hand);

	void lordTakeOutFreeNormalLvl(Hand &hand);

	void lordTakeOutHigherLvl(Hand &hand);

	void lordTakeOutHigherFarmerOnly1CardLvl(Hand &hand);

	void lordTakeOutHigherFarmerOnly2CardsLvl(Hand &hand);

	void lordTakeOutHigherNormalLvl(Hand &hand);

	void lordTakeOutHigherRebuildLvl(Hand &hand);

	void farmerTakeOutLordOnly1CardLvl(Hand &hand);

	void farmerTakeOutLordOnly2CardsLvl(Hand &hand);

	void farmerMustTakeOutLordOnly1CardLvl(Hand &hand);

	void farmerMustTakeOutLordChargeLvl(Hand &hand);

	void goodFarmerOverOtherFarmerLvl(Hand &hand);

	void downGoodFarmerTakeOutLvl(Hand &hand);

	void downBadFarmerTakeOutLvl(Hand &hand);

	void upGoodFarmerTakeOutLvl(Hand &hand);

	void upBadFarmerTakeOutLvl(Hand &hand);

	void downFarmerTakeOutUpFarmerOnly1CardLvl(Hand &hand);

	void countHighHand(std::map<HandType, std::vector<Hand>> &allHands,
						std::map<HandType, int> &uniHighHands,
						std::map<HandType, int> &HighHands,
						std::map<HandType, int> &goodLowHands,
						int &trioCount);

	HandsMapSummary getHandsMapSummaryLvl(std::map<HandType, std::vector<Hand>> &allHands,
						std::map<HandType, int> &uniHighHands,
						std::map<HandType, int> &HighHands,
						std::map<HandType, int> &goodLowHands,
						int trioCount);

	int findOppHighestPoint(HandType type);

	int findOppHighestChain(HandType type, int len);

	int addAllValue(std::map<HandType, int> &handsCount);

	void findChargeHandFirstLvl(Hand &hand, bool typeCountFirst);

	bool mustHighLevel;
	
	int level;

	int aiSeat;

	int lordSeat;

	int downSeat;

	int upSeat;

	Position aiPosition;

	std::vector<int> leftoverCards;

	std::vector<int> aiCardsVec;

	CardsInfo playerInfo[PLAYER_NUM];

	int remainPoints[CARD_POINT_NUM];

	int otherPoints[CARD_POINT_NUM];

	std::map<HandType, std::vector<Hand>> handsMap;

	HandsMapSummary summary;

	std::map<HandType, int> uniHighHandCount;

	std::map<HandType, int> highHandCount;

	std::map<HandType, int> goodLowHandCount;

	int handsTrioCount;

	CardPoint lowestControl;

	int controlNum;

	int maxOppNumber;

	int minOppNumber;

    int maxScore;

	int curScore;

	int curCaller;
	
	Hand curHand;

	int curHandSeat;

	std::vector<std::pair<int, int>> callHistory;

	std::vector<std::pair<int, Hand>> history;

	int errCode;
};

#endif // OGLordRobotAI_h__
