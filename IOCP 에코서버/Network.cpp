#pragma comment(lib, "ws2_32")
#include "Network.h"
#include "Log.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>
#include "RingBuffer.h"
#include "SerializationBuffer.h"
#include <map>
#include <locale.h>

#define EXIT_THREAD_CODE	

struct WsaOverlappedEX
{
	WSAOVERLAPPED overlapped;
	void* ptrSession;
};

struct Session
{
	SOCKET socket;
	SOCKADDR_IN clientAddr;
	SESSIONID sessionID;// 사용자가 socket에 직접 접근하지 못하게 하기 위해서,
	WsaOverlappedEX sendOverlapped;
	WsaOverlappedEX recvOverlapped;
	RingBuffer sendRingBuffer;
	RingBuffer recvRingBuffer;
	int overlappedIOCnt;
	int waitSend;

	Session(SOCKET sock, SOCKADDR_IN* addr, SESSIONID id)
	: socket(sock)
	, clientAddr(*addr)
	, sessionID(id)
	, sendRingBuffer(1048576)
	, recvRingBuffer(1048576)
	, overlappedIOCnt(0)
	, waitSend(false)
	{
		sendOverlapped.ptrSession = this;
		recvOverlapped.ptrSession = this;
	}
};

SESSIONID gSessionID = 0;
SOCKET gListenSock;
SOCKADDR_IN gServerAddr;
SYSTEM_INFO gSystemInfo;
DWORD numberOfConcurrentThread;
DWORD numberOfCreateIOCPWorkerThread;
HANDLE hIOCP;
HANDLE hThreadAccept;
HANDLE* hThreadIOCPWorker;
std::map<SESSIONID, Session*> sessionMap;
SRWLOCK	srwlock = RTL_SRWLOCK_INIT;
static void (*OnRecv)(SESSIONID sessionID, SerializationBuffer& packet) = nullptr;

void ReleaseServerResource()
{
	_Log(dfLOG_LEVEL_SYSTEM, "서버 리소스 해제 시작");
	Session* ptrSession;
	std::map<SESSIONID, Session*>::iterator iter = sessionMap.begin();
	while (iter != sessionMap.end())
	{
		ptrSession = iter->second;
		closesocket(ptrSession->socket);
		delete ptrSession;
		++iter;
	}
	sessionMap.clear();
	_Log(dfLOG_LEVEL_SYSTEM, "서버 리소스 해제 완료");
}

void RequestExitNetworkLibThread(void)
{
	_Log(dfLOG_LEVEL_SYSTEM, "서버 종료 처리 시작");
	closesocket(gListenSock); //Accept Thread 종료를 위해서 리슨 소켓 종료;
	for (DWORD i = 0; i < numberOfCreateIOCPWorkerThread; ++i)
	{
		PostQueuedCompletionStatus(hIOCP, 0, 0, nullptr);
	}

	HANDLE* hThreadAll = new HANDLE[numberOfCreateIOCPWorkerThread + 1];
	hThreadAll[0] = hThreadAccept;
	for (DWORD i = 0; i < numberOfCreateIOCPWorkerThread; ++i)
	{
		hThreadAll[i + 1] = hThreadIOCPWorker[i];
	}
	if (WaitForMultipleObjects(numberOfCreateIOCPWorkerThread + 1, hThreadAll, true, INFINITE) == WAIT_FAILED)
	{
		printf("RequestExitProcess - WaitForMultipleObjects error code: %d\n", GetLastError());
	}
	_Log(dfLOG_LEVEL_SYSTEM, "스레드 종료 완료");
	ReleaseServerResource();
	_Log(dfLOG_LEVEL_SYSTEM, "서버 종료 처리 완료");
}

bool InitNetworkLib(WORD port)
{
	if (OnRecv == nullptr)
	{
		_Log(dfLOG_LEVEL_SYSTEM, L"OnRecv 이벤트를 등록하세요");
		return false;
	}

	_wsetlocale(LC_ALL, L"korean");

	WSADATA wsa;
	WCHAR wstrIp[16];
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != NO_ERROR)
	{
		_Log(dfLOG_LEVEL_SYSTEM, L"WSAStartup() errorcode: %d", WSAGetLastError());
		return false;
	}

	gListenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (gListenSock == INVALID_SOCKET)
	{
		_Log(dfLOG_LEVEL_SYSTEM, L"listen socket() errorcode: %d", WSAGetLastError());
		return false;
	}

	linger optLinger;
	optLinger.l_onoff = 1;
	optLinger.l_linger = 0;
	setsockopt(gListenSock, SOL_SOCKET, SO_LINGER, (char*)&optLinger, sizeof(linger));

	DWORD soSndBUfSize = 0;
	setsockopt(gListenSock, SOL_SOCKET, SO_SNDBUF, (char*)&soSndBUfSize, sizeof(DWORD));

	gServerAddr.sin_family = AF_INET;
	gServerAddr.sin_port = htons(port);
	gServerAddr.sin_addr.s_addr = INADDR_ANY;
	if (bind(gListenSock, (SOCKADDR*)&gServerAddr, sizeof(gServerAddr)) == SOCKET_ERROR)
	{
		_Log(dfLOG_LEVEL_SYSTEM, L"listen socket() errorcode: %d", WSAGetLastError());
		goto InitError;
	}

	if (listen(gListenSock, SOMAXCONN) == SOCKET_ERROR)
	{
		_Log(dfLOG_LEVEL_SYSTEM, L"listen() errorcode: %d", WSAGetLastError());
		goto InitError;
	}

	if (InitNetworkIOThread() == false)
	{
		goto InitError;
	}

	InetNtop(AF_INET, &gServerAddr.sin_addr, wstrIp, 16);
	_Log(dfLOG_LEVEL_SYSTEM, L"Server Init OK [%s/%d]", wstrIp, ntohs(gServerAddr.sin_port));

	return true;

InitError:
	closesocket(gListenSock);
	return false;
}

bool InitNetworkIOThread(void)
{
	_int64 idx;
	GetSystemInfo(&gSystemInfo);
	numberOfConcurrentThread = gSystemInfo.dwNumberOfProcessors / 2;
	hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, numberOfConcurrentThread);
	if (hIOCP == nullptr)
	{
		_Log(dfLOG_LEVEL_SYSTEM, "IOCP HANDLE CreateIoCompletionPort() error code: %d", GetLastError());
		return false;
	}
	
	numberOfCreateIOCPWorkerThread = numberOfConcurrentThread - 1;
	hThreadIOCPWorker = new HANDLE[numberOfCreateIOCPWorkerThread];
	for (idx = 0; idx < numberOfCreateIOCPWorkerThread; ++idx)
	{
		hThreadIOCPWorker[idx] = (HANDLE)_beginthreadex(nullptr, 0, IOCPWorkerThread, (void*)idx, 0, nullptr);
		if (hThreadIOCPWorker == nullptr)
		{
			closesocket(gListenSock);
			_Log(dfLOG_LEVEL_SYSTEM, "_beginthreadex(IOCPWorkerThread) error code: %d", GetLastError());
			for (DWORD j = 0; j < idx; ++j)
			{
				PostQueuedCompletionStatus(hIOCP, 0, 0, nullptr);
			}
			return false;
		}
	}

	hThreadAccept = (HANDLE)_beginthreadex(nullptr, 0, AcceptThread, nullptr, 0, nullptr);
	if (hThreadAccept == nullptr)
	{
		closesocket(gListenSock);
		_Log(dfLOG_LEVEL_SYSTEM, "_beginthreadex(AcceptThread) error code: %d", GetLastError());
		for (idx = 0; idx < numberOfCreateIOCPWorkerThread; ++idx)
		{
			PostQueuedCompletionStatus(hIOCP, 0, 0, nullptr);
		}
		return false;
	}

	return true;
}

Session* CreateSession(SOCKET clientSock, SOCKADDR_IN* clientAddr)
{
	CreateIoCompletionPort((HANDLE)clientSock, hIOCP, gSessionID, 0);
	Session* ptrNewSession = new Session(clientSock, clientAddr, gSessionID);
	AcquireSRWLockExclusive(&srwlock);
	sessionMap.insert({ gSessionID, ptrNewSession });
	ReleaseSRWLockExclusive(&srwlock);
	gSessionID += 1;
	return ptrNewSession;
}

void ReleaseSession(Session* ptrSession)
{
	//세션 삭제
	closesocket(ptrSession->socket);
	AcquireSRWLockExclusive(&srwlock);
	sessionMap.erase(ptrSession->sessionID);
	ReleaseSRWLockExclusive(&srwlock);
	delete ptrSession;
}

void SetOnRecvEvent(void (*_OnRecv)(SESSIONID sessionID, SerializationBuffer& packet))
{
	OnRecv = _OnRecv;
}

void PostRecv(Session* ptrSession)
{
	RingBuffer* ptrRecvRingBuffer = &ptrSession->recvRingBuffer;
	WSABUF wsabuf[2];
	int wsabufCnt = 1;
	int recvRingBufferFreeSize;
	DWORD flag = 0;
	int wsaRecvErrorCode;

	wsabuf[0].buf = ptrRecvRingBuffer->GetRearBufferPtr();
	wsabuf[0].len = ptrRecvRingBuffer->GetDirectEnqueueSize();
	recvRingBufferFreeSize = ptrRecvRingBuffer->GetFreeSize();
	if (recvRingBufferFreeSize > (int)wsabuf[0].len)
	{
		wsabuf[1].buf = ptrRecvRingBuffer->GetInternalBufferPtr();
		wsabuf[1].len = recvRingBufferFreeSize - (int)wsabuf[0].len;
		++wsabufCnt;
	}

	ZeroMemory(&ptrSession->recvOverlapped, sizeof(WSAOVERLAPPED));
	InterlockedIncrement((LONG*)&ptrSession->overlappedIOCnt);
	if (WSARecv(ptrSession->socket, wsabuf, wsabufCnt, nullptr, &flag
		, (LPWSAOVERLAPPED)&ptrSession->recvOverlapped, nullptr) == SOCKET_ERROR
		&& (wsaRecvErrorCode = WSAGetLastError()) != WSA_IO_PENDING)
	{
		_Log(dfLOG_LEVEL_SYSTEM, "WSARecv error code: %d", wsaRecvErrorCode);
		if (InterlockedDecrement((LONG*)&ptrSession->overlappedIOCnt) == 0)
		{
			ReleaseSession(ptrSession);
		}
	}
}

void PostSend(Session* ptrSession)
{
	if (InterlockedExchange((LONG*)&ptrSession->waitSend, true))
	{
		return;
	}

	RingBuffer* ptrSendRingBuffer = &ptrSession->sendRingBuffer;
	WSABUF wsabuf[2];
	int wsabufCnt;
	int wsaSendErrorCode;
	char* ptrRear;
	_int64 distanceOfRearToFront;
	
	/*************************************
	** PostSend가 호출되는 2가지 경로
	* 1. 수신 IO 처리시에 OnRecv 함수 안에서 SendPacket 호출한 경우, 호출될 수 있음
	* 2. 송신 완료 처리후에 버퍼에 남은 데이터가 있다면 호출될 수 있음
	* 
	* 1번 경로로 PostSend 실행의 경우, 송신 완료 처리 스레드가 Interlock으로 잠금을 풀어주지 않으면,
	*     수신 IO에서 PostSend 호출이 불가능하다. WSASend를 실행할 수 없다.
	*     그래서 송신 완료 결과가 캐시에 반영아 안됬다가
	*	  , PostSend 로직 실행중에 반영이 될지말지 여부로 고민할 필요없다. 
		  Interlock API가 store buffer의 내용을 무조건 캐시에 반영 시키기 때문이다. 무조건 반영되어 있다.
	* 2번 경로로 PostSend가 실행된 경우, 수신 완료 IO 처리 스레드에서, 송신 링버퍼에 인큐를 동시에 할 수 있는
	*     상황이 발생한다.
	*     PostSend 로직이 도는 순간에도 Dequeue 가능한 크기가 가변적으로 계속 증가할 수 있다는 것이다.
	*     그로 인해 WSASend에 호출시 WSABUF 배열의 2번재 인자의 버퍼주소랑 길이를 잘못 넘겨주면
	*     잘못된 데이터를 전송하게 되는 오류가 발생할 수 있다.
	* 
	*     그래서 Front랑 Rear 포인터 값을 가져와서 로직에서, Front와 Rear 사이의 거리를 직접 계산해서
	*     WSABUF에 넘겨줄 값들을 특정하도록 하였다.
	*     Front 포인터는 PostSend로직이 도는 중에 변경될 걱정이 없고
	*     Rear 포인터는 충분히 위치가 바뀔수 있기 때문에,
	*     Front를 기준으로 Rear의 위치가 어디에 있는지만 파악하면
	*     아무리 Rear가 계속 바뀌고 있다고 하더라도 아래와 같은 로직으로 문제없이
	*	  정확한 데이터를 송신할 수 있다.
	**************************************/
	wsabuf[0].buf = ptrSendRingBuffer->GetFrontBufferPtr();

	ptrRear = ptrSendRingBuffer->GetRearBufferPtr();
	distanceOfRearToFront = (ptrRear - wsabuf[0].buf);

	if (distanceOfRearToFront > 0)
	{
		wsabuf[0].len = ptrSendRingBuffer->GetDirectDequeueSize();
		wsabufCnt = 1;
	}
	else if (distanceOfRearToFront < 0)
	{
		wsabuf[0].len = ptrSendRingBuffer->GetDirectDequeueSize();

		wsabuf[1].buf = ptrSendRingBuffer->GetInternalBufferPtr();
		wsabuf[1].len = (ULONG)(ptrSendRingBuffer->GetRearBufferPtr() - wsabuf[1].buf);
		wsabufCnt = 2;
	}
	else
	{
		InterlockedExchange((LONG*)&ptrSession->waitSend, false);
		return;
	}

	ZeroMemory(&ptrSession->sendOverlapped, sizeof(WSAOVERLAPPED));
	InterlockedIncrement((LONG*)&ptrSession->overlappedIOCnt);
	if (WSASend(ptrSession->socket, wsabuf, wsabufCnt, nullptr, 0
		, (LPWSAOVERLAPPED)&ptrSession->sendOverlapped, nullptr) == SOCKET_ERROR
		&& (wsaSendErrorCode = WSAGetLastError()) != WSA_IO_PENDING)
	{
		_Log(dfLOG_LEVEL_SYSTEM, "WSASend error code: %d", wsaSendErrorCode);
		InterlockedDecrement((LONG*)&ptrSession->overlappedIOCnt);
	}
}

void SendPacket(SESSIONID sessionID, SerializationBuffer& sendPacket)
{
	Session* ptrSession;
	RingBuffer* ptrSendRingBuffer;
	WORD sendPacketHeader;

	AcquireSRWLockShared(&srwlock);
	ptrSession = sessionMap.at(sessionID);
	ReleaseSRWLockShared(&srwlock);

	if ((sendPacketHeader = sendPacket.GetUseSize()) == 0)
	{
		_Log(dfLOG_LEVEL_SYSTEM, "송신패킷 SendRingBuffer Enqueue 실패 크기: %d"
			, (int)(sizeof(sendPacketHeader) + sendPacketHeader));
		return;
	}

	ptrSendRingBuffer = &ptrSession->sendRingBuffer;
	if (ptrSendRingBuffer->GetFreeSize() < (sizeof(sendPacketHeader) + sendPacketHeader))
	{
		_Log(dfLOG_LEVEL_SYSTEM, "송신패킷 SendRingBuffer Enqueue 실패 크기: %d"
			, (int)(sizeof(sendPacketHeader) + sendPacketHeader));
		return;
	}

	ptrSendRingBuffer->Enqueue((char*)&sendPacketHeader, sizeof(sendPacketHeader));
	ptrSendRingBuffer->Enqueue(sendPacket.GetFrontBufferPtr(), sendPacketHeader);
	sendPacket.MoveFront(sendPacketHeader);
	PostSend(ptrSession);
}

unsigned WINAPI AcceptThread(LPVOID args)
{
	int retval;
	int errorCode;
	int acceptErrorCode;
	DWORD flags = 0;
	SOCKET clientSock;
	SOCKADDR_IN clientAddr;
	int addrlen = sizeof(clientAddr);
	_Log(dfLOG_LEVEL_SYSTEM, "AcceptThread");
	for (;;)
	{
		clientSock = accept(gListenSock, (SOCKADDR*)&clientAddr, &addrlen);
		if (clientSock == INVALID_SOCKET)
		{
			acceptErrorCode = WSAGetLastError();
			if (acceptErrorCode == WSAENOTSOCK || acceptErrorCode == WSAEINTR)
			{
				_Log(dfLOG_LEVEL_SYSTEM, "AcceptThread Exit");
				return 0;
			}

			_Log(dfLOG_LEVEL_SYSTEM, "accept() error code: %d", acceptErrorCode);
			closesocket(clientSock);
			continue;
		}

		PostRecv(CreateSession(clientSock, &clientAddr));
		/*WSABUF recvWsaBuf;
		recvWsaBuf.buf = ptrNewSession->recvRingBuffer.GetRearBufferPtr();
		recvWsaBuf.len = ptrNewSession->recvRingBuffer.GetDirectEnqueueSize();
		ZeroMemory(&ptrNewSession->recvOverlapped, sizeof(WSAOVERLAPPED));
		InterlockedIncrement((LONG*)&(ptrNewSession->overlappedIOCnt));
		if (WSARecv(clientSock, &recvWsaBuf, 1, nullptr, &flags
			, (LPWSAOVERLAPPED)&ptrNewSession->recvOverlapped, nullptr) == SOCKET_ERROR
			&& (errorCode = WSAGetLastError()) != WSA_IO_PENDING)
		{
			if (InterlockedDecrement((LONG*)&(ptrNewSession->overlappedIOCnt)) == 0)
			{
				ReleaseSession(ptrNewSession);
			}
		}*/
	}
}

unsigned WINAPI IOCPWorkerThread(LPVOID args)
{
	DWORD numberOfBytesTransferred = 0;
	SESSIONID sessionID = 0;
	WsaOverlappedEX* overlapped = 0;
	Session* ptrSession;
	int retvalGQCS;
	SerializationBuffer recvPacket;
	WORD recvPacketHeader;

	_Log(dfLOG_LEVEL_SYSTEM, "[No.%lld] IOCPWorkerThread", (_int64)args);
	for (;;)
	{
		numberOfBytesTransferred = 0;
		sessionID = 0;
		overlapped = 0;
		retvalGQCS = GetQueuedCompletionStatus(hIOCP, &numberOfBytesTransferred, &sessionID, (LPOVERLAPPED*)&overlapped, INFINITE);
		if (numberOfBytesTransferred == 0 && sessionID == 0 && overlapped == nullptr)
		{
			_Log(dfLOG_LEVEL_SYSTEM, "[No.%lld] IOCPWorkerThread Exit", (_int64)args);
			return 0;
		}

		ptrSession = (Session*)overlapped->ptrSession;
		if (retvalGQCS == TRUE && numberOfBytesTransferred != 0)
		{
			if (&(ptrSession->recvOverlapped) == overlapped)
			{
				//char testbuffer[2048] = { 0, };
				ptrSession->recvRingBuffer.MoveRear(numberOfBytesTransferred);
				for (;;)
				{
					if (ptrSession->recvRingBuffer.GetUseSize() <= sizeof(recvPacketHeader))
					{
						break;
					}

					ptrSession->recvRingBuffer.Peek((char*)&recvPacketHeader, sizeof(recvPacketHeader));
					if (ptrSession->recvRingBuffer.GetUseSize() < (sizeof(recvPacketHeader) + recvPacketHeader))
					{
						break;
					}
					ptrSession->recvRingBuffer.MoveFront(sizeof(recvPacketHeader));
					recvPacket.MoveRear(ptrSession->recvRingBuffer.Dequeue(recvPacket.GetRearBufferPtr(), recvPacketHeader));
					OnRecv(sessionID, recvPacket);
					recvPacket.ClearBuffer();
				}
				
				//ptrSession->sendRingBuffer.Enqueue(recvPacket.GetFrontBufferPtr(), numberOfBytesTransferred);
				//recvPacket.MoveFront(numberOfBytesTransferred);

				//PostSend(ptrSession);
				PostRecv(ptrSession);
			}
			else if (&(ptrSession->sendOverlapped) == overlapped)
			{
				ptrSession->sendRingBuffer.MoveFront(numberOfBytesTransferred);
				InterlockedExchange((LONG*)&ptrSession->waitSend, false);
				if (ptrSession->sendRingBuffer.GetUseSize() > 0)
				{
					PostSend(ptrSession);
				}
			}
		}

		if (InterlockedDecrement((LONG*)&ptrSession->overlappedIOCnt) == 0)
		{
			//세션 삭제
			ReleaseSession(ptrSession);
		}
	}
}