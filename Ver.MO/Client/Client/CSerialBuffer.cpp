#include "stdafx.h"
#include "CSerialBuffer.h"

using namespace mylib;

CSerialBuffer::CSerialBuffer(int iBufferSize)
{
	_iBufferSize = iBufferSize;
	_pBuffer = new char[iBufferSize];
	_iRead = 0;
	_iWrite = 0;
}

CSerialBuffer::~CSerialBuffer()
{
	if (_pBuffer != nullptr)
		delete[] _pBuffer;
	_pBuffer = nullptr;
}


int CSerialBuffer::GetUseSize()
{
	return _iWrite - _iRead;
}

int CSerialBuffer::GetFreeSize()
{
	return _iBufferSize - (_iWrite - _iRead) - 1;
}

int CSerialBuffer::Enqueue(char * chpData, int iSize)
{
	if (_iBufferSize < iSize)
		return 0;

	if (_iBufferSize - (_iWrite - _iRead) - 1 < iSize)
		return 0;

	memcpy((_pBuffer + _iWrite), chpData, iSize);
	_iWrite += iSize;

	return iSize;
}

int CSerialBuffer::Dequeue(char * pDest, int iSize)
{
	if (_iRead == _iWrite)
		return 0;

	if (_iWrite - _iRead < iSize)
		return 0;

	memcpy(pDest, (_pBuffer + _iRead), iSize);
	_iRead += iSize;

	return iSize;
}

int CSerialBuffer::Peek(char * pDest, int iSize)
{
	if (_iRead == _iWrite)
		return 0;

	if (_iWrite - _iRead < iSize)
		return 0;

	memcpy(pDest, (_pBuffer + _iRead), iSize);

	return iSize;
}

int CSerialBuffer::MoveReadPos(int iSize)
{
	if (_iRead == _iWrite)
		return 0;

	if (_iWrite - _iRead < iSize)
		return 0;

	_iRead += iSize;

	return iSize;
}

int CSerialBuffer::MoveWritePos(int iSize)
{
	if (_iBufferSize < iSize)
		return 0;

	if (_iBufferSize - (_iWrite - _iRead) - 1 < iSize)
		return 0;

	_iWrite += iSize;

	return iSize;
}

void CSerialBuffer::Clear()
{
	_iRead = _iWrite = 0;
}

char * CSerialBuffer::GetBufferPtr()
{
	return _pBuffer;
}

char * CSerialBuffer::GetReadBufferPtr()
{
	return &_pBuffer[_iRead];
}

char * CSerialBuffer::GetWriteBufferPtr()
{
	return &_pBuffer[_iWrite];
}

CSerialBuffer& CSerialBuffer::operator =(const CSerialBuffer & clSrCSerialBuffer)
{
	if (_pBuffer != nullptr)
		delete[] _pBuffer;
	_pBuffer = nullptr;

	_iRead = clSrCSerialBuffer._iRead;
	_iWrite = clSrCSerialBuffer._iWrite;
	_iBufferSize = clSrCSerialBuffer._iBufferSize;
	_pBuffer = new char[_iBufferSize];
	memcpy(_pBuffer, clSrCSerialBuffer._pBuffer, clSrCSerialBuffer._iBufferSize);

	return *this;
}

CSerialBuffer& CSerialBuffer::operator <<(const BYTE byValue)
{
	Enqueue((char*)&byValue, sizeof(BYTE));
	return *this;
}

CSerialBuffer & CSerialBuffer::operator<<(const char chValue)
{
	Enqueue((char*)&chValue, sizeof(char));
	return *this;
}

CSerialBuffer & CSerialBuffer::operator<<(const short shValue)
{
	Enqueue((char*)&shValue, sizeof(short));
	return *this;
}


CSerialBuffer & CSerialBuffer::operator<<(const WORD wValue)
{
	Enqueue((char*)&wValue, sizeof(WORD));
	return *this;
}

CSerialBuffer & CSerialBuffer::operator<<(const int iValue)
{
	Enqueue((char*)&iValue, sizeof(int));
	return *this;
}

CSerialBuffer & CSerialBuffer::operator<<(const DWORD dwValue)
{
	Enqueue((char*)&dwValue, sizeof(DWORD));
	return *this;
}

CSerialBuffer & CSerialBuffer::operator<<(const float fValue)
{
	Enqueue((char*)&fValue, sizeof(float));
	return *this;
}

CSerialBuffer & CSerialBuffer::operator<<(const LONG lValue)
{
	Enqueue((char*)&lValue, sizeof(LONG));
	return *this;
}

CSerialBuffer & CSerialBuffer::operator<<(const __int64 iValue)
{
	Enqueue((char*)&iValue, sizeof(__int64));
	return *this;
}

CSerialBuffer & CSerialBuffer::operator<<(const double dValue)
{
	Enqueue((char*)&dValue, sizeof(double));
	return *this;
}

CSerialBuffer & CSerialBuffer::operator<<(const UINT uiValue)
{
	Enqueue((char*)&uiValue, sizeof(UINT));
	return *this;
}

CSerialBuffer & CSerialBuffer::operator<<(const UINT64 uiValue)
{
	Enqueue((char*)&uiValue, sizeof(UINT64));
	return *this;
}

CSerialBuffer& CSerialBuffer::operator >> (BYTE& byValue)
{
	Dequeue((char*)&byValue, sizeof(BYTE));
	return *this;
}

CSerialBuffer & CSerialBuffer::operator >> (char & chValue)
{
	Dequeue((char*)&chValue, sizeof(char));
	return *this;
}

CSerialBuffer & CSerialBuffer::operator >> (short & shValue)
{
	Dequeue((char*)&shValue, sizeof(short));
	return *this;
}

CSerialBuffer & CSerialBuffer::operator >> (WORD & wValue)
{
	Dequeue((char*)&wValue, sizeof(WORD));
	return *this;
}

CSerialBuffer & CSerialBuffer::operator >> (int & iValue)
{
	Dequeue((char*)&iValue, sizeof(int));
	return *this;
}

CSerialBuffer & CSerialBuffer::operator >> (DWORD & dwValue)
{
	Dequeue((char*)&dwValue, sizeof(DWORD));
	return *this;
}

CSerialBuffer & CSerialBuffer::operator >> (float & fValue)
{
	Dequeue((char*)&fValue, sizeof(float));
	return *this;
}

CSerialBuffer & CSerialBuffer::operator >> (LONG & lValue)
{
	Dequeue((char*)&lValue, sizeof(LONG));
	return *this;
}

CSerialBuffer & CSerialBuffer::operator >> (__int64 & iValue)
{
	Dequeue((char*)&iValue, sizeof(__int64));
	return *this;
}

CSerialBuffer & CSerialBuffer::operator >> (double & dValue)
{
	Dequeue((char*)&dValue, sizeof(double));
	return *this;
}

CSerialBuffer & CSerialBuffer::operator >> (UINT & uiValue)
{
	Dequeue((char*)&uiValue, sizeof(UINT));
	return *this;
}

CSerialBuffer & CSerialBuffer::operator >> (UINT64 & uiValue)
{
	Dequeue((char*)&uiValue, sizeof(UINT64));
	return *this;
}
