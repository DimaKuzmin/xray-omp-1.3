#include "stdafx.h"
#include "pch_script.h"
#include "ai_stalker.h"

#include "../../ai_object_location.h"
#include "../../game_graph.h"
#include "../../ai_space.h"
#include "../../CharacterPhysicsSupport.h"
#include "../../stalker_movement_manager_smart_cover.h"
#include "../../inventory.h"
#include "../../stalker_animation_manager.h"
#include "../../Weapon.h"

#include "../../CharacterPhysicsSupport.h"
#include "../../../xrphysics/iPHWorld.h"
#include "../../PHMovementControl.h"

#include "net_physics_state.h"
#include "../xrphysics/phvalide.h"

extern int g_cl_InterpolationType;

void CAI_Stalker::net_Save(NET_Packet& P)
{
	inherited::net_Save(P);
	m_pPhysics_support->in_NetSave(P);
}

BOOL CAI_Stalker::net_SaveRelevant()
{
	return (inherited::net_SaveRelevant() || BOOL(PPhysicsShell() != NULL));
}

void CAI_Stalker::net_Export(NET_Packet& P)
{
	R_ASSERT(Local());
	
	if (IsGameTypeSingle()) 
	{
		// export last known packet
		R_ASSERT(!NET.empty());
		net_update& N = NET.back();
		//	P.w_float						(inventory().TotalWeight());
		//	P.w_u32							(m_dwMoney);

		P.w_float(GetfHealth());

		P.w_u32(N.dwTimeStamp);
		P.w_u8(0);
		P.w_vec3(N.p_pos);
		P.w_float /*w_angle8*/(N.o_model);
		P.w_float /*w_angle8*/(N.o_torso.yaw);
		P.w_float /*w_angle8*/(N.o_torso.pitch);
		P.w_float /*w_angle8*/(N.o_torso.roll);
		P.w_u8(u8(g_Team()));
		P.w_u8(u8(g_Squad()));
		P.w_u8(u8(g_Group()));


		float					f1 = 0;
		GameGraph::_GRAPH_ID		l_game_vertex_id = ai_location().game_vertex_id();
		P.w(&l_game_vertex_id, sizeof(l_game_vertex_id));
		P.w(&l_game_vertex_id, sizeof(l_game_vertex_id));
		//	P.w						(&f1,						sizeof(f1));
		//	P.w						(&f1,						sizeof(f1));
		if (ai().game_graph().valid_vertex_id(l_game_vertex_id)) {
			f1 = Position().distance_to(ai().game_graph().vertex(l_game_vertex_id)->level_point());
			P.w(&f1, sizeof(f1));
			f1 = Position().distance_to(ai().game_graph().vertex(l_game_vertex_id)->level_point());
			P.w(&f1, sizeof(f1));
		}
		else {
			P.w(&f1, sizeof(f1));
			P.w(&f1, sizeof(f1));
		}

		P.w_stringZ(m_sStartDialog);
	}
	else
	{
		stalker_network_state.FillState(this);
		stalker_network_state.CSE_StateWrite(P);
	}
}

void CAI_Stalker::net_Import(NET_Packet& P)
{
	R_ASSERT(Remote());
	if (IsGameTypeSingle())
	{
		net_update						N;

		u8 flags;

		P.r_float();
		set_money(P.r_u32(), false);

		float health;
		P.r_float(health);
		SetfHealth(health);
		//	fEntityHealth = health;

		P.r_u32(N.dwTimeStamp);
		P.r_u8(flags);
		P.r_vec3(N.p_pos);
		P.r_float /*r_angle8*/(N.o_model);
		P.r_float /*r_angle8*/(N.o_torso.yaw);
		P.r_float /*r_angle8*/(N.o_torso.pitch);
		P.r_float /*r_angle8*/(N.o_torso.roll);
		id_Team = P.r_u8();
		id_Squad = P.r_u8();
		id_Group = P.r_u8();


		GameGraph::_GRAPH_ID				graph_vertex_id = movement().game_dest_vertex_id();
		P.r(&graph_vertex_id, sizeof(GameGraph::_GRAPH_ID));
		graph_vertex_id = ai_location().game_vertex_id();
		P.r(&graph_vertex_id, sizeof(GameGraph::_GRAPH_ID));

		if (NET.empty() || (NET.back().dwTimeStamp < N.dwTimeStamp)) {
			NET.push_back(N);
			NET_WasInterpolating = TRUE;
		}

		P.r_float();
		P.r_float();

		P.r_stringZ(m_sStartDialog);

		setVisible(TRUE);
		setEnabled(TRUE);
	}
	else
	{
		stalker_network_state.CSE_StateRead(P);
		stalker_network_state.GetState(this);

		setVisible(TRUE);
		setEnabled(TRUE);
	}
}

void CAI_Stalker::postprocess_packet(stalker_interpolation::net_update_A &N_A)
{

	if (!NET_A.empty())
		N_A.dwTimeStamp = NET_A.back().dwTimeStamp;
	else
		N_A.dwTimeStamp = Level().timeServer();

	N_A.State.previous_position = N_A.State.position;
	N_A.State.previous_quaternion = N_A.State.quaternion;

	if (Local() && OnClient() || !g_Alive())
		return;

	if (!NET_A.empty() && N_A.dwTimeStamp < NET_A.back().dwTimeStamp) return;
	if (!NET_A.empty() && N_A.dwTimeStamp == NET_A.back().dwTimeStamp)
	{
		NET_A.back() = N_A;
	}
	else
	{
		VERIFY(valid_pos(N_A.State.position));
		NET_A.push_back(N_A);
		if (NET_A.size() > 5)
		{
			NET_A.pop_front();
		}
	};

	if (!NET_A.empty()) m_bInterpolate = true;

	Level().AddObject_To_Objects4CrPr(this);
	CrPr_SetActivated(false);
	CrPr_SetActivationStep(0);
}


void CAI_Stalker::PH_B_CrPr()
{
	if (IsGameTypeSingle())
	{
		inherited::PH_B_CrPr();
		return;
	}

	if (CrPr_IsActivated()) return;
	if (CrPr_GetActivationStep() > physics_world()->StepsNum()) return;
	

	if (g_Alive())
	{
		CrPr_SetActivated(true);
			   
		stalker_interpolation::InterpData* pIStart = &IStart;
		pIStart->Pos = Position();
		pIStart->Vel = m_pPhysics_support->movement()->GetVelocity();

		pIStart->o_torso.yaw = angle_normalize(movement().m_body.current.yaw);
		pIStart->o_torso.pitch = angle_normalize(movement().m_body.current.pitch);
		//pIStart->o_torso.roll = angle_normalize(movement().m_body.current.roll);

		pIStart->head.pitch = angle_normalize(movement().m_head.current.pitch);
		pIStart->head.yaw = angle_normalize(movement().m_head.current.yaw);

		CPHSynchronize* pSyncObj = NULL;
		pSyncObj = PHGetSyncItem(0);
		if (!pSyncObj) return;
		pSyncObj->get_State(LastState);

		if (Local() && OnClient())
		{
			PHUnFreeze();
			pSyncObj->set_State(NET_A.back().State);
		}
		else
		{
			auto N_A = NET_A.back();
			NET_A_Last = N_A;

			if (!N_A.State.enabled)
			{
				pSyncObj->set_State(N_A.State);
			}
			else
			{
				PHUnFreeze();
				pSyncObj->set_State(N_A.State);
				Position().set(IStart.Pos);
			};
		};
	}
	else
	{
		CrPr_SetActivated(true);
		PHUnFreeze();
	}
}

void CAI_Stalker::PH_I_CrPr()
{
	if (IsGameTypeSingle())
	{
		inherited::PH_I_CrPr();
		return;
	}

	if (!CrPr_IsActivated()) return;
	
	if (g_Alive())
	{
		CPHSynchronize* pSyncObj = NULL;
		pSyncObj = PHGetSyncItem(0);
		if (!pSyncObj) return;
		pSyncObj->get_State(RecalculatedState);	
	};
}

void CAI_Stalker::PH_A_CrPr()
{
	if (IsGameTypeSingle())
	{
		inherited::PH_A_CrPr();
		return;
	}

	if (!CrPr_IsActivated()) return;
	if (!g_Alive()) return;
	
	CPHSynchronize* pSyncObj = NULL;
	pSyncObj = PHGetSyncItem(0);
	if (!pSyncObj) return;

	pSyncObj->get_State(PredictedState);
	pSyncObj->set_State(RecalculatedState);

	if (!m_bInterpolate)
	{
		return;
	}

	CalculateInterpolationParams();
}


void CAI_Stalker::ApplyAnimation(Motions_NUM* legs, Motions_NUM* torso, Motions_NUM* head)
{
	IKinematicsAnimated* ka = smart_cast<IKinematicsAnimated*>(Visual());
	if (!ka)
		return;
 
	if (torso->idx != idx_torso_cl && torso->id.valid())
	{
		Msg("Apply Anim : idx: %u Torso", torso->idx);
		torso_blend = ka->LL_PlayCycle(
			ka->LL_GetMotionDef(torso->id)->bone_or_part,
			torso->id,
			torso->loop,
			ka->LL_GetMotionDef(torso->id)->Accrue(),
			ka->LL_GetMotionDef(torso->id)->Falloff(),
			ka->LL_GetMotionDef(torso->id)->Speed(),
			//ka->LL_GetMotionDef(torso->id)->StopAtEnd(),
			torso->stop_at_end,
			0, 0, 0
		);
		idx_torso_cl = torso->idx;
	}

	if (head->idx != idx_head_cl && head->id.valid())
	{
		Msg("Apply Anim : idx: %u head", head->idx);

		head_blend = ka->LL_PlayCycle(
			ka->LL_GetMotionDef(head->id)->bone_or_part,
			head->id,
			head->loop,
			ka->LL_GetMotionDef(head->id)->Accrue(),
			ka->LL_GetMotionDef(head->id)->Falloff(),
			ka->LL_GetMotionDef(head->id)->Speed(),
			//ka->LL_GetMotionDef(torso->id)->StopAtEnd(), 
			head->stop_at_end,
			0, 0, 0
		);
		idx_head_cl = head->idx;
	}

	if (legs->idx != idx_legs_cl && legs->id.valid())
	{
		Msg("Apply Anim : idx: %u legs", legs->idx);


		CBlend* blend = ka->LL_PlayCycle(
			ka->LL_GetMotionDef(legs->id)->bone_or_part,
			legs->id,
			legs->loop,
			ka->LL_GetMotionDef(legs->id)->Accrue(),
			ka->LL_GetMotionDef(legs->id)->Falloff(),
			ka->LL_GetMotionDef(legs->id)->Speed(),
			//ka->LL_GetMotionDef(torso->id)->StopAtEnd(),
			legs->stop_at_end,
			0, 0, 0
		);

		CStepManager::on_animation_start(legs->id, blend);
		idx_legs_cl = legs->idx;
		legs_blend = blend;
	}



}



void CAI_Stalker::OnAnimationChange(void* pair_manager, MotionID id, CBlend* blend, bool mix, bool global)
{
	if (blend->bone_or_part == 0) //LEGS
	{
		if (num_legs_sv.idx > 254)
			num_legs_sv.idx = 0;

		num_legs_sv.idx++;
		num_legs_sv.id = blend->motionID;
		num_legs_sv.loop = mix;
		num_legs_sv.global = global;
		num_legs_sv.stop_at_end = pair_manager == &animation().script() ? blend->stop_at_end : FALSE;
	}

	if (blend->bone_or_part == 1) //TORSO
	{
		if (num_torso_sv.idx > 254)
			num_torso_sv.idx = 0;

		num_torso_sv.idx++;
		num_torso_sv.id = blend->motionID;
		num_torso_sv.loop = mix;
		num_torso_sv.global = global;
		num_torso_sv.stop_at_end = pair_manager == &animation().script() ? blend->stop_at_end : FALSE;
	}

	if (blend->bone_or_part == 2) //HEAD
	{
		if (num_head_sv.idx > 254)
			num_head_sv.idx = 0;

		num_head_sv.idx++;
		num_head_sv.id = blend->motionID;
		num_head_sv.loop = mix;
		num_head_sv.global = global;
		num_head_sv.stop_at_end = pair_manager == &animation().script() ? blend->stop_at_end : FALSE;
	}

}

void CAI_Stalker::CalculateInterpolationParams()
{
	CPHSynchronize* pSyncObj = NULL;
	pSyncObj = PHGetSyncItem(0);

	stalker_interpolation::InterpData* pIStart = &IStart;
	//stalker_interpolation::InterpData* pIRec = &IRec;
	stalker_interpolation::InterpData* pIEnd = &IEnd;

	//pIRec->Pos = RecalculatedState.position;
	//pIRec->Vel = RecalculatedState.linear_vel;
	//pIRec->o_torso = NET_A_Last.o_torso;
	//pIRec->head = NET_A_Last.head;

	pIEnd->Pos = PredictedState.position;
	pIEnd->Vel = PredictedState.linear_vel;
	pIEnd->o_torso = NET_A_Last.o_torso;
	pIEnd->head = NET_A_Last.head;

	Fvector SP0, SP1, SP2, SP3;
	Fvector HP0, HP1, HP2, HP3;

	SP0 = pIStart->Pos;
	HP0 = pIStart->Pos;

	if (m_bInInterpolation)
	{
		u32 CurTime = Level().timeServer();
		float factor = float(CurTime - m_dwIStartTime) / (m_dwIEndTime - m_dwIStartTime);
		if (factor > 1.0f) factor = 1.0f;

		float c = factor;
		for (u32 k = 0; k < 3; k++)
		{
			SP0[k] = c * (c*(c*SCoeff[k][0] + SCoeff[k][1]) + SCoeff[k][2]) + SCoeff[k][3];
			SP1[k] = (c*c*SCoeff[k][0] * 3 + c * SCoeff[k][1] * 2 + SCoeff[k][2]) / 3; //     3       !!!!

			HP0[k] = c * (c*(c*HCoeff[k][0] + HCoeff[k][1]) + HCoeff[k][2]) + HCoeff[k][3];
			HP1[k] = (c*c*HCoeff[k][0] * 3 + c * HCoeff[k][1] * 2 + HCoeff[k][2]) / 3; //     3       !!!!
		};

		SP1.add(SP0);
	}
	else
	{
		if (LastState.linear_vel.x == 0 && LastState.linear_vel.y == 0 && LastState.linear_vel.z == 0)
		{
			HP1.sub(RecalculatedState.position, RecalculatedState.previous_position);
		}
		else
		{
			HP1.sub(LastState.position, LastState.previous_position);
		};
		HP1.mul(1.0f / fixed_step);
		SP1.add(HP1, SP0);
	}

	HP2.sub(PredictedState.position, PredictedState.previous_position);
	HP2.mul(1.0f / fixed_step);
	SP2.sub(PredictedState.position, HP2);

	SP3.set(PredictedState.position);
	HP3.set(PredictedState.position);

	Fvector TotalPath;
	TotalPath.sub(SP3, SP0);
	float TotalLen = TotalPath.magnitude();

	SPHNetState	State0 = (NET_A.back()).State;
	SPHNetState	State1 = PredictedState;

	float lV0 = State0.linear_vel.magnitude();
	float lV1 = State1.linear_vel.magnitude();

	u32 ConstTime = u32((fixed_step - physics_world()->FrameTime()) * 1000) + Level().GetInterpolationSteps()*u32(fixed_step * 1000);

	m_dwIStartTime = m_dwILastUpdateTime;
	m_dwIEndTime = m_dwIStartTime + ConstTime;

	Fvector V0, V1;
	V0.set(HP1);
	V1.set(HP2);
	lV0 = V0.magnitude();
	lV1 = V1.magnitude();

	if (TotalLen != 0)
	{
		if (V0.x != 0 || V0.y != 0 || V0.z != 0)
		{
			if (lV0 > TotalLen / 3)
			{
				HP1.normalize();
				//				V0.normalize();
				//				V0.mul(TotalLen/3);
				HP1.normalize();
				HP1.mul(TotalLen / 3);
				SP1.add(HP1, SP0);
			}
		}

		if (V1.x != 0 || V1.y != 0 || V1.z != 0)
		{
			if (lV1 > TotalLen / 3)
			{
				//				V1.normalize();
				//				V1.mul(TotalLen/3);
				HP2.normalize();
				HP2.mul(TotalLen / 3);
				SP2.sub(SP3, HP2);
			};
		}
	};
	/////////////////////////////////////////////////////////////////////////////
	for (u32 i = 0; i < 3; i++)
	{
		SCoeff[i][0] = SP3[i] - 3 * SP2[i] + 3 * SP1[i] - SP0[i];
		SCoeff[i][1] = 3 * SP2[i] - 6 * SP1[i] + 3 * SP0[i];
		SCoeff[i][2] = 3 * SP1[i] - 3 * SP0[i];
		SCoeff[i][3] = SP0[i];

		HCoeff[i][0] = 2 * HP0[i] - 2 * HP3[i] + HP1[i] + HP2[i];
		HCoeff[i][1] = -3 * HP0[i] + 3 * HP3[i] - 2 * HP1[i] - HP2[i];
		HCoeff[i][2] = HP1[i];
		HCoeff[i][3] = HP0[i];
	};
	/////////////////////////////////////////////////////////////////////////////
	m_bInInterpolation = true;

	if (m_pPhysicsShell) m_pPhysicsShell->NetInterpolationModeON();
}

void CAI_Stalker::make_Interpolation()
{
	m_dwILastUpdateTime = Level().timeServer();

	if (g_Alive() && m_bInInterpolation)
	{
		u32 CurTime = m_dwILastUpdateTime;

		if (CurTime >= m_dwIEndTime)
		{
			m_bInInterpolation = false;

			CPHSynchronize* pSyncObj = NULL;
			pSyncObj = PHGetSyncItem(0);
			if (!pSyncObj) return;
			pSyncObj->set_State(PredictedState);
			VERIFY2(_valid(renderable.xform), *cName());
		}
		else
		{
			float factor = 0.0f;

			if (m_dwIEndTime != m_dwIStartTime)
				factor = float(CurTime - m_dwIStartTime) / (m_dwIEndTime - m_dwIStartTime);

			clamp(factor, 0.f, 1.0f);

			Fvector NewPos;
			NewPos.lerp(IStart.Pos, IEnd.Pos, factor);

			VERIFY2(_valid(renderable.xform), *cName());
			
			movement().m_body.current.pitch = angle_lerp(IStart.o_torso.pitch, IEnd.o_torso.pitch, factor);
			//movement().m_body.current.roll = angle_lerp(IStart.o_torso.roll, IEnd.o_torso.roll, factor);
			movement().m_body.current.yaw = angle_lerp(IStart.o_torso.yaw, IEnd.o_torso.yaw, factor);
			movement().m_head.current.pitch = angle_lerp(IStart.head.pitch, IEnd.head.pitch, factor);
			movement().m_head.current.yaw = angle_lerp(IStart.head.yaw, IEnd.head.yaw, factor);


			for (u32 k = 0; k < 3; k++)
			{
				IPosL[k] = NewPos[k];
				IPosS[k] = factor * (factor*(factor*SCoeff[k][0] + SCoeff[k][1]) + SCoeff[k][2]) + SCoeff[k][3];
				IPosH[k] = factor * (factor*(factor*HCoeff[k][0] + HCoeff[k][1]) + HCoeff[k][2]) + HCoeff[k][3];
			};

			Fvector SpeedVector, ResPosition;
			switch (g_cl_InterpolationType)
			{
			case 0:
				{
					ResPosition.set(IPosL);
					SpeedVector.sub(IEnd.Pos, IStart.Pos);
					SpeedVector.div(float(m_dwIEndTime - m_dwIStartTime) / 1000.0f);
				}break;
			case 1:
				{
					for (int k = 0; k < 3; k++)
						SpeedVector[k] = (factor*factor*SCoeff[k][0] * 3 + factor * SCoeff[k][1] * 2 + SCoeff[k][2]) / 3; //     3       !!!!

					ResPosition.set(IPosS);
				}break;
			case 2:
				{
					for (int k = 0; k < 3; k++)
						SpeedVector[k] = (factor*factor*HCoeff[k][0] * 3 + factor * HCoeff[k][1] * 2 + HCoeff[k][2]);

					ResPosition.set(IPosH);
				}break;
			default:
				R_ASSERT2(0, "Unknown interpolation curve type!");
				break;
			}
			Position().set(ResPosition);
			character_physics_support()->movement()->SetPosition(ResPosition); // we need it ?
			character_physics_support()->movement()->SetVelocity(SpeedVector);
		};
	}
	else
	{
		m_bInInterpolation = false;
	};
};
