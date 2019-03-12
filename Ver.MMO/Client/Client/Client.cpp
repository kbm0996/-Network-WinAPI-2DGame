// Client.cpp : ���� ���α׷��� ���� �������� �����մϴ�.
//

#include "stdafx.h"
#include "Client.h"

#define MAX_LOADSTRING 100

// ���� ����:
HWND g_hWnd;
HINSTANCE g_hInst;                              // ���� �ν��Ͻ��Դϴ�.
HIMC g_hOldIMC;

////////////////////////////////////////////////////////////////////////////////
// Windows Info
//
////////////////////////////////////////////////////////////////////////////////
WCHAR g_szIP[INET_ADDRSTRLEN];
WCHAR szTitle[MAX_LOADSTRING];                  // ���� ǥ���� �ؽ�Ʈ�Դϴ�.
WCHAR szWindowClass[MAX_LOADSTRING];            // �⺻ â Ŭ���� �̸��Դϴ�.
bool g_bActiveApp;

////////////////////////////////////////////////////////////////////////////////
// Update() State
//
////////////////////////////////////////////////////////////////////////////////
int g_iState = GAME;
CLinkedlist<CBaseObject*> g_lst;
CPlayer* g_pMyPlayer;

////////////////////////////////////////////////////////////////////////////////
// Frame Skip
//
////////////////////////////////////////////////////////////////////////////////
CFrameSkip g_FrameSkip(50);

// �� �ڵ� ��⿡ ��� �ִ� �Լ��� ������ �����Դϴ�.
void InitialGame(void);
void Update(void);
void Update_Title(void);
void Update_Game(void);
void Action(void);
void Draw(void);

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
BOOL CALLBACK		DialogProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: ���⿡ �ڵ带 �Է��մϴ�.
	timeBeginPeriod(1);

	////////////////////////////////////////////////////////////////////////////////
	// Server IP Input Dialog
	//
	////////////////////////////////////////////////////////////////////////////////
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_IPADDRESS), NULL, (DLGPROC)DialogProc);

    // ���� ���ڿ��� �ʱ�ȭ�մϴ�.
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_CLIENT, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // ���� ���α׷� �ʱ�ȭ�� �����մϴ�.
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

	////////////////////////////////////////////////////////////////////////////////
	// Game & Network Initialization
	//
	////////////////////////////////////////////////////////////////////////////////
	InitialGame();
	if (NetworkInit(g_szIP) != 0)
		return FALSE;

   

    MSG msg;

	/////////////////////////////////////////////////////////////////////////////////
	// Message Loop
	//
	/////////////////////////////////////////////////////////////////////////////////
	/* TODO : GetMessage() �Լ�
	 �޼���ť�� �޼����� ������ �״�� ��ȯ ���� ��ȯ���� �ʰ� ��� ���·� �ִ´�. 
	�޼����� ����� �ش� �޼����� �����ϰ� ��ȯ ������ 1�� �����Ѵ�. 
	WM_QUIT �޼����� �߻��������� 0�� �����Ѵ�.
	*/
	//HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CLIENT));
    //while (GetMessage(&msg, nullptr, 0, 0))
    //{
    //    if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
    //    {
    //        TranslateMessage(&msg);
    //        DispatchMessage(&msg);
    //    }
    //}

	/* TODO : PeekMessage() �Լ�
	 �������� �Լ��� ����Ǹ� �޼��� ť�� �˻��ϰ� �׻� ��ȯ ������ 1�� ��ȯ �Ѵ�. 
	��, �����°� ����. �Ű������� �־��� MSG����ü�� �޼��� ������ ������ �ش�.
	*/
	while (1)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			// ���� �޼����� �ִ� ���, �޼��� ó��
			if (msg.message == WM_QUIT)
				break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			Update();
		}
	}

	NetworkClose();
	timeEndPeriod(1);

    return (int) msg.wParam;
}



void InitialGame(void)
{
	g_cSprite.LoadDibSprite(eMAP, L"Data\\_Map.bmp", 0, 0);
	g_cSprite.LoadDibSprite(eMAP_TILE, L"Data\\Tile_01.bmp", 0, 0);

	g_cSprite.LoadDibSprite(ePLAYER_STAND_L01, L"Data\\Stand_L_01.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_STAND_L02, L"Data\\Stand_L_02.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_STAND_L03, L"Data\\Stand_L_03.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_STAND_L04, L"Data\\Stand_L_02.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_STAND_L05, L"Data\\Stand_L_01.bmp", 71, 90);

	g_cSprite.LoadDibSprite(ePLAYER_STAND_R01, L"Data\\Stand_R_01.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_STAND_R02, L"Data\\Stand_R_02.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_STAND_R03, L"Data\\Stand_R_03.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_STAND_R04, L"Data\\Stand_R_02.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_STAND_R05, L"Data\\Stand_R_01.bmp", 71, 90);

	g_cSprite.LoadDibSprite(ePLAYER_MOVE_L01, L"Data\\Move_L_01.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_MOVE_L02, L"Data\\Move_L_02.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_MOVE_L03, L"Data\\Move_L_03.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_MOVE_L04, L"Data\\Move_L_04.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_MOVE_L05, L"Data\\Move_L_05.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_MOVE_L06, L"Data\\Move_L_06.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_MOVE_L07, L"Data\\Move_L_07.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_MOVE_L08, L"Data\\Move_L_08.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_MOVE_L09, L"Data\\Move_L_09.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_MOVE_L10, L"Data\\Move_L_10.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_MOVE_L11, L"Data\\Move_L_11.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_MOVE_L12, L"Data\\Move_L_12.bmp", 71, 90);

	g_cSprite.LoadDibSprite(ePLAYER_MOVE_R01, L"Data\\Move_R_01.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_MOVE_R02, L"Data\\Move_R_02.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_MOVE_R03, L"Data\\Move_R_03.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_MOVE_R04, L"Data\\Move_R_04.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_MOVE_R05, L"Data\\Move_R_05.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_MOVE_R06, L"Data\\Move_R_06.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_MOVE_R07, L"Data\\Move_R_07.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_MOVE_R08, L"Data\\Move_R_08.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_MOVE_R09, L"Data\\Move_R_09.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_MOVE_R10, L"Data\\Move_R_10.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_MOVE_R11, L"Data\\Move_R_11.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_MOVE_R12, L"Data\\Move_R_12.bmp", 71, 90);

	g_cSprite.LoadDibSprite(ePLAYER_ATTACK1_L01, L"Data\\Attack1_L_01.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_ATTACK1_L02, L"Data\\Attack1_L_02.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_ATTACK1_L03, L"Data\\Attack1_L_03.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_ATTACK1_L04, L"Data\\Attack1_L_04.bmp", 71, 90);

	g_cSprite.LoadDibSprite(ePLAYER_ATTACK1_R01, L"Data\\Attack1_R_01.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_ATTACK1_R02, L"Data\\Attack1_R_02.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_ATTACK1_R03, L"Data\\Attack1_R_03.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_ATTACK1_R04, L"Data\\Attack1_R_04.bmp", 71, 90);

	g_cSprite.LoadDibSprite(ePLAYER_ATTACK2_L01, L"Data\\Attack2_L_01.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_ATTACK2_L02, L"Data\\Attack2_L_02.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_ATTACK2_L03, L"Data\\Attack2_L_03.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_ATTACK2_L04, L"Data\\Attack2_L_04.bmp", 71, 90);

	g_cSprite.LoadDibSprite(ePLAYER_ATTACK2_R01, L"Data\\Attack2_R_01.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_ATTACK2_R02, L"Data\\Attack2_R_02.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_ATTACK2_R03, L"Data\\Attack2_R_03.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_ATTACK2_R04, L"Data\\Attack2_R_04.bmp", 71, 90);

	g_cSprite.LoadDibSprite(ePLAYER_ATTACK3_L01, L"Data\\Attack3_L_01.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_ATTACK3_L02, L"Data\\Attack3_L_02.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_ATTACK3_L03, L"Data\\Attack3_L_03.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_ATTACK3_L04, L"Data\\Attack3_L_04.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_ATTACK3_L05, L"Data\\Attack3_L_05.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_ATTACK3_L06, L"Data\\Attack3_L_06.bmp", 71, 90);

	g_cSprite.LoadDibSprite(ePLAYER_ATTACK3_R01, L"Data\\Attack3_R_01.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_ATTACK3_R02, L"Data\\Attack3_R_02.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_ATTACK3_R03, L"Data\\Attack3_R_03.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_ATTACK3_R04, L"Data\\Attack3_R_04.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_ATTACK3_R05, L"Data\\Attack3_R_05.bmp", 71, 90);
	g_cSprite.LoadDibSprite(ePLAYER_ATTACK3_R06, L"Data\\Attack3_R_06.bmp", 71, 90);

	g_cSprite.LoadDibSprite(eEFFECT_SPARK_01, L"Data\\xSpark_1.bmp", 70, 70);
	g_cSprite.LoadDibSprite(eEFFECT_SPARK_02, L"Data\\xSpark_2.bmp", 70, 70);
	g_cSprite.LoadDibSprite(eEFFECT_SPARK_03, L"Data\\xSpark_3.bmp", 70, 70);
	g_cSprite.LoadDibSprite(eEFFECT_SPARK_04, L"Data\\xSpark_4.bmp", 70, 70);

	g_cSprite.LoadDibSprite(eGUAGE_HP, L"Data\\HPGuage.bmp", 0, 0);
	g_cSprite.LoadDibSprite(eSHADOW, L"Data\\Shadow.bmp", 32, 4);
}

void Update(void)
{
	switch (g_iState)
	{
	case TITLE:
		Update_Title();
		break;
	case GAME:
		Update_Game();
		break;
	}
}

void Update_Title(void)
{
}

//Select ���� ����ϴ� ���, Update �κп� select �Լ� ȣ�� �� ��Ʈ��ũ �˻簡 ������
//AsyncSelect ���� ����ϱ� ������ Update �Լ��� ������ �ʿ䰡 ����
void Update_Game(void)
{
	if (g_bActiveApp)
		KeyProcess(g_pMyPlayer);

	Action();
	if (!g_FrameSkip.FrameSkip(g_hWnd))
		Draw();

	g_cScreenDib.DrawBuffer(g_hWnd);
}

void Action(void)
{
	CLinkedlist<CBaseObject*>::Node* pNode;
	CBaseObject* pTempData;
	for (auto Object_iter = g_lst.begin(); Object_iter != g_lst.end(); ++Object_iter)
	{
		(*Object_iter)->Action();
	}

	for (auto Object_iter = g_lst.begin(); Object_iter != g_lst.end(); ++Object_iter)
	{
		for (auto Object_Inneriter = g_lst.begin(); Object_Inneriter != g_lst.end(); ++Object_Inneriter)
		{
			if ((*Object_iter)->GetObjectType() != eTYPE_EFFECT)					// ����Ʈ�� �ڷ� �а�
				if ((*Object_iter)->GetCurY() < (*Object_Inneriter)->GetCurY())		// Y��ǥ�� �������� ����
				{
					pTempData = *Object_iter;
					*Object_iter = *Object_Inneriter;
					*Object_Inneriter = pTempData;
				}
		}
	}

	for (auto Object_iter = g_lst.begin(); Object_iter != g_lst.end(); ++Object_iter)		// ����Ʈ �����
	{
		if ((*Object_iter)->GetObjectType() == eTYPE_EFFECT)
		{
			if ((*Object_iter)->IsEndFrame())
			{
				pNode = Object_iter.getNode();
				delete pNode->_Data;
				g_lst.erase(Object_iter);
				break;
			}
		}
	}
}

void Draw(void)
{
	CLinkedlist<CBaseObject*>::iterator Object_iter;

	BYTE *bypDest = g_cScreenDib.GetDibBuffer();
	int iDestWidth = g_cScreenDib.GetWidth();
	int iDestHeight = g_cScreenDib.GetHeight();
	int iDestPitch = g_cScreenDib.GetPitch();
	int iMapX, iMapY;

	//g_cSprite.DrawImage(eMAP, 0, 0, bypDest, iDestWidth, iDestHeight, iDestPitch);

	if (NULL == g_pMyPlayer)
	{
		iMapX = iMapY = 0;
	}
	else
	{
		iMapX = g_pMyPlayer->GetCurX();
		iMapY = g_pMyPlayer->GetCurY();
		g_TileMap.SetScrollPos(iMapX - dfSCREEN_WIDTH / 2, iMapY - dfSCREEN_HEIGHT / 2 - 50);// +50);
	}

	g_TileMap.DrawTile(bypDest, iDestWidth, iDestHeight, iDestPitch);

	for (Object_iter = g_lst.begin(); Object_iter != g_lst.end(); ++Object_iter)
	{
		(*Object_iter)->Draw(bypDest, iDestWidth, iDestHeight, iDestPitch);
	}
}

//
//  �Լ�: MyRegisterClass()
//
//  ����: â Ŭ������ ����մϴ�.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CLIENT));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_CLIENT);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   �Լ�: InitInstance(HINSTANCE, int)
//
//   ����: �ν��Ͻ� �ڵ��� �����ϰ� �� â�� ����ϴ�.
//
//   ����:
//
//        �� �Լ��� ���� �ν��Ͻ� �ڵ��� ���� ������ �����ϰ�
//        �� ���α׷� â�� ���� ���� ǥ���մϴ�.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   g_hInst = hInstance; // �ν��Ͻ� �ڵ��� ���� ������ �����մϴ�.

   g_hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!g_hWnd)
   {
      return FALSE;
   }

   ShowWindow(g_hWnd, nCmdShow);
   UpdateWindow(g_hWnd);

   ////////////////////////////////////////////////////////////////////////////////
   // Window Size Optimization
   //
   ////////////////////////////////////////////////////////////////////////////////
   SetFocus(g_hWnd);
   RECT WindowRect;
   WindowRect.top = 0;
   WindowRect.left = 0;
   WindowRect.right = 640;
   WindowRect.bottom = 480;

   AdjustWindowRectEx(&WindowRect, GetWindowStyle(g_hWnd), GetMenu(g_hWnd) != NULL, GetWindowExStyle(g_hWnd));

   int iX = (GetSystemMetrics(SM_CXSCREEN) / 2) - (640 / 2);
   int iY = (GetSystemMetrics(SM_CYSCREEN) / 2) - (480 / 2);

   MoveWindow(g_hWnd, iX, iY, WindowRect.right - WindowRect.left, WindowRect.bottom - WindowRect.top, TRUE);

   return TRUE;
}

BOOL CALLBACK DialogProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	HWND	hIPCon;
	switch (iMsg)
	{
	case WM_INITDIALOG:
		memset(g_szIP, 0, sizeof(g_szIP));
		hIPCon = GetDlgItem(hWnd, IDC_IPADDRESS);
		SetWindowText(hIPCon, L"127.0.0.1");
		return TRUE;

	case WM_COMMAND:
		switch (wParam)
		{
		case IDOK:
			GetDlgItemText(hWnd, IDC_IPADDRESS, g_szIP, 16);
			EndDialog(hWnd, 99939);
			return TRUE;
		}
		break;
	case WM_DESTROY:
		break;
	}
	return FALSE;
}

//
//  �Լ�: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  ����:  �� â�� �޽����� ó���մϴ�.
//
//  WM_COMMAND  - ���� ���α׷� �޴��� ó���մϴ�.
//  WM_PAINT    - �� â�� �׸��ϴ�.
//  WM_DESTROY  - ���� �޽����� �Խ��ϰ� ��ȯ�մϴ�.
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;
    switch (message)
    {
    //case WM_COMMAND:
    //    {
    //        int wmId = LOWORD(wParam);
    //        // �޴� ������ ���� �м��մϴ�.
    //        switch (wmId)
    //        {
    //        case IDM_ABOUT:
    //            DialogBox(g_hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
    //            break;
    //        case IDM_EXIT:
    //            DestroyWindow(hWnd);
    //            break;
    //        default:
    //            return DefWindowProc(hWnd, message, wParam, lParam);
    //        }
    //    }
    //    break;
	case WM_CREATE:
		g_hOldIMC = ImmAssociateContext(hWnd, NULL);
		break;
	case UM_NETWORK:
		if (!NetworkProc(wParam, lParam))
		{
			MessageBox(hWnd, L"������ ����Ǿ����ϴ�.", L"����", MB_OK);
			// ���α׷� ����
			ImmAssociateContext(hWnd, g_hOldIMC);
			PostQuitMessage(0);
		}
		break;
	case WM_ACTIVATEAPP:
		g_bActiveApp = (BOOL)wParam;
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		ImmAssociateContext(hWnd, g_hOldIMC);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);

	}
    return 0;
}

// ���� ��ȭ ������ �޽��� ó�����Դϴ�.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
