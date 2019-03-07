#include <locale>
#include "Contents.h"
#include "NetworkProc.h"

void main()
{
	_wsetlocale(LC_ALL, L"korean");
	timeBeginPeriod(1);
	srand((unsigned int)time(NULL));
	

	wprintf(L"Server IP : ");
	wscanf_s(L"%s", &g_szIP, sizeof(g_szIP));
	wprintf(L"Client Number : ");
	wscanf_s(L"%d", &g_MaxClient);

	netStartUp();
	Sleep(10000);

	while (1)
	{
		NetworkProcess();
		Update();
		Sleep(1);
	}

	netCleanUp();
	timeEndPeriod(1);
}