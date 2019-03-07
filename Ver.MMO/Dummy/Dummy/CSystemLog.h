/*---------------------------------------------------------------
Log

�α� Ŭ����.
���� �� �ֿܼ� �ܰ躰(DEBUG, WARNG, ERROR, SYSTEM) �α� ���


- ����

LOG_SET(LOG_CONSOLE | LOG_FILE, LOG_DEBUG, L"MMOSERVERLOG");

LOG(L"SYSTEM", LOG_ERROR, L"Error() StringCchVPrintf() Error");
----------------------------------------------------------------*/
#ifndef __SYSTEM_LOG__
#define __SYSTEM_LOG__

#include <stdio.h>
#include <tchar.h>
#include <Strsafe.h>
#include <Windows.h>

namespace mylib
{
	class CSystemLog
	{
	public:
		enum en_SYSTEM_LOG
		{
			// ��� ��� (��Ʈ ����)
			en_CONSOLE = 1,
			en_FILE = 2,

			// �α� ����
			LEVEL_DEBUG = 100,
			LEVEL_WARNG = 101,
			LEVEL_ERROR = 102,
			LEVEL_SYSTM = 103
		};

	private:
		CSystemLog()
		{
			InitializeSRWLock(&_srwLock);
			_iWriteFlag = en_CONSOLE;
			_iLogLevel = LEVEL_DEBUG;
			_lLogCnt = 0;
			_wmkdir(L"_LOG");
		}

		~CSystemLog() {}

	public:
		static CSystemLog *GetInstance()
		{
			static CSystemLog Ins;
			return &Ins;
		}

	public:
		// �⺻ ����(_LOG) ����
		void LOG_SET(int iWriteFlag, int iLogLevel)
		{
			_iWriteFlag = iWriteFlag;
			_iLogLevel = iLogLevel;
			wsprintf(_szDir_Subname, L"_LOG");
		}

		// ���� ����(_LOG\\szDir_SubName) ����
		void LOG_SET(int iWriteFlag, int iLogLevel, WCHAR *szDir_SubName)
		{
			_iWriteFlag = iWriteFlag;
			_iLogLevel = iLogLevel;
			wsprintf(_szDir_Subname, L"_LOG\\%s", szDir_SubName);
			_wmkdir(_szDir_Subname);
		}

		void LOG_SET_DIR(WCHAR *szDir)
		{
			wsprintf(_szDir, L"%s\\%s", _szDir_Subname, szDir);
			_wmkdir(_szDir);
		}

		void LOG(WCHAR *szType, int iLogLevel, WCHAR *szString, ...)
		{
			// ������ `�α� ����` ������ ��� �α� ���
			if (iLogLevel < _iLogLevel)
				return;

			// �α� Ƚ��
			long lLogCount = InterlockedIncrement(&_lLogCnt);

			// `�α� ����` ���ڿ�
			WCHAR szLogLevel[6];
			switch (iLogLevel)
			{
			case LEVEL_DEBUG:
				wsprintf(szLogLevel, L"DEBUG");
				break;
			case LEVEL_WARNG:
				wsprintf(szLogLevel, L"WARNG");
				break;
			case LEVEL_ERROR:
				wsprintf(szLogLevel, L"ERROR");
				break;
			case LEVEL_SYSTM:
				wsprintf(szLogLevel, L"SYSTM");
				break;
			}

			// �������� ó��
			WCHAR szLogString[1024] = { 0, };
			va_list vaList;
			va_start(vaList, szString);
			HRESULT hResult = StringCchVPrintf(szLogString, sizeof(szLogString), szString, vaList);
			va_end(vaList);
			if (FAILED(hResult))
			{
				szLogString[1023] = '\0';
				LOG(szType, LEVEL_WARNG, L"Too Long Log");
			}

			SYSTEMTIME stSysTime;
			GetLocalTime(&stSysTime);

			// WriteFlag�� Ȯ���Ͽ� flag�� �ش��ϴ� �͸� ���
			if (_iWriteFlag & en_CONSOLE)
			{
				// �ֿܼ� �α� ���
				wprintf(L"[%s] [%02d:%02d:%02d / %s] %s\n", szType, stSysTime.wHour, stSysTime.wMinute, stSysTime.wSecond, szLogLevel, szLogString);
			}

			if (_iWriteFlag & en_FILE)
			{
				// ���Ͽ� �α� ����
				// ����
				LOG_SET_DIR(szType);

				// ���ϸ�
				WCHAR szFilename[MAX_PATH];
				wsprintf(szFilename, L"%s\\%s_%d%02d.txt", _szDir, szType, stSysTime.wYear, stSysTime.wMonth);

				// �α�
				WCHAR szLog[2048] = { 0, };
				wsprintf(szLog, L"[%s] [%d-%02d-%02d %02d:%02d:%02d / %s / %08d] %s\n", szType, stSysTime.wYear, stSysTime.wMonth, stSysTime.wDay,
					stSysTime.wHour, stSysTime.wMinute, stSysTime.wSecond, szLogLevel, lLogCount, szLogString);

				AcquireSRWLockExclusive(&_srwLock);
				FILE *fLog;
				do
				{
					_wfopen_s(&fLog, szFilename, L"a");
				} while (fLog == NULL);

				fputws(szLog, fLog);
				fclose(fLog);
				ReleaseSRWLockExclusive(&_srwLock);
			}
		}

	private:
		int		_iWriteFlag;		// ��� ����
		int		_iLogLevel;			// ��� �α� ����
		long	_lLogCnt;			// �α� Ƚ��
		WCHAR	_szDir[64];			// �α� ���� �ּ�
		WCHAR	_szDir_Subname[64];
		SRWLOCK _srwLock;
	};

#define LOG			mylib::CSystemLog::GetInstance()->LOG
#define LOG_SET		mylib::CSystemLog::GetInstance()->LOG_SET

#define LOG_CONSOLE	mylib::CSystemLog::en_CONSOLE
#define LOG_FILE	mylib::CSystemLog::en_FILE

#define LOG_DEBUG	mylib::CSystemLog::LEVEL_DEBUG
#define LOG_WARNG	mylib::CSystemLog::LEVEL_WARNG
#define LOG_ERROR	mylib::CSystemLog::LEVEL_ERROR
#define LOG_SYSTM	mylib::CSystemLog::LEVEL_SYSTM
}
#endif