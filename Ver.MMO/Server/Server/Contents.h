#ifndef __CONTENTS_H__
#define __CONTENTS_H__
#include "NetworkProc.h"
#include "_DefineEnum.h"
#include <map>
#include <list>

///////////////////////////////////////////////////////
// Contents
///////////////////////////////////////////////////////
bool			CreateCharacter(st_SESSION *session);
st_CHARACTER *	FindCharacter(DWORD dwSessionID);
bool			DisconnectCharacter(st_SESSION * pSession);

void Update();

// Character
bool CharacterMoveCheck(SHORT shX, SHORT shY);
int DeadReckoningPos(DWORD dwAction, DWORD dwActionTick, int iOldPosX, int iOldPosY, int *pPosX, int *pPosY);

// Sector
void GetSectorAround(int iSectorX, int iSectorY, st_SECTOR_AROUND *pSectorAround);
void GetUpdateSectorAround(st_CHARACTER *pCharacter, st_SECTOR_AROUND *pRemoveSector, st_SECTOR_AROUND *pAddSector);

bool Sector_UpdateCharacter(st_CHARACTER *pCharacter);
void Sector_RemoveCharacter(st_CHARACTER *pCharacter);
void Sector_AddCharacter(st_CHARACTER *pCharacter);
void CharacterSectorUpdatePacket(st_CHARACTER *pCharacter);

// Debug
void PrintSectorInfo();

extern DWORD g_dwLoopCnt;
extern std::map<DWORD, st_CHARACTER *>	g_CharacterMap;
extern std::list<st_CHARACTER *>		g_Sector[dfSECTOR_MAX_Y][dfSECTOR_MAX_X];

#endif