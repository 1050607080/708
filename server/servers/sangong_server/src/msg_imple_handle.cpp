//
// Created by toney on 16/4/21.
//

#include "msg_imple_handle.h"
#include "game_room_mgr.h"
#include "stdafx.h"
#include "pb/msg_define.pb.h"

// ��Ϸ����Ϣ
int  CHandleImpleMsg::handle_msg_gameing_oper(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head)
{
    //LOG_DEBUG("��Ϸ�ڲ���Ϣ:uid:%d--msg:%d",head->uin,head->cmd);
    CGamePlayer* pGamePlayer = GetGamePlayer(head);
    if(pGamePlayer == NULL) {
        return 0;
    }
    CGameTable* pGameTable = pGamePlayer->GetTable();
    if(pGameTable != NULL)
    {
        pGameTable->OnGameMessage(pGamePlayer,head->cmd,pkt_buf,buf_len);
    }
    return 0;
}