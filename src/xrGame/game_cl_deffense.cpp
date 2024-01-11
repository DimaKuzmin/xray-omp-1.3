#include "stdafx.h"
#include "game_cl_deffense.h"
#include "clsid_game.h"
#include "xr_level_controller.h"

#include "VoiceChat.h"
#include "ui/UIMainIngameWnd.h"
#include "UIGameDeffense.h"

#include "Actor.h"

game_cl_deffense::game_cl_deffense()
{
	if (!g_dedicated_server)
		m_pVoiceChat = xr_new<CVoiceChat>();
	else
		m_pVoiceChat = NULL;
}

game_cl_deffense::~game_cl_deffense()
{
	xr_delete(m_pVoiceChat);
}

void game_cl_deffense::OnVoiceMessage(NET_Packet* P)
{
	m_pVoiceChat->ReceiveMessage(P);
}

// Create UI

void game_cl_deffense::OnConnected()
{
	inherited::OnConnected();
	if (m_game_ui)
	{
		R_ASSERT(!g_dedicated_server);
		m_game_ui = smart_cast<CUIGameDeffense*>	(CurrentGameUI());
		m_game_ui->SetClGame(this);
	}

	luabind::functor<void>	funct;
	R_ASSERT(ai().script_engine().functor("mp_game_cl.on_connected", funct));
	funct();
}

void game_cl_deffense::SetGameUI(CUIGameCustom* uigame)
{
	inherited::SetGameUI(uigame);
	m_game_ui = smart_cast<CUIGameDeffense*>(uigame);
	R_ASSERT(m_game_ui);

	if (m_pVoiceChat)
 		m_game_ui->UIMainIngameWnd->SetVoiceDistance(m_pVoiceChat->GetDistance());
 
}

CUIGameCustom* game_cl_deffense::createGameUI()
{
	if (g_dedicated_server)
		return NULL;

	CLASS_ID clsid = CLSID_GAME_UI_FREEMP;
	m_game_ui = smart_cast<CUIGameDeffense*> (NEW_INSTANCE(clsid));
	R_ASSERT(m_game_ui);
	m_game_ui->Load();
	m_game_ui->SetClGame(this);
	return					m_game_ui;
}

// END UI Create


// SHEDULE
void game_cl_deffense::shedule_Update(u32 dt)
{
	game_cl_GameState::shedule_Update(dt);

	if (!local_player)
		return;

	if (!g_dedicated_server && m_pVoiceChat)
	{
		const bool started = m_pVoiceChat->IsStarted();
		const bool is_dead = !local_player || local_player->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD);
		const bool has_shown_dialogs = CurrentGameUI()->HasShownDialogs();
		if (started && (is_dead || has_shown_dialogs))
		{
			m_pVoiceChat->Stop();
			CurrentGameUI()->UIMainIngameWnd->SetActiveVoiceIcon(false);
		}
		m_pVoiceChat->Update();
	}

	// синхронизация имени и денег игроков для InventoryOwner
	for (auto cl : players)
	{
		game_PlayerState* ps = cl.second;
		if (!ps || ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD)) continue;

		CActor* pActor = smart_cast<CActor*>(Level().Objects.net_Find(ps->GameID));
		if (!pActor || !pActor->g_Alive()) continue;

		pActor->SetName(ps->getName());
		pActor->cName_set(ps->getName());

		if (ps->team != pActor->Community())
		{
			CHARACTER_COMMUNITY	community;
			community.set(ps->team);
			pActor->SetCommunity(community.index());
			pActor->ChangeTeam(community.team(), 0, 0);
		}

		if (local_player->GameID == ps->GameID)
		{
			pActor->set_money((u32)ps->money_for_round, false);
		}
	}
}

// VOICE CHAT OnRender
void game_cl_deffense::OnRender()
{
	inherited::OnRender();

	if (m_pVoiceChat)
		m_pVoiceChat->OnRender();
}


// Определение кнопок (пока Freemp)

bool game_cl_deffense::OnKeyboardPress(int key)
{
	switch (key)
	{
	case kVOICE_CHAT:
	{
		if (local_player && !local_player->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
		{
			if (!m_pVoiceChat->IsStarted())
			{
				m_pVoiceChat->Start();
				CurrentGameUI()->UIMainIngameWnd->SetActiveVoiceIcon(true);
			}
		}
		return true;
	}break;

	case kVOICE_DISTANCE:
	{
		if (local_player && !local_player->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
		{
			u8 distance = m_pVoiceChat->SwitchDistance();
			CurrentGameUI()->UIMainIngameWnd->SetVoiceDistance(distance);
		}
		return true;
	}break;

	case kJUMP:
	{
		bool b_need_to_send_ready = false;

		CObject* curr = Level().CurrentControlEntity();
		if (!curr) return(false);

		bool is_actor = !!smart_cast<CActor*>(curr);
		bool is_spectator = !!smart_cast<CSpectator*>(curr);

		game_PlayerState* ps = local_player;
				
		if (is_spectator || (is_actor && ps && ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD)))
		{
			b_need_to_send_ready = true;
		}

		if (b_need_to_send_ready)
		{
			CGameObject* GO = smart_cast<CGameObject*>(curr);
			NET_Packet			P;
			GO->u_EventGen(P, GE_GAME_EVENT, GO->ID());
			P.w_u16(GAME_EVENT_PLAYER_READY);
			GO->u_EventSend(P);
			return				true;
		}
		else
		{
			return false;
		}
	}break;

	default:
		break;
	}

	return inherited::OnKeyboardPress(key);
}

LPCSTR game_cl_deffense::GetGameScore(string32&	score_dest)
{
	s32 frags = local_player ? local_player->frags() : 0;
	xr_sprintf(score_dest, "[%d]", frags);
	return score_dest;
}

bool game_cl_deffense::OnKeyboardRelease(int key)
{
	switch (key)
	{
	case kVOICE_CHAT:
	{
		m_pVoiceChat->Stop();
		CurrentGameUI()->UIMainIngameWnd->SetActiveVoiceIcon(false);
		return true;
	}break;

	default:
		break;
	}

	return inherited::OnKeyboardRelease(key);
}



void game_cl_deffense::OnScreenResolutionChanged()
{
	if (m_game_ui && m_pVoiceChat)
	{
		m_game_ui->UIMainIngameWnd->SetVoiceDistance(m_pVoiceChat->GetDistance());
	}
}