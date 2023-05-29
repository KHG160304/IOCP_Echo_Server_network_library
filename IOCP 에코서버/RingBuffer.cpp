
#include "RingBuffer.h"
#include <memory.h>

static int defaultCapacity = sizeof(size_t);//1000;//1048576; // 1mb : 1024byte(1kb) * 1024
static int minCapacity = 2; // 100byte is minimum size

RingBuffer::RingBuffer(void)
	: __capacity(defaultCapacity + 1)
	, __internalBuffer(new char[defaultCapacity + 1])
	, __queueFrontIndex(0)
	, __queueRearIndex(0)
{
}

RingBuffer::RingBuffer(int capacity)
	: __queueFrontIndex(0)
	, __queueRearIndex(0)
{
	if (capacity < minCapacity)
	{
		this->__capacity = minCapacity + 1;
	}
	else
	{
		this->__capacity = capacity + 1;
	}
	this->__internalBuffer = new char[this->__capacity];
}

RingBuffer::~RingBuffer()
{
	delete[] __internalBuffer;
}

bool RingBuffer::Resize(int size)
{
	return false;
}

int RingBuffer::GetBufferSize()
{
	return this->__capacity - 1;
}

int RingBuffer::GetUseSize(void)
{
	/*
		_load* 변수의 용도
		멀티스레딩시 한쪽은 Enqueue, 한쪽은 Dequeue 할때
		GetUseSize 로직이 작동하는 중에, 다른 스레드에 의해서
		변수의 값이 변경될 것을 우려하여, 함수를 호출한 스레드의
		지역변수에 값을 로드하여 사용한다.
		공유 메모리 사용을 하지 않기위해서 이다.
	*/
	int _loadRearIndex = this->__queueRearIndex;
	int _loadFrontIndex = this->__queueFrontIndex;
	if (_loadRearIndex >= _loadFrontIndex)
	{
		return (_loadRearIndex - _loadFrontIndex);
	}
	else
	{
		return this->__capacity - (_loadFrontIndex - _loadRearIndex);
	}
}

int RingBuffer::GetFreeSize()
{
	/*
		_load* 변수의 용도
		멀티스레딩시 한쪽은 Enqueue, 한쪽은 Dequeue 할때
		GetFreeSize 로직이 작동하는 중에, 다른 스레드에 의해서
		변수의 값이 변경될 것을 우려하여, 함수를 호출한 스레드의
		지역변수에 값을 로드하여 사용한다. 
		공유 메모리 사용을 하지 않기위해서 이다.
	*/
	int _loadRearIndex = this->__queueRearIndex;
	int _loadFrontIndex = this->__queueFrontIndex;
	if (_loadRearIndex >= _loadFrontIndex)
	{
		return this->__capacity - (_loadRearIndex - _loadFrontIndex) - 1;
	}
	else
	{
		return (_loadFrontIndex - _loadRearIndex) - 1;
	}
}

int RingBuffer::GetDirectEnqueueSize(void) const
{
	int _loadRearIndex = this->__queueRearIndex;
	int _loadFrontIndex = this->__queueFrontIndex;
	if (_loadRearIndex >= _loadFrontIndex)
	{
		if (_loadFrontIndex > 0)
		{
			return this->__capacity - _loadRearIndex;
		}
		else
		{
			return this->__capacity - _loadRearIndex - 1;
		}
	}
	else
	{
		return _loadFrontIndex - _loadRearIndex - 1;
	}
}

int RingBuffer::GetDirectDequeueSize(void) const
{
	int _loadRearIndex = this->__queueRearIndex;
	int _loadFrontIndex = this->__queueFrontIndex;
	if (_loadRearIndex >= _loadFrontIndex)
	{
		return _loadRearIndex - _loadFrontIndex;
	}
	else
	{
		return this->__capacity - _loadFrontIndex;
	}
}

int RingBuffer::Enqueue(const char* data, int size)
{ 
	if (this->GetFreeSize() < size || size < 1)
	{
		return 0;
	}

	int nextRearIndex = (this->__queueRearIndex + size) % this->__capacity;
	if (nextRearIndex <= this->__queueRearIndex && nextRearIndex != 0)
	{
		memcpy(this->__internalBuffer + this->__queueRearIndex, data, size - nextRearIndex);
		memcpy(this->__internalBuffer, data + (size - nextRearIndex), nextRearIndex);
	}
	else
	{
		memcpy(this->__internalBuffer + this->__queueRearIndex, data, size);
	}

	this->__queueRearIndex = nextRearIndex;
	return size;
}

int RingBuffer::Dequeue(char* const buffer, int size)
{
	if (this->GetUseSize() < size || size < 1)
	{
		return 0;
	}

	int nextFrontIndex = (this->__queueFrontIndex + size) % this->__capacity;
	if (nextFrontIndex <= this->__queueFrontIndex && nextFrontIndex != 0)
	{
		memcpy(buffer, this->__internalBuffer + this->__queueFrontIndex, size - nextFrontIndex);
		memcpy(buffer + (size - nextFrontIndex), this->__internalBuffer, nextFrontIndex);
	}
	else
	{
		memcpy(buffer, this->__internalBuffer + this->__queueFrontIndex, size);
	}

	this->__queueFrontIndex = nextFrontIndex;
	return size;
}

int RingBuffer::Peek(char* const buffer, int size)
{
	if (this->GetUseSize() < size || size < 1)
	{
		return 0;
	}

	int nextFrontIndex = (this->__queueFrontIndex + size) % this->__capacity;
	if (nextFrontIndex <= this->__queueFrontIndex && nextFrontIndex != 0)
	{
		memcpy(buffer, this->__internalBuffer + this->__queueFrontIndex, size - nextFrontIndex);
		memcpy(buffer + (size - nextFrontIndex), this->__internalBuffer, nextFrontIndex);
	}
	else
	{
		memcpy(buffer, this->__internalBuffer + this->__queueFrontIndex, size);
	}

	return size;
}

int RingBuffer::MoveRear(int size)
{
	if (this->GetFreeSize() < size || size < 1)
	{
		return 0;
	}
	this->__queueRearIndex = (this->__queueRearIndex + size) % this->__capacity;
	return size;
}

int RingBuffer::MoveFront(int size)
{
	if (this->GetUseSize() < size || size < 1)
	{
		return 0;
	}
	this->__queueFrontIndex = (this->__queueFrontIndex + size) % this->__capacity;
	return size;
}

void RingBuffer::ClearBuffer(void)
{
	this->__queueRearIndex = 0;
	this->__queueFrontIndex = 0;
}

char* RingBuffer::GetRearBufferPtr(void) const
{
	return this->__internalBuffer + this->__queueRearIndex;
}

char* RingBuffer::GetFrontBufferPtr(void) const
{
	return this->__internalBuffer + this->__queueFrontIndex;
}

char* RingBuffer::GetInternalBufferPtr(void) const
{
	return this->__internalBuffer;
}