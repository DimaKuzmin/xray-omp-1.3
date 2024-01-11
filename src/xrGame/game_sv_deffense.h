#pragma once
#include "game_sv_mpalife.h"
#include "../xrEngine/pure_relcase.h"

class game_sv_deffense : public game_sv_mpalife, private pure_relcase 
{
   	typedef game_sv_mp inherited;

private: 
	virtual		bool				UseSKin() const { return false; }

	virtual		LPCSTR				type_name() const { return "deffense"; };
	
public:	
	// Base Components
	void __stdcall					net_Relcase(CObject* O) {};
	      							game_sv_deffense();
	virtual							~game_sv_deffense();
	virtual		void				Create(shared_str &options);
 
	// Connection Flags
	virtual		void OnPlayerConnect(ClientID id_who);
	virtual		void OnPlayerConnectFinished(ClientID id_who);
	virtual		void OnEvent(NET_Packet& P, u16 type, u32 time, ClientID sender);
 	
	virtual void OnPlayerReady(ClientID id_who);
	virtual void OnPlayerDisconnect(ClientID id_who, LPSTR Name, u16 GameID);
	virtual void OnPlayerKillPlayer(game_PlayerState* ps_killer, game_PlayerState* ps_killed, KILL_TYPE KillType, SPECIAL_KILL_TYPE SpecialKillType, CSE_Abstract* pWeaponA);

	// Helper
	virtual void AddMoneyToPlayer(game_PlayerState* ps, s32 amount);
	virtual void SpawnItemToActor(u16 actorId, LPCSTR name);
	virtual void OnTransferMoney(NET_Packet& P, ClientID const& clientID);

	// Trade 
	virtual void OnPlayerTrade(NET_Packet& P, ClientID const& clientID);
	virtual void OnPlayerRepairItem(NET_Packet& P, ClientID const& clientID);
	virtual void OnPlayerInstallUpgrade(NET_Packet& P, ClientID const& clientID);
};