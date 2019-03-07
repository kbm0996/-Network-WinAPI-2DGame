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

		// if���� Ÿ�� �� ��ü�� ���ϰ� ũ�� ������ �������� �׽�Ʈ �ÿ� �ּ� ó��
		ServerControl();

		++g_dwLoopCnt;
	}

	netCleanUp();
	timeEndPeriod(1);
}