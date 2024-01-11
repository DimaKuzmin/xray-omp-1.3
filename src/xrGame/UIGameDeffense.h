#pragma once
#include "UIGameMP.h"
 
class game_cl_deffense;
class CUITextWnd;

class CUIGameDeffense : public UIGameMP
{
private:
	game_cl_deffense *	m_game;
	typedef UIGameMP inherited;

protected:
	CUITextWnd*			m_stats;

public:
				CUIGameDeffense();
	virtual		~CUIGameDeffense();

	virtual	void Init(int stage);

	virtual void SetClGame(game_cl_GameState* g);

 
	virtual void	_BCL OnFrame();

	virtual bool IR_UIOnKeyboardPress(int dik);
};
