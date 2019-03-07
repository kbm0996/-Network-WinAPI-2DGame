#ifndef __CONTENTS_H__
#define __CONTENTS_H__
#include "NetworkProc.h"
#include "_DefineEnum.h"
#include <map>
#include <list>

///////////////////////////////////////////////////////
// Contents
///////////////////////////////////////////////////////
st_CHARACTER *	FindCharacter(DWORD dwSessionID);

void Update();

// Character
void AI_Stop(st_CHARACTER * pCharacter);
void AI_Move(st_CHARACTER * pCharacter);
void AI_Attack(st_CHARACTER * pCharacter);
void AI_Echo(st_CHARACTER * pCharacter);
int DeadReckoningPos(DWORD dwAction, DWORD dwActionTick, int iOldPosX, int iOldPosY, int *pPosX, int *pPosY);

void Monitoring();

extern DWORD g_dwLoopCnt;
extern std::map<DWORD, st_CHARACTER *>	g_CharacterMap;

#endif