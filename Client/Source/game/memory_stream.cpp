#include "pch.h"
#include "memory_stream.h"




MemoryStream::MemoryStream()
{
}

MemoryStream::~MemoryStream()
{
}

MemoryStream * MemoryStream::create(const void * buffer, size_t bufferSize)
{
    if (!buffer)
        return NULL;

    MemoryStream * res = new MemoryStream();
    res->_readBuffer = reinterpret_cast<const uint8_t *>(buffer);
    res->_writeBuffer = NULL;
    res->_bufferSize = bufferSize;
    res->_cursor = 0;

    return res;
}

MemoryStream * MemoryStream::create(void * buffer, size_t bufferSize)
{
    if (!buffer)
        return NULL;

    MemoryStream * res = new MemoryStream();
    res->_readBuffer = reinterpret_cast<const uint8_t *>(buffer);
    res->_writeBuffer = reinterpret_cast<uint8_t *>(buffer);
    res->_bufferSize = bufferSize;
    res->_cursor = 0;

    return res;
}

size_t MemoryStream::read(void* ptr, size_t size, size_t count)
{
    size_t maxReadBytes = std::min(_bufferSize - _cursor, size * count);
    size_t maxReadElements = maxReadBytes / size;

    if (maxReadElements == 0)
        return 0;

    maxReadBytes = maxReadElements * size;
    memcpy(ptr, _readBuffer + _cursor, maxReadBytes);
    _cursor += maxReadBytes;
    return maxReadElements;
}

char* MemoryStream::readLine(char* str, int num)
{
    if (num <= 0)
        return NULL;

    size_t maxReadBytes = std::min<size_t>(_bufferSize - _cursor, num);
    char * strSave = str;
    while (maxReadBytes > 0)
    {
        const char& ch = *(_readBuffer + _cursor);
        if (ch == '\r' || ch == '\n')
        {
            *str++ = ch;
            if (str - strSave < num)
                *str++ = '\0';
            return strSave;
        }

        *str++ = ch;
        _cursor++;
        maxReadBytes--;
    }

    return nullptr;
}

size_t MemoryStream::write(const void* ptr, size_t size, size_t count)
{
    if (!canWrite())
        return 0;

    size_t maxWriteBytes = std::min(_bufferSize - _cursor, size * count);
    size_t maxWriteElements = maxWriteBytes / size;

    if (maxWriteBytes == 0)
        return 0;

    maxWriteBytes = maxWriteElements * size;
    memcpy(_writeBuffer + _cursor, ptr, maxWriteBytes);
    _cursor += maxWriteBytes;
    return maxWriteElements;
}

bool MemoryStream::seek(long int offset, int origin)
{
    switch (origin)
    {
    case SEEK_SET:
        _cursor = static_cast<size_t>(offset);
        break;
    case SEEK_CUR:
        _cursor = static_cast<size_t>(position() + offset);
        break;
    case SEEK_END:
        _cursor = static_cast<size_t>(static_cast< long int >(_bufferSize) + offset);
        break;
    }

    return _cursor <= _bufferSize;
}