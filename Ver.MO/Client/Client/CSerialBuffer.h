/*---------------------------------------------------------------
����ȭ ���� Serialization Buffer

��Ʈ��ũ ��Ŷ�� Ŭ����
1ȸ��. �����ϰ� ��Ŷ�� ������� ����Ÿ�� �����

**Queue�� �ƴϹǷ� ��ȯ ������ ������� �ʰ� ������ if���� ��� ����
�� �ֱ�(<<).����(>>) �� �ʹ� ���� �Բ� ����ϸ� �ȵȴ�.


- ����

Packet clPacket; // ����

// �Է�
clPacket << 40030;		or	clPacket << iValue;		(int �ֱ�)
clPacket << (BYTE)3;	or	clPacket << byValue;	(BYTE �ֱ�)
clPacket << 1.4f;		or	clPacket << fValue;		(float �ֱ�)

// ���
clPacket >> iValue;		(int ����)
clPacket >> byValue;	(BYTE ����)
clPacket >> fValue;		(float ����)

!.	���ԵǴ� ����Ÿ FIFO ������ �����ȴ�.
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
		// ������ ��� ������ ����
		//
		/////////////////////////////////////////////////////////////////////////
		void	Clear();


		//////////////////////////////////////////////////////////////////////////
		// ���� ũ��
		//
		// Parameters: 
		// Return: (int)��Ŷ ���� ũ��
		//////////////////////////////////////////////////////////////////////////
		int		GetBufferSize() { return _iBufferSize; }


		//////////////////////////////////////////////////////////////////////////
		// ���� ��� ���� ũ��
		//
		// Parameters: 
		// Return: (int)������� ������ ũ��
		//////////////////////////////////////////////////////////////////////////
		int		GetUseSize();


		/////////////////////////////////////////////////////////////////////////
		// ���� ���ۿ� ���� ũ��
		//
		// Parameters: 
		// Return: (int)���� ũ��
		/////////////////////////////////////////////////////////////////////////
		int		GetFreeSize();

		//////////////////////////////////////////////////////////////////////////
		// ������ �Է� :: WritePos �̵�
		//
		// Parameters: (char *)SrcPtr (int)SrcSize
		// Return: (int)�Էµ� ũ��
		//////////////////////////////////////////////////////////////////////////
		int		Enqueue(char *chpData, int iSize);


		//////////////////////////////////////////////////////////////////////////
		// ������ ��� :: ReadPos �̵�
		//
		// Parameters: (char *)DestPtr (int)Size
		// Return: (int)����� ũ��
		//////////////////////////////////////////////////////////////////////////
		int		Dequeue(char *pDest, int iSize);


		/////////////////////////////////////////////////////////////////////////
		// �б� ������(ReadPos) �������� ��� :: ReadPos �ε�
		//
		// Parameters: (char *)������Ptr (int)Size
		// Return: (int)����� ũ��
		/////////////////////////////////////////////////////////////////////////
		int		Peek(char *pDest, int iSize);


		/////////////////////////////////////////////////////////////////////////
		// �Է��� ũ�⸸ŭ WritePos �̵�
		//
		/////////////////////////////////////////////////////////////////////////
		int		MoveWritePos(int iSize);


		/////////////////////////////////////////////////////////////////////////
		// �Է��� ũ�⸸ŭ ReadPos �̵�
		//
		/////////////////////////////////////////////////////////////////////////
		int		MoveReadPos(int iSize);


		/////////////////////////////////////////////////////////////////////////
		// ���� ������
		//
		// Parameters: 
		// Return: (char *) BufferPtr
		/////////////////////////////////////////////////////////////////////////
		char*	GetBufferPtr();


		/////////////////////////////////////////////////////////////////////////
		// ���� ReadPos ������
		//
		// Parameters: 
		// Return: (char *) BufferPtr
		/////////////////////////////////////////////////////////////////////////
		char*	GetReadBufferPtr();


		/////////////////////////////////////////////////////////////////////////
		// ���� WritePos ������
		//
		// Parameters:
		// Return: (char *) BufferPtr
		/////////////////////////////////////////////////////////////////////////
		char*	GetWriteBufferPtr();


		/* ============================================================================= */
		// ������ �������̵�
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