#ifndef __NETWORK_H__
#define __NETWORK_H__


#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"winmm.lib")

#include "CRingBuffer.h"
#include "CSerialBuffer.h"

#include "_Protocol.h"
#include "_DefineEnum.h"

#define dfNETWORK_PORT 20000
#define dfNETWORK_PACKET_RECV_TIMEOUT 300000
#define dfNETWORK_MAX_PACKET_SIZE 1024


///////////////////////////////////////////////////////
// Structure
///////////////////////////////////////////////////////

struct st_SESSION
{
	SOCKET		Sock;
	DWORD		dwSessionID;
	mylib::CRingBuffer	SendQ;
	mylib::CRingBuffer	RecvQ;

	// Debugging
	DWORD		dwLastRecvTick;			// 트래픽 체크
	DWORD		dwTrafficCount;			// 1초마다 수신된 Tick 수
	DWORD		dwTrafficSecondTick;	// 
};

struct st_SECTOR
{
	int iX, iY;
};

struct st_CHARACTER
{
	st_SESSION	*pSession;
	DWORD		dwSessionID;
	BYTE		byHP;
	SHORT		shX, shY;

	// Dead Reckoning
	DWORD		dwAction;
	DWORD		dwActionTick;
	BYTE		byDirection;	 // 서있는 방향 RR / LL
	BYTE		byMoveDirection; // 이동 방향

								 // Sector
	st_SECTOR	OldSector, CurSector;
};

struct st_SECTOR_AROUND
{
	int iCount;
	st_SECTOR Around[9];
};

///////////////////////////////////////////////////////
// Network Process
///////////////////////////////////////////////////////
bool NetworkInit();
void NetworkClose();

void NetworkProc();
bool SelectSocket(SOCKET* pTableSocket, FD_SET *pReadSet, FD_SET *pWriteSet, int iSockCnt);


bool RecvEvent(SOCKET Sock);
int RecvComplete(st_SESSION * pSession);

bool SendEvent(SOCKET Sock);
void SendPacket_Unicast(st_SESSION * pSession, mylib::CSerialBuffer * pPacket);

///////////////////////////////////////////////////////
// Session 
///////////////////////////////////////////////////////
st_SESSION *	FindSession(SOCKET sock);
st_SESSION *	CreateSession(SOCKET Socket);
bool			DisconnectSession(SOCKET sock);

bool OnRecv(st_SESSION * session, WORD wType, mylib::CSerialBuffer* Packet);
bool OnRecv_CreateMyCharacter(st_SESSION * session, mylib::CSerialBuffer *pPacket);
bool OnRecv_CreateOtherCharacter(st_SESSION * session, mylib::CSerialBuffer * pPacket);
bool OnRecv_DeleteCharacter(st_SESSION * session, mylib::CSerialBuffer * pPacket);
bool OnRecv_Move_Start(st_SESSION *session, mylib::CSerialBuffer* Packet);
bool OnRecv_Move_Stop(st_SESSION *session, mylib::CSerialBuffer* Packet);
bool OnRecv_Attack1(st_SESSION *session, mylib::CSerialBuffer* Packet);
bool OnRecv_Attack2(st_SESSION *session, mylib::CSerialBuffer* Packet);
bool OnRecv_Attack3(st_SESSION *session, mylib::CSerialBuffer* Packet);
bool OnRecv_Damage(st_SESSION * session, mylib::CSerialBuffer * pPacket);
bool OnRecv_Sync(st_SESSION * session, mylib::CSerialBuffer *pPacket);
bool OnRecv_Echo(st_SESSION *session, mylib::CSerialBuffer* Packet);

void DummyConnect();

///////////////////////////////////////////////////////
// Packet
///////////////////////////////////////////////////////
void mpMoveStart(mylib::CSerialBuffer *pPacket, BYTE Dir, WORD X, WORD Y);
void mpMoveStop(mylib::CSerialBuffer *pPacket, BYTE Dir, WORD X, WORD Y);
void mpAttack1(mylib::CSerialBuffer *pPacket, BYTE Dir, WORD X, WORD Y);
void mpAttack2(mylib::CSerialBuffer *pPacket, BYTE Dir, WORD X, WORD Y);
void mpAttack3(mylib::CSerialBuffer *pPacket, BYTE Dir, WORD X, WORD Y);
void mpEcho(mylib::CSerialBuffer *pPacket, DWORD Time);


/////////////////////////////////////////
// Server Control
/////////////////////////////////////////
void ServerControl();

extern WCHAR g_szIP[16];
extern DWORD g_MaxClient;

// Log Data
extern UINT64	g_ConnectTry;
extern UINT64	g_ConnectSuccess;
extern UINT64	g_ConnectFail;
extern UINT64	g_SendBytes;
extern UINT64	g_RecvBytes;
extern UINT64	g_PacketCnt;

extern DWORD	g_EchoCnt;
extern DWORD	g_RTTSum;
extern DWORD	g_RTTAvr;
extern DWORD	g_RTTMax;

extern BOOL g_bShutdown;

#endif