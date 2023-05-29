#include "Network.h"
#include <stdio.h>
#include <conio.h>

#define SERVERPORT	6000

int shutdown = false;
int main(void)
{
	int key;
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



