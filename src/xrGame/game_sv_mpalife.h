#pragma once
#include "game_sv_mp.h"
 
class game_sv_mpalife : public game_sv_mp 
{
	typedef game_sv_mp base_class;
protected:
		CALifeSimulator					*m_alife_simulator;
public:

	game_sv_mpalife();
	~game_sv_mpalife();

	
	virtual		void				Create(shared_str &options);

	IC			xrServer			&server					() const
	{
		VERIFY						(m_server);
		return						(*m_server);
	}

	IC			CALifeSimulator		&alife					() const
	{
		VERIFY						(m_alife_simulator);
		return						(*m_alife_simulator);
	}


	bool SpawnItem(LPCSTR section, u16 parent);

	bool SpawnItemToPos(LPCSTR section, Fvector3 position);

	bool TeleportPlayerTo(ClientID id, Fvector3 P);

	bool TeleportPlayerTo(ClientID id, Fvector3 P, Fvector3 A);

 };

