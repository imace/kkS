
#ifndef __LOG_H__
#define __LOG_H__

#include "Base.h"
#include "Assertx.h"

//////////////////////////////////////////////////////////////////////////
template<int32_t _BufSize, int32_t _BlockSize, class _Tag>
class LogSink
{
	enum 
	{
		LOGNAME_LEN=64,
		LOGTIME_LEN=64,
	};
public:
	explicit LogSink<_BufSize, _BlockSize, _Tag>(const char* fileName)
	{
		m_BufferedSize = 0;
		Init(fileName);
	}
	explicit LogSink<_BufSize, _BlockSize, _Tag>()
	{
		m_BufferedSize = 0;
		zeroMemory(m_LogFileName, LOGNAME_LEN);
		tsnprintf(m_LogFileName, LOGNAME_LEN, "%s", _Tag::Name());
		RebuildPath();
	}
	~ LogSink<_BufSize, _BlockSize, _Tag>(){ Flush(); }
public:
	void Init(const CHAR* fileName, int32_t param)
	{
		__ENTER_FUNCTION
		m_Parameter = param;
		zeroMemory(m_LogFileName, LOGNAME_LEN);
		tsnprintf(m_LogFileName, LOGNAME_LEN, "%s", fileName);
		RebuildPath();
		__LEAVE_FUNCTION
	}
	void Flush()
	{
		AutoLock_T lock(m_FileLock);
		_flushUnlock();
	}
	void RebuildPath()
	{
		AutoLock_T lock(m_FileLock);

		TIME64 now  = TimeUtil::Now();

		zeroMemory(m_LogPath, sizeof(m_LogPath));
		tsnprintf(m_LogFileCurName, LOGNAME_LEN, "%s.%04d-%02d-%02d-%02d.log",
			m_LogFileName, now.year+1900, now.mon, now.day, now.hour);

		tsnprintf(m_LogPath, _MAX_PATH, "./Runtime/Log/%s", m_LogFileCurName );
		m_LogFileName[_MAX_PATH-1] = 0;
	}
	void SaveLog(const CHAR* log, bool bFlush, LogColor color)
	{
		__ENTER_FUNCTION

			if( !log ) return ;

		AutoLock_T lock(m_FileLock);

		if( (m_BufferedSize + _BlockSize) >= _BufSize)
		{
			_flushUnlock();
		}

		TIME64 now  = TimeUtil::Now();
		char curTime[LOGTIME_LEN] = {0};
		tsnprintf(curTime, LOGTIME_LEN, "%04d-%02d-%02d %02d:%02d:%02d",
			now.year+1900, now.mon, now.day, now.hour, now.min, now.sec);

		char szBlock[_BlockSize] = {0};
		int32_t len = tsnprintf(szBlock, _BlockSize, "%s(%s)\n", log, curTime);
		if( len > 0 )
		{
			if(m_BufferedSize >= 0 && m_BufferedSize < _BufSize)
			{
				tsnprintf(m_LogBuffer+m_BufferedSize, _BufSize - m_BufferedSize, "%s", szBlock);
			}
			m_BufferedSize += len;

			if(g_Config.m_LogConfig.m_LogAssert2Stderr)
			{
				coloredWriteToStderr(szBlock, ::strlen(szBlock), color);
			}
		}

		if(bFlush) _flushUnlock();

		__LEAVE_FUNCTION
	}
private:
	void _flushUnlock()
	{
		__ENTER_FUNCTION_EX

			if(m_BufferedSize > 0)
			{
				//bfs::fstream fs(m_LogPath, std::ios_base::app);
				FILE* fp = fopen(m_LogPath, "a+");
				if( fp )
				{
					fwrite(m_LogBuffer, 1, strlen(m_LogBuffer), fp);
					fclose(fp);
				}
				else
				{
					char errorDesc[64] = {0};
#if defined(__WINDOWS__)
					strerror_s(errorDesc, 128, errno);
#elif defined(__LINUX__)
					strerror_r(errno, errorDesc, 128);

#endif
					char assertMsg[256] = {0};
					tsnprintf(assertMsg, 256,"open file %s is failed(%s).",
						m_LogFileCurName, errorDesc);
					AssertEx(0, errorDesc);
				}
				m_BufferedSize = 0;
				zeroMemory(m_LogBuffer, _BufSize);
			}

			__LEAVE_FUNCTION_EX
	}
private:
	MyLock m_FileLock;
	int32_t m_Parameter;
	int32_t m_BufferedSize;
	CHAR m_LogPath[_MAX_PATH];
	char m_LogFileName[LOGNAME_LEN];
	char m_LogFileCurName[LOGNAME_LEN];
	char m_LogBuffer[_BufSize];
};

//////////////////////////////////////////////////////////////////////////
#define LOG_INST(LOGTYPE)		g_LogSink_##LOGTYPE
#define LOG_IMPL(LOGTYPE)		LogSink<16*1024, 4*1024, Tag##LOGTYPE> LOG_INST(LOGTYPE);
#define LOG_FLUSH(LOGTYPE)		LOG_INST(LOGTYPE).Flush()
#define LOG_REBUILD(LOGTYPE)	LOG_INST(LOGTYPE).RebuildPath()
#define LOG_DECL(LOGTYPE)		class Tag##LOGTYPE{public: static const char* Name(){return #LOGTYPE;} };\
								extern  LogSink<16*1024, 4*1024, Tag##LOGTYPE> LOG_INST(LOGTYPE)

#define LOG_DEF(LOGTYPE)		class Tag##LOGTYPE{public: static const char* Name(){return #LOGTYPE;} };\
								LogSink<16*1024, 4*1024, Tag##LOGTYPE> LOG_INST(LOGTYPE)
//////////////////////////////////////////////////////////////////////////
template<typename _LogSink>
void _LogSinkPrint(_LogSink& rSink, LogColor color, /*const CHAR* keyVal,*/ const CHAR* func, int32_t line, const CHAR* fmt, ...)
{
	__ENTER_FUNCTION_EX
	{
		CHAR fmtBuffer[4096] = {0};
		int32_t writeBytes = tsnprintf(fmtBuffer, 4096,"(###)[%s][%d]",
			/*keyVal, */func, line);

		if(writeBytes > 0 && writeBytes < 4096)
		{
			va_list ap;
			va_start(ap,fmt);
			int32_t fmtBytes = (int32_t)vsnprintf(fmtBuffer+writeBytes, 4096 - writeBytes, fmt, ap);
			va_end(ap);

			if(fmtBytes > 0) writeBytes += fmtBytes;
		}

		if(writeBytes>0)
		{
			fmtBuffer[writeBytes%4096] = 0;
			rSink.SaveLog(fmtBuffer, false, color);
		}

	}
	__LEAVE_FUNCTION_EX
}


//////////////////////////////////////////////////////////////////////////
#define LOG_INFO(SINK, ...)		_LogSinkPrint(LOG_INST(SINK), COLOR_DEFAULT, __FUNCTION__, __LINE__, __VA_ARGS__)
#define LOG_DEBUG(SINK, ...)	_LogSinkPrint(LOG_INST(SINK), COLOR_GREEN,	 __FUNCTION__, __LINE__, __VA_ARGS__)
#define LOG_WARN(SINK, ...)		_LogSinkPrint(LOG_INST(SINK), COLOR_YELLOW,	 __FUNCTION__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(SINK, ...)	_LogSinkPrint(LOG_INST(SINK), COLOR_RED,	 __FUNCTION__, __LINE__, __VA_ARGS__)


#endif
