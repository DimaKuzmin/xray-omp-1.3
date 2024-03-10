#pragma once

#include "../Include/xrRender/animation_motion.h"
#include "movement_manager.h"

struct Motions_NUM
{
	MotionID id;
	u8 idx;
	bool loop;
	bool global;
	bool stop_at_end;

};

struct MovementSyncData
{
	u16 movement_game_id;
	u32 movement_level_dest;
	bool movement_wait;
};

enum AnimationType
{
	eTypeHead,
	eTypeLegs,
	eTypeTorso,
	eTypeGlobal,
	eTypeScript
};
