#pragma once
#ifndef __ROBOT__	
#define __ROBOT__	
#include <vector>
const int ALL_CARDS_NUM = 54;

const int CARD_POINT_NUM = 16;

const int PLAYER_NUM = 3;

const int SOLO_CHAIN_CHARGE_LEN[8] = {7, 9, 11, 13, 15, 17, 18, 18};

const int LAIZI_TOTAL_COUNT = 4;

enum CardPoint
{
	CARD_3 = 0,
	CARD_4,
	CARD_5,
	CARD_6,
	CARD_7,
	CARD_8,
	CARD_9,
	CARD_T,
	CARD_J,
	CARD_Q,
	CARD_K,
	CARD_A,
	CARD_2,
	BLACK_JOKER,
	RED_JOKER,		//14	
	CARD_LZ,		//���
};


const char POINT_CHAR[CARD_POINT_NUM] = {'3', '4', '5', '6', '7', '8', '9', 'T', 'J', 'Q', 'K', 'A', '2', 'X', 'D','L'};

 //������ͻ �Ƶ���������Ӧ�����cpp�ļ�
enum HandType
{
	NOTHING,   
	TRIO_CHAIN_PAIR, // �ɻ������ӳ��
	TRIO_CHAIN_SOLO, // �ɻ������Ƴ��
	TRIO_CHAIN, // ��˳(�ɻ��������)
	PAIR_CHAIN, // ˫˳
	SOLO_CHAIN, // ��˳
	TRIO_PAIR, // ������
	TRIO_SOLO, // ������
	TRIO, // ����
	PAIR, // ����
	SOLO, // ����
	FOUR_DUAL_PAIR,  // �Ĵ���	
	FOUR_DUAL_SOLO,  // �Ĵ���	
	LZBOMB,	//���ը��
	//RUANBOMB��//��ը,֮����ʱ������
	BOMB,  // ը�� 
	NUKE,  // ���
	LZ,//���
};

enum Position
{
	LORD,  
	UPFARMER,  
	DOWNFARMER,
};


enum HandType;
enum Position;

struct CardsInfo
{
	int points[CARD_POINT_NUM];
	int total;  //�ܹ��ж�������
	int control;
};

enum HandProp
{
	NORMAL,
	HIGHEST,
	UNI_HIGHEST,	
};

struct Hand
{
	HandType type;  // ����
	CardPoint keyPoint;  // ���Ʊ�ǩ
	int len;  
	CardPoint kicker[5];
	HandProp prob;
	int laizi_num;
	std::vector<int> m_vecReplaceCards;
};

struct HandsMapSummary
{
	int realHandsCount; //��������������ը��
	int unChargeHandsCount; //�ǳ������
	int extraBombCount; //�ǿ�������ɵ�ը����
	int twoControlsHandsCount; //�ж��ſ����Ƶ�����
	int effectiveHandsCount; //��Ч����
	int handsTypeCount; //�������࣬������ը������˳���ֳ���
	int soloCount; //������
	CardPoint lowestSolo; //��С����
	CardPoint sencondLowestSolo; //��С����
};

struct HandsMapSummaryLvl
{
	int realHandsCount; //��ը��������
	int bombCount; //ը����
	int effectiveHandsCount; //��Ч����
	int handsTypeCount; //�������࣬������ը������˳���ֳ���
	int soloCount; //������
	CardPoint lowestSolo; //��С����
	CardPoint sencondLowestSolo; //��С����
};
#endif
