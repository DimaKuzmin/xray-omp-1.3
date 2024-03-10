#include "stdafx.h"
#include "aistalker_state_net.h"
#include "stalker_movement_manager_smart_cover.h"
#include "Inventory.h"
#include "Weapon.h"
#include "ai/stalker/ai_stalker.h"

#include "CharacterPhysicsSupport.h"

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

	tNetPacket.w_float(o_torso.pitch);
	tNetPacket.w_float(o_torso.yaw);
	//tNetPacket.w_float(o_torso.roll);

	tNetPacket.w_float(o_head.pitch);
	tNetPacket.w_float(o_head.yaw);
	//tNetPacket.w_float(o_head.roll);

	tNetPacket.w_u16(u_active_slot);

	if (u_active_slot != 0)
	{
		tNetPacket.w_u16(u_active_id);
		tNetPacket.w_u8(u_active_stripped);
	}

	tNetPacket.w(&torso_anim, sizeof(torso_anim));
	tNetPacket.w(&legs_anim, sizeof(legs_anim));
	tNetPacket.w(&head_anim, sizeof(head_anim));

	tNetPacket.w_u8(m_wounded);

}


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

	tNetPacket.r_float(o_torso.pitch);
	tNetPacket.r_float(o_torso.yaw);
	//tNetPacket.r_angle8(o_torso.roll);

	tNetPacket.r_float(o_head.pitch);
	tNetPacket.r_float(o_head.yaw);
	//tNetPacket.r_angle8(o_head.roll);

	tNetPacket.r_u16(u_active_slot);

	if (u_active_slot != 0)
	{
		tNetPacket.r_u16(u_active_id);
		tNetPacket.r_u8(u_active_stripped);
	}

	tNetPacket.r(&torso_anim, sizeof(torso_anim));
	tNetPacket.r(&legs_anim, sizeof(legs_anim));
	tNetPacket.r(&head_anim, sizeof(head_anim));

	tNetPacket.r_u8(m_wounded);

}



void aistalker_state_net::FillState(CAI_Stalker* stalker)
{
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

	//float										h, p, b;
	//stalker->XFORM().getHPB(h, p, b);
	//o_torso.yaw = -h;
	//o_torso.pitch = p;
	//o_torso.roll = b;

	o_torso.pitch = stalker->movement().m_body.current.pitch;
	o_torso.yaw = stalker->movement().m_body.current.yaw;
	//o_torso.roll = stalker->movement().m_body.current.roll;

	o_head.pitch = stalker->movement().m_head.current.pitch;
	o_head.yaw = stalker->movement().m_head.current.yaw;
	//o_head.roll = stalker->movement().m_head.current.roll;

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

	head_anim = stalker->num_torso_sv;
	legs_anim = stalker->num_legs_sv;
	torso_anim = stalker->num_head_sv;

	m_wounded = stalker->wounded();

}

void aistalker_state_net::GetState(CAI_Stalker* stalker)
{

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
		N_A.o_torso = o_torso;
		N_A.head = o_head;
		N_A.dwTimeStamp = physics_state.dwTimeStamp;

		stalker->movement().m_body.current.pitch = o_torso.pitch;
		stalker->movement().m_body.current.yaw = o_torso.yaw;
		//stalker->movement().m_body.current.roll = o_torso.roll;

		stalker->movement().m_head.current.pitch = o_head.pitch;
		stalker->movement().m_head.current.yaw = o_head.yaw;
		//stalker->movement().m_head.current.roll = o_head.roll;

		// interpocation
		stalker->postprocess_packet(N_A);
	}
	else
	{

		stalker->movement().m_body.current.pitch = o_torso.pitch;
		stalker->movement().m_body.current.yaw = o_torso.yaw;
		//stalker->movement().m_body.current.roll = o_torso.roll;

		stalker->movement().m_head.current.pitch = o_head.pitch;
		stalker->movement().m_head.current.yaw = o_head.yaw;
		//stalker->movement().m_head.current.roll = o_head.roll;

		stalker->Position().set(Position);

		stalker->NET_A.clear();
		//TODO: disable interpolation?
	}

	// Pavel: create structure for animation?
	if (stalker->g_Alive())
		stalker->ApplyAnimation(&legs_anim, &torso_anim, &head_anim);


	//	if (state_manager.Position.distance_to_sqr(Position()) > 24 * 24)
	//	{
	//		XFORM().translate_over(state_manager.Position);
	//		Position().set(state_manager.Position); // we need it?
	//		character_physics_support()->movement()->SetPosition(state_manager.Position); // we need it?
	//	}

}
