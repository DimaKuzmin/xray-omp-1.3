#pragma once
#include "UIGameFMP.h"
#include "UIGameMP.h"
 
class game_cl_deffense;
class CUITextWnd;

class CUIGameDeffense : public CUIGameFMP
{
private:
	game_cl_deffense *	m_game;
	typedef CUIGameFMP inherited;
 	typedef UIGameMP basemp;
public:
	CUIGameDeffense() {m_game = 0;};
	~CUIGameDeffense() {};

 
	virtual void SetClGame(game_cl_GameState* g);

 
};
