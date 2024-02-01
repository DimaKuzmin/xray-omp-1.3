#include "stdafx.h"
#include "aistalker_state_net.h"
#include "stalker_movement_manager_smart_cover.h"
#include "Inventory.h"
#include "Weapon.h"
#include "ai/stalker/ai_stalker.h"

#include "CharacterPhysicsSupport.h"


#include "../Include/xrRender/animation_blend.h"

void aistalker_state_net::ApplyAnimation()
{
	IKinematicsAnimated* ka = smart_cast<IKinematicsAnimated*>(stalker->Visual());
	if (!ka)
		return;

	if (torso.idx != last_torso && torso.id.valid())
	{
		ka->LL_PlayCycle(
			ka->LL_GetMotionDef(torso.id)->bone_or_part,
			torso.id,
			torso.loop,
			ka->LL_GetMotionDef(torso.id)->Accrue(),
			ka->LL_GetMotionDef(torso.id)->Falloff(),
			ka->LL_GetMotionDef(torso.id)->Speed(),
			//ka->LL_GetMotionDef(torso->id)->StopAtEnd(),
			torso.stop_at_end,
			0, 0, 0
		);
		 last_torso = torso.idx;
	}

	if (head.idx != last_head && head.id.valid())
	{
		ka->LL_PlayCycle(
			ka->LL_GetMotionDef(head.id)->bone_or_part,
			head.id,
			head.loop,
			ka->LL_GetMotionDef(head.id)->Accrue(),
			ka->LL_GetMotionDef(head.id)->Falloff(),
			ka->LL_GetMotionDef(head.id)->Speed(),
			//ka->LL_GetMotionDef(torso->id)->StopAtEnd(), 
			head.stop_at_end,
			0, 0, 0
		);
		last_head = head.idx;
	}

	if (legs.idx != last_legs && legs.id.valid())
	{
		MotionID id = legs.id;

		auto blend = ka->LL_PlayCycle(
			ka->LL_GetMotionDef(legs.id)->bone_or_part,
			legs.id,
			legs.loop,
			ka->LL_GetMotionDef(legs.id)->Accrue(),
			ka->LL_GetMotionDef(legs.id)->Falloff(),
			ka->LL_GetMotionDef(legs.id)->Speed(),
			//ka->LL_GetMotionDef(torso->id)->StopAtEnd(),
			legs.stop_at_end,
			0, 0, 0
		);

		last_legs = legs.idx;
		stalker->CStepManager::on_animation_start(id, blend);
	}

}

void aistalker_state_net::OnAnimationChanged(void* pair_manager, MotionID id, CBlend* blend, bool mix, bool global)
{
}

// PACKET WRITE
void aistalker_state_net::CSE_StateWrite(NET_Packet& tNetPacket)
{
	tNetPacket.w_u8(phSyncFlag);
	if (phSyncFlag)
	{
		physics_state.write(tNetPacket);
	}
	else
	{
		tNetPacket.w_vec3(Position);
	}

	tNetPacket.w_float(health);

	tNetPacket.w_angle8(o_torso.pitch);
	tNetPacket.w_angle8(o_torso.yaw);
	tNetPacket.w_angle8(o_torso.roll);

	tNetPacket.w_angle8(o_head.pitch);
	tNetPacket.w_angle8(o_head.yaw);
	tNetPacket.w_angle8(o_head.roll);

	tNetPacket.w_u16(u_active_slot);
	if (u_active_slot != 0)
	{
		tNetPacket.w_u16(u_active_id);
		tNetPacket.w_u8(u_active_stripped);
	}

	tNetPacket.w(&torso, sizeof(torso));
	tNetPacket.w(&legs, sizeof(legs));
	tNetPacket.w(&head, sizeof(head));

	tNetPacket.w_u8(m_wounded);
 
}

// PACKET READ
void aistalker_state_net::CSE_StateRead(NET_Packet& tNetPacket)
{
	tNetPacket.r_u8(phSyncFlag);
	if (phSyncFlag)
	{
		physics_state.read(tNetPacket);
		Position.set(physics_state.physics_position);
	}
	else
	{
		Position.set(tNetPacket.r_vec3());
	}

	tNetPacket.r_float(health);

	tNetPacket.r_angle8(o_torso.pitch);
	tNetPacket.r_angle8(o_torso.yaw);
	tNetPacket.r_angle8(o_torso.roll);

	tNetPacket.r_angle8(o_head.pitch);
	tNetPacket.r_angle8(o_head.yaw);
	tNetPacket.r_angle8(o_head.roll);

	tNetPacket.r_u16(u_active_slot);

	if (u_active_slot != 0)
	{
		tNetPacket.r_u16(u_active_id);
		tNetPacket.r_u8(u_active_stripped);
	}

	tNetPacket.r(&torso, sizeof(torso));
	tNetPacket.r(&legs, sizeof(legs));
	tNetPacket.r(&head, sizeof(head));


	tNetPacket.r_u8(m_wounded);
 
}


// Server NET_EXPORT
void aistalker_state_net::FillState()
{
	if (stalker == nullptr)
	{
		Msg("Check Stalker PTR: !!");
		return;
	}


	CPHSynchronize* sync = stalker->PHGetSyncItem(0);

	if (sync)
	{
		phSyncFlag = 1;

		SPHNetState state;
		sync->get_State(state);
		physics_state.fill(state, Level().timeServer());
	}
	else
	{
		phSyncFlag = 0;
		Position = stalker->Position();
	}

	// health
	health = stalker->GetfHealth();

	// agnles
	o_torso.pitch = stalker->movement().m_body.current.pitch;
	o_torso.yaw = stalker->movement().m_body.current.yaw;
	o_torso.roll = stalker->movement().m_body.current.roll;

	o_head.pitch = stalker->movement().m_head.current.pitch;
	o_head.yaw = stalker->movement().m_head.current.yaw;
	o_head.roll = stalker->movement().m_head.current.roll;

	// inventory
	CWeapon* weapon = smart_cast<CWeapon*>(stalker->inventory().ActiveItem());

	if (weapon)
	{
		u_active_slot = stalker->inventory().ActiveItem()->CurrSlot();
		u_active_id = weapon->ID();
		u_active_stripped = weapon->strapped_mode();
	}
	else
	{
		u_active_slot = NO_ACTIVE_SLOT;
	}

	m_wounded = stalker->wounded();
 }

// CLIENT NET_IMPORT
void aistalker_state_net::GetState()	
{
	if (stalker == nullptr)
	{
		Msg("Check Stalker PTR: !!");
		return;
	}
	

	stalker->SetfHealth(health);
	stalker->m_wounded = m_wounded;

	PIItem item = stalker->inventory().ItemFromSlot(u_active_slot);
	CWeapon* wpn = smart_cast<CWeapon*>(item);

	if (u_active_slot != 0)
		if (item && item->object_id() != u_active_id || !item)
		{
			CObject* obj = Level().Objects.net_Find(u_active_id);
			CInventoryItem* itemINV = smart_cast<CInventoryItem*>(obj);
			CGameObject* game_object = smart_cast<CGameObject*>(obj);

			if (itemINV && itemINV->parent_id() == stalker->ID())
			{
				stalker->inventory().Slot(u_active_slot, itemINV, true, true);
			}
		}

	stalker->inventory().SetActiveSlot(u_active_slot);

	if (wpn && wpn->strapped_mode() != u_active_stripped)
		wpn->strapped_mode(u_active_stripped);

	if (phSyncFlag)
	{
		stalker_interpolation::net_update_A N_A;
	
		N_A.State.enabled = physics_state.physics_state_enabled;
		N_A.State.linear_vel = physics_state.physics_linear_velocity;
		N_A.State.position = physics_state.physics_position;
		
		N_A.o_torso.pitch = o_torso.pitch;
		N_A.o_torso.yaw = o_torso.yaw;
		N_A.o_torso.roll = o_torso.roll;

		N_A.head.pitch	= o_head.pitch;
		N_A.head.yaw	= o_head.yaw;
		N_A.head.roll	= o_head.roll;


		N_A.dwTimeStamp = physics_state.dwTimeStamp;

	 
		// interpocation
		stalker->postprocess_packet(N_A);
	}
	else
	{
		stalker->movement().m_body.current.pitch = o_torso.pitch;
		stalker->movement().m_body.current.yaw = o_torso.yaw;
		stalker->movement().m_body.current.roll = o_torso.roll;

		stalker->movement().m_head.current.pitch = o_head.pitch;
		stalker->movement().m_head.current.yaw = o_head.yaw;
		stalker->movement().m_head.current.roll = o_head.roll;

 		stalker->Position().set(Position);

		stalker->NET_A.clear();
		//TODO: disable interpolation?
	}

	// Pavel: create structure for animation?
	if (stalker->g_Alive())
		ApplyAnimation();


	//	if (state_manager.Position.distance_to_sqr(Position()) > 24 * 24)
	//	{
	//		XFORM().translate_over(state_manager.Position);
	//		Position().set(state_manager.Position); // we need it?
	//		character_physics_support()->movement()->SetPosition(state_manager.Position); // we need it?
	//	}

}
