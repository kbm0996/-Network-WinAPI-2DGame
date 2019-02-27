// stdafx.h : 자주 사용하지만 자주 변경되지는 않는
// 표준 시스템 포함 파일 또는 프로젝트 관련 포함 파일이
// 들어 있는 포함 파일입니다.
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // 거의 사용되지 않는 내용은 Windows 헤더에서 제외합니다.
// Windows 헤더 파일:
#include <windows.h>

// C 런타임 헤더 파일입니다.
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>


// TODO: 프로그램에 필요한 추가 헤더는 여기에서 참조합니다.
#define _WINSOCK_DEPRECATED_NO_WARNINGS
////////////////////////////////////////
// Network & System Option
////////////////////////////////////////
#include <cstdlib>
#include <cstdio>
#include <winSock2.h>
#include <Ws2tcpip.h>
#include <windowsx.h>
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "imm32.lib")

#include <Mmsystem.h>
#pragma comment(lib, "Winmm.lib")

#include "_DefineEnum.h"
#include "_Protocol.h"
#include "NetworkProc.h"

#define df_SERVER_PORT 5000
#define UM_NETWORK (WM_USER+1)
#define TITLE 0
#define GAME 1

////////////////////////////////////////
// Container
////////////////////////////////////////
#include "CRingBuffer.h"
#include "CSerialBuffer.h"
#include "CLinkedlist.h"

////////////////////////////////////////
// Rendering
////////////////////////////////////////
#include "CScreenDib.h"
#include "CSpriteDib.h"
#include "CFrameSkip.h"

////////////////////////////////////////
// Object
////////////////////////////////////////
#include "CBaseObject.h"
#include "CEffect.h"
#include "CPlayer.h"
#include "KeyProcess.h"


extern HWND g_hWnd;
extern ObjectList<CBaseObject*> g_lst;