#pragma once
#ifndef __NETWORK_H__
#define	__NETWORK_H__
#include "SerializationBuffer.h"
#define WINAPI	__stdcall

typedef unsigned short		WORD;
typedef	void*				LPVOID;
typedef unsigned long long	SESSIONID;

void SetOnRecvEvent(void (*_OnRecv)(SESSIONID sessionID, SerializationBuffer& packet));
void SendPacket(SESSIONID sessionID, SerializationBuffer& sendPacket);

void RequestExitNetworkLibThread(void);
bool InitNetworkLib(WORD port);
bool InitNetworkIOThread(void);
unsigned int WINAPI AcceptThread(LPVOID args);
unsigned int WINAPI	IOCPWorkerThread(LPVOID args);

#endif // !__NETWORK_H__