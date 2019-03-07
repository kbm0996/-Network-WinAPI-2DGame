#include "CRingBuffer.h"

mylib::CRingBuffer::CRingBuffer(int iBufferSize)
{
	_iBufferSize = iBufferSize;
	_pBuffer = new char[iBufferSize];
	_iRead = 0;
	_iWrite = 0;

	InitializeSRWLock(&_srwLock);
}

mylib::CRingBuffer::~CRingBuffer()
{
	if (_pBuffer != nullptr)
		delete[] _pBuffer;
	_pBuffer = nullptr;
}

void mylib::CRingBuffer::Lock()
{
	AcquireSRWLockExclusive(&_srwLock);
}

void mylib::CRingBuffer::Unlock()
{
	ReleaseSRWLockExclusive(&_srwLock);
}

int mylib::CRingBuffer::GetUseSize()
{
	if (_iWrite >= _iRead)
		return _iWrite - _iRead;
	else
		return (_iBufferSize - _iRead) + _iWrite;
}

int mylib::CRingBuffer::GetFreeSize()
{
	return _iBufferSize - GetUseSize() - 1;
}

int mylib::CRingBuffer::GetUnbrokenDequeueSize()
{
	if (_iWrite >= _iRead)
		return _iWrite - _iRead;
	else
		return _iBufferSize - _iRead;
}

int mylib::CRingBuffer::GetUnbrokenEnqueueSize()
{
	if ((_iWrite + 1) % _iBufferSize == _iRead)
		return 0;

	if (_iWrite <= ((_iRead + _iBufferSize - 1) % _iBufferSize))
		return ((_iRead + _iBufferSize - 1) % _iBufferSize) - _iWrite;
	else if (_iWrite >= _iRead)
		return _iBufferSize - _iWrite;

	return 0;
}

int mylib::CRingBuffer::Enqueue(char * chpData, int iSize)
{
	if ((_iWrite + 1) % _iBufferSize == _iRead)
		return 0;

	if (GetFreeSize() <= iSize)
		iSize = GetFreeSize();

	if (GetUnbrokenEnqueueSize() >= iSize)
		memcpy((_pBuffer + _iWrite), chpData, iSize);
	else
	{
		int PutSize = GetUnbrokenEnqueueSize();
		memcpy((_pBuffer + _iWrite), chpData, PutSize);
		memcpy(_pBuffer, (chpData + PutSize), iSize - PutSize);
	}

	_iWrite = (_iWrite + iSize) % _iBufferSize;

	return iSize;
}

int mylib::CRingBuffer::Dequeue(char * pDest, int iSize)
{
	if (_iRead == _iWrite)
		return 0;

	if (GetUseSize() <= iSize)
		iSize = GetUseSize();

	if (GetUnbrokenDequeueSize() >= iSize)
		memcpy(pDest, (_pBuffer + _iRead), iSize);
	else
	{
		int GetSize = GetUnbrokenDequeueSize();
		memcpy(pDest, (_pBuffer + _iRead), GetSize);
		memcpy((pDest + GetSize), _pBuffer, iSize - GetSize);
	}

	_iRead = (_iRead + iSize) % _iBufferSize;

	return iSize;
}

int mylib::CRingBuffer::Peek(char * pDest, int iSize)
{
	if (_iRead == _iWrite)
		return 0;

	if (GetUseSize() <= iSize)
		iSize = GetUseSize();


	if (GetUnbrokenDequeueSize() >= iSize)
		memcpy(pDest, (_pBuffer + _iRead), iSize);
	else
	{
		int GetSize = GetUnbrokenDequeueSize();
		memcpy(pDest, (_pBuffer + _iRead), GetSize);
		memcpy((pDest + GetSize), _pBuffer, iSize - GetSize);
	}

	return iSize;
}

void mylib::CRingBuffer::MoveReadPos(int iSize)
{
	_iRead = (_iRead + iSize) % _iBufferSize;
}

int mylib::CRingBuffer::MoveWritePos(int iSize)
{
	_iWrite = (_iWrite + iSize) % _iBufferSize;
	return iSize;
}

void mylib::CRingBuffer::Clear()
{
	_iRead = _iWrite = 0;
}

char * mylib::CRingBuffer::GetBufferPtr()
{
	return _pBuffer;
}

char * mylib::CRingBuffer::GetReadBufferPtr()
{
	return &_pBuffer[_iRead];
}

char * mylib::CRingBuffer::GetWriteBufferPtr()
{
	return &_pBuffer[_iWrite];
}