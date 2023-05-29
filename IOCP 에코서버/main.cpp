#include "Network.h"
#include <stdio.h>
#include <conio.h>

#define SERVERPORT	6000

int shutdown = false;

void OnRecv(SESSIONID sessionID, SerializationBuffer& packet)
{
	SerializationBuffer sendPacket(8);
	_int64 echoBody;
	packet >> echoBody;
	sendPacket << echoBody;
	SendPacket(sessionID, sendPacket);
}

int main(void)
{
	int key;
	SetOnRecvEvent(OnRecv);
	if (!InitNetworkLib(SERVERPORT))
	{
		return 0;
	}

	while (!shutdown)
	{
		if (_kbhit())
		{
			key = _getch();
			if (key == 'Q' || key == 'q')
			{
				RequestExitNetworkLibThread();
				shutdown = true;
			}
		}
	}

	printf("메인 스레드 종료 완료\n");
	return 0;
}



