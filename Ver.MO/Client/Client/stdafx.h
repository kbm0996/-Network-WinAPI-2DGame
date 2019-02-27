// stdafx.h : ���� ��������� ���� ��������� �ʴ�
// ǥ�� �ý��� ���� ���� �Ǵ� ������Ʈ ���� ���� ������
// ��� �ִ� ���� �����Դϴ�.
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // ���� ������ �ʴ� ������ Windows ������� �����մϴ�.
// Windows ��� ����:
#include <windows.h>

// C ��Ÿ�� ��� �����Դϴ�.
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>


// TODO: ���α׷��� �ʿ��� �߰� ����� ���⿡�� �����մϴ�.
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