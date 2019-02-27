/*---------------------------------------------------------------
직렬화 버퍼 Serialization Buffer

네트워크 패킷용 클래스
1회용. 간편하게 패킷에 순서대로 데이타를 입출력

**Queue가 아니므로 순환 구조는 고려하지 않고 복잡한 if문은 모두 제거
→ 넣기(<<).빼기(>>) 를 너무 많이 함께 사용하면 안된다.


- 사용법

Packet clPacket; // 선언

// 입력
clPacket << 40030;		or	clPacket << iValue;		(int 넣기)
clPacket << (BYTE)3;	or	clPacket << byValue;	(BYTE 넣기)
clPacket << 1.4f;		or	clPacket << fValue;		(float 넣기)

// 출력
clPacket >> iValue;		(int 빼기)
clPacket >> byValue;	(BYTE 빼기)
clPacket >> fValue;		(float 빼기)

!.	삽입되는 데이타 FIFO 순서로 관리된다.
---------------------------------------------------------------*/
#ifndef  __SERIAL_BUFFER__
#define  __SERIAL_BUFFER__
#include <Windows.h>

namespace mylib
{
	class CSerialBuffer
	{
	public:

		enum en_PACKET
		{
			en_BUFFER_DEFALUT = 3000,
		};

		CSerialBuffer(int iBufferSize = en_BUFFER_DEFALUT);
		virtual	~CSerialBuffer();


		/////////////////////////////////////////////////////////////////////////
		// 버퍼의 모든 데이터 삭제
		//
		/////////////////////////////////////////////////////////////////////////
		void	Clear();


		//////////////////////////////////////////////////////////////////////////
		// 버퍼 크기
		//
		// Parameters: 
		// Return: (int)패킷 버퍼 크기
		//////////////////////////////////////////////////////////////////////////
		int		GetBufferSize() { return _iBufferSize; }


		//////////////////////////////////////////////////////////////////////////
		// 현재 사용 중인 크기
		//
		// Parameters: 
		// Return: (int)사용중인 데이터 크기
		//////////////////////////////////////////////////////////////////////////
		int		GetUseSize();


		/////////////////////////////////////////////////////////////////////////
		// 현재 버퍼에 남은 크기
		//
		// Parameters: 
		// Return: (int)남은 크기
		/////////////////////////////////////////////////////////////////////////
		int		GetFreeSize();

		//////////////////////////////////////////////////////////////////////////
		// 데이터 입력 :: WritePos 이동
		//
		// Parameters: (char *)SrcPtr (int)SrcSize
		// Return: (int)입력된 크기
		//////////////////////////////////////////////////////////////////////////
		int		Enqueue(char *chpData, int iSize);


		//////////////////////////////////////////////////////////////////////////
		// 데이터 출력 :: ReadPos 이동
		//
		// Parameters: (char *)DestPtr (int)Size
		// Return: (int)출력한 크기
		//////////////////////////////////////////////////////////////////////////
		int		Dequeue(char *pDest, int iSize);


		/////////////////////////////////////////////////////////////////////////
		// 읽기 포인터(ReadPos) 기준으로 출력 :: ReadPos 부동
		//
		// Parameters: (char *)데이터Ptr (int)Size
		// Return: (int)출력한 크기
		/////////////////////////////////////////////////////////////////////////
		int		Peek(char *pDest, int iSize);


		/////////////////////////////////////////////////////////////////////////
		// 입력한 크기만큼 WritePos 이동
		//
		/////////////////////////////////////////////////////////////////////////
		int		MoveWritePos(int iSize);


		/////////////////////////////////////////////////////////////////////////
		// 입력한 크기만큼 ReadPos 이동
		//
		/////////////////////////////////////////////////////////////////////////
		int		MoveReadPos(int iSize);


		/////////////////////////////////////////////////////////////////////////
		// 버퍼 포인터
		//
		// Parameters: 
		// Return: (char *) BufferPtr
		/////////////////////////////////////////////////////////////////////////
		char*	GetBufferPtr();


		/////////////////////////////////////////////////////////////////////////
		// 버퍼 ReadPos 포인터
		//
		// Parameters: 
		// Return: (char *) BufferPtr
		/////////////////////////////////////////////////////////////////////////
		char*	GetReadBufferPtr();


		/////////////////////////////////////////////////////////////////////////
		// 버퍼 WritePos 포인터
		//
		// Parameters:
		// Return: (char *) BufferPtr
		/////////////////////////////////////////////////////////////////////////
		char*	GetWriteBufferPtr();


		/* ============================================================================= */
		// 연산자 오버라이딩
		/* ============================================================================= */
		CSerialBuffer	&operator = (const CSerialBuffer &clSrcCSerialBuffer);

		CSerialBuffer	& operator << (const BYTE byValue);
		CSerialBuffer	& operator << (const char chValue);
		CSerialBuffer	& operator << (const short shValue);
		CSerialBuffer	& operator << (const WORD wValue);
		CSerialBuffer	& operator << (const int iValue);
		CSerialBuffer	& operator << (const DWORD dwValue);
		CSerialBuffer	& operator << (const float fValue);
		CSerialBuffer	& operator << (const LONG lSrc);
		CSerialBuffer	& operator << (const __int64 iValue);
		CSerialBuffer	& operator << (const double dValue);
		CSerialBuffer	& operator << (const UINT uiValue);
		CSerialBuffer	& operator << (const UINT64 uiValue);

		CSerialBuffer	& operator >> (BYTE &byValue);
		CSerialBuffer	& operator >> (char &chValue);
		CSerialBuffer	& operator >> (short &shValue);
		CSerialBuffer	& operator >> (WORD &wValue);
		CSerialBuffer	& operator >> (int &iValue);
		CSerialBuffer	& operator >> (DWORD &dwValue);
		CSerialBuffer	& operator >> (float &fValue);
		CSerialBuffer	& operator >> (LONG &lSrc);
		CSerialBuffer	& operator >> (__int64 &iValue);
		CSerialBuffer	& operator >> (double &dValue);
		CSerialBuffer	& operator >> (UINT &uiValue);
		CSerialBuffer	& operator >> (UINT64 &uiValue);

	private:
		// BUFFER
		char*	_pBuffer;
		int		_iRead;
		int		_iWrite;

		// MONITOR
		int		_iBufferSize;
	};
}

#endif