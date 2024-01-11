#pragma once
#include "game_cl_mp.h"

class CVoiceChat;
class CUIGameDeffense;

class game_cl_deffense  : public game_cl_mp
{
private:
	typedef game_cl_mp inherited;

public:
	CUIGameDeffense *m_game_ui;
	CVoiceChat* m_pVoiceChat = nullptr;
   	void OnVoiceMessage(NET_Packet* P);

	virtual void TranslateGameMessage(u32 msg, NET_Packet& P);

public:
	virtual void SetGameUI(CUIGameCustom*);
	virtual CUIGameCustom* createGameUI();

	  		game_cl_deffense();
	virtual	~game_cl_deffense();


	virtual void shedule_Update(u32 dt);
	virtual void OnRender();

	virtual	bool OnKeyboardPress(int key);
	virtual	bool OnKeyboardRelease(int key);
 
	
	virtual LPCSTR GetGameScore(string32&	score_dest);
	virtual bool Is_Rewarding_Allowed()  const { return false; };

	virtual void OnConnected();

	virtual void OnScreenResolutionChanged();
};

