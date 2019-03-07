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
bool netStartUp();
void netCleanUp();

void NetworkProcess();
bool SelectSocket(SOCKET* pTableSocket, FD_SET *pReadSet, FD_SET *pWriteSet, int iSockCnt);

bool ProcAccept();
bool ProcRecv(SOCKET Sock);
int CompleteRecvPacket(st_SESSION * pSession);

bool ProcSend(SOCKET Sock);
void SendPacket_Unicast(st_SESSION * pSession, mylib::CSerialBuffer * pPacket);
void SendPacket_SectorOne(int iSectorX, int iSectorY, mylib::CSerialBuffer * pPacket, st_SESSION *pExceptSession = NULL);
void SendPacket_Around(st_SESSION * pSession, mylib::CSerialBuffer * pPacket, bool bSendMe = false);

///////////////////////////////////////////////////////
// Session 
///////////////////////////////////////////////////////
st_SESSION *	FindSession(SOCKET sock);
st_SESSION *	CreateSession(SOCKET Socket);
bool			DisconnectSession(SOCKET sock);

bool PacketProc(st_SESSION * session, WORD wType, mylib::CSerialBuffer* Packet);
bool PacketProc_Move_Start(st_SESSION *session, mylib::CSerialBuffer* Packet);
bool PacketProc_Move_Stop(st_SESSION *session, mylib::CSerialBuffer* Packet);
bool PacketProc_Attack1(st_SESSION *session, mylib::CSerialBuffer* Packet);
bool PacketProc_Attack2(st_SESSION *session, mylib::CSerialBuffer* Packet);
bool PacketProc_Attack3(st_SESSION *session, mylib::CSerialBuffer* Packet);
bool PacketProc_Echo(st_SESSION *session, mylib::CSerialBuffer* Packet);

///////////////////////////////////////////////////////
// Packet
///////////////////////////////////////////////////////
void mpCreateMyCharacter(mylib::CSerialBuffer *pPacket, DWORD SessionID, BYTE Dir, WORD X, WORD Y, BYTE HP);
void mpCreateOtherCharacter(mylib::CSerialBuffer *pPacket, DWORD SessionID, BYTE Dir, WORD X, WORD Y, BYTE HP);
void mpDeleteCharacter(mylib::CSerialBuffer *pPacket, DWORD SessionID);
void mpMoveStart(mylib::CSerialBuffer *pPacket, DWORD SessionID, BYTE Dir, WORD X, WORD Y);
void mpMoveStop(mylib::CSerialBuffer *pPacket, DWORD SessionID, BYTE Dir, WORD X, WORD Y);
void mpAttack1(mylib::CSerialBuffer *pPacket, DWORD SessionID, BYTE Dir, WORD X, WORD Y);
void mpAttack2(mylib::CSerialBuffer *pPacket, DWORD SessionID, BYTE Dir, WORD X, WORD Y);
void mpAttack3(mylib::CSerialBuffer *pPacket, DWORD SessionID, BYTE Dir, WORD X, WORD Y);
void mpDamage(mylib::CSerialBuffer *pPacket, DWORD AttackID, DWORD DamageID, BYTE DamageHP);
void mpSync(mylib::CSerialBuffer *pPacket, DWORD SessionID, WORD X, WORD Y);
void mpEcho(mylib::CSerialBuffer *pPacket, DWORD Time);

/////////////////////////////////////////
// Server Control
/////////////////////////////////////////
void ServerControl();

extern BOOL g_bShutdown;

#endif