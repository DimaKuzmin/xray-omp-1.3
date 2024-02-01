#pragma once
#include "net_physics_state.h"
#include "../Include/xrRender/animation_motion.h"

class CAI_Stalker;
class MotionID;
class CBlend;

struct Motions_NUM
{
	MotionID id;
	u8 idx;
	bool loop;
	bool global;
	bool stop_at_end;

};


struct	SRotation_Stalker
{
	float  yaw, pitch, roll;
	SRotation_Stalker() { yaw = pitch = roll = 0; }
	SRotation_Stalker(float y, float p, float r) { yaw = y; pitch = p; roll = r; }
};

class aistalker_state_net
{
	net_physics_state				physics_state;
	CAI_Stalker* stalker;

	u16								u_active_slot = 0;
	u16								u_active_id = 0;
	u8								u_active_stripped = 0;

	Motions_NUM						head;
	Motions_NUM						legs;
	Motions_NUM						torso;

	u8 last_torso;
	u8 last_head;
	u8 last_legs;

	u8								phSyncFlag = 0;
	u8								m_wounded = 0;


	void ApplyAnimation();

public:

	Fvector							Position;
	SRotation_Stalker						o_torso;
	SRotation_Stalker						o_head;

	float							health = 1.0f;

	void OnAnimationChanged(void* pair_manager, MotionID id, CBlend* blend, bool mix, bool global);
 
	void CSE_StateWrite(NET_Packet& p);
	void CSE_StateRead(NET_Packet& p);

	void FillState();
	void GetState();

	aistalker_state_net()
	{

	}

	aistalker_state_net(void* s) : stalker( (CAI_Stalker*) s)
	{
 
	};

	~aistalker_state_net() { stalker = 0; }

};

