//
// Created on 19/8/12.
//
#include <data_cfg_mgr.h>
#include "pb/msg_define.pb.h"
#include "game_room.h"
#include "player_mgr.h"
#include "game_imple_table.h"
#include "robot_oper_mgr.h"

using namespace std;
using namespace svrlib;

namespace {
	const static uint32 s_FreeTime = 3 * 1000;       // 空闲时间
	const static uint32 s_PlaceJettonTime = 10 * 1000;       // 下注时间
	const static uint32 s_DispatchTime = 25 * 1000;      // 发牌时间
};

CGameTable* CGameRoom::CreateTable(uint32 tableID) {
    CGameTable *pTable = NULL;
    switch(m_roomCfg.roomType)
    {
    case emROOM_TYPE_COMMON:           // 普通
        {
            pTable = new CGameTwoeightbarTable(this, tableID, emTABLE_TYPE_SYSTEM);
        }break;
    case emROOM_TYPE_MATCH:            // 比赛
        {
            pTable = new CGameTwoeightbarTable(this, tableID, emTABLE_TYPE_SYSTEM);
        }break;
    case emROOM_TYPE_PRIVATE:          // 私人房
        {
            pTable = new CGameTwoeightbarTable(this,tableID,emTABLE_TYPE_PLAYER);
        }break;
    default:
        {
            assert(false);
            return NULL;
        }break;
    }
    return pTable;
}

// 二八杠游戏桌子
CGameTwoeightbarTable::CGameTwoeightbarTable(CGameRoom* pRoom,uint32 tableID,uint8 tableType)
 : CGameTable(pRoom,tableID,tableType) {
	//个人下注
	for (uint8 i = 0; i < AREA_COUNT; ++i)
		m_userJettonScore[i].clear();
	m_pCurBanker = NULL; // 庄家
	m_tagControlPalyer.Init();
}

/// 重载基类函数 ///
bool CGameTwoeightbarTable::CanEnterTable(CGamePlayer* pPlayer){
	if (pPlayer->GetTable() != NULL)
		return false;
	// 限额进入
	if (IsFullTable() || GetPlayerCurScore(pPlayer) < GetEnterMin())
		return false;
	return true;
}

bool CGameTwoeightbarTable::CanLeaveTable(CGamePlayer* pPlayer) {
	if (m_pCurBanker == pPlayer || IsSetJetton(pPlayer->GetUID()))
		return false;
	if (IsInApplyList(pPlayer->GetUID()))
		return false;
	return true;
}

bool CGameTwoeightbarTable::IsFullTable() {
	if (m_mpLookers.size() >= 200)
		return true;
	return false;
}

void CGameTwoeightbarTable::GetTableFaceInfo(net::table_face_info* pInfo) {
	net::twoeight_table_info* pTableInfo = pInfo->mutable_twoeight();
	pTableInfo->set_tableid(GetTableID());
	pTableInfo->set_tablename(m_conf.tableName);
	if (m_conf.passwd.length() > 1)
		pTableInfo->set_is_passwd(1);
	else
		pTableInfo->set_is_passwd(0);
	pTableInfo->set_hostname(m_conf.hostName);
	pTableInfo->set_basescore(m_conf.baseScore);
	pTableInfo->set_consume(m_conf.consume);
	pTableInfo->set_entermin(m_conf.enterMin);
	pTableInfo->set_duetime(m_conf.dueTime);
	pTableInfo->set_feetype(m_conf.feeType);
	pTableInfo->set_feevalue(m_conf.feeValue);
	pTableInfo->set_card_time(s_PlaceJettonTime);
	pTableInfo->set_table_state(GetGameState());
	pTableInfo->set_sitdown(m_pHostRoom->GetSitDown());
	pTableInfo->set_apply_banker_condition(GetApplyBankerCondition());
	pTableInfo->set_apply_banker_maxscore(GetApplyBankerConditionLimit());
	pTableInfo->set_banker_max_time(m_BankerTimeLimit);
	pTableInfo->set_max_jetton_rate(m_iMaxJettonRate);
	LOG_DEBUG("roomid:%d,tableid:%d,tableName=%s", GetRoomID(), GetTableID(), m_conf.tableName);
}

// 初始化，桌子对象创建后执行的第一个函数
bool CGameTwoeightbarTable::Init() {
	if (m_pHostRoom == NULL)
		return false;

	SetGameState(net::TABLE_STATE_TWOEIGHT_FREE);

	m_vecAreaWinCount.resize(AREA_COUNT);
	m_vecAreaLostCount.resize(AREA_COUNT);

	for (uint32 i = 0; i < m_vecAreaWinCount.size(); ++i)
		m_vecAreaWinCount[i] = 0;
	for (uint32 i = 0; i < m_vecAreaLostCount.size(); ++i)
		m_vecAreaLostCount[i] = 0;

	m_vecPlayers.resize(GAME_PLAYER);
	for (uint8 i = 0; i < GAME_PLAYER; ++i)
		m_vecPlayers[i].Reset();
	
	m_BankerTimeLimit = CApplication::Instance().call<int>("bainiubankertime"); // 调用server_config.lua里定义的bainiubankertime函数

	m_robotApplySize = g_RandGen.RandRange(4, 8);//机器人申请人数
	m_robotChairSize = g_RandGen.RandRange(2, 4);//机器人座位数

	ReAnalysisParam();
	CRobotOperMgr::Instance().PushTable(this);
	SetMaxChairNum(GAME_PLAYER);
	return true;
}

bool CGameTwoeightbarTable::ReAnalysisParam() {
	LOG_DEBUG("reader json parse success - roomid:%d,tableid:%d", GetRoomID(), GetTableID());
	return true;
}

//复位桌子
void CGameTwoeightbarTable::ResetTable() {
	ResetGameData();
}

void CGameTwoeightbarTable::OnTimeTick() {
	OnTableTick();

	uint8 tableState = GetGameState();
	if (m_coolLogic.isTimeOut()) {
		switch (tableState) {
		case TABLE_STATE_TWOEIGHT_FREE:           // 空闲
		{
			if (OnGameStart()) {
				InitChessID();
				SetGameState(TABLE_STATE_TWOEIGHT_PLACE_JETTON);

				m_brc_table_status = emTABLE_STATUS_START;
				m_brc_table_status_time = s_PlaceJettonTime;

				//同步刷新百人场控制界面的桌子状态信息
				OnBrcControlFlushTableStatus();

			}
			else {
				m_coolLogic.beginCooling(s_FreeTime);
			}
		}break;
		case TABLE_STATE_TWOEIGHT_PLACE_JETTON:   // 下注时间
		{
			SetGameState(TABLE_STATE_TWOEIGHT_GAME_END);
			m_coolLogic.beginCooling(s_DispatchTime);
			DispatchTableCard();
			m_bIsChairRobotAlreadyJetton = false;
			m_bIsRobotAlreadyJetton = false;
			OnGameEnd(INVALID_CHAIR, GER_NORMAL);

			m_brc_table_status = emTABLE_STATUS_END;
			m_brc_table_status_time = 0;

			//同步刷新百人场控制界面的桌子状态信息
			OnBrcControlFlushTableStatus();

		}break;
		case TABLE_STATE_TWOEIGHT_GAME_END:       // 结束游戏
		{
			//切换庄家
			ClearTurnGameData();
			ChangeBanker(false);
			SetGameState(TABLE_STATE_NIUNIU_FREE);
			m_coolLogic.beginCooling(s_FreeTime);

			m_brc_table_status = emTABLE_STATUS_FREE;
			m_brc_table_status_time = 0;

			//同步刷新百人场控制界面的桌子状态信息
			OnBrcControlFlushTableStatus();

			//刷新上庄列表
			OnBrcControlFlushAppleList();

		}break;
		default:
			break;
		}
	}

	if (tableState == TABLE_STATE_TWOEIGHT_PLACE_JETTON && m_coolLogic.getPassTick() > 0) {
		OnChairRobotJetton();
		OnRobotJetton();
	}
}

void CGameTwoeightbarTable::OnRobotTick() {
	uint8 tableState = GetGameState();
	int64 passtick = m_coolLogic.getPassTick();
	
	if (tableState == net::TABLE_STATE_NIUNIU_PLACE_JETTON && m_coolLogic.getPassTick() > 500) {
		OnRobotPlaceJetton(m_chairRobotPlaceJetton);
		OnRobotPlaceJetton(m_RobotPlaceJetton);
	}
}

//游戏消息
int CGameTwoeightbarTable::OnGameMessage(CGamePlayer *pPlayer, uint16 cmdID, const uint8 *pkt_buf, uint16 buf_len) {
	if (pPlayer == NULL) {
		LOG_DEBUG("table recv - roomid:%d,tableid:%d,pPlayer:%p,cmdID:%d", GetRoomID(), GetTableID(), pPlayer, cmdID);
		return 0;
	}
	if (pPlayer != NULL && !pPlayer->IsRobot())
		LOG_DEBUG("table recv - roomid:%d,tableid:%d,uid:%d,cmdID:%d", GetRoomID(), GetTableID(), pPlayer->GetUID(), cmdID);

	switch (cmdID)
	{
	case net::C2S_MSG_TWOEIGHT_PLACE_JETTON:  // 用户加注
	{
		if (GetGameState() != TABLE_STATE_TWOEIGHT_PLACE_JETTON) {
			LOG_DEBUG("not jetton state can't jetton");
			return 0;
		}
		net::msg_twoeight_place_jetton_req msg;
		PARSE_MSG_FROM_ARRAY(msg);
		return OnUserPlaceJetton(pPlayer, msg.jetton_area(), msg.jetton_score());
	}break;
	case net::C2S_MSG_TWOEIGHT_APPLY_BANKER:  // 申请庄家
	{
		net::msg_twoeight_apply_banker msg;
		PARSE_MSG_FROM_ARRAY(msg);
		if (msg.apply_oper() == 1)
			return OnUserApplyBanker(pPlayer, msg.apply_score(), msg.auto_addscore());
		else
			return OnUserCancelBanker(pPlayer);
	}break;
	case net::C2S_MSG_TWOEIGHT_JUMP_APPLY_QUEUE:// 插队
	{
		net::msg_twoeight_jump_apply_queue_req msg;
		PARSE_MSG_FROM_ARRAY(msg);

		return OnUserJumpApplyQueue(pPlayer);
	}break;
	case net::C2S_MSG_TWOEIGHT_CONTINUOUS_PRESSURE_REQ: //
	{
		net::msg_player_continuous_pressure_jetton_req msg;
		PARSE_MSG_FROM_ARRAY(msg);

		return OnUserContinuousPressure(pPlayer, msg);
	}break;
	/*case net::C2S_MSG_BRC_CONTROL_ENTER_TABLE_REQ: //
	{
		net::msg_brc_control_user_enter_table_req msg;
		PARSE_MSG_FROM_ARRAY(msg);

		return OnBrcControlEnterControlInterface(pPlayer);
	}break;
	case net::C2S_MSG_BRC_CONTROL_LEAVE_TABLE_REQ: //
	{
		net::msg_brc_control_user_leave_table_req msg;
		PARSE_MSG_FROM_ARRAY(msg);

		return OnBrcControlPlayerLeaveInterface(pPlayer);
	}break;
	case net::C2S_MSG_BRC_CONTROL_FORCE_LEAVE_BANKER_REQ: //
	{
		net::msg_brc_control_force_leave_banker_req msg;
		PARSE_MSG_FROM_ARRAY(msg);

		return OnBrcControlApplePlayer(pPlayer, msg.uid());
	}break;
	case net::C2S_MSG_BRC_CONTROL_AREA_INFO_REQ: //
	{
		net::msg_brc_control_area_info_req msg;
		PARSE_MSG_FROM_ARRAY(msg);

		return OnBrcControlPlayerBetArea(pPlayer, msg);
	}break;*/
	default:
		return 0;
	}
	return 0;
}

// 用户断线或重连
bool CGameTwoeightbarTable::OnActionUserNetState(CGamePlayer* pPlayer, bool bConnected, bool isJoin) {
	if (bConnected) { // 断线重连
		if (isJoin) {
			pPlayer->SetPlayDisconnect(false);
			PlayerSetAuto(pPlayer, 0);
			SendTableInfoToClient(pPlayer);
			SendSeatInfoToClient(pPlayer);
			if (m_mpLookers.find(pPlayer->GetUID()) != m_mpLookers.end())
				NotifyPlayerJoin(pPlayer, true);
			SendLookerListToClient(pPlayer);
			SendGameScene(pPlayer);
			SendPlayLog(pPlayer);
		}
		uint32 uid = 0;
		int64 lockScore = 0;
		if (pPlayer != NULL) {
			uid = pPlayer->GetUID();
			lockScore = m_ApplyUserScore[pPlayer->GetUID()];
		}
		int64 lCurScore = GetPlayerCurScore(pPlayer);
		LOG_DEBUG("uid:%d,isJoin:%d,lockScore:%lld,lCurScore:%lld", uid, isJoin, lockScore, lCurScore);
	} else
		pPlayer->SetPlayDisconnect(true);
	return true;
}

//用户坐下
bool CGameTwoeightbarTable::OnActionUserSitDown(uint16 wChairID, CGamePlayer* pPlayer) {
	SendSeatInfoToClient();
	return true;
}
//用户起立
bool CGameTwoeightbarTable::OnActionUserStandUp(uint16 wChairID, CGamePlayer* pPlayer) {
	SendSeatInfoToClient();
	return true;
}

// 游戏开始
bool CGameTwoeightbarTable::OnGameStart() {
	if (m_pHostRoom == NULL)
		return false;
	if (!m_pHostRoom->GetCanGameStart()) {
		LOG_DEBUG("start_error - GameType:%d,roomid:%d,tableid:%d", GetGameType(), GetRoomID(), GetTableID());
		return false;
	}
	LOG_DEBUG("game_start - GameType:%d,roomid:%d,tableid:%d,m_pCurBanker:%p", GetGameType(), GetRoomID(), GetTableID(), m_pCurBanker);

	if (m_pCurBanker == NULL) {
		CheckRobotApplyBanker();
		ChangeBanker(false);
		return false;
	}
	m_coolLogic.beginCooling(s_PlaceJettonTime);

	net::msg_twoeight_start_rep gameStart;
	gameStart.set_time_leave(m_coolLogic.getCoolTick());
	gameStart.set_banker_score(m_lBankerScore);
	gameStart.set_banker_id(GetBankerUID());
	gameStart.set_banker_buyin_score(m_lBankerBuyinScore);
	SendMsgToAll(&gameStart, net::S2C_MSG_TWOEIGHT_START);
	OnTableGameStart();
	OnRobotStandUp();
	return true;
}

void CGameTwoeightbarTable::AddGameCount(){
	if (GetGameType() == net::GAME_CATE_TWOEIGHT)
		++m_uBairenTotalCount;
}

void CGameTwoeightbarTable::InitAreaSize(uint32 count) {
	if (GetGameType() == net::GAME_CATE_TWOEIGHT && count < 128) {
		m_vecAreaWinCount.resize(count);
		m_vecAreaLostCount.resize(count);
		for (uint32 i = 0; i < m_vecAreaWinCount.size(); ++i)
			m_vecAreaWinCount[i] = 0;
		for (uint32 i = 0; i < m_vecAreaLostCount.size(); ++i)
			m_vecAreaLostCount[i] = 0;
	}
}

void CGameTwoeightbarTable::OnTablePushAreaWin(uint32 index, int win) {
	if (index < m_vecAreaWinCount.size()) {
		if (win == 1)
			++m_vecAreaWinCount[index];
		else
			++m_vecAreaLostCount[index];
	}
}

//游戏结束
bool CGameTwoeightbarTable::OnGameEnd(uint16 chairID, uint8 reason) {
	LOG_DEBUG("game end:table:%d,%d", GetTableID(), m_wBankerTime);
	switch (reason)
	{
	case GER_NORMAL:		//常规结束
	{
		InitBlingLog();
		WriteBankerInfo();
		AddPlayerToBlingLog();

		//计算分数
		m_curr_banker_win = 0;

		int64 lBankerWinScore = CalculateScore();
		LOG_DEBUG("OnGameEnd GER_NORMAL - roomid:%d,tableid:%d,bankUid:%d,chessid:%s,m_lBankerScore:%lld,curScore:%lld,lBankerWinScore:%lld,m_curr_banker_win:%lld",
			GetRoomID(), GetTableID(), GetBankerUID(), GetChessID().c_str(), m_lBankerScore, GetPlayerCurScore(m_pCurBanker), lBankerWinScore, m_curr_banker_win);

		int64 bankerfee = CalcPlayerInfo(GetBankerUID(), lBankerWinScore, m_curr_banker_win, true);
		lBankerWinScore += bankerfee;
		int64 playerAllWinScore = 0; // 玩家总赢分
		if (IsBankerRealPlayer())
			playerAllWinScore = lBankerWinScore;
		WriteMaxCardType(GetBankerUID(), m_cbTableCardType[MAX_SEAT_INDEX - 1]);

		// 递增次数
		++m_wBankerTime;
		AddGameCount();

		//结束消息
		net::msg_twoeight_game_end msg;
		for (uint8 i = 0; i < MAX_SEAT_INDEX; ++i) {
			net::msg_cards* pCards = msg.add_table_cards();
			for (uint8 j = 0; j < SINGLE_CARD_NUM; ++j)
				pCards->add_cards(m_cbTableCardArray[i][j]);
		}
		for (uint8 i = 0; i < MAX_SEAT_INDEX; ++i)
			msg.add_card_types(m_cbTableCardType[i]);
		for (uint8 i = 0; i < AREA_COUNT; ++i)
			msg.add_win_multiple(m_winMultiple[i]);

		for (uint8 i = 0; i < AREA_COUNT; ++i)
			OnTablePushAreaWin(i, m_winMultiple[i] > 0);
		msg.set_total_gamecount(m_uBairenTotalCount);

		for (uint8 i = 0; i < AREA_COUNT; ++i) {
			msg.add_area_wincount(m_vecAreaWinCount[i]);
			msg.add_area_lostcount(m_vecAreaLostCount[i]);
		}

		msg.set_time_leave(m_coolLogic.getCoolTick());
		msg.set_banker_time(m_wBankerTime);
		msg.set_banker_win_score(lBankerWinScore);
		msg.set_banker_total_score(m_lBankerWinScore);
		msg.set_settle_accounts_type(m_cbBrankerSettleAccountsType);

		int64 lRobotScoreResult = 0;
		for (uint16 wUserIndex = 0; wUserIndex < GAME_PLAYER; ++wUserIndex) {
			CGamePlayer *pPlayer = GetPlayer(wUserIndex);
			if (pPlayer == NULL)
				continue;
			if (pPlayer->IsRobot())
				lRobotScoreResult += m_mpUserWinScore[pPlayer->GetUID()];
		}
		//发送旁观者积分
		for (map<uint32, CGamePlayer*>::iterator it_win_robot_score = m_mpLookers.begin(); it_win_robot_score != m_mpLookers.end(); ++it_win_robot_score) {
			CGamePlayer* pPlayer = it_win_robot_score->second;
			if (pPlayer == NULL)
				continue;
			if (pPlayer->IsRobot())
				lRobotScoreResult += m_mpUserWinScore[pPlayer->GetUID()];
		}
		if (lBankerWinScore > 0 && lRobotScoreResult < 0) {
			lRobotScoreResult = -lRobotScoreResult;
			int64 fee = -(lRobotScoreResult * m_conf.feeValue / PRO_DENO_10000);
			lRobotScoreResult += fee;
			lRobotScoreResult = -lRobotScoreResult;
		}

		 //发送积分
		for (uint16 wUserIndex = 0; wUserIndex < GAME_PLAYER; ++wUserIndex) {
			//设置成绩
			CGamePlayer *pPlayer = GetPlayer(wUserIndex);
			if (pPlayer == NULL)
				msg.add_player_score(0);
			else {
				int64 lUserScoreFree = CalcPlayerInfo(pPlayer->GetUID(), m_mpUserWinScore[pPlayer->GetUID()], m_mpWinScoreForFee[pPlayer->GetUID()]);
				lUserScoreFree = lUserScoreFree + m_mpUserWinScore[pPlayer->GetUID()];
				m_mpUserWinScore[pPlayer->GetUID()] = lUserScoreFree;
				msg.add_player_score(m_mpUserWinScore[pPlayer->GetUID()]);
				if (!pPlayer->IsRobot())
					playerAllWinScore += lUserScoreFree;
			}
		}

		// 投2个骰子
		for (uint8 i = 0; i < 2; ++i)
			msg.add_dice_value(g_RandGen.GetRandi(1, 6));

		for (uint16 wUserIndex = 0; wUserIndex < GAME_PLAYER; ++wUserIndex) {
			CGamePlayer *pPlayer = GetPlayer(wUserIndex);
			if (pPlayer == NULL)
				continue;
			msg.set_user_score(m_mpUserWinScore[pPlayer->GetUID()]);
			pPlayer->SendMsgToClient(&msg, net::S2C_MSG_TWOEIGHT_GAME_END);

			//精准控制统计
			OnBrcControlSetResultInfo(pPlayer->GetUID(), m_mpUserWinScore[pPlayer->GetUID()]);
		}

		//发送旁观者积分
		for (map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin(); it != m_mpLookers.end(); ++it) {
			CGamePlayer *pPlayer = it->second;
			if (pPlayer == NULL)
				continue;

			int64 lUserScoreFree = CalcPlayerInfo(pPlayer->GetUID(), m_mpUserWinScore[pPlayer->GetUID()], m_mpWinScoreForFee[pPlayer->GetUID()]);
			lUserScoreFree = lUserScoreFree + m_mpUserWinScore[pPlayer->GetUID()];
			m_mpUserWinScore[pPlayer->GetUID()] = lUserScoreFree;
			msg.set_user_score(lUserScoreFree);
			pPlayer->SendMsgToClient(&msg, net::S2C_MSG_TWOEIGHT_GAME_END);

			//精准控制统计
			OnBrcControlSetResultInfo(pPlayer->GetUID(), m_mpUserWinScore[pPlayer->GetUID()]);
			if (!pPlayer->IsRobot())
				playerAllWinScore += lUserScoreFree;
		}

		//奖池统计
		int64 lPlayerScoreResult = 0;
		for(uint16 wUserIndex = 0; wUserIndex < GAME_PLAYER; ++wUserIndex) {
			CGamePlayer *pPlayer = GetPlayer(wUserIndex);
			if (pPlayer == NULL)
				continue;
			if (!pPlayer->IsRobot())
				lPlayerScoreResult += m_mpUserWinScore[pPlayer->GetUID()];
		}
		for (map<uint32, CGamePlayer*>::iterator it_lookers = m_mpLookers.begin(); it_lookers != m_mpLookers.end(); ++it_lookers) {
			CGamePlayer *pPlayer = it_lookers->second;
			if (pPlayer == NULL)
				continue;
			if (!pPlayer->IsRobot())
				lPlayerScoreResult += m_mpUserWinScore[pPlayer->GetUID()];
		}

		if (m_pCurBanker != NULL && m_pCurBanker->IsRobot() && m_pHostRoom != NULL && lPlayerScoreResult != 0)
			m_pHostRoom->UpdateJackpotScore(-lPlayerScoreResult);
		if (m_pCurBanker != NULL && !m_pCurBanker->IsRobot() && m_pHostRoom != NULL && lRobotScoreResult != 0)
			m_pHostRoom->UpdateJackpotScore(lRobotScoreResult);

		//更新活跃福利数据            
		int64 curr_win = m_mpUserWinScore[m_aw_ctrl_uid];
		UpdateActiveWelfareInfo(m_aw_ctrl_uid, curr_win);

		LOG_DEBUG("OnGameEnd2 roomid:%d,tableid:%d,lPlayerScoreResult:%lld,lRobotScoreResult:%lld,playerAllWinScore:%lld",
			GetRoomID(), GetTableID(), lPlayerScoreResult, lRobotScoreResult, playerAllWinScore);

		//如果当前庄家为真实玩家，需要更新精准控制统计
		if (m_pCurBanker && !m_pCurBanker->IsRobot())
			OnBrcControlSetResultInfo(GetBankerUID(), lBankerWinScore);

		m_mpUserWinScore[GetBankerUID()] = 0;
		SaveBlingLog();
		CheckRobotCancelBanker();

		//同步所有玩家数据到控端
		//OnBrcFlushSendAllPlayerInfo();
		m_pHostRoom->UpdateStock(this, playerAllWinScore);
		OnTableGameEnd();
		return true;
	}break;
	case GER_DISMISS:		//游戏解散
	{
		LOG_ERROR("强制游戏解散 roomid:%d,tableid:%d", GetRoomID(), GetTableID());
		for (uint8 i = 0; i < GAME_PLAYER; ++i)
			if (m_vecPlayers[i].pPlayer != NULL)
				LeaveTable(m_vecPlayers[i].pPlayer);
		ResetTable();
		return true;
	}break;
	case GER_NETWORK_ERROR:		//用户强退
	case GER_USER_LEAVE:
	default:
		break;
	}
	//错误断言
	assert(false);
	return false;
}

// 玩家进入或离开
void CGameTwoeightbarTable::OnPlayerJoin(bool isJoin, uint16 chairID, CGamePlayer* pPlayer) {
	uint32 uid = 0;
	if (pPlayer != NULL)
		uid = pPlayer->GetUID();

	LOG_DEBUG("PlayerJoin -  uid:%d,isJoin:%d,chairID:%d,lCurScore:%lld", uid, isJoin, chairID, GetPlayerCurScore(pPlayer));
	UpdateEnterScore(isJoin, pPlayer);

	CGameTable::OnPlayerJoin(isJoin, chairID, pPlayer);

	if (isJoin) {
		SendApplyUser(pPlayer);
		SendGameScene(pPlayer);
		SendPlayLog(pPlayer);
	} else {
		OnUserCancelBanker(pPlayer);
		RemoveApplyBanker(pPlayer->GetUID());
		for (uint8 i = 0; i < AREA_COUNT; ++i)
			m_userJettonScore[i].erase(pPlayer->GetUID());
	}

	//刷新控制界面的玩家数据
	//if (!pPlayer->IsRobot())
	//	OnBrcFlushSendAllPlayerInfo();
}

// 发送场景信息(断线重连)
void CGameTwoeightbarTable::SendGameScene(CGamePlayer* pPlayer) {
	int64 lCurScore = GetPlayerCurScore(pPlayer);
	LOG_DEBUG("send game scene - uid:%d,m_gameState:%d,lCurScore:%lld", pPlayer->GetUID(), m_gameState, lCurScore);
	switch (m_gameState)
	{
	case net::TABLE_STATE_TWOEIGHT_FREE:          // 空闲状态
	{
		net::msg_twoeight_game_info_free_rep msg;
		msg.set_time_leave(m_coolLogic.getCoolTick());
		msg.set_banker_id(GetBankerUID());
		msg.set_banker_time(m_wBankerTime);
		msg.set_banker_win_score(m_lBankerWinScore);
		msg.set_banker_score(m_lBankerScore);
		msg.set_banker_buyin_score(m_lBankerBuyinScore);

		pPlayer->SendMsgToClient(&msg, net::S2C_MSG_TWOEIGHT_GAME_FREE_INFO);
	}break;
	case net::TABLE_STATE_TWOEIGHT_PLACE_JETTON:  // 游戏状态
	case net::TABLE_STATE_TWOEIGHT_GAME_END:      // 结束状态
	{
		net::msg_twoeight_game_info_play_rep msg;
		for (uint8 i = 0; i < AREA_COUNT; ++i)
			msg.add_all_jetton_score(m_allJettonScore[i]);
		msg.set_banker_id(GetBankerUID());
		msg.set_banker_time(m_wBankerTime);
		msg.set_banker_win_score(m_lBankerWinScore);
		msg.set_banker_score(m_lBankerScore);
		msg.set_banker_buyin_score(m_lBankerBuyinScore);
		msg.set_time_leave(m_coolLogic.getCoolTick());
		msg.set_game_status(m_gameState);
		for (uint8 i = 0; i < AREA_COUNT; ++i)
			msg.add_self_jetton_score(m_userJettonScore[i][pPlayer->GetUID()]);
		if (GetBankerUID() == pPlayer->GetUID() && m_needLeaveBanker)
			msg.set_need_leave_banker(1);
		else
			msg.set_need_leave_banker(0);

		pPlayer->SendMsgToClient(&msg, net::S2C_MSG_TWOEIGHT_GAME_PLAY_INFO);

		//刷新所有控制界面信息协议---用于断线重连的处理
		/*if (pPlayer->GetCtrlFlag()) {
			//刷新百人场桌子状态
			m_brc_table_status_time = m_coolLogic.getCoolTick();
			OnBrcControlFlushTableStatus(pPlayer);

			//发送控制区域信息
			OnBrcControlFlushAreaInfo(pPlayer);
			//发送所有真实玩家列表
			OnBrcControlSendAllPlayerInfo(pPlayer);
			//发送机器人总下注信息
			OnBrcControlSendAllRobotTotalBetInfo();
			//发送真实玩家总下注信息
			OnBrcControlSendAllPlayerTotalBetInfo();
			//发送申请上庄玩家列表
			OnBrcControlFlushAppleList();
		}*/

	}break;
	default:
		break;
	}
	SendLookerListToClient(pPlayer);
	SendApplyUser(pPlayer);
	SendFrontJettonInfo(pPlayer);
}

int64 CGameTwoeightbarTable::CalcPlayerInfo(uint32 uid, int64 winScore, int64 OnlywinScore, bool isBanker) {
	int64 fee = GetBrcFee(uid, OnlywinScore, true);
	winScore += fee;
	CalcPlayerGameInfoForBrc(uid, winScore, 0, true, isBanker);

	if (isBanker) {// 庄家池子减少响应筹码,否则账目不平
		m_lBankerWinScore += fee;
		//当前积分
		m_lBankerScore += fee;
	}
	LOG_DEBUG("report to lobby:%d winScore:%lld OnlywinScore:%lld fee:%lld", uid, winScore, OnlywinScore, fee);
	return fee;
}

// 重置每局游戏数据
void CGameTwoeightbarTable::ResetGameData() {
	//总下注数
	ZeroMemory(m_allJettonScore, sizeof(m_allJettonScore));
	ZeroMemory(m_playerJettonScore, sizeof(m_playerJettonScore));
	ZeroMemory(m_winMultiple, sizeof(m_winMultiple));

	//个人下注
	for (uint8 i = 0; i < AREA_COUNT; ++i)
		m_userJettonScore[i].clear();

	m_curr_bet_user.clear();
	//玩家成绩	
	m_mpUserWinScore.clear();
	//扑克信息
	ZeroMemory(m_cbTableCardArray, sizeof(m_cbTableCardArray));
	//庄家信息
	m_pCurBanker = NULL;
	m_wBankerTime = 0;
	m_lBankerWinScore = 0L;

	m_bankerAutoAddScore = 0;                   //自动补币
	m_needLeaveBanker = false;               //离开庄位

	m_wBankerTime = 0;							//做庄次数
	m_wBankerWinTime = 0;                       //胜利次数
	m_lBankerScore = 0;							//庄家积分
	m_lBankerWinScore = 0;						//累计成绩
	m_lBankerBuyinScore = 0;                    //庄家带入
	m_lBankerInitBuyinScore = 0;                //庄家初始带入
	m_lBankerWinMaxScore = 0;                   //庄家最大输赢
	m_lBankerWinMinScore = 0;                   //庄家最惨输赢
}

void CGameTwoeightbarTable::ClearTurnGameData() {
	//总下注数
	ZeroMemory(m_allJettonScore, sizeof(m_allJettonScore));
	ZeroMemory(m_playerJettonScore, sizeof(m_playerJettonScore));

	//个人下注
	for (uint8 i = 0; i < AREA_COUNT; ++i)
		m_userJettonScore[i].clear();

	m_curr_bet_user.clear();

	//玩家成绩	
	m_mpUserWinScore.clear();
	//扑克信息
	ZeroMemory(m_cbTableCardArray, sizeof(m_cbTableCardArray));
	ZeroMemory(m_isTableCardPointKill, sizeof(m_isTableCardPointKill));
}

// 写入出牌log
void CGameTwoeightbarTable::WriteOutCardLog(uint16 chairID, uint8 cardData[], int32 mulip, bool isPointKill) {
	uint8 cardType = TwoeightLogic::GetCardType(cardData);
	Json::Value logValue;
	logValue["p"] = chairID;
	logValue["m"] = mulip;
	logValue["cardtype"] = cardType;
	logValue["pk"] = isPointKill ? 1 : 0;
	for (uint32 i = 0; i < SINGLE_CARD_NUM; ++i)
		logValue["c"].append(cardData[i]);
	m_operLog["card"].append(logValue);
}

// 写入加注log
void CGameTwoeightbarTable::WriteAddScoreLog(uint32 uid, uint8 area, int64 score) {
	if (score == 0)
		return;
	Json::Value logValue;
	logValue["uid"] = uid;
	logValue["p"] = area;
	logValue["s"] = score;
	m_operLog["op"].append(logValue);
}

// 写入最大牌型
void CGameTwoeightbarTable::WriteMaxCardType(uint32 uid, uint8 cardType) {
	Json::Value logValue;
	logValue["uid"] = uid;
	logValue["mt"] = cardType;
	m_operLog["maxcard"].append(logValue);
}

// 写入庄家信息
void CGameTwoeightbarTable::WriteBankerInfo() {
	tagJackpotScore tmpJackpotScore;
	if (m_pHostRoom != NULL)
	    tmpJackpotScore = m_pHostRoom->GetJackpotScoreInfo();
	m_operLog["banker"] = GetBankerUID();
	m_operLog["cps"] = tmpJackpotScore.lCurPollScore;
	m_operLog["swp"] = tmpJackpotScore.uSysWinPro;
	m_operLog["slp"] = tmpJackpotScore.uSysLostPro;
}

//加注事件
bool CGameTwoeightbarTable::OnUserPlaceJetton(CGamePlayer* pPlayer, uint8 cbJettonArea, int64 lJettonScore) {
	//效验参数

	if (pPlayer != NULL && !pPlayer->IsRobot())
		LOG_DEBUG("table_recv - roomid:%d,tableid:%d,uid:%d,baner:%d,GetGameState:%d,cbJettonArea:%d,lJettonScore:%lld,curScore:%lld",
			GetRoomID(), GetTableID(), pPlayer->GetUID(), GetBankerUID(), GetGameState(), cbJettonArea, lJettonScore, GetPlayerCurScore(pPlayer));

	if (cbJettonArea > ID_DI_MEN || lJettonScore <= 0) {
		if (pPlayer != NULL && !pPlayer->IsRobot())
			LOG_DEBUG("jetton_failed - roomid:%d,tableid:%d,uid:%d,baner:%d,GetGameState:%d,cbJettonArea:%d,lJettonScore:%lld,curScore:%lld",
				GetRoomID(), GetTableID(), pPlayer->GetUID(), GetBankerUID(), GetGameState(), cbJettonArea, lJettonScore, GetPlayerCurScore(pPlayer));
		return false;
	}
	if (GetGameState() != net::TABLE_STATE_TWOEIGHT_PLACE_JETTON) {
		net::msg_twoeight_place_jetton_rep msg;
		msg.set_jetton_area(cbJettonArea);
		msg.set_jetton_score(lJettonScore);
		msg.set_result(net::RESULT_CODE_FAIL);

		//发送消息
		pPlayer->SendMsgToClient(&msg, net::S2C_MSG_TWOEIGHT_PLACE_JETTON_REP);

		if (pPlayer != NULL && pPlayer->IsRobot() == false)
			LOG_DEBUG("jetton_failed - roomid:%d,tableid:%d,uid:%d,baner:%d,GetGameState:%d,cbJettonArea:%d,lJettonScore:%lld,curScore:%lld",
				GetRoomID(), GetTableID(), pPlayer->GetUID(), GetBankerUID(), GetGameState(), cbJettonArea, lJettonScore, GetPlayerCurScore(pPlayer));

		return true;
	}
	//庄家判断
	if (pPlayer->GetUID() == GetBankerUID()) {
		if (pPlayer != NULL && !pPlayer->IsRobot())
			LOG_DEBUG("jetton_failed - roomid:%d,tableid:%d,uid:%d,baner:%d,GetGameState:%d,cbJettonArea:%d,lJettonScore:%lld,curScore:%lld",
				GetRoomID(), GetTableID(), pPlayer->GetUID(), GetBankerUID(), GetGameState(), cbJettonArea, lJettonScore, GetPlayerCurScore(pPlayer));
		return true;
	}

	//变量定义
	int64 lJettonCount = 0L;
	for (int nAreaIndex = 0; nAreaIndex < AREA_COUNT; ++nAreaIndex)
		lJettonCount += m_userJettonScore[nAreaIndex][pPlayer->GetUID()];
	if (!TableJettonLimmit(pPlayer, lJettonScore, lJettonCount)) {
		if (pPlayer != NULL && !pPlayer->IsRobot())
			LOG_DEBUG("table_jetton_limit - roomid:%d,tableid:%d,uid:%d,cbJettonArea:%d,lJettonScore:%lld,curScore:%lld,jettonmin:%lld",
				GetRoomID(), GetTableID(), pPlayer->GetUID(), cbJettonArea, lJettonScore, GetPlayerCurScore(pPlayer), GetJettonMin());

		net::msg_twoeight_place_jetton_rep msg;
		msg.set_jetton_area(cbJettonArea);
		msg.set_jetton_score(lJettonScore);
		msg.set_result(net::RESULT_CODE_FAIL);

		//发送消息
		pPlayer->SendMsgToClient(&msg, net::S2C_MSG_TWOEIGHT_PLACE_JETTON_REP);
		return true;
	}

	//玩家积分
	int64 lUserScore = GetPlayerCurScore(pPlayer);

	//合法校验
	if (lUserScore < lJettonCount + lJettonScore){
		if (pPlayer != NULL && !pPlayer->IsRobot())
			LOG_DEBUG("jetton_failed - roomid:%d,tableid:%d,uid:%d,baner:%d,GetGameState:%d,cbJettonArea:%d,lJettonScore:%lld,curScore:%lld,lJettonCount:%lld",
				GetRoomID(), GetTableID(), pPlayer->GetUID(), GetBankerUID(), GetGameState(), cbJettonArea, lJettonScore, GetPlayerCurScore(pPlayer), lJettonCount);
		return true;
	}
	//成功标识
	bool bPlaceJettonSuccess = true;
	//合法验证
	int64 lMaxUserJetton = GetUserMaxJetton(pPlayer/*, cbJettonArea*/);
	if (lMaxUserJetton >= lJettonScore) {
		//保存下注
		m_allJettonScore[cbJettonArea] += lJettonScore;
		if (!pPlayer->IsRobot())
			m_playerJettonScore[cbJettonArea] += lJettonScore;
		m_userJettonScore[cbJettonArea][pPlayer->GetUID()] += lJettonScore;
		if (pPlayer->IsRobot() == false)
			m_curr_bet_user.insert(pPlayer->GetUID());
	} else {
		if (pPlayer != NULL && !pPlayer->IsRobot())
			LOG_DEBUG("jetton_failed - roomid:%d,tableid:%d,uid:%d,baner:%d,GetGameState:%d,cbJettonArea:%d,lJettonScore:%lld,curScore:%lld,lJettonCount:%lld,lMaxUserJetton:%lld",
				GetRoomID(), GetTableID(), pPlayer->GetUID(), GetBankerUID(), GetGameState(), cbJettonArea, lJettonScore, GetPlayerCurScore(pPlayer), lJettonCount, lMaxUserJetton);
		bPlaceJettonSuccess = false;
	}

	if (bPlaceJettonSuccess) {
		RecordPlayerBaiRenJettonInfo(pPlayer, cbJettonArea, lJettonScore);
		OnAddPlayerJetton(pPlayer->GetUID(), lJettonScore);

		net::msg_twoeight_place_jetton_rep msg;
		msg.set_jetton_area(cbJettonArea);
		msg.set_jetton_score(lJettonScore);
		msg.set_result(net::RESULT_CODE_SUCCESS);
		//发送消息
		pPlayer->SendMsgToClient(&msg, net::S2C_MSG_TWOEIGHT_PLACE_JETTON_REP);

		net::msg_twoeight_place_jetton_broadcast broad;
		broad.set_uid(pPlayer->GetUID());
		broad.set_jetton_area(cbJettonArea);
		broad.set_jetton_score(lJettonScore);
		broad.set_total_jetton_score(m_allJettonScore[cbJettonArea]);

		SendMsgToAll(&broad, net::S2C_MSG_TWOEIGHT_PLACE_JETTON_BROADCAST);

		//刷新百人场控制界面的下注信息
		//OnBrcControlBetDeal(pPlayer);
	} else {
		net::msg_twoeight_place_jetton_rep msg;
		msg.set_jetton_area(cbJettonArea);
		msg.set_jetton_score(lJettonScore);
		msg.set_result(net::RESULT_CODE_FAIL);

		//发送消息
		pPlayer->SendMsgToClient(&msg, net::S2C_MSG_TWOEIGHT_PLACE_JETTON_REP);

	}
	return true;
}

// 玩家续押
bool CGameTwoeightbarTable::OnUserContinuousPressure(CGamePlayer* pPlayer, net::msg_player_continuous_pressure_jetton_req & msg)
{
	LOG_DEBUG("roomid:%d,tableid:%d,uid:%d,branker:%d,GetGameState:%d,info_size:%d",
		GetRoomID(), GetTableID(), pPlayer->GetUID(), GetBankerUID(), GetGameState(), msg.info_size());

	net::msg_player_continuous_pressure_jetton_rep rep;
	rep.set_result(net::RESULT_CODE_FAIL);
	if (msg.info_size() == 0 || GetGameState() != net::TABLE_STATE_TWOEIGHT_PLACE_JETTON || pPlayer->GetUID() == GetBankerUID())
	{
		pPlayer->SendMsgToClient(&rep, net::S2C_MSG_TWOEIGHT_CONTINUOUS_PRESSURE_REP);
		return false;
	}
	//效验参数
	int64 lTotailScore = 0;
	for (int i = 0; i < msg.info_size(); ++i) {
		net::bairen_jetton_info info = msg.info(i);
		if (info.score() <= 0 || info.area() > ID_DI_MEN) {
			pPlayer->SendMsgToClient(&rep, net::S2C_MSG_TWOEIGHT_CONTINUOUS_PRESSURE_REP);
			return false;
		}
		lTotailScore += info.score();
	}

	//变量定义
	int64 lJettonCount = 0L;
	for (int nAreaIndex = 0; nAreaIndex < AREA_COUNT; ++nAreaIndex)
		lJettonCount += m_userJettonScore[nAreaIndex][pPlayer->GetUID()];

	//玩家积分
	int64 lUserScore = GetPlayerCurScore(pPlayer);
	//合法校验
	if (GetJettonMin() > GetEnterScore(pPlayer) || (lUserScore < lJettonCount + lTotailScore)) {
		pPlayer->SendMsgToClient(&rep, net::S2C_MSG_TWOEIGHT_CONTINUOUS_PRESSURE_REP);
		LOG_DEBUG("the jetton more than you have - uid:%d", pPlayer->GetUID());
		return true;
	}

	int64 lUserMaxHettonScore = GetUserMaxJetton(pPlayer/*, 0*/);
	if (lUserMaxHettonScore < lTotailScore) {
		pPlayer->SendMsgToClient(&rep, net::S2C_MSG_TWOEIGHT_CONTINUOUS_PRESSURE_REP);
		LOG_DEBUG("error_pressu - uid:%d,lUserMaxHettonScore:%lld,lUserScore:%lld,lJettonCount:%lld,, lTotailScore:%lld,, GetJettonMin:%lld",
			pPlayer->GetUID(), lUserMaxHettonScore, lUserScore, lJettonCount, lTotailScore, GetJettonMin());

		return false;
	}

	for (int i = 0; i < msg.info_size(); ++i) {
		net::bairen_jetton_info info = msg.info(i);
		m_allJettonScore[info.area()] += info.score();
		if (!pPlayer->IsRobot()) {
			m_playerJettonScore[info.area()] += info.score();
			m_curr_bet_user.insert(pPlayer->GetUID());
		}
		m_userJettonScore[info.area()][pPlayer->GetUID()] += info.score();

		RecordPlayerBaiRenJettonInfo(pPlayer, info.area(), info.score());

		BYTE cbJettonArea = info.area();
		int64 lJettonScore = info.score();

		net::msg_twoeight_place_jetton_rep msg;
		msg.set_jetton_area(cbJettonArea);
		msg.set_jetton_score(lJettonScore);
		msg.set_result(net::RESULT_CODE_SUCCESS);
		//发送消息
		pPlayer->SendMsgToClient(&msg, net::S2C_MSG_TWOEIGHT_PLACE_JETTON_REP);

		net::msg_twoeight_place_jetton_broadcast broad;
		broad.set_uid(pPlayer->GetUID());
		broad.set_jetton_area(cbJettonArea);
		broad.set_jetton_score(lJettonScore);
		broad.set_total_jetton_score(m_allJettonScore[cbJettonArea]);

		SendMsgToAll(&broad, net::S2C_MSG_TWOEIGHT_PLACE_JETTON_BROADCAST);
	}

	rep.set_result(net::RESULT_CODE_SUCCESS);
	pPlayer->SendMsgToClient(&rep, net::S2C_MSG_TWOEIGHT_CONTINUOUS_PRESSURE_REP);

	//刷新百人场控制界面的下注信息
	//OnBrcControlBetDeal(pPlayer);

	return true;
}

//申请庄家
bool CGameTwoeightbarTable::OnUserApplyBanker(CGamePlayer* pPlayer, int64 bankerScore, uint8 autoAddScore) {
	int64 lCurScore = GetPlayerCurScore(pPlayer);
	if (pPlayer != NULL && !pPlayer->IsRobot())
		LOG_DEBUG("player  begin apply banker - roomid:%d,tableid:%d,uid:%d,bankerScore:%lld,lCurScore:%lld",
			GetRoomID(), GetTableID(), pPlayer->GetUID(), bankerScore, lCurScore);

	//构造变量
	net::msg_twoeight_apply_banker_rep msg;
	msg.set_apply_oper(1);
	msg.set_buyin_score(bankerScore);

	if (m_pCurBanker == pPlayer) {
		LOG_DEBUG("you is the banker");
		msg.set_result(net::RESULT_CODE_ERROR_STATE);
		pPlayer->SendMsgToClient(&msg, net::S2C_MSG_TWOEIGHT_APPLY_BANKER);
		return false;
	}
	if (IsSetJetton(pPlayer->GetUID())) {// 下注不能上庄
		msg.set_result(net::RESULT_CODE_ERROR_STATE);
		pPlayer->SendMsgToClient(&msg, net::S2C_MSG_TWOEIGHT_APPLY_BANKER);
		return false;
	}
	//合法判断
	int64 lUserScore = GetPlayerCurScore(pPlayer);
	if (bankerScore > lUserScore) {
		LOG_DEBUG("you not have more score:%d", pPlayer->GetUID());
		msg.set_result(net::RESULT_CODE_ERROR_PARAM);
		pPlayer->SendMsgToClient(&msg, net::S2C_MSG_TWOEIGHT_APPLY_BANKER);
		return false;
	}

	if (bankerScore < GetApplyBankerCondition() || bankerScore > GetApplyBankerConditionLimit()) {
		LOG_DEBUG("you score less than condition roomid:%d,tableid:%d,uid:%d,IsRobot:%d,%lld--%lld，faild",
			GetRoomID(), GetTableID(), pPlayer->GetUID(), pPlayer->IsRobot(), GetApplyBankerCondition(), GetApplyBankerConditionLimit());
		msg.set_result(net::RESULT_CODE_ERROR_PARAM);
		pPlayer->SendMsgToClient(&msg, net::S2C_MSG_TWOEIGHT_APPLY_BANKER);
		return false;
	}

	//存在判断
	for (uint32 nUserIdx = 0; nUserIdx < m_ApplyUserArray.size(); ++nUserIdx) {
		uint32 id = m_ApplyUserArray[nUserIdx]->GetUID();
		if (id == pPlayer->GetUID()) {
			LOG_DEBUG("you is in apply list roomid:%d,tableid:%d,uid:%d,IsRobot:%d", GetRoomID(), GetTableID(), pPlayer->GetUID(), pPlayer->IsRobot());
			msg.set_result(net::RESULT_CODE_ERROR_STATE);
			pPlayer->SendMsgToClient(&msg, net::S2C_MSG_TWOEIGHT_APPLY_BANKER);
			return false;
		}
	}

	//保存信息 
	m_ApplyUserArray.push_back(pPlayer);
	m_mpApplyUserInfo[pPlayer->GetUID()] = autoAddScore;
	LockApplyScore(pPlayer, bankerScore);

	msg.set_result(net::RESULT_CODE_SUCCESS);
	pPlayer->SendMsgToClient(&msg, net::S2C_MSG_TWOEIGHT_APPLY_BANKER);
	//切换判断
	if (GetGameState() == net::TABLE_STATE_TWOEIGHT_FREE && m_ApplyUserArray.size() == 1)
		ChangeBanker(false);

	FlushApplyUserSort();
	SendApplyUser();

	//刷新控制界面的上庄列表
	OnBrcControlFlushAppleList();

	int64 lCurScoreFinish = GetPlayerCurScore(pPlayer);
	LOG_DEBUG("player finish apply banker - uid:%d,bankerScore:%lld,lCurScore:%lld", pPlayer->GetUID(), bankerScore, lCurScoreFinish);
	return true;
}

// 坐庄插队
bool CGameTwoeightbarTable::OnUserJumpApplyQueue(CGamePlayer* pPlayer) {
	LOG_DEBUG("player jump queue:%d", pPlayer->GetUID());
	int64 cost = CDataCfgMgr::Instance().GetJumpQueueCost();
	net::msg_twoeight_jump_apply_queue_rep msg;
	if (pPlayer->GetAccountValue(emACC_VALUE_COIN) < cost) {
		LOG_DEBUG("the jump cost can't pay:%lld", cost);
		msg.set_result(net::RESULT_CODE_CION_ERROR);
		pPlayer->SendMsgToClient(&msg, net::S2C_MSG_TWOEIGHT_JUMP_APPLY_QUEUE);
		return false;
	}
	//存在判断
	for (uint32 nUserIdx = 0; nUserIdx < m_ApplyUserArray.size(); ++nUserIdx) {
		uint32 id = m_ApplyUserArray[nUserIdx]->GetUID();
		if (id == pPlayer->GetUID()) {
			if (nUserIdx == 0) {
				LOG_DEBUG("you is the first queue");
				return false;
			}
			m_ApplyUserArray.erase(m_ApplyUserArray.begin() + nUserIdx);
			m_ApplyUserArray.insert(m_ApplyUserArray.begin(), pPlayer);

			SendApplyUser(NULL);
			msg.set_result(net::RESULT_CODE_SUCCESS);
			pPlayer->SendMsgToClient(&msg, net::S2C_MSG_TWOEIGHT_JUMP_APPLY_QUEUE);

			//刷新控制界面的上庄列表
			OnBrcControlFlushAppleList();

			cost = -cost;
			pPlayer->SyncChangeAccountValue(emACCTRAN_OPER_TYPE_JUMPQUEUE, 0, 0, cost, 0, 0, 0, 0);
			return true;
		}
	}

	return false;
}

// 取消申请庄家
bool CGameTwoeightbarTable::OnUserCancelBanker(CGamePlayer* pPlayer) {
	LOG_DEBUG("cance banker:%d", pPlayer->GetUID());

	net::msg_twoeight_apply_banker_rep msg;
	msg.set_apply_oper(0);
	msg.set_result(net::RESULT_CODE_SUCCESS);

	//前三局不能下庄
	if (pPlayer->GetUID() == GetBankerUID() && m_wBankerTime < 3)
		return false;
	//当前庄家 
	if (pPlayer->GetUID() == GetBankerUID() && GetGameState() != net::TABLE_STATE_TWOEIGHT_FREE) {
		//发送消息
		LOG_DEBUG("the game is run,you can't cance banker");
		m_needLeaveBanker = true;
		pPlayer->SendMsgToClient(&msg, net::S2C_MSG_TWOEIGHT_APPLY_BANKER);
		return true;
	}
	// 存在判断
	for (uint16 i = 0; i < m_ApplyUserArray.size(); ++i) {
		//获取玩家
		CGamePlayer *pTmp = m_ApplyUserArray[i];
		//条件过滤
		if (pTmp != pPlayer)
			continue;
		//删除玩家
		RemoveApplyBanker(pPlayer->GetUID());
		if (m_pCurBanker != pPlayer)
			pPlayer->SendMsgToClient(&msg, net::S2C_MSG_TWOEIGHT_APPLY_BANKER);
		else if (m_pCurBanker == pPlayer)
			//切换庄家 
			ChangeBanker(true);
		SendApplyUser();
		return true;
	}
	return false;
}

// 取得桌面4组牌的小到大排序
void CGameTwoeightbarTable::GetCardSortIndex(uint8 uArSortIndex[]) {
	uint8 cbTableCard[MAX_SEAT_INDEX][SINGLE_CARD_NUM] = { {0} };
	memcpy(cbTableCard, m_cbTableCardArray, sizeof(m_cbTableCardArray));

	uint8 uArSortCardIndex[MAX_SEAT_INDEX] = { 0,1,2,3 };

	for (uint8 i = 0; i < MAX_SEAT_INDEX - 1; ++i)
		for (uint8 j = i + 1; j < MAX_SEAT_INDEX; ++j) {
			int multiple;
			bool bIsSwapCardIndex = (TwoeightLogic::CompareCard(cbTableCard[j], cbTableCard[i], multiple) == 1);
			if (bIsSwapCardIndex) {
				uint8 tmp[SINGLE_CARD_NUM];
				memcpy(tmp, cbTableCard[j], SINGLE_CARD_NUM);
				memcpy(cbTableCard[j], cbTableCard[i], SINGLE_CARD_NUM);
				memcpy(cbTableCard[i], tmp, SINGLE_CARD_NUM);

				uint8 uTempIndex = uArSortCardIndex[i];
				uArSortCardIndex[i] = uArSortCardIndex[j];
				uArSortCardIndex[j] = uTempIndex;
			}
		}

	memcpy(uArSortIndex, uArSortCardIndex, MAX_SEAT_INDEX);
}

// 指定牌组是否满足规则
bool CGameTwoeightbarTable::IsCurTableCardRuleAllow(uint8 cbTableCardArray[][SINGLE_CARD_NUM]) {
	return true;
}

// 获取非机器人玩家赢金币数
int64 CGameTwoeightbarTable::GetPlayerWinScore(uint8 cbTableCardArray[][SINGLE_CARD_NUM]) {
	return 0;
}

bool CGameTwoeightbarTable::SetControlPlayerWin(uint32 control_uid) {
	return false;
}

bool CGameTwoeightbarTable::SetControlPlayerLost(uint32 control_uid) {
	return false;
}

bool CGameTwoeightbarTable::DosWelfareCtrl() {
	return true;
}

// 非福利控制
int CGameTwoeightbarTable::NotWelfareCtrl() {
	bool bBrankerIsRobot = false;
	bool bBrankerIsControl = false;
	bool bBrankerIsPlayer = false;
	bool bIsControlPlayerIsJetton = false;
	bool bIsFalgControl = false;

	uint32 control_uid = m_tagControlPalyer.uid;
	uint32 game_count = m_tagControlPalyer.count;
	uint32 control_type = m_tagControlPalyer.type;

	int ret = 0; 
	/*if (m_pCurBanker != NULL) {
		bBrankerIsRobot = m_pCurBanker->IsRobot();
		bBrankerIsPlayer = !bBrankerIsRobot;
		if (control_uid == m_pCurBanker->GetUID()) {
			bBrankerIsControl = true;
			ret = 1;
		}
	}

	if (bBrankerIsControl && game_count > 0 && (control_type == GAME_CONTROL_WIN || control_type == GAME_CONTROL_LOST)) {
		if (control_type == GAME_CONTROL_WIN)
			bIsFalgControl = SetPlayerBrankerWin();
		if (control_type == GAME_CONTROL_LOST)
			bIsFalgControl = SetPlayerBrankerLost();
		if (bIsFalgControl && m_tagControlPalyer.count > 0)
			if (m_pHostRoom != NULL)
				m_pHostRoom->SynControlPlayer(GetTableID(), m_tagControlPalyer.uid, -1, m_tagControlPalyer.type);
	}


	if (!bBrankerIsControl && control_uid != 0 && game_count > 0 && control_type != GAME_CONTROL_CANCEL)
		for (uint8 i = 0; i < AREA_COUNT; ++i)
			if (m_userJettonScore[i][control_uid] > 0) {
				bIsControlPlayerIsJetton = true;
				ret = 1;
				break;
			}

	if (bIsControlPlayerIsJetton && game_count > 0 && (control_type == GAME_CONTROL_WIN || control_type == GAME_CONTROL_LOST)) {
		if (control_type == GAME_CONTROL_WIN)
			bIsFalgControl = SetControlPlayerWin(control_uid);
		if (control_type == GAME_CONTROL_LOST)
			bIsFalgControl = SetControlPlayerLost(control_uid);
		if (bIsFalgControl && m_tagControlPalyer.count > 0)
			if (m_pHostRoom != NULL)
				m_pHostRoom->SynControlPlayer(GetTableID(), m_tagControlPalyer.uid, -1, m_tagControlPalyer.type);
	}

	bool isUserPlaceJetton = IsUserPlaceJetton();
	// 是否触发点杀
	if (ret == 0 && isUserPlaceJetton && SetTableCardPointKill())
		ret = 4;
	// modify by har end

	// 库存控制
	if (ret == 0 && SetStockWinLose())
		ret = 6;*/

	LOG_DEBUG("CGameBaiNiuTable::NotWelfareCtrl - roomid:%d,tableid:%d,bBrankerIsControl:%d,bIsControlPlayerIsJetton:%d,bBrankerIsRobot:%d,robotBankerWinPro:%d,robotBankerMaxCardPro:%d,bIsControlPlayerIsJetton:%d,control_uid:%d,control_type:%d,game_count:%d,bIsFalgControl:%d,bBrankerIsPlayer:%d,ret:%d",
		m_pHostRoom->GetRoomID(), GetTableID(), bBrankerIsControl, bIsControlPlayerIsJetton, bBrankerIsRobot, m_robotBankerWinPro, m_robotBankerMaxCardPro, bIsControlPlayerIsJetton, control_uid, control_type, game_count, bIsFalgControl, bBrankerIsPlayer, ret);

	return ret;
}

//发送扑克
bool CGameTwoeightbarTable::DispatchTableCard() {
	//重新洗牌
	TwoeightLogic::RandCardList(m_cbTableCardArray);

	bool bAreaCtrl = false; // OnBrcAreaControl(); // TODO: 点控等后面再做
	SetIsAllRobotOrPlayerJetton(IsAllRobotOrPlayerJetton());
	LOG_DEBUG("bAreaCtrl:%d,bIsDispatchTableCardStock:%d", bAreaCtrl, GetIsAllRobotOrPlayerJetton());

	int nNotWelfareCtrlRet = 0; // 非福利控制返回值
	if (bAreaCtrl)
	    return true;
 
	nNotWelfareCtrlRet = NotWelfareCtrl();

		/*bool static bWinShunMen0, bWinTianMen0, bWinDiMen0;
		uint8 ShunMultiple0, TianMultiple0, DiMultiple0;
		ShunMultiple0 = 1;
		TianMultiple0 = 1;
		DiMultiple0 = 1;

		DeduceWinner(bWinShunMen0, bWinTianMen0, bWinDiMen0, ShunMultiple0, TianMultiple0, DiMultiple0);

		int64 oldPlayerWinScore = GetPlayerWinScore(m_cbTableCardArray);
		int allWLNeedChangeIndex = -1;
		// 将5组牌的大小按小到大排序
		vector<uint8> vSortCardIndexs; // 牌索引数组，按牌面值从小到大排序
		SortCardIndexs(m_cbTableCardArray, vSortCardIndexs);
		if (vSortCardIndexs[0] == 0 || vSortCardIndexs[4] == 0) { // 庄家通输或通赢
			if (nNotWelfareCtrlRet != 4) {
				m_bIsConputeBankerAllWinLose = true;
				++m_bankerAllWinLoseCount;
				if (m_bankerAllWinLoseCount > m_confBankerAllWinLoseLimitCount) {
					int randIndex = g_RandGen.RandRange(1, 3);
					allWLNeedChangeIndex = vSortCardIndexs[randIndex];
					uint8 tmp[5];
					memcpy(tmp, m_cbTableCardArray[0], 5);
					memcpy(m_cbTableCardArray[0], m_cbTableCardArray[allWLNeedChangeIndex], 5);
					memcpy(m_cbTableCardArray[allWLNeedChangeIndex], tmp, 5);
				}
			}
		}

		int oldBankerAllWinLoseComputeCount = m_bankerAllWinLoseComputeCount;
		int oldBankerAllWinLoseCount = m_bankerAllWinLoseCount;
		if (m_bIsConputeBankerAllWinLose) {
			if (nNotWelfareCtrlRet != 4 || (vSortCardIndexs[0] != 0 && vSortCardIndexs[4] != 0)) {
				oldBankerAllWinLoseComputeCount = ++m_bankerAllWinLoseComputeCount;
				if (m_bankerAllWinLoseComputeCount >= m_confBankerAllWinLoseMaxCount) {
					m_bankerAllWinLoseComputeCount = 0;
					m_bankerAllWinLoseCount = 0;
					m_bIsConputeBankerAllWinLose = false;
				}
			}
		}

		bool static bWinShunMen, bWinTianMen, bWinDiMen;
		uint8 ShunMultiple, TianMultiple, DiMultiple;
		ShunMultiple = 1;
		TianMultiple = 1;
		DiMultiple = 1;
		DeduceWinner(bWinShunMen, bWinTianMen, bWinDiMen, ShunMultiple, TianMultiple, DiMultiple);
		bool bBrankerIsRobot = false;
		if (m_pCurBanker && m_pCurBanker->IsRobot())
			bBrankerIsRobot = true;
		int64 playerWinScore = GetPlayerWinScore(m_cbTableCardArray);

		LOG_DEBUG("DispatchTableCard - roomid:%d,tableid:%d,bAreaCtrl:%d,ChessWelfare:%d,bBrankerIsRobot:%d,nNotWelfareCtrlRet=%d,bankerAllWinLoseComputeCount:%d,bankerAllWinLoseCount:%d,allWLNeedChangeIndex:%d, vSortCardIndexs:%d-%d-%d-%d-%d, old_win:%d-%d-%d,old_multiple:%d-%d-%d, win:%d-%d-%d,multiple:%d-%d-%d, oldPlayerWinScore:%lld,playerWinScore:%lld",
			GetRoomID(), GetTableID(), bAreaCtrl, GetChessWelfare(),
			bBrankerIsRobot, nNotWelfareCtrlRet, oldBankerAllWinLoseComputeCount, oldBankerAllWinLoseCount, allWLNeedChangeIndex,
			vSortCardIndexs[0], vSortCardIndexs[1], vSortCardIndexs[2], vSortCardIndexs[3], vSortCardIndexs[4],
			bWinShunMen0, bWinTianMen0, bWinDiMen0, ShunMultiple0, TianMultiple0, DiMultiple0,
			bWinShunMen, bWinTianMen, bWinDiMen, ShunMultiple, TianMultiple, DiMultiple, oldPlayerWinScore, playerWinScore);
*/
	return true;
}

//发送庄家
void CGameTwoeightbarTable::SendApplyUser(CGamePlayer* pPlayer) {
	net::msg_twoeight_apply_list msg;
	for (uint32 nUserIdx = 0; nUserIdx < m_ApplyUserArray.size(); ++nUserIdx) {
		CGamePlayer *pTmp = m_ApplyUserArray[nUserIdx];
		//庄家判断
		if (pTmp == m_pCurBanker)
			continue;
		msg.add_player_ids(pTmp->GetUID());
		msg.add_apply_score(m_ApplyUserScore[pTmp->GetUID()]);
	}
	LOG_DEBUG("发送庄家列表:%d", msg.player_ids_size());
	if (pPlayer)
		pPlayer->SendMsgToClient(&msg, net::S2C_MSG_TWOEIGHT_APPLY_LIST);
	else
		SendMsgToAll(&msg, net::S2C_MSG_TWOEIGHT_APPLY_LIST);
}

//排序庄家,按上分数从大到小排序
void CGameTwoeightbarTable::FlushApplyUserSort() {
	if (m_ApplyUserArray.size() > 1)
		for (uint32 i = 0; i < m_ApplyUserArray.size() - 1; ++i)
			for (uint32 j = i + 1; j < m_ApplyUserArray.size(); ++j)
				if (CompareApplyBankers(m_ApplyUserArray[i], m_ApplyUserArray[j])) {
					CGamePlayer* pTmp = m_ApplyUserArray[i];
					m_ApplyUserArray[i] = m_ApplyUserArray[j];
					m_ApplyUserArray[j] = pTmp;
				}
}

//更换庄家
bool CGameTwoeightbarTable::ChangeBanker(bool bCancelCurrentBanker) {
	//切换标识
	bool bChangeBanker = false;

	//取消当前
	if (bCancelCurrentBanker) {
		CalcBankerScore();
		TakeTurns();
		bChangeBanker = true;
	} else if (m_pCurBanker != NULL) { //轮庄判断
		//自动补币
		AutoAddBankerScore();
		//次数判断
		if (m_needLeaveBanker || GetBankerTimeLimit() <= m_wBankerTime || m_lBankerScore < GetApplyBankerCondition()) {
			LOG_DEBUG("the timesout or the score less,you down banker:%d-%lld", m_wBankerTime, m_lBankerScore);
			CalcBankerScore();
			TakeTurns();
			bChangeBanker = true;
		}
	} else if (m_pCurBanker == NULL && m_ApplyUserArray.size() != 0) { //系统做庄
		//轮换判断
		TakeTurns();
		bChangeBanker = true;
	}
	//切换判断
	if (bChangeBanker) {
		//设置变量
		m_wBankerTime = 0;
		m_lBankerWinScore = 0;

		//发送消息
		net::msg_twoeight_change_banker msg;
		msg.set_banker_user(GetBankerUID());
		msg.set_banker_score(m_lBankerScore);

		SendMsgToAll(&msg, net::S2C_MSG_TWOEIGHT_CHANGE_BANKER);

		SendApplyUser(NULL);
	}

	return bChangeBanker;
}

//轮换判断
void CGameTwoeightbarTable::TakeTurns() {
	if (GetGameState() != net::TABLE_STATE_TWOEIGHT_FREE) {
		return;
	}
	vector<uint32> delIDs;
	for (uint32 i = 0; i < m_ApplyUserArray.size(); ++i) {
		//获取分数
		CGamePlayer *pPlayer = m_ApplyUserArray[i];
		if (pPlayer->GetNetState() == 0) {
			delIDs.push_back(pPlayer->GetUID());
			continue;
		}
		if (m_ApplyUserScore[pPlayer->GetUID()] >= GetApplyBankerCondition()) {
			m_pCurBanker = pPlayer;
			m_lBankerScore = m_ApplyUserScore[pPlayer->GetUID()];
			m_lBankerBuyinScore = m_lBankerScore;      //庄家带入
			m_lBankerInitBuyinScore = m_lBankerBuyinScore;

			m_bankerAutoAddScore = m_mpApplyUserInfo[pPlayer->GetUID()];//自动补币
			m_needLeaveBanker = false;

			RemoveApplyBanker(pPlayer->GetUID());
			StandUpBankerSeat(pPlayer);

			m_BankerTimeLimit = CApplication::Instance().call<int>("bainiubankertime");
			break;
		}
	}
	for (uint32 i = 0; i < delIDs.size(); ++i)
		RemoveApplyBanker(delIDs[i]);
}

//结算庄家
void CGameTwoeightbarTable::CalcBankerScore()
{
	if (m_pCurBanker == NULL)
		return;
	net::msg_twoeight_banker_calc_rep msg;
	msg.set_banker_time(m_wBankerTime);
	msg.set_win_count(m_wBankerWinTime);
	msg.set_buyin_score(m_lBankerBuyinScore);
	msg.set_win_score(m_lBankerWinScore);
	msg.set_win_max(m_lBankerWinMaxScore);
	msg.set_win_min(m_lBankerWinMinScore);

	m_pCurBanker->SendMsgToClient(&msg, net::S2C_MSG_TWOEIGHT_BANKER_CALC);

	int64 score = m_lBankerWinScore;
	int32 pro = 0;
	switch (m_wBankerTime)
	{
	case 3:
		pro = 8;
		break;
	case 4:
		pro = 6;
		break;
	case 5:
		pro = 5;
		break;
	default:
		break;
	}
	if (score > 200 && pro > 0) {
		int64 decScore = score * pro / PRO_DENO_100;
		LOG_DEBUG("提前下庄扣分:%lld", decScore);
		m_pCurBanker->SyncChangeAccountValue(emACCTRAN_OPER_TYPE_FEE, 2, 0, -decScore, 0, 0, 0, 0);
	}

	LOG_DEBUG("the turn the banker win:%lld,rest:%lld,buyin:%lld", score, m_lBankerScore, m_lBankerBuyinScore);
	RemoveApplyBanker(m_pCurBanker->GetUID());

	//设置庄家
	m_pCurBanker = NULL;
	m_robotApplySize = g_RandGen.RandRange(4, 8);//机器人申请人数
	m_robotChairSize = g_RandGen.RandRange(5, 7);//机器人座位数

	ResetGameData();
}
//自动补币
void CGameTwoeightbarTable::AutoAddBankerScore() {
	//轮庄判断
	if (m_pCurBanker == NULL || m_bankerAutoAddScore == 0 || m_needLeaveBanker || GetBankerTimeLimit() <= m_wBankerTime)
		return;

	int64 diffScore = m_lBankerInitBuyinScore - m_lBankerScore;
	int64 canAddScore = GetPlayerCurScore(m_pCurBanker) - m_lBankerScore;

	LOG_DEBUG("1 - roomid:%d,tableid:%d,uid:%d,GetApplyBankerCondition:%lld,m_lBankerInitBuyinScore:%lld,curScore:%lld,m_lBankerBuyinScore:%lld,m_lBankerScore:%lld,canAddScore:%lld,diffScore:%lld",
		GetRoomID(), GetTableID(), GetBankerUID(), GetApplyBankerCondition(), m_lBankerInitBuyinScore, GetPlayerCurScore(m_pCurBanker), m_lBankerBuyinScore, m_lBankerScore, canAddScore, diffScore);

	//判断金币是否够
	if (diffScore <= 0)
	{
		return;
	}
	if (canAddScore <= 0)
	{
		return;
	}
	if (canAddScore < diffScore)
	{
		diffScore = canAddScore;
	}
	if ((m_lBankerBuyinScore + diffScore) < GetApplyBankerCondition())
	{
		return;
	}

	m_lBankerBuyinScore += diffScore;
	m_lBankerScore += diffScore;

	net::msg_twoeight_add_bankerscore_rep msg;
	msg.set_buyin_score(diffScore);

	LOG_DEBUG("5 - roomid:%d,tableid:%d,uid:%d,GetApplyBankerCondition:%lld,m_lBankerInitBuyinScore:%lld,curScore:%lld,m_lBankerBuyinScore:%lld,m_lBankerScore:%lld,canAddScore:%lld,diffScore:%lld",
		GetRoomID(), GetTableID(), GetBankerUID(), GetApplyBankerCondition(), m_lBankerInitBuyinScore, GetPlayerCurScore(m_pCurBanker), m_lBankerBuyinScore, m_lBankerScore, canAddScore, diffScore);

	m_pCurBanker->SendMsgToClient(&msg, net::S2C_MSG_TWOEIGHT_ADD_BANKER_SCORE);
}

//发送游戏记录
void CGameTwoeightbarTable::SendPlayLog(CGamePlayer* pPlayer) {
	net::msg_twoeight_play_log_rep msg;
	for (uint16 i = 0; i < m_vecRecord.size(); ++i) {
		net::twoeight_play_log* plog = msg.add_logs();
		stTwoeightGameRecord& record = m_vecRecord[i];
		for (uint16 j = 0; j < AREA_COUNT; ++j)
			plog->add_seats_win(record.wins[j]);
	}
	LOG_DEBUG("发送牌局记录:%d", msg.logs_size());
	if (pPlayer != NULL)
		pPlayer->SendMsgToClient(&msg, net::S2C_MSG_TWOEIGHT_PLAY_LOG);
	else
		SendMsgToAll(&msg, net::S2C_MSG_TWOEIGHT_PLAY_LOG);
}

//最大下注
int64 CGameTwoeightbarTable::GetUserMaxJetton(CGamePlayer* pPlayer/*, uint8 cbJettonArea*/) {
	int iTimer = m_iMaxJettonRate;
	//已下注额
	int64 lNowJetton = 0;
	for (int nAreaIndex = 0; nAreaIndex < AREA_COUNT; ++nAreaIndex)
		lNowJetton += m_userJettonScore[nAreaIndex][pPlayer->GetUID()];
	//庄家金币
	int64 lBankerScore = 0;
	if (m_pCurBanker != NULL)
		lBankerScore = m_lBankerScore;
	for (int nAreaIndex = 0; nAreaIndex < AREA_COUNT; ++nAreaIndex)
		lBankerScore -= m_allJettonScore[nAreaIndex] * iTimer;

	//个人限制
	int64 lMeMaxScore = (GetPlayerCurScore(pPlayer) - lNowJetton * iTimer) / iTimer;

	//庄家限制
	lMeMaxScore = min(lMeMaxScore, lBankerScore / iTimer);

	//非零限制
	lMeMaxScore = MAX(lMeMaxScore, 0);

	if (pPlayer != NULL && !pPlayer->IsRobot())
		LOG_DEBUG("roomid:%d,tableid:%d,uid:%d,m_pCurBanker:%p,m_lBankerScore:%lld,iTimer:%d,curScore:%lld,lNowJetton:%lld,lBankerScore:%lld,lMeMaxScore:%lld",
			GetRoomID(), GetTableID(), pPlayer->GetUID(), m_pCurBanker, m_lBankerScore, iTimer, GetPlayerCurScore(pPlayer), lNowJetton, lBankerScore, lMeMaxScore);

	return (lMeMaxScore);
}

//庄家站起
void CGameTwoeightbarTable::StandUpBankerSeat(CGamePlayer* pPlayer) {
	for (uint8 i = 0; i < m_vecPlayers.size(); ++i)
		if (m_vecPlayers[i].pPlayer == pPlayer) {
			m_vecPlayers[i].Reset();
			LOG_DEBUG("standup banker seat:room:%d--tb:%d,chairID:%d,uid:%d", m_pHostRoom->GetRoomID(), GetTableID(), i, pPlayer->GetUID());
			AddLooker(pPlayer);
			OnActionUserStandUp(i, pPlayer);
		}
}

bool CGameTwoeightbarTable::RobotLeavaReadJetton(uint32 uid) {
	for (vector<tagRobotPlaceJetton>::iterator iter_begin_look = m_RobotPlaceJetton.begin(); iter_begin_look != m_RobotPlaceJetton.end(); ++iter_begin_look)
		if (iter_begin_look->uid == uid) {
			m_RobotPlaceJetton.erase(iter_begin_look);
			break;
		}
	for (vector<tagRobotPlaceJetton>::iterator iter_begin_chairid = m_chairRobotPlaceJetton.begin(); iter_begin_chairid != m_chairRobotPlaceJetton.end(); ++iter_begin_chairid)
		if (iter_begin_chairid->uid == uid) {
			m_chairRobotPlaceJetton.erase(iter_begin_chairid);
			break;
		}
	return true;
}

bool CGameTwoeightbarTable::IsSetJetton(uint32 uid) {
	if (TABLE_STATE_TWOEIGHT_GAME_END == GetGameState())
		return false;

	for (uint8 i = 0; i < AREA_COUNT; ++i)
		if (m_userJettonScore[i][uid] > 0)
			return true;

	for (uint32 i = 0; i < m_chairRobotPlaceJetton.size(); ++i)
		if (uid == m_chairRobotPlaceJetton[i].uid)
			return true;

	for (uint32 i = 0; i < m_RobotPlaceJetton.size(); ++i)
		if (uid == m_RobotPlaceJetton[i].uid)
			return true;

	return false;
}

bool CGameTwoeightbarTable::IsInApplyList(uint32 uid) {
	//存在判断
	for (uint32 nUserIdx = 0; nUserIdx < m_ApplyUserArray.size(); ++nUserIdx) {
		uint32 id = m_ApplyUserArray[nUserIdx]->GetUID();
		if (id == uid)
			return true;
	}
	return false;
}

//计算得分
int64 CGameTwoeightbarTable::CalculateScore() {
	//推断玩家
	bool static bWinShunMen, bWinTianMen, bWinDiMen;
	int ShunMultiple, TianMultiple, DiMultiple;
	ShunMultiple = 1;
	TianMultiple = 1;
	DiMultiple = 1;
	DeduceWinner(bWinShunMen, bWinTianMen, bWinDiMen, ShunMultiple, TianMultiple, DiMultiple);

	LOG_DEBUG("win:%d-%d-%d,multiple:%d-%d-%d", bWinShunMen, bWinTianMen, bWinDiMen, ShunMultiple, TianMultiple, DiMultiple);

	int cbMultiple[] = { 1, 1, 1 };
	cbMultiple[ID_SHUN_MEN] = ShunMultiple;
	cbMultiple[ID_TIAN_MEN] = TianMultiple;
	cbMultiple[ID_DI_MEN] = DiMultiple;

	m_winMultiple[ID_SHUN_MEN] = bWinShunMen ? ShunMultiple : -ShunMultiple;
	m_winMultiple[ID_TIAN_MEN] = bWinTianMen ? TianMultiple : -TianMultiple;
	m_winMultiple[ID_DI_MEN] = bWinDiMen ? DiMultiple : -DiMultiple;

	//游戏记录
	for (uint32 i = 0; i < AREA_COUNT; ++i)
		m_record.wins[i] = m_winMultiple[i] > 0 ? 1 : 0;

	m_vecRecord.push_back(m_record);
	if (m_vecRecord.size() > 20) //保留最近20局
		m_vecRecord.erase(m_vecRecord.begin());

	SendGameEndLogInfo();

	for (uint16 i = 0; i < MAX_SEAT_INDEX; ++i) {
		m_cbTableCardType[i] = TwoeightLogic::GetCardType(m_cbTableCardArray[i]);
		WriteOutCardLog(i, m_cbTableCardArray[i], m_winMultiple[i], m_isTableCardPointKill[i]);
	}

	//庄家总赢分
	int64 lBankerWinScore = 0;

	//玩家成绩
	m_mpUserWinScore.clear();
	m_mpWinScoreForFee.clear();
	unordered_map<uint32, int64> mpUserLostScore;

	//胜利标识
	bool static bWinFlag[AREA_COUNT];
	bWinFlag[ID_SHUN_MEN] = bWinShunMen;
	bWinFlag[ID_TIAN_MEN] = bWinTianMen;
	bWinFlag[ID_DI_MEN] = bWinDiMen;

	bool bIsUserPlaceJetton = false;
	for (uint16 wAreaIndex = ID_SHUN_MEN; wAreaIndex < AREA_COUNT; ++wAreaIndex)
		if (m_allJettonScore[wAreaIndex] > 0) {
			bIsUserPlaceJetton = true;
			break;
		}

	if (!bWinTianMen && !bWinDiMen && !bWinShunMen && bIsUserPlaceJetton)
		m_cbBrankerSettleAccountsType = BRANKER_TYPE_TAKE_ALL;
	else if (bWinTianMen && bWinDiMen && bWinShunMen && bIsUserPlaceJetton)
		m_cbBrankerSettleAccountsType = BRANKER_TYPE_COMPENSATION;
	else
		m_cbBrankerSettleAccountsType = BRANKER_TYPE_NULL;

	//计算座位积分
	for (uint16 wChairID = 0; wChairID < GAME_PLAYER; ++wChairID) {
		//获取用户
		CGamePlayer * pPlayer = GetPlayer(wChairID);
		uint8 maxCardType = emCardTypePoint;
		if (pPlayer == NULL)
			continue;
		for (uint16 wAreaIndex = ID_SHUN_MEN; wAreaIndex < AREA_COUNT; ++wAreaIndex) {
			if (m_userJettonScore[wAreaIndex][pPlayer->GetUID()] == 0)
				continue;
			int64 scoreWin = (m_userJettonScore[wAreaIndex][pPlayer->GetUID()] * cbMultiple[wAreaIndex]);
			if (true == bWinFlag[wAreaIndex]) { // 赢了 
				m_mpUserWinScore[pPlayer->GetUID()] += scoreWin;
				lBankerWinScore -= scoreWin;
			} else { // 输了
				mpUserLostScore[pPlayer->GetUID()] -= scoreWin;
				lBankerWinScore += m_userJettonScore[wAreaIndex][pPlayer->GetUID()] * cbMultiple[wAreaIndex];
				m_curr_banker_win += scoreWin;
			}
			WriteAddScoreLog(pPlayer->GetUID(), wAreaIndex, m_userJettonScore[wAreaIndex][pPlayer->GetUID()]);
			maxCardType = maxCardType < m_cbTableCardType[wAreaIndex + 1] ? m_cbTableCardType[wAreaIndex + 1] : maxCardType;
		}
		//总的分数
		m_mpWinScoreForFee[pPlayer->GetUID()] = m_mpUserWinScore[pPlayer->GetUID()];
		m_mpUserWinScore[pPlayer->GetUID()] += mpUserLostScore[pPlayer->GetUID()];
		mpUserLostScore[pPlayer->GetUID()] = 0;
		WriteMaxCardType(pPlayer->GetUID(), maxCardType);
	}
	//计算旁观者积分
	for (map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin(); it != m_mpLookers.end(); ++it) {
		CGamePlayer *pPlayer = it->second;
		uint8 maxCardType = emCardTypePoint;
		if (pPlayer == NULL)
			continue;
		for (uint16 wAreaIndex = ID_SHUN_MEN; wAreaIndex < AREA_COUNT; ++wAreaIndex) {
			if (m_userJettonScore[wAreaIndex][pPlayer->GetUID()] == 0)
				continue;
			int64 scoreWin = (m_userJettonScore[wAreaIndex][pPlayer->GetUID()] * cbMultiple[wAreaIndex]);

			if (true == bWinFlag[wAreaIndex]) { // 赢了
				m_mpUserWinScore[pPlayer->GetUID()] += scoreWin;
				lBankerWinScore -= scoreWin;
			} else { // 输了
				mpUserLostScore[pPlayer->GetUID()] -= scoreWin;
				lBankerWinScore += scoreWin;
				m_curr_banker_win += scoreWin;
			}
			WriteAddScoreLog(pPlayer->GetUID(), wAreaIndex, m_userJettonScore[wAreaIndex][pPlayer->GetUID()]);
			maxCardType = maxCardType < m_cbTableCardType[wAreaIndex + 1] ? m_cbTableCardType[wAreaIndex + 1] : maxCardType;
		}
		//总的分数
		m_mpWinScoreForFee[pPlayer->GetUID()] = m_mpUserWinScore[pPlayer->GetUID()];
		m_mpUserWinScore[pPlayer->GetUID()] += mpUserLostScore[pPlayer->GetUID()];
		mpUserLostScore[pPlayer->GetUID()] = 0;
		WriteMaxCardType(pPlayer->GetUID(), maxCardType);
	}
	//累计积分
	m_lBankerWinScore += lBankerWinScore;
	//当前积分
	m_lBankerScore += lBankerWinScore;
	if (lBankerWinScore > 0)
		++m_wBankerWinTime;
	m_lBankerWinMaxScore = MAX(lBankerWinScore, m_lBankerWinMaxScore);
	m_lBankerWinMinScore = MIN(lBankerWinScore, m_lBankerWinMinScore);

	return lBankerWinScore;
}

bool CGameTwoeightbarTable::IsUserPlaceJetton() {
	for (uint16 wChairID = 0; wChairID < GAME_PLAYER; ++wChairID) {
		CGamePlayer *pPlayer = GetPlayer(wChairID);
		if (pPlayer == NULL)
			continue;
		if (pPlayer->IsRobot())
			continue;
		for (uint16 wAreaIndex = ID_SHUN_MEN; wAreaIndex < AREA_COUNT; ++wAreaIndex)
			if (m_userJettonScore[wAreaIndex][pPlayer->GetUID()] > 0)
				return true;
	}

	for (map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin(); it != m_mpLookers.end(); ++it) {
		CGamePlayer *pPlayer = it->second;
		if (pPlayer == NULL)
			continue;
		if (pPlayer->IsRobot())
			continue;
		for (uint16 wAreaIndex = ID_SHUN_MEN; wAreaIndex <= AREA_COUNT; ++wAreaIndex)
			if (m_userJettonScore[wAreaIndex][pPlayer->GetUID()] > 0)
				return true;
	}
	return false;
}

// 推断赢家处理 add by har
void CGameTwoeightbarTable::DeduceWinnerDeal(bool &bWinShun, bool &bWinTian, bool &bWinDi, int &ShunMultiple, int &TianMultiple, int &diMultiple, uint8 cbTableCardArray[][SINGLE_CARD_NUM]) {
	//大小比较
	bWinShun = TwoeightLogic::CompareCard(cbTableCardArray[0], cbTableCardArray[ID_SHUN_MEN + 1], ShunMultiple) == 1 ? true : false;
	bWinTian = TwoeightLogic::CompareCard(cbTableCardArray[0], cbTableCardArray[ID_TIAN_MEN + 1], TianMultiple) == 1 ? true : false;
	bWinDi = TwoeightLogic::CompareCard(cbTableCardArray[0], cbTableCardArray[ID_DI_MEN + 1], diMultiple) == 1 ? true : false;
}

//推断赢家
void CGameTwoeightbarTable::DeduceWinner(bool &bWinShun, bool &bWinTian, bool &bWinDi, int &ShunMultiple, int &TianMultiple, int &diMultiple) {
	DeduceWinnerDeal(bWinShun, bWinTian, bWinDi, ShunMultiple, TianMultiple, diMultiple, m_cbTableCardArray);
}

//次数限制
int32 CGameTwoeightbarTable::GetBankerTimeLimit()
{
	return m_BankerTimeLimit;
}

//申请庄家队列排序
bool CGameTwoeightbarTable::CompareApplyBankers(CGamePlayer* pBanker1, CGamePlayer* pBanker2)
{
	if (m_ApplyUserScore[pBanker1->GetUID()] < m_ApplyUserScore[pBanker2->GetUID()])
		return true;

	return false;
}

// 申请上庄条件
int64 CGameTwoeightbarTable::GetApplyBankerCondition() {
	return GetBaseScore();
}

int64 CGameTwoeightbarTable::GetApplyBankerConditionLimit() {
	return GetBaseScore() * 20;
}

void CGameTwoeightbarTable::OnRobotJettonDeal(vector<tagRobotPlaceJetton> &vecRobotPlaceJetton, CGamePlayer *pPlayer) {
	if (pPlayer == NULL || !pPlayer->IsRobot() || pPlayer == m_pCurBanker)
		return;

	int iJettonCount = g_RandGen.RandRange(5, 9);
	uint8 cbJettonArea = g_RandGen.RandRange(ID_SHUN_MEN, ID_DI_MEN);
	int64 lUserRealJetton = GetRobotJettonScore(pPlayer);
	if (lUserRealJetton == 0)
		return;

	int iJettonTypeCount = g_RandGen.RandRange(1, 2);
	int iJettonStartCount = g_RandGen.RandRange(2, 3);
	int64 lOldRealJetton = -1;
	int64 iJettonOldTime = -1;

	bool bIsContinuouslyJetton = false;
	int iPreRatio = g_RandGen.RandRange(5, 10);
	if (g_RandGen.RandRatio(iPreRatio, PRO_DENO_100)) {
		bIsContinuouslyJetton = true;
		if (lUserRealJetton == 100 || lUserRealJetton == 1000)
			iJettonCount = g_RandGen.RandRange(5, 18);
	}
	for (int iIndex = 0; iIndex < iJettonCount; ++iIndex) {
		if (bIsContinuouslyJetton == false) {
			cbJettonArea = g_RandGen.RandRange(ID_SHUN_MEN, ID_DI_MEN);
			lUserRealJetton = GetRobotJettonScore(pPlayer);
			if (lOldRealJetton == -1)
				lOldRealJetton = lUserRealJetton;
			if (lOldRealJetton != lUserRealJetton && iJettonTypeCount == 1)
				lUserRealJetton = lOldRealJetton;
			if (lOldRealJetton != lUserRealJetton && iJettonTypeCount == 2 && iJettonStartCount == iIndex) {
				lUserRealJetton = lUserRealJetton;
				lOldRealJetton = lUserRealJetton;
			} else
				lUserRealJetton = lOldRealJetton;
		}
		if (lUserRealJetton == 0)
			continue;

		tagRobotPlaceJetton robotPlaceJetton;
		robotPlaceJetton.uid = pPlayer->GetUID();
		int64 uRemainTime = m_coolLogic.getCoolTick();
		int64 passtick = m_coolLogic.getPassTick();
		int64 uMaxDelayTime = s_PlaceJettonTime;

		if (iJettonOldTime >= uMaxDelayTime - 500)
			iJettonOldTime = g_RandGen.RandRange(100, 5000);

		if (iJettonOldTime == -1) {
			robotPlaceJetton.time = g_RandGen.RandRange(100, uMaxDelayTime - 500);
			iJettonOldTime = robotPlaceJetton.time;
			if (bIsContinuouslyJetton) {
				robotPlaceJetton.time = g_RandGen.RandRange(100, 4000);
				iJettonOldTime = robotPlaceJetton.time;
			}
		} else {
			robotPlaceJetton.time = g_RandGen.RandRange(100, uMaxDelayTime - 500);
			iJettonOldTime = robotPlaceJetton.time;
		}

		if (bIsContinuouslyJetton) {
			robotPlaceJetton.time = iJettonOldTime + 100;
			iJettonOldTime = robotPlaceJetton.time;
		}

		if (robotPlaceJetton.time <= 0 || robotPlaceJetton.time > uMaxDelayTime - 500)
			continue;

		robotPlaceJetton.area = cbJettonArea;
		robotPlaceJetton.jetton = lUserRealJetton;
		robotPlaceJetton.bflag = false;
		vecRobotPlaceJetton.push_back(robotPlaceJetton);
	}
}

// 椅子机器人押注准备
void CGameTwoeightbarTable::OnChairRobotJetton() {
	if (m_bIsChairRobotAlreadyJetton)
		return;
	m_bIsChairRobotAlreadyJetton = true;
	m_chairRobotPlaceJetton.clear();
	for (uint32 i = 0; i < GAME_PLAYER; ++i)
		OnRobotJettonDeal(m_chairRobotPlaceJetton, GetPlayer(i));
	LOG_DEBUG("chair_robot_jetton - roomid:%d,tableid:%d,m_chairRobotPlaceJetton.size:%d", GetRoomID(), GetTableID(), m_chairRobotPlaceJetton.size());
}

void CGameTwoeightbarTable::OnRobotPlaceJetton(vector<tagRobotPlaceJetton> &vRobotPlaceJetton) {
	if (vRobotPlaceJetton.empty())
		return;

	int64 passtick = m_coolLogic.getPassTick(); // 下注开始后经历的时间(毫秒)
	int delNum = 0; // 要删除的下注项数量
	unordered_set<uint32> usRobotPlaceJetton; // 每个玩家每次定时器tick只能下注一次
	for (uint32 i = 0; i < vRobotPlaceJetton.size(); ++i) {
		tagRobotPlaceJetton &robotPlaceJetton = vRobotPlaceJetton[i];
		if (robotPlaceJetton.bflag) {
			++delNum;
			continue;
		}
		CGamePlayer *pPlayer = static_cast<CGamePlayer *>(CPlayerMgr::Instance().GetPlayer(robotPlaceJetton.uid));
		if (pPlayer == NULL)
			continue;

		if (passtick > robotPlaceJetton.time && s_PlaceJettonTime - passtick > 500 &&
			    usRobotPlaceJetton.find(robotPlaceJetton.uid) == usRobotPlaceJetton.end()) {
			if (IsInTableRobot(robotPlaceJetton.uid, pPlayer)) {
				robotPlaceJetton.bflag = OnUserPlaceJetton(pPlayer, robotPlaceJetton.area, robotPlaceJetton.jetton);
				if (robotPlaceJetton.bflag)
					++delNum;
			}
			usRobotPlaceJetton.insert(robotPlaceJetton.uid);
		}
	}

	uint32 j = 0;
	for (int i = 0; i < delNum; ++i) {
		for (vector<tagRobotPlaceJetton>::iterator iter_begin = vRobotPlaceJetton.begin() + j; j < vRobotPlaceJetton.size(); ++iter_begin, ++j)
			if (iter_begin->bflag) {
				iter_swap(iter_begin, vRobotPlaceJetton.end() - 1);
				vRobotPlaceJetton.pop_back();
				break;
			}
		if (j >= vRobotPlaceJetton.size())
			break;
	}
	if (delNum > 0)
		LOG_DEBUG("roomid:%d,tableid:%d,delNum:%lld,m_chairRobotPlaceJetton.size:%lld,m_RobotPlaceJetton.size:%lld,m_mpLookers.size:%lld",
			GetRoomID(), GetTableID(), delNum, m_chairRobotPlaceJetton.size(), m_RobotPlaceJetton.size(), m_mpLookers.size());
}

// 观众机器人押注准备
void CGameTwoeightbarTable::OnRobotJetton() {
	if (m_bIsRobotAlreadyJetton)
		return;
	m_bIsRobotAlreadyJetton = true;
	m_RobotPlaceJetton.clear();
	int64 iJettonOldTime = -1;
	for (map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin(); it != m_mpLookers.end(); ++it) {
		if (g_RandGen.RandRatio(50, PRO_DENO_100))
			continue;
		OnRobotJettonDeal(m_RobotPlaceJetton, it->second);
	}
	LOG_DEBUG("robot_jetton - roomid:%d,tableid:%d,m_chairRobotPlaceJetton.size:%lld,m_mpLookers.size:%lld",
		GetRoomID(), GetTableID(), m_RobotPlaceJetton.size(), m_mpLookers.size());
}

bool CGameTwoeightbarTable::IsInTableRobot(uint32 uid, CGamePlayer *pPlayer) {
	for (uint32 i = 0; i < GAME_PLAYER; ++i)
		if (pPlayer != NULL && pPlayer == GetPlayer(i) && pPlayer->GetUID() == uid)
			return true;

	map<uint32, CGamePlayer*>::iterator iter_player = m_mpLookers.find(uid);
	if (iter_player != m_mpLookers.end())
		if (pPlayer != NULL && pPlayer == iter_player->second && pPlayer->GetUID() == iter_player->first)
			return true;

	return false;
}

int64 CGameTwoeightbarTable::GetRobotJettonScore(CGamePlayer* pPlayer/*, uint8 area*/) {
	int64 lUserRealJetton = 100;
	int64 lUserMinJetton = 100;
	int64 lUserMaxJetton = GetUserMaxJetton(pPlayer/*, area*/);
	int64 lUserCurJetton = GetPlayerCurScore(pPlayer);
	
	if (lUserCurJetton < 2000)
		lUserRealJetton = 0;
	else if (lUserCurJetton >= 2000 && lUserCurJetton < 50000) {
		if (g_RandGen.RandRatio(77, PRO_DENO_100))
			lUserRealJetton = 100;
		else if (g_RandGen.RandRatio(15, PRO_DENO_100))
			lUserRealJetton = 1000;
		else
			lUserRealJetton = 5000;
	} else if (lUserCurJetton >= 50000 && lUserCurJetton < 200000) {
		if (g_RandGen.RandRatio(60, PRO_DENO_100))
			lUserRealJetton = 1000;
		else if (g_RandGen.RandRatio(17, PRO_DENO_100))
			lUserRealJetton = 5000;
		else if (g_RandGen.RandRatio(20, PRO_DENO_100))
			lUserRealJetton = 10000;
		else
			lUserRealJetton = 50000;
	} else if (lUserCurJetton >= 200000 && lUserCurJetton < 2000000) {
		if (g_RandGen.RandRatio(3470, PRO_DENO_10000))
			lUserRealJetton = 1000;
		else if (g_RandGen.RandRatio(2500, PRO_DENO_10000))
			lUserRealJetton = 5000;
		else if (g_RandGen.RandRatio(3500, PRO_DENO_10000))
			lUserRealJetton = 10000;
		else
			lUserRealJetton = 50000;
	} else if (lUserCurJetton >= 2000000) {
		if (g_RandGen.RandRatio(6300, PRO_DENO_10000))
			lUserRealJetton = 5000;
		else if (g_RandGen.RandRatio(3000, PRO_DENO_10000))
			lUserRealJetton = 10000;
		else
			lUserRealJetton = 50000;
	} else
		lUserRealJetton = 100;

	if (lUserRealJetton > lUserMaxJetton && lUserRealJetton == 50000)
		lUserRealJetton = 10000;
	if (lUserRealJetton > lUserMaxJetton && lUserRealJetton == 10000)
		lUserRealJetton = 5000;
	if (lUserRealJetton > lUserMaxJetton && lUserRealJetton == 5000)
		lUserRealJetton = 1000;
	if (lUserRealJetton > lUserMaxJetton && lUserRealJetton == 1000)
		lUserRealJetton = 100;
	if (lUserRealJetton > lUserMaxJetton && lUserRealJetton == 100)
		lUserRealJetton = 0;
	if (lUserRealJetton < lUserMinJetton)
		lUserRealJetton = 0;
	return lUserRealJetton;
}

void CGameTwoeightbarTable::OnRobotStandUp() {
	// 保持一两个座位给玩家    
	vector<uint16> emptyChairs;
	vector<uint16> robotChairs;
	for (uint8 i = 0; i < GAME_PLAYER; ++i) {
		CGamePlayer* pPlayer = GetPlayer(i);
		if (pPlayer == NULL) {
			emptyChairs.push_back(i);
			continue;
		}
		if (pPlayer->IsRobot())
			robotChairs.push_back(i);
	}

	if (GetChairPlayerNum() > m_robotChairSize && robotChairs.size() > 0) { // 机器人站起
		uint16 chairID = robotChairs[g_RandGen.RandUInt() % robotChairs.size()];
		CGamePlayer* pPlayer = GetPlayer(chairID);
		if (pPlayer != NULL && pPlayer->IsRobot() && CanStandUp(pPlayer)) {
			PlayerSitDownStandUp(pPlayer, false, chairID);
			return;
		}
	}
	if (GetChairPlayerNum() < m_robotChairSize && emptyChairs.size() > 0) { //机器人坐下
		for (map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin(); it != m_mpLookers.end(); ++it) {
			CGamePlayer* pPlayer = it->second;
			if (pPlayer == NULL || !pPlayer->IsRobot() || pPlayer == m_pCurBanker)
				continue;

			uint16 chairID = emptyChairs[g_RandGen.RandUInt() % emptyChairs.size()];
			if (CanSitDown(pPlayer, chairID)) {
				PlayerSitDownStandUp(pPlayer, true, chairID);
				return;
			}
		}
	}
}

void CGameTwoeightbarTable::CheckRobotCancelBanker() {
	if (m_pCurBanker != NULL && m_pCurBanker->IsRobot())
		if (m_wBankerTime > 3 && m_lBankerWinScore > m_lBankerBuyinScore / 2)
			if (g_RandGen.RandRatio(65, 100))
				OnUserCancelBanker(m_pCurBanker);
}

/*void CGameTwoeightbarTable::GetAllRobotPlayer(vector<CGamePlayer*> & robots){
}*/

// 检查机器人申请庄家
void CGameTwoeightbarTable::CheckRobotApplyBanker() {
	if (m_pCurBanker != NULL || m_ApplyUserArray.size() >= m_robotApplySize)
		return;

	uint32 roomid = 255;
	if (m_pHostRoom != NULL)
		roomid = m_pHostRoom->GetRoomID();

	vector<CGamePlayer*> robots;
	GetAllRobotPlayer(robots);

	LOG_DEBUG("robot apply banker - roomid:%d,tableid:%d,robots.size:%d, --------------------------------", roomid, GetTableID(), robots.size());

	for (uint32 uIndex = 0; uIndex < robots.size(); ++uIndex) {
		CGamePlayer *pPlayer = robots[uIndex];
		if (pPlayer == NULL || !pPlayer->IsRobot())
			continue;
		int64 curScore = GetPlayerCurScore(pPlayer);
		LOG_DEBUG("robot_ApplyBanker - roomid:%d,tableid:%d,uid:%d,curScore:%lld,GetApplyBankerCondition:%lld,m_ApplyUserArray.size:%d,", roomid, GetTableID(), pPlayer->GetUID(), curScore, GetApplyBankerCondition(), m_ApplyUserArray.size());
		if (curScore < GetApplyBankerCondition())
			continue;

		int64 buyinScore = GetApplyBankerCondition() * 2;

		if (curScore < buyinScore) {
			buyinScore = curScore;
			buyinScore = (buyinScore / 10000) * 10000;
			OnUserApplyBanker(pPlayer, buyinScore, 0);

		} else {
			buyinScore = g_RandGen.RandRange(buyinScore, curScore);
			buyinScore = (buyinScore / 10000) * 10000;

			OnUserApplyBanker(pPlayer, buyinScore, 1);
		}
		if (m_ApplyUserArray.size() > m_robotApplySize)
			break;
	}
}

void CGameTwoeightbarTable::AddPlayerToBlingLog() {
	for (map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin(); it != m_mpLookers.end(); ++it) {
		CGamePlayer* pPlayer = it->second;
		if (pPlayer == NULL)
			continue;
		for (uint8 i = 0; i < AREA_COUNT; ++i) {
			if (m_userJettonScore[i][pPlayer->GetUID()] > 0) {
				AddUserBlingLog(pPlayer);
				break;
			}
		}
	}
	AddUserBlingLog(m_pCurBanker);
}

void CGameTwoeightbarTable::OnNewDay() {
	m_uBairenTotalCount = 0;
	for (uint32 i = 0; i < m_vecAreaWinCount.size(); ++i)
		m_vecAreaWinCount[i] = 0;
	for (uint32 i = 0; i < m_vecAreaLostCount.size(); ++i)
		m_vecAreaLostCount[i] = 0;
}

void CGameTwoeightbarTable::GetGamePlayLogInfo(net::msg_game_play_log* pInfo) {
	net::msg_twoeight_play_log_rep* pplay = pInfo->mutable_twoeight();
	for (uint32 i = 0; i < m_vecRecord.size(); ++i) {
		net::twoeight_play_log* plog = pplay->add_logs();
		stTwoeightGameRecord& record = m_vecRecord[i];
		for (uint32 j = 0; j < AREA_COUNT; ++j)
			plog->add_seats_win(record.wins[j]);
	}
}

void CGameTwoeightbarTable::GetGameEndLogInfo(net::msg_game_play_log* pInfo) {
	net::msg_twoeight_play_log_rep *pplay = pInfo->mutable_twoeight();
	net::twoeight_play_log *plog = pplay->add_logs();
	stTwoeightGameRecord &record = m_record;
	for (uint16 j = 0; j < AREA_COUNT; ++j)
		plog->add_seats_win(record.wins[j]);
}

/*void CGameTwoeightbarTable::OnBrcControlSendAllPlayerInfo(CGamePlayer* pPlayer) {
}

void CGameTwoeightbarTable::OnBrcControlNoticeSinglePlayerInfo(CGamePlayer* pPlayer){
}

void CGameTwoeightbarTable::OnBrcControlSendAllRobotTotalBetInfo() {
}

void CGameTwoeightbarTable::OnBrcControlSendAllPlayerTotalBetInfo() {
}

bool CGameTwoeightbarTable::OnBrcControlEnterControlInterface(CGamePlayer* pPlayer) {
}

void CGameTwoeightbarTable::OnBrcControlBetDeal(CGamePlayer* pPlayer) {
}

bool CGameTwoeightbarTable::OnBrcAreaControl() {
}

void CGameTwoeightbarTable::OnBrcFlushSendAllPlayerInfo() {
}

bool CGameTwoeightbarTable::OnBrcAreaControlForB(set<uint8> &area_list) {
}

bool CGameTwoeightbarTable::OnBrcAreaControlForA(uint8 ctrl_area_a) {
}*/

bool CGameTwoeightbarTable::SetControlBankerScore(bool isWin) {
	LOG_DEBUG("Set Control Banker Score - isWin:%d", isWin);

	uint8 cbTableCard[CARD_COUNT];

	uint32 bankeruid = GetBankerUID();

	int64 banker_score = 0;

	int irount_count = 1000;
	int iRountIndex = 0;

	for (; iRountIndex < irount_count; ++iRountIndex) {
		bool static bWinShunMen, bWinTianMen, bWinDiMen;
		int ShunMultiple, TianMultiple, DiMultiple;
		ShunMultiple = 1;
		TianMultiple = 1;
		DiMultiple = 1;
		DeduceWinner(bWinShunMen, bWinTianMen, bWinDiMen, ShunMultiple, TianMultiple, DiMultiple);

		int cbMultiple[] = { 1, 1, 1 };
		cbMultiple[ID_SHUN_MEN] = TianMultiple;
		cbMultiple[ID_TIAN_MEN] = TianMultiple;
		cbMultiple[ID_DI_MEN] = DiMultiple;

		//胜利标识
		bool static bWinFlag[AREA_COUNT];
		bWinFlag[ID_SHUN_MEN] = bWinShunMen;
		bWinFlag[ID_TIAN_MEN] = bWinTianMen;
		bWinFlag[ID_DI_MEN] = bWinDiMen;

		//计算座位积分
		for (uint16 wChairID = 0; wChairID < GAME_PLAYER; ++wChairID) {
			//获取用户
			CGamePlayer *pPlayer = GetPlayer(wChairID);
			if (pPlayer == NULL)
				continue;
			for (uint16 wAreaIndex = ID_SHUN_MEN; wAreaIndex <= AREA_COUNT; ++wAreaIndex) {
				if (m_userJettonScore[wAreaIndex][pPlayer->GetUID()] == 0)
					continue;
				if (bWinFlag[wAreaIndex])// 赢了
					banker_score -= (m_userJettonScore[wAreaIndex][pPlayer->GetUID()] * cbMultiple[wAreaIndex]);
				else // 输了
					banker_score += m_userJettonScore[wAreaIndex][pPlayer->GetUID()] * cbMultiple[wAreaIndex];
			}
		}

		//计算旁观者积分
		for (map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin(); it != m_mpLookers.end(); ++it) {
			CGamePlayer* pPlayer = it->second;
			if (pPlayer == NULL)
				continue;
			for (uint16 wAreaIndex = ID_SHUN_MEN; wAreaIndex < AREA_COUNT; ++wAreaIndex) {
				if (m_userJettonScore[wAreaIndex][pPlayer->GetUID()] == 0)
					continue;
				if (true == bWinFlag[wAreaIndex])// 赢了
					banker_score -= (m_userJettonScore[wAreaIndex][pPlayer->GetUID()] * cbMultiple[wAreaIndex]);
				else // 输了
					banker_score += m_userJettonScore[wAreaIndex][pPlayer->GetUID()] * cbMultiple[wAreaIndex];
			}
		}

		if ((isWin && banker_score > 0) || (!isWin && banker_score < 0))
			break;
		else
			//重新洗牌
			TwoeightLogic::RandCardList(m_cbTableCardArray);
	}
	LOG_DEBUG("set banker win or lose - roomid:%d,tableid:%d,bankeruid:%d,banker_score:%lld,isWin:%d", m_pHostRoom->GetRoomID(), GetTableID(), bankeruid, banker_score, isWin);
	if (iRountIndex >= irount_count)
		return false;
	return true;
}

void CGameTwoeightbarTable::OnNotityForceApplyUser(CGamePlayer* pPlayer) {
	LOG_DEBUG("Notity Force Apply uid:%d.", pPlayer->GetUID());
	net::msg_twoeight_apply_banker_rep msg;
	msg.set_apply_oper(0);
	msg.set_result(net::RESULT_CODE_SUCCESS);

	pPlayer->SendMsgToClient(&msg, net::S2C_MSG_TWOEIGHT_APPLY_BANKER);
}

// 获取单个下注的是机器人还是玩家
void CGameTwoeightbarTable::IsRobotOrPlayerJetton(CGamePlayer *pPlayer, bool &isAllPlayer, bool &isAllRobot) {
	for (uint16 wAreaIndex = 0; wAreaIndex < AREA_COUNT; ++wAreaIndex) {
		if (m_userJettonScore[wAreaIndex][pPlayer->GetUID()] == 0)
			continue;
		if (pPlayer->IsRobot())
			isAllPlayer = false;
		else
			isAllRobot = false;
		return;
	}
}

// 设置库存输赢
bool CGameTwoeightbarTable::SetStockWinLose() {
	int64 stockChange = m_pHostRoom->IsStockChangeCard(this);
	if (stockChange == 0)
		return false;

	int64 playerAllWinScore = 0;
	int i = 0;
	// 循环，直到找到满足条件的牌组合
	while (true) {
		playerAllWinScore = GetPlayerWinScore(m_cbTableCardArray);
		if (IsCurTableCardRuleAllow(m_cbTableCardArray) && CheckStockChange(stockChange, playerAllWinScore, i)) {
			LOG_DEBUG("SetStockWinLose suc  roomid:%d,tableid:%d,stockChange:%lld,i:%d,playerAllWinScore:%d,IsBankerRealPlayer:%d",
				GetRoomID(), GetTableID(), stockChange, i, playerAllWinScore, IsBankerRealPlayer());
			return true;
		}
		if (++i > 999)
			break;
		//重新洗牌
		TwoeightLogic::RandCardList(m_cbTableCardArray);
	}

	LOG_ERROR("SetStockWinLose fail! roomid:%d,tableid:%d,playerAllWinScore:%lld,stockChange:%lld,IsBankerRealPlayer:%d", GetRoomID(), GetTableID(), playerAllWinScore, stockChange, IsBankerRealPlayer());
	return false;
}
