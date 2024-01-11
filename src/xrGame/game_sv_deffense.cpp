#include "stdafx.h"
#include "game_sv_deffense.h"
#include "Level.h"

 
game_sv_deffense::game_sv_deffense()  : pure_relcase(&game_sv_deffense::net_Relcase)
{
}

game_sv_deffense::~game_sv_deffense()
{
}

void game_sv_deffense::Create(shared_str& options)
{
	inherited::Create(options);

	R_ASSERT2(rpoints[0].size(), "rpoints for players not found");

	switch_Phase(GAME_PHASE_PENDING);

	::Random.seed(GetTickCount());
	m_CorpseList.clear();
}


void game_sv_deffense::OnEvent(NET_Packet &P, u16 type, u32 time, ClientID sender)
{
	switch (type)
	{
	case GAME_EVENT_PLAYER_KILL: // (g_kill)
		{
			u16 ID = P.r_u16();
			xrClientData *l_pC = (xrClientData*)get_client(ID);
			if (!l_pC) break;
			KillPlayer(l_pC->ID, l_pC->ps->GameID);
		}
		break;
	case GAME_EVENT_MP_TRADE:
		{
			OnPlayerTrade(P, sender);
		}
		break;
	case GAME_EVENT_MP_REPAIR:
		{
			OnPlayerRepairItem(P, sender);
		}
		break;
	case GAME_EVENT_MP_INSTALL_UPGRADE:
		{
			OnPlayerInstallUpgrade(P, sender);
		}
		break;
	case GAME_EVENT_TRANSFER_MONEY:
		{
			OnTransferMoney(P, sender);
		}
		break;
	default:
		inherited::OnEvent(P, type, time, sender);
	};
}