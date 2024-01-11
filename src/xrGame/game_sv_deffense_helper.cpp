#include "StdAfx.h"
#include "game_sv_deffense.h"

void game_sv_deffense::AddMoneyToPlayer(game_PlayerState * ps, s32 amount)
{
	if (!ps) return;

	Msg("- Add money to player: [%u]%s, %d amount", ps->GameID, ps->getName(), amount);

	s64 total_money = ps->money_for_round;
	total_money += amount;

	if (total_money < 0)
		total_money = 0;

	if (total_money > std::numeric_limits<s32>().max())
	{
		Msg("! The limit of the maximum amount of money has been exceeded.");
		total_money = std::numeric_limits<s32>().max() - 1;
	}

	ps->money_for_round = s32(total_money);
	signal_Syncronize();
}

void game_sv_deffense::SpawnItemToActor(u16 actorId, LPCSTR name)
{
	if (!name) return;

	CSE_Abstract *E = spawn_begin(name);
	E->ID_Parent = actorId;
	E->s_flags.assign(M_SPAWN_OBJECT_LOCAL);	// flags

	CSE_ALifeItemWeapon		*pWeapon = smart_cast<CSE_ALifeItemWeapon*>(E);
	if (pWeapon)
	{
		u16 ammo_magsize = pWeapon->get_ammo_magsize();
		pWeapon->a_elapsed = ammo_magsize;
	}

	CSE_ALifeItemPDA *pPda = smart_cast<CSE_ALifeItemPDA*>(E);
	if (pPda)
	{
		pPda->m_original_owner = actorId;
	}

	spawn_end(E, m_server->GetServerClient()->ID);
}


void game_sv_deffense::OnTransferMoney(NET_Packet & P, ClientID const & clientID)
{
	ClientID to;
	s32 money;

	P.r_clientID(to);
	P.r_s32(money);

	Msg("* Try to transfer money from %u to %u. Amount: %d", clientID.value(), to.value(), money);

	game_PlayerState* ps_from = get_id(clientID);
	if (!ps_from)
	{
		Msg("! Can't find player state with id=%u", clientID.value());
		return;
	}

	game_PlayerState* ps_to = get_id(to);
	if (!ps_to)
	{
		Msg("! Can't find player state with id=%u", to.value());
		return;
	}

	if (money <= 0 || ps_from->money_for_round < money) return;

	AddMoneyToPlayer(ps_from, -money);
	AddMoneyToPlayer(ps_to, money);
}