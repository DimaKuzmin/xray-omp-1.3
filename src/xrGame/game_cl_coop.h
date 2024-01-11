#pragma once

#include "game_cl_freemp.h"
class CUIGameCoop;

class game_cl_coop : public game_cl_freemp
{
private:
	typedef game_cl_freemp inherited;

public:
 	CUIGameCoop *m_game_ui;

	virtual void SetGameUI(CUIGameCustom*);
	virtual CUIGameCustom* createGameUI();

	  		game_cl_coop();
	virtual	~game_cl_coop();

	
	virtual void OnConnected();

};

