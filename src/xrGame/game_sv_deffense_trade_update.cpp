#include "stdafx.h"
#include "game_sv_deffense.h"
#include "Level.h"
#include "trade.h"
#include "actor_mp_client.h"
#include "inventory_upgrade_manager.h"

// Pavel: отключено, так как нет нормальной синхронизации 
// #define ENABLE_DETAILED_TRADING_CHECK 1

void game_sv_deffense::OnPlayerTrade(NET_Packet &P, ClientID const & clientID)
{
	game_PlayerState* ps = get_id(clientID);
	if (!ps) return;

	bool isSelling = P.r_u8();
	u16 traderID = P.r_u16();
	u16 playerID = P.r_u16();
	s32 totalPrice = P.r_s32();

	auto pActor = smart_cast<CActor*>(Level().Objects.net_Find(playerID));
	if (!pActor)
		return;

	auto pTrader = smart_cast<CInventoryOwner*>(Level().Objects.net_Find(traderID));
	if (!pTrader)
		return;

	CTrade* pTrade = pTrader->GetTrade();
	if (!pTrade)
		return;

	if (isSelling)
	{
		u32	itemsCount = P.r_u32();
		xr_vector<CInventoryItem*> items;
		items.reserve(itemsCount);

		s32 svTotalPrice = 0;
		float itemCondition;
		u16 itemId;
 
		for (u32 i = 0; i != itemsCount; ++i)
		{
			P.r_u16(itemId);
			P.r_float(itemCondition);
			auto pItem = smart_cast<CInventoryItem*>(Level().Objects.net_Find(itemId));
			if (pItem)
			{
				items.push_back(pItem);
			}
			else
			{
				Msg("! Warning: item with id=%u not found, skip for trade", itemId);
			}
		}
 
		if (pTrader->get_money() < totalPrice)
		{
			Msg("NPC %s does not have enough money", pTrader->Name());
			return;
		}

		if (!pTrader->InfinitiveMoney())
			pTrader->set_money(pTrader->get_money() - totalPrice, true);

		AddMoneyToPlayer(ps, totalPrice);
		signal_Syncronize();

		for (const auto pItem : items)
		{
			NET_Packet P;
			P.w_begin(M_EVENT);
			P.w_u32(Device.dwTimeGlobal - 2 * NET_Latency);
			P.w_u16(GE_DESTROY);
			P.w_u16(pItem->object_id());
			Level().Send(P, net_flags(TRUE, TRUE));
		}
	}
	else
	{
		if (ps->money_for_round < totalPrice)
		{
			Msg("! Cheater! Buy Items. Player name:  %s; money: %d total price: %d",
				ps->getName(),
				ps->money_for_round,
				totalPrice
			);
			return;
		}

		xr_map<CInventoryItem*, u16> sellMap;
		s32 svTotalPrice = 0;
		u32	itemsMapSize;
		u16 itemId, itemCount;

		P.r_u32(itemsMapSize);
 
		for (u32 i = 0; i != itemsMapSize; ++i)
		{
			P.r_u16(itemId);
			P.r_u16(itemCount);

			auto pItem = smart_cast<CInventoryItem*>(Level().Objects.net_Find(itemId));
			if (pItem)
			{
				sellMap[pItem] = itemCount;
			}
			else
			{
				Msg("! Warning: item with id=%u not found, skip for trade", itemId);
			}
		}
 

		if (!pTrader->InfinitiveMoney())
			pTrader->set_money(pTrader->get_money() + totalPrice, true);

		AddMoneyToPlayer(ps, -totalPrice);
		signal_Syncronize();

		auto it = sellMap.cbegin(), it_e = sellMap.cend();
		for (; it != it_e; it++)
		{
			LPCSTR itemName = it->first->m_section_id.c_str();
			u16 itemsCount = it->second;

			for (u16 i = 0; i < itemsCount; ++i)
			{
				CSE_Abstract *E = spawn_begin(itemName);
				E->ID_Parent = playerID;
				E->s_flags.assign(M_SPAWN_OBJECT_LOCAL);
				spawn_end(E, m_server->GetServerClient()->ID);
			}
		}
	}
}



void game_sv_deffense::OnPlayerRepairItem(NET_Packet& P, ClientID const& clientID)
{
	game_PlayerState* ps = get_id(clientID);
	if (!ps) return;

	u16 itemId = P.r_u16();
	s32 cost = P.r_s32();

	PIItem item = smart_cast<CInventoryItem*>(Level().Objects.net_Find(itemId));
	if (!item) return;

	if (ps->money_for_round < cost) return;

	AddMoneyToPlayer(ps, -cost);

	NET_Packet NP;
	CGameObject::u_EventGen(NP, GE_REPAIR_ITEM, itemId);
	CGameObject::u_EventSend(NP);

	GenerateGameMessage(NP);
	NP.w_u32(GAME_EVENT_MP_REPAIR_SUCCESS);
	NP.w_u16(itemId);
	m_server->SendTo(clientID, NP);
}

void game_sv_deffense::OnPlayerInstallUpgrade(NET_Packet& P, ClientID const& clientID)
{
	game_PlayerState* ps = get_id(clientID);
	if (!ps) return;

	shared_str upgrade_id;

	u16 itemId = P.r_u16();
	P.r_stringZ(upgrade_id);

	PIItem item = smart_cast<CInventoryItem*>(Level().Objects.net_Find(itemId));
	if (!item) return;


	luabind::functor<s32> funct;
	R_ASSERT2(
		ai().script_engine().functor("inventory_upgrades.how_much_upgrade", funct),
		make_string("Failed to get functor <inventory_upgrades.how_much_upgrade>, upgrade id = %s", pSettings->r_string(upgrade_id, "section"))
	);

	s32 cost = funct(pSettings->r_string(upgrade_id, "section"));

	if (ps->money_for_round < cost) return;

	AddMoneyToPlayer(ps, -cost);

	auto& upgrade_manager = Level().inventory_upgrade_manager();

	if (!upgrade_manager.can_upgrade_install(*item, upgrade_id, false))
	{
		Msg("! Warning: cannot install upgrade \"%s\" to \"%s\" Player \"%s\".", upgrade_id.c_str(), item->m_name.c_str(), ps->getName());
		return;
	}

	if (upgrade_manager.upgrade_install_mp(*item, upgrade_id, false))
	{
		NET_Packet NP;
		GenerateGameMessage(NP);
		NP.w_u32(GAME_EVENT_MP_INSTALL_UPGRADE_SUCCESS);
		NP.w_u16(itemId);
		m_server->SendTo(clientID, NP);
	}
}
