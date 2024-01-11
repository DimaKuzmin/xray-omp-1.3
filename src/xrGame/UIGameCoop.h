#pragma once
#include "UIGameFMP.h"

class game_cl_coop;

class CUIGameCoop : public CUIGameFMP
{
private:
	game_cl_coop *	m_game;
	typedef UIGameMP inherited;
public: 
	CUIGameCoop() {m_game = 0;};
	~CUIGameCoop() {};

	virtual void SetClGame(game_cl_GameState * g);
};