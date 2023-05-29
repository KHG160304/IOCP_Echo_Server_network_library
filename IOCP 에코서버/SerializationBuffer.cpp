#include "SerializationBuffer.h"

SerializationBuffer::SerializationBuffer()
	: __internalBuffer(new char[SerializationBuffer::eCapacity::DEFAULT])
	, __capacity(SerializationBuffer::eCapacity::DEFAULT)
	, __useSize(0)
	, __freeSize(SerializationBuffer::eCapacity::DEFAULT)
	, __queueFrontIndex(0)
	, __queueRearIndex(0)
{
}

SerializationBuffer::SerializationBuffer(int capacity)
	: __internalBuffer(new char[capacity])
	, __capacity(capacity)
	, __useSize(0)
	, __freeSize(capacity)
	, __queueFrontIndex(0)
	, __queueRearIndex(0)
{
}

SerializationBuffer::~SerializationBuffer()
{
	delete[] this->__internalBuffer;
}

int SerializationBuffer::GetBufferSize()
{
	return this->__capacity;
}

int SerializationBuffer::GetUseSize()
{
	return this->__useSize;
}

int SerializationBuffer::GetFreeSize()
{
	return this->__freeSize;
}

int SerializationBuffer::Enqueue(const char* buffer, int size)
{
	if (this->__freeSize < size || size < 1)
	{
		return 0;
	}

	memcpy(this->__internalBuffer + this->__queueRearIndex, buffer, size);

	this->__queueRearIndex += size;
	this->__useSize += size;
	this->__freeSize -= size;

	return size;
}

int SerializationBuffer::Dequeue(char* const buffer, int size)
{
	if (this->__useSize < size || size < 1)
	{
		return 0;
	}

	memcpy(buffer, this->__internalBuffer + this->__queueFrontIndex, size);

	this->__queueFrontIndex += size;
	this->__useSize -= size;
	return size;
}

int SerializationBuffer::MoveRear(int size)
{
	if (this->__freeSize < size || size < 1)
	{
		return 0;
	}

	this->__queueRearIndex += size;
	this->__useSize += size;
	this->__freeSize -= size;

	return size;
}

int SerializationBuffer::MoveFront(int size)
{
	if (this->__useSize < size || size < 1)
	{
		return 0;
	}

	this->__queueFrontIndex += size;
	this->__useSize -= size;
	return size;
}

void SerializationBuffer::ClearBuffer(void)
{
	this->__queueFrontIndex = 0;
	this->__queueRearIndex = 0;
	this->__useSize = 0;
	this->__freeSize = this->__capacity;
}

char* SerializationBuffer::GetRearBufferPtr(void) const
{
	return this->__internalBuffer + this->__queueRearIndex;
}

char* SerializationBuffer::GetFrontBufferPtr(void) const
{
	return this->__internalBuffer + this->__queueFrontIndex;
}

SerializationBuffer& SerializationBuffer::operator<<(unsigned char data)
{
	if (this->__freeSize < sizeof(data))
	{
		return *this;
	}

	*((unsigned char*)(this->__internalBuffer + this->__queueRearIndex)) = data;
	this->__queueRearIndex += sizeof(data);
	this->__useSize += sizeof(data);
	this->__freeSize -= sizeof(data);
	return *this;
}
SerializationBuffer& SerializationBuffer::operator<<(char data)
{
	if (this->__freeSize < sizeof(data))
	{
		return *this;
	}

	*((char*)(this->__internalBuffer + this->__queueRearIndex)) = data;
	this->__queueRearIndex += sizeof(data);
	this->__useSize += sizeof(data);
	this->__freeSize -= sizeof(data);
	return *this;
}

SerializationBuffer& SerializationBuffer::operator<<(unsigned short data)
{
	if (this->__freeSize < sizeof(data))
	{
		return *this;
	}

	*((unsigned short*)(this->__internalBuffer + this->__queueRearIndex)) = data;
	this->__queueRearIndex += sizeof(data);
	this->__useSize += sizeof(data);
	this->__freeSize -= sizeof(data);
	return *this;
}

SerializationBuffer& SerializationBuffer::operator<<(short data)
{
	if (this->__freeSize < sizeof(data))
	{
		return *this;
	}

	*((short*)(this->__internalBuffer + this->__queueRearIndex)) = data;
	this->__queueRearIndex += sizeof(data);
	this->__useSize += sizeof(data);
	this->__freeSize -= sizeof(data);
	return *this;
}


SerializationBuffer& SerializationBuffer::operator<<(unsigned int data)
{
	if (this->__freeSize < sizeof(data))
	{
		return *this;
	}

	*((unsigned int*)(this->__internalBuffer + this->__queueRearIndex)) = data;
	this->__queueRearIndex += sizeof(data);
	this->__useSize += sizeof(data);
	this->__freeSize -= sizeof(data);
	return *this;
}

SerializationBuffer& SerializationBuffer::operator<<(int data)
{
	if (this->__freeSize < sizeof(data))
	{
		return *this;
	}

	*((int*)(this->__internalBuffer + this->__queueRearIndex)) = data;
	this->__queueRearIndex += sizeof(data);
	this->__useSize += sizeof(data);
	this->__freeSize -= sizeof(data);
	return *this;
}

SerializationBuffer& SerializationBuffer::operator<<(unsigned long data)
{
	if (this->__freeSize < sizeof(data))
	{
		return *this;
	}

	*((unsigned long*)(this->__internalBuffer + this->__queueRearIndex)) = data;
	this->__queueRearIndex += sizeof(data);
	this->__useSize += sizeof(data);
	this->__freeSize -= sizeof(data);
	return *this;
}

SerializationBuffer& SerializationBuffer::operator<<(long data)
{
	if (this->__freeSize < sizeof(data))
	{
		return *this;
	}

	*((long*)(this->__internalBuffer + this->__queueRearIndex)) = data;
	this->__queueRearIndex += sizeof(data);
	this->__useSize += sizeof(data);
	this->__freeSize -= sizeof(data);
	return *this;
}

SerializationBuffer& SerializationBuffer::operator<<(unsigned __int64 data)
{
	if (this->__freeSize < sizeof(data))
	{
		return *this;
	}

	*((unsigned __int64*)(this->__internalBuffer + this->__queueRearIndex)) = data;
	this->__queueRearIndex += sizeof(data);
	this->__useSize += sizeof(data);
	this->__freeSize -= sizeof(data);
	return *this;
}

SerializationBuffer& SerializationBuffer::operator<<(__int64 data)
{
	if (this->__freeSize < sizeof(data))
	{
		return *this;
	}

	*((__int64*)(this->__internalBuffer + this->__queueRearIndex)) = data;
	this->__queueRearIndex += sizeof(data);
	this->__useSize += sizeof(data);
	this->__freeSize -= sizeof(data);
	return *this;
}

SerializationBuffer& SerializationBuffer::operator<<(float data)
{
	if (this->__freeSize < sizeof(data))
	{
		return *this;
	}

	*((float*)(this->__internalBuffer + this->__queueRearIndex)) = data;
	this->__queueRearIndex += sizeof(data);
	this->__useSize += sizeof(data);
	this->__freeSize -= sizeof(data);
	return *this;
}

SerializationBuffer& SerializationBuffer::operator<<(double data)
{
	if (this->__freeSize < sizeof(data))
	{
		return *this;
	}

	*((double*)(this->__internalBuffer + this->__queueRearIndex)) = data;
	this->__queueRearIndex += sizeof(data);
	this->__useSize += sizeof(data);
	this->__freeSize -= sizeof(data);
	return *this;
}

SerializationBuffer& SerializationBuffer::operator<<(long double data)
{
	if (this->__freeSize < sizeof(data))
	{
		return *this;
	}

	*((long double*)(this->__internalBuffer + this->__queueRearIndex)) = data;
	this->__queueRearIndex += sizeof(data);
	this->__useSize += sizeof(data);
	this->__freeSize -= sizeof(data);
	return *this;
}

//---------------------------

SerializationBuffer& SerializationBuffer::operator>>(unsigned char& data)
{
	if (this->__useSize < sizeof(data))
	{
		return *this;
	}

	data = *((unsigned char*)(this->__internalBuffer + this->__queueFrontIndex));
	this->__queueFrontIndex += sizeof(data);
	this->__useSize -= sizeof(data);
	return *this;
}
SerializationBuffer& SerializationBuffer::operator>>(char& data)
{
	if (this->__useSize < sizeof(data))
	{
		return *this;
	}

	data = *((char*)(this->__internalBuffer + this->__queueFrontIndex));
	this->__queueFrontIndex += sizeof(data);
	this->__useSize -= sizeof(data);
	return *this;
}

SerializationBuffer& SerializationBuffer::operator>>(unsigned short& data)
{
	if (this->__useSize < sizeof(data))
	{
		return *this;
	}

	data = *((unsigned short*)(this->__internalBuffer + this->__queueFrontIndex));
	this->__queueFrontIndex += sizeof(data);
	this->__useSize -= sizeof(data);
	return *this;
}

SerializationBuffer& SerializationBuffer::operator>>(short& data)
{
	if (this->__useSize < sizeof(data))
	{
		return *this;
	}

	data = *((short*)(this->__internalBuffer + this->__queueFrontIndex));
	this->__queueFrontIndex += sizeof(data);
	this->__useSize -= sizeof(data);
	return *this;
}


SerializationBuffer& SerializationBuffer::operator>>(unsigned int& data)
{
	if (this->__useSize < sizeof(data))
	{
		return *this;
	}

	data = *((unsigned int*)(this->__internalBuffer + this->__queueFrontIndex));
	this->__queueFrontIndex += sizeof(data);
	this->__useSize -= sizeof(data);
	return *this;
}

SerializationBuffer& SerializationBuffer::operator>>(int& data)
{
	if (this->__useSize < sizeof(data))
	{
		return *this;
	}

	data = *((int*)(this->__internalBuffer + this->__queueFrontIndex));
	this->__queueFrontIndex += sizeof(data);
	this->__useSize -= sizeof(data);
	return *this;
}

SerializationBuffer& SerializationBuffer::operator>>(unsigned long& data)
{
	if (this->__useSize < sizeof(data))
	{
		return *this;
	}

	data = *((unsigned long*)(this->__internalBuffer + this->__queueFrontIndex));
	this->__queueFrontIndex += sizeof(data);
	this->__useSize -= sizeof(data);
	return *this;
}

SerializationBuffer& SerializationBuffer::operator>>(long& data)
{
	if (this->__useSize < sizeof(data))
	{
		return *this;
	}

	data = *((long*)(this->__internalBuffer + this->__queueFrontIndex));
	this->__queueFrontIndex += sizeof(data);
	this->__useSize -= sizeof(data);
	return *this;
}

SerializationBuffer& SerializationBuffer::operator>>(unsigned __int64& data)
{
	if (this->__useSize < sizeof(data))
	{
		return *this;
	}

	data = *((unsigned __int64*)(this->__internalBuffer + this->__queueFrontIndex));
	this->__queueFrontIndex += sizeof(data);
	this->__useSize -= sizeof(data);
	return *this;
}

SerializationBuffer& SerializationBuffer::operator>>(__int64& data)
{
	if (this->__useSize < sizeof(data))
	{
		return *this;
	}

	data = *((__int64*)(this->__internalBuffer + this->__queueFrontIndex));
	this->__queueFrontIndex += sizeof(data);
	this->__useSize -= sizeof(data);
	return *this;
}

SerializationBuffer& SerializationBuffer::operator>>(float& data)
{
	if (this->__useSize < sizeof(data))
	{
		return *this;
	}

	data = *((float*)(this->__internalBuffer + this->__queueFrontIndex));
	this->__queueFrontIndex += sizeof(data);
	this->__useSize -= sizeof(data);
	return *this;
}

SerializationBuffer& SerializationBuffer::operator>>(double& data)
{
	if (this->__useSize < sizeof(data))
	{
		return *this;
	}

	data = *((double*)(this->__internalBuffer + this->__queueFrontIndex));
	this->__queueFrontIndex += sizeof(data);
	this->__useSize -= sizeof(data);
	return *this;
}

SerializationBuffer& SerializationBuffer::operator>>(long double& data)
{
	if (this->__useSize < sizeof(data))
	{
		return *this;
	}

	data = *((long double*)(this->__internalBuffer + this->__queueFrontIndex));
	this->__queueFrontIndex += sizeof(data);
	this->__useSize -= sizeof(data);
	return *this;
}
