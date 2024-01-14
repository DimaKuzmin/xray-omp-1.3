#include "stdafx.h"
#include "game_sv_mpalife.h"
#include "Level.h"

#include "alife_simulator.h"
#include "alife_object_registry.h"
#include "alife_graph_registry.h"
#include "alife_time_manager.h"

#include "ai_space.h"
#include "level_graph.h"

#include "restriction_space.h"
#include "Actor.h"


game_sv_mpalife::game_sv_mpalife()  
{
	m_alife_simulator			= NULL;
}

game_sv_mpalife::~game_sv_mpalife()
{  
	delete_data(m_alife_simulator);
}

void game_sv_mpalife::Create(shared_str& options)
{
	base_class::Create(options);

	string_path					file_name;

	if (FS.exist(file_name, "$level$", "alife", ".spawn"))
	{
		m_alife_simulator = xr_new<CALifeSimulator>(&server(), &options);
	}
	else
	{
		Msg("Multiplayer>> alife.spawn not found! No A-life");
	}

}

/*void game_sv_mpalife::sls_default()
{
	alife().update_switch	();
}
*/

bool game_sv_mpalife::SpawnItem(LPCSTR section, u16 parent)
{
	if (!pSettings->section_exist(section))
	{
		Msg("! WARNING section \"%s\" doesnt exist", section);
		return false;
	}

	CSE_Abstract *E = spawn_begin(section);
	E->ID_Parent = parent;
	spawn_end(E, parent);

	return true;
};

bool game_sv_mpalife::SpawnItemToPos(LPCSTR section, Fvector3 position)
{
	if (!pSettings->section_exist(section))
	{
		Msg("! WARNING section \"%s\" doesnt exist", section);
		return false;
	}

	CSE_Abstract *E = spawn_begin(section);

	if (E->cast_human_abstract() || E->cast_monster_abstract())
	{
		if (!m_alife_simulator)
		{
			Msg("! You can't spawn \"%s\" because alife simulator is not initialized!", section);
			return false;
		}

		u32 LV = ai().get_level_graph()->vertex_id(position);

		if (ai().get_level_graph()->valid_vertex_id(LV))
			alife().spawn_item(section, position, ai().get_level_graph()->vertex_id(position), 0, 0xffff);
		else
			Msg("! Level vertex incorrect");

		F_entity_Destroy(E);
	}
	else if (E->cast_anomalous_zone())
	{
		CShapeData::shape_def		_shape;
		_shape.data.sphere.P.set(0.0f, 0.0f, 0.0f);
		_shape.data.sphere.R = 3;
		_shape.type = CShapeData::cfSphere;

		CSE_ALifeAnomalousZone *anomaly = E->cast_anomalous_zone();
		anomaly->assign_shapes(&_shape, 1);
		anomaly->m_owner_id = u32(-1);
		anomaly->m_space_restrictor_type = RestrictionSpace::eRestrictorTypeNone;

		anomaly->o_Position = position;
		spawn_end(anomaly, m_server->GetServerClient()->ID);
	}
	else
	{
		E->o_Position = position;
		spawn_end(E, m_server->GetServerClient()->ID);
	}

	return true;
};

bool game_sv_mpalife::TeleportPlayerTo(ClientID id, Fvector3 P)
{
	xrClientData* CL = static_cast<xrClientData*>(Level().Server->GetClientByID(id));
	if (!CL || !CL->net_Ready || !CL->owner || !CL->ps || CL->ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
	{
		return false;
	}

	CActor* pActor = smart_cast<CActor*>(Level().Objects.net_Find(CL->ps->GameID));
	if (!pActor)
		return false;

	SRotation rotation = pActor->Orientation();
	Fvector A;
	A.set(-rotation.pitch, rotation.yaw, 0);

	CL->net_PassUpdates = FALSE;
	CL->net_LastMoveUpdateTime = Level().timeServer();

	NET_Packet MovePacket;
	MovePacket.w_begin(M_MOVE_PLAYERS);
	MovePacket.w_u8(1);
	MovePacket.w_u16(CL->owner->ID);
	MovePacket.w_vec3(P);
	MovePacket.w_vec3(A);
	Level().Server->SendTo(CL->ID, MovePacket, net_flags(TRUE, TRUE));

	return true;
};

bool game_sv_mpalife::TeleportPlayerTo(ClientID id, Fvector3 P, Fvector3 A)
{
	xrClientData* CL = static_cast<xrClientData*>(Level().Server->GetClientByID(id));
	if (!CL || !CL->net_Ready || !CL->owner || !CL->ps || CL->ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
	{
		return false;
	}

	CL->net_PassUpdates = FALSE;
	CL->net_LastMoveUpdateTime = Level().timeServer();

	NET_Packet MovePacket;
	MovePacket.w_begin(M_MOVE_PLAYERS);
	MovePacket.w_u8(1);
	MovePacket.w_u16(CL->owner->ID);
	MovePacket.w_vec3(P);
	MovePacket.w_vec3(A);
	Level().Server->SendTo(CL->ID, MovePacket, net_flags(TRUE, TRUE));

	return true;
};


