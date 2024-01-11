#include "stdafx.h"
#include "game_cl_coop.h"
#include "clsid_game.h"
#include "UIGameCoop.h"

void game_cl_coop::SetGameUI(CUIGameCustom* uigame)
{
	inherited::SetGameUI(uigame);
	m_game_ui = smart_cast<CUIGameCoop*>(uigame);
	R_ASSERT(m_game_ui); 
}

void game_cl_coop::OnConnected()
{
	inherited::OnConnected();
	m_game_ui = smart_cast<CUIGameCoop*>(CurrentGameUI());
}


CUIGameCustom* game_cl_coop::createGameUI()
{
	if (g_dedicated_server)
		return NULL;

	CLASS_ID clsid = CLSID_GAME_UI_COOP;
	m_game_ui = smart_cast<CUIGameCoop*> (NEW_INSTANCE(clsid));
	R_ASSERT(m_game_ui);
	m_game_ui->Load();
	m_game_ui->SetClGame(this);
	return					m_game_ui;
}

game_cl_coop::game_cl_coop()
{
}

game_cl_coop::~game_cl_coop()
{
}

