#include <locale>
#include "Contents.h"
#include "NetworkProc.h"

void main()
{
	_wsetlocale(LC_ALL, L"korean");
	timeBeginPeriod(1);
	srand((unsigned int)time(NULL));
	netStartUp();

	while (!g_bShutdown)
	{
		NetworkProcess();
		Update();

		// if문을 타는 것 자체가 부하가 크기 때문에 본격적인 테스트 시엔 주석 처리
		ServerControl();

		++g_dwLoopCnt;
	}

	netCleanUp();
	timeEndPeriod(1);
}