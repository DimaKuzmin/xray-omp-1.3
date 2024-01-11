#include "StdAfx.h"
#include "game_sv_deffense.h"
#include "Level.h"


// Connection Flags Setup

// player connect #1
void game_sv_deffense::OnPlayerConnect(ClientID id_who)
{
	inherited::OnPlayerConnect(id_who);

	xrClientData* xrCData = m_server->ID_to_client(id_who);
	game_PlayerState*	ps_who = get_id(id_who);

	if (!xrCData->flags.bReconnect)
	{
		ps_who->clear();
		ps_who->team = 0;
		ps_who->skin = -1;
	};
	ps_who->setFlag(GAME_PLAYER_FLAG_SPECTATOR);

	ps_who->resetFlag(GAME_PLAYER_FLAG_SKIP);

	if (g_dedicated_server && (xrCData == m_server->GetServerClient()))
	{
		ps_who->setFlag(GAME_PLAYER_FLAG_SKIP);
		return;
	}
}

// player connect #2
void game_sv_deffense::OnPlayerConnectFinished(ClientID id_who)
{
	xrClientData* xrCData = m_server->ID_to_client(id_who);
	SpawnPlayer(id_who, "spectator");

	if (xrCData)
	{
		R_ASSERT2(xrCData->ps, "Player state not created yet");
		NET_Packet					P;
		GenerateGameMessage(P);
		P.w_u32(GAME_EVENT_PLAYER_CONNECTED);
		P.w_clientID(id_who);
		xrCData->ps->team = 0;
		xrCData->ps->setFlag(GAME_PLAYER_FLAG_SPECTATOR);
		xrCData->ps->setFlag(GAME_PLAYER_FLAG_READY);
		xrCData->ps->net_Export(P, TRUE);
		u_EventSend(P);
		xrCData->net_Ready = TRUE;
	};
}

// 

void game_sv_deffense::OnPlayerReady(ClientID id_who)
{
	switch (Phase())
	{
	case GAME_PHASE_INPROGRESS:
	{
		xrClientData*	xrCData = (xrClientData*)m_server->ID_to_client(id_who);
		game_PlayerState*	ps = get_id(id_who);

		if (ps->IsSkip())
			break;

		if (!(ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD)))
			break;

		RespawnPlayer(id_who, true);
	} break;

	default:
		break;
	};
}

// player disconnect
void game_sv_deffense::OnPlayerDisconnect(ClientID id_who, LPSTR Name, u16 GameID)
{
	inherited::OnPlayerDisconnect(id_who, Name, GameID);
}

void game_sv_deffense::OnPlayerKillPlayer(game_PlayerState * ps_killer, game_PlayerState * ps_killed, KILL_TYPE KillType, SPECIAL_KILL_TYPE SpecialKillType, CSE_Abstract * pWeaponA)
{
	if (ps_killed)
	{
		ps_killed->setFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD);
		ps_killed->DeathTime = Device.dwTimeGlobal;
	}
	signal_Syncronize();
}