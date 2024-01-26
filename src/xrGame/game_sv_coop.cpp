#include "stdafx.h"
#include "game_sv_coop.h"


#include "alife_simulator.h"
#include "alife_object_registry.h"
#include "alife_graph_registry.h"
#include "alife_time_manager.h"

#include "ai_space.h"
#include "level_graph.h"

game_sv_coop::game_sv_coop()
{
	m_type = eGameIDCoop;
}

game_sv_coop::~game_sv_coop()
{
}

shared_str game_sv_coop::level_name(const shared_str& server_options)
{
	Msg("Coop");
	return ai().alife().level_name();
}
  