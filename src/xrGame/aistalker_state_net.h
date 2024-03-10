#pragma once
#include "net_physics_state.h"
#include "ai_stalker_animations.h"

class CAI_Stalker;

class aistalker_state_net
{

public:
	net_physics_state				physics_state;


	u16								u_active_slot = 0;
	u16								u_active_id = 0;
	u8								u_active_stripped = 0;

	Motions_NUM						head_anim;
	Motions_NUM						legs_anim;
	Motions_NUM						torso_anim;

	u8								phSyncFlag = 0;
	u8								m_wounded = 0;

	Fvector							Position;
	SRotation						o_torso;
	SRotation						o_head;
	float							health = 1.0f;


	void CSE_StateWrite(NET_Packet& p);
	void CSE_StateRead(NET_Packet& p);

	void FillState(CAI_Stalker* stalker);
	void GetState(CAI_Stalker* stalker);

};

