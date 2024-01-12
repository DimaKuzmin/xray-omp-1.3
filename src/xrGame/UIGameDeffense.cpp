#include "stdafx.h"
#include "UIGameDeffense.h"
#include "game_cl_deffense.h"

#include "Level.h"
#include "Actor.h"
#include "xr_level_controller.h"
#include "ui/UIStatic.h"
#include "ui/UIXmlInit.h"


void CUIGameDeffense::SetClGame(game_cl_GameState* g)
{
	basemp::SetClGame(g);
	m_game = smart_cast<game_cl_deffense*>(g);
	R_ASSERT(m_game);
}