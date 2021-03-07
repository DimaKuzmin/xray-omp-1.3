#include "stdafx.h"
#include "IClientStatistic.h"

#pragma warning(push)
#pragma warning(disable:4995)
#include <DPlay/dplay8.h>
#include "GameNetworkingSockets/steam/steamnetworkingsockets.h"
#pragma warning(pop)

void IClientStatistic::Update(DPN_CONNECTION_INFO& CI)
{
	u32 time_global = TimeGlobal(device_timer);
	if (time_global - dwBaseTime >= 999)
	{
		dwBaseTime = time_global;

		mps_recive = CI.dwMessagesReceived - mps_receive_base;
		mps_receive_base = CI.dwMessagesReceived;

		u32	cur_msend = CI.dwMessagesTransmittedHighPriority + CI.dwMessagesTransmittedNormalPriority + CI.dwMessagesTransmittedLowPriority;
		mps_send = cur_msend - mps_send_base;
		mps_send_base = cur_msend;

		dwBytesSendedPerSec = dwBytesSended;
		dwBytesSended = 0;
		dwBytesReceivedPerSec = dwBytesReceived;
		dwBytesReceived = 0;
	}

	dwRoundTripLatencyMS = CI.dwRoundTripLatencyMS;
	dwThroughputBPS = CI.dwThroughputBPS;
	dwPeakThroughputBPS = CI.dwPeakThroughputBPS;
	dwPacketsDropped = CI.dwPacketsDropped;
	dwPacketsRetried = CI.dwPacketsRetried;
}

void IClientStatistic::Update(SteamNetworkingQuickConnectionStatus & status)
{
	u32 time_global = TimeGlobal(device_timer);
	if (time_global - dwBaseTime >= 999)
	{
		dwBaseTime = time_global;
			 
		dwBytesSendedPerSec = dwBytesSended; // from other place
		dwBytesSended = 0;
		dwBytesReceivedPerSec = dwBytesReceived; // from other place
		dwBytesReceived = 0;
	}
	
	dwRoundTripLatencyMS = status.m_nPing;
}
