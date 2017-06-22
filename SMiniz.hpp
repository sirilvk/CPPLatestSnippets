/*
 * C++ Wrapper for Miniz compression library 
 *
 */

#pragma once
#include "miniz.h"
#include <algorithm>

#define BUF_SIZE (1048576)

typedef std::function<bool(const unsigned char* data, unsigned int length)> WriteFunc;

class SMiniz
{
private:
    int _compressionLevel = Z_BEST_COMPRESSION;
    z_stream _stream;
    unsigned char _outBuf[BUF_SIZE];
    bool _inCompression = false;
    WriteFunc _writeFunc = nullptr;
	
public:
    SMiniz(WriteFunc writeFunc):_writeFunc(writeFunc)
    {
	memset(&_stream, 0, sizeof(_stream));
	_stream.avail_out = BUF_SIZE;
	_stream.next_out = _outBuf;
	_stream.avail_in = 0;
    }

    ~SMiniz()
    {
	if (_inCompression)
	{
	    // ending the compression
	    try
	    {
		if (_writeFunc)
		    compressData(reinterpret_cast<const unsigned char*>(""), 0, Z_FINISH);
		endCompression();
	    }
	    catch (std::exception& ex)
	    {
		// can't throw in destructor .. so just log
		std::string errMsg = "Failed to finalize the deflate because of ";
		errMsg += ex.what();
		std::cout << errMsg << std::endl;
	    }
	}
	else
	{
	    // ending decompression.
	    try
	    {
		endDecompression();
	    }
	    catch (std::exception& ex)
	    {
		// can't throw in destructor .. so just log
		std::string errMsg = "Failed to finalize the deflate because of ";
		errMsg += ex.what();
		std::cout << errMsg << std::endl;
	    }
	}
    }

    void initiateCompression()
    {
	_inCompression = true;
	if (deflateInit(&_stream, _compressionLevel) != Z_OK)
	{
	    throw std::runtime_error("deflateInit() failed!");
	}
    }

    void initiateDecompression()
    {
	if (inflateInit(&_stream))
	{
	    throw std::runtime_error("inflateInit() failed!");
	}
    }

    void endCompression()
    {
	if (deflateEnd(&_stream) != Z_OK)
	{
	    throw std::runtime_error("deflateEnd() failed!");
	}
    }

    void endDecompression()
    {
	if (inflateEnd(&_stream) != Z_OK)
	{
	    throw std::runtime_error("inflateEnd() failed!\n");
	}
    }

    int writeDeflate(int flush)
    {
	int status = deflate(&_stream, flush);
	if ((status == Z_STREAM_END) || (!_stream.avail_out))
	{
	    unsigned int writenData = BUF_SIZE - _stream.avail_out;
	    _writeFunc(_outBuf, writenData);
	    _stream.next_out = _outBuf;
	    _stream.avail_out = BUF_SIZE;
	}
	else if (status != Z_OK)
	{
	    throw std::runtime_error("Deflate() failed with status [" + std::to_string(status) + "]");
	}
	return status;
    }

    int writeInflate(int flush)
    {
	int status = inflate(&_stream, flush);
	if ((status == Z_STREAM_END) || (!_stream.avail_out))
	{
	    unsigned int writenData = BUF_SIZE - _stream.avail_out;
	    _writeFunc(_outBuf, writenData);
	    _stream.next_out = _outBuf;
	    _stream.avail_out = BUF_SIZE;
	}
	else if (status != Z_OK)
	{
	    throw std::runtime_error("Inflate() failed with status [" + std::to_string(status) + "]");
	}
	return status;
    }

    void compressData(const unsigned char* srcData, unsigned int length, int flush = Z_NO_FLUSH)
    {
	if (flush == Z_FINISH)
	{
	    _stream.next_in = reinterpret_cast<const unsigned char*>("");
	    _stream.avail_in = 0;
	    int status = Z_OK;
	    do
	    {
		// flush everything out
		status = writeDeflate(flush);
	    }while(status != Z_STREAM_END);
	}
	else
	{
	    unsigned int index = 0;
	    while(_stream.avail_in || (index < length))
	    {
		if (!_stream.avail_in)
		{
		    // now read data
		    if ((length - index) > BUF_SIZE)
		    {
			_stream.next_in = srcData + index;
			index += BUF_SIZE;
			_stream.avail_in = BUF_SIZE;
		    }
		    else
		    {
			_stream.next_in = srcData + index;
			_stream.avail_in = (length - index);
			index += (length - index);
		    }
		}

		writeDeflate(flush);
	    }
	}
    }

    void printStat(int bufLength, unsigned int index, unsigned int length, int flush)
    {
	std::cout << ((flush == Z_FINISH) ? "Finish:" : "Buffered:") << bufLength << ":" << index << ":" << length << ":" << _stream.avail_in << ":" <<_stream.next_in << std::endl;
    }


    void decompressData(const unsigned char* srcData, unsigned int length)
    {
	unsigned int index = 0;
	int status = Z_OK;
	while((status != Z_STREAM_END) || (_stream.avail_in || (index < length)))
	{
	    if (!_stream.avail_in)
	    {
		if (index < length)
		{
		    // now read data if available
		    if ((length - index) > BUF_SIZE)
		    {
			_stream.next_in = srcData + index;
			index += BUF_SIZE;
			_stream.avail_in = BUF_SIZE;
		    }
		    else
		    {
			_stream.next_in = srcData + index;
			_stream.avail_in = (length - index);
			index += (length - index);
		    }
		}
	    }

	    status = writeInflate(Z_SYNC_FLUSH);
	}
    }
};
