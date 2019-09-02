//
// Created by toney on 16/4/6.
//
// ţţ�������߼�

#ifndef SERVER_GAME_IMPLE_TABLE_H
#define SERVER_GAME_IMPLE_TABLE_H

#include <json/value.h>
#include "game_table.h"
#include "game_player.h"
#include "svrlib.h"
#include "pb/msg_define.pb.h"
#include "poker/sangong_logic.h"

using namespace svrlib;
using namespace std;
using namespace game_sangong;

class CGamePlayer;
class CGameRoom;

// ������Ϸ����
class CGameSanGongTable : public CGameTable
{
public:
    CGameSanGongTable(CGameRoom* pRoom,uint32 tableID,uint8 tableType);
    virtual ~CGameSanGongTable();

// ���ػ��ຯ��
public:
    virtual bool    CanEnterTable(CGamePlayer* pPlayer);
    virtual bool    CanLeaveTable(CGamePlayer* pPlayer);

    virtual void    GetTableFaceInfo(net::table_face_info* pInfo);

public:
    //��������
    virtual bool Init();
    virtual void ShutDown();

    //��λ����
    virtual void ResetTable();
    virtual void OnTimeTick();
    //��Ϸ��Ϣ
    virtual int  OnGameMessage(CGamePlayer* pPlayer,uint16 cmdID, const uint8* pkt_buf, uint16 buf_len);
 
public:
    // ��Ϸ��ʼ
    virtual bool OnGameStart();
    // ��Ϸ����
    virtual bool OnGameEnd(uint16 chairID,uint8 reason);
    //�û�ͬ��
    virtual bool OnActionUserOnReady(WORD wChairID,CGamePlayer* pPlayer);
    //��ҽ�����뿪
    virtual void OnPlayerJoin(bool isJoin,uint16 chairID,CGamePlayer* pPlayer);

public:
    // ���ͳ�����Ϣ(��������)
    virtual void    SendGameScene(CGamePlayer* pPlayer);
    
    void    CalcPlayerInfo(uint16 chairID,int64 winScore);
    // ������Ϸ����
    void    ResetGameData();
    // ���Ʒ���
	void	DispatchCard();
protected:
    // д�����log
    void    WriteOutCardLog(uint16 chairID,uint8 cardData[],uint8 cardCount,int64 score);
    // д��ׯ��λlog
    void    WriteBankerLog(uint16 chairID);

    // �Ƿ��ܹ���ʼ��Ϸ
    bool    IsCanStartGame();
    // �������Ƿ���ȷ
    bool    CheckJetton(uint16 chairID,int64 score);
    // ��û����˵�����
    int64   GetRobotJetton(uint16 chairID);

    uint16  GetPlayNum();

    void    InitBanker(bool isApply);
    void    SendCardToClient();

    // �������
    void    CalculateScore();

    // �����ǰ����
    void    CheckOverTime();

    //��Ϸ�¼�
protected:
	//��Ϸ״̬
	bool    IsUserPlaying(WORD wChairID);
    
	//��ע�¼�
    bool    OnUserAddScore(WORD wChairID, int64 lScore);
    //����ׯ��
    bool    OnUserApplyBanker(WORD wChairID,int32 score);
    //����
    bool    OnUserChangeCard(WORD wChairID);
    //���Ͱ���
    void    SendChangeCard(WORD wChairID,CGamePlayer* pPlayer);
    int32   GetChangeCardNum();

    //�����˲���
protected:
    bool    OnRobotOper();
    bool    OnRobotApplyBanker();
    bool    OnRobotReady();
    bool    OnRobotChangeCard();

    void    CheckAddRobot();
    void    CheckRobotLeave();
    void    SetRobotThinkTime();


	//��Ϸ����
protected:
	WORD							m_wBankerUser;							//ׯ���û�
    bool                            m_isNeedBanker;                         //�Ƿ���Ҫׯ���û�

	//�û�״̬
protected:
	BYTE							m_cbPlayStatus[GAME_PLAYER];			//��Ϸ״̬
    BYTE                            m_szApplyBanker[GAME_PLAYER];           //����ׯ��״̬
    BYTE                            m_szShowCardState[GAME_PLAYER];         //����״̬

	//�˿˱���
protected:
	BYTE							m_cbHandCardData[GAME_PLAYER][MAX_COUNT];       //�����˿�


	//��ע��Ϣ
protected:
    int64						    m_lTableScore[GAME_PLAYER];				//��ע��Ŀ
    BYTE                            m_cbTableCardType[GAME_PLAYER];         //��������
    BYTE                            m_winMultiple[GAME_PLAYER];             //��Ӯ����
    int64                           m_lWinScore[GAME_PLAYER];               //��Ӯ����
    int64                           m_lTurnMaxScore[GAME_PLAYER];           //�����ע

protected:
    CSangongLogic                   m_gameLogic;                            //��Ϸ�߼�
    CCooling                        m_coolRobot;                            //������CD

};

#endif //SERVER_GAME_IMPLE_TABLE_H

