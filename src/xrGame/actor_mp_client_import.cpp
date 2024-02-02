#include "stdafx.h"
#include "actor_mp_client.h"
#include "inventory.h"
#include "level.h"
#include "GamePersistent.h"
#include "game_cl_base.h"
#include "../xrEngine/camerabase.h"
//#include "Physics.h"
#include "../xrphysics/phvalide.h"
#include "CharacterPhysicsSupport.h"

void CActorMP::net_Import	( NET_Packet &P)
{
	net_update			N;

	m_state_holder.read	(P);
	if (!valid_pos(m_state_holder.state().position))
	{
		Msg("ActorCL net_Import [%u] bad position: [%f][%f][%f]", ID(), VPUSH(m_state_holder.state().position));
		return;
	}
	//R_ASSERT2(valid_pos(m_state_holder.state().position), "imported bad position");

	
	/*if (m_i_am_dead)
		return;*/

	if (OnClient())
	{
/*#ifdef DEBUG
		if (GetfHealth() != m_state_holder.state().health)
			Msg("net_Import: [%d][%s], is going to set health to %2.04f", this->ID(), Name(), m_state_holder.state().health);
#endif*/
		
		game_PlayerState* ps = Game().GetPlayerByGameID(this->object_id());
		float new_health = m_state_holder.state().health;
		if (GetfHealth() < new_health)
		{
			SetfHealth(new_health);
		} 
		else
		{
			if (!ps || !ps->testFlag(GAME_PLAYER_FLAG_INVINCIBLE))
			{
				SetfHealth(new_health);
			}
		}
	}

	if (PPhysicsShell() != NULL)
	{
		return;
	}
	

	if (OnClient())
		SetfRadiation	(m_state_holder.state().radiation*100.0f);

	u16		ActiveSlot = m_state_holder.state().inventory_active_slot;

	if (OnClient() && (inventory().GetActiveSlot()!=ActiveSlot) )
	{
#ifdef DEBUG
		Msg("Client-SetActiveSlot[%d][%d]",ActiveSlot, Device.dwFrame);
#endif // #ifdef DEBUG
		inventory().SetActiveSlot(ActiveSlot);
	}

	N.mstate			= m_state_holder.state().body_state_flags;

	N.dwTimeStamp		= m_state_holder.state().time;
	N.p_pos				= m_state_holder.state().position;

	N.o_model			= m_state_holder.state().model_yaw;
	N.o_torso.yaw		= m_state_holder.state().camera_yaw;
	N.o_torso.pitch		= m_state_holder.state().camera_pitch;
	N.o_torso.roll		= m_state_holder.state().camera_roll;

	if (N.o_torso.roll > PI)
		N.o_torso.roll	-= PI_MUL_2;

	{
		if (Level().IsDemoPlay() || OnServer() || Remote())
		{
			unaffected_r_torso.yaw		= N.o_torso.yaw;
			unaffected_r_torso.pitch	= N.o_torso.pitch;
			unaffected_r_torso.roll		= N.o_torso.roll;

			cam_Active()->yaw	= -N.o_torso.yaw;
			cam_Active()->pitch = N.o_torso.pitch;
		};
	};

	//CSE_ALifeCreatureActor
	N.p_accel				= m_state_holder.state().logic_acceleration;

	process_packet			(N);

	net_update_A			N_A;
	m_States.clear			();

	N_A.State.enabled		= m_state_holder.state().physics_state_enabled;
	N_A.State.angular_vel	= m_state_holder.state().physics_angular_velocity;
	N_A.State.linear_vel	= m_state_holder.state().physics_linear_velocity;
	N_A.State.force			= m_state_holder.state().physics_force;
	N_A.State.torque		= m_state_holder.state().physics_torque;
	N_A.State.position		= m_state_holder.state().physics_position;
	N_A.State.quaternion	= m_state_holder.state().physics_quaternion;


	if (g_Alive() && (Remote() || OnServer()))
	{
		Fvector currentPosition;
		character_physics_support()->movement()->GetPosition(currentPosition);

		// Pavel: ���� ����� ������� ������ ������� �� 4 �����, �� ������������� ������, � ����� ������������.
		// ����� ��� ��������� ������.
		if (currentPosition.distance_to(N_A.State.position) > 16.f) // distance > 4m
		{
			character_physics_support()->movement()->SetPosition(N_A.State.position);
			character_physics_support()->movement()->SetVelocity(0, 0, 0);
			character_physics_support()->movement()->GetPosition(Position());

			const float block_damage_time_seconds = 2.f;
			character_physics_support()->movement()->BlockDamageSet(u64(block_damage_time_seconds / fixed_step));

			unaffected_r_torso.yaw = N.o_torso.yaw;
			unaffected_r_torso.pitch = N.o_torso.pitch;
			unaffected_r_torso.roll = N.o_torso.roll;
			cam_Active()->Set(-unaffected_r_torso.yaw, unaffected_r_torso.pitch, 0);//, unaffected_r_torso.roll);

			NET.clear();
			NET.push_back(N);
			NET_Last = N;

			NET_A.clear();

			mstate_real = mstate_wishful = NET_Last.mstate;
			NET_SavedAccel = NET_Last.p_accel;

			m_bInInterpolation = false;

			Level().RemoveObject_From_4CrPr(this);
			return;
		}
	}

	// interpolcation
	postprocess_packet		(N_A);
}

void CActorMP::postprocess_packet	(net_update_A &N_A)
{

	if (!NET.empty())
		N_A.dwTimeStamp		= NET.back().dwTimeStamp;
	else
		N_A.dwTimeStamp		= Level().timeServer();

	N_A.State.previous_position	= N_A.State.position;
	N_A.State.previous_quaternion = N_A.State.quaternion;

	if (Local() && OnClient() || !g_Alive()) return;

	{
		//-----------------------------------------------
		if (!NET_A.empty() && N_A.dwTimeStamp < NET_A.back().dwTimeStamp) return;
		if (!NET_A.empty() && N_A.dwTimeStamp == NET_A.back().dwTimeStamp)
		{
			NET_A.back() = N_A;
		}
		else
		{
			VERIFY(valid_pos(N_A.State.position));
			NET_A.push_back			(N_A);
			if (NET_A.size()>5) NET_A.pop_front();
		};

		if (!NET_A.empty()) m_bInterpolate = true;
	};

	Level().AddObject_To_Objects4CrPr	(this);
	CrPr_SetActivated					(false);
	CrPr_SetActivationStep				(0);
}

void CActorMP::process_packet		(net_update &N)
{
	if (Local() && OnClient())
		return;

	if (!NET.empty() && (N.dwTimeStamp < NET.back().dwTimeStamp))
		return;

	if (g_Alive()) {
		if (!MpInvisibility())
			setVisible		((BOOL)!HUDview	());
		setEnabled		(TRUE);
	};

	if (!NET.empty() && (N.dwTimeStamp == NET.back().dwTimeStamp)) {
		NET.back()		= N;
		return;
	}

	NET.push_back		(N);

	if (NET.size() > 5)
		NET.pop_front	();
}
