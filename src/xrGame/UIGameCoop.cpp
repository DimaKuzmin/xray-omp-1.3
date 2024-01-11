#include "StdAfx.h"
#include "UIGameCoop.h"

#include "game_cl_coop.h"

void CUIGameCoop::SetClGame(game_cl_GameState* g)
{
	inherited::SetClGame(g);
	m_game = smart_cast<game_cl_coop*>(g);
	R_ASSERT(m_game);
}