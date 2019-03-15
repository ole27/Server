#pragma once

#include <vector>
#include <string>

#include "def/TypeDef.h"
#include "memory_buffer/NetPack.h"

#include "ConsoleLog.h"
#include "OS.h"


class NetPackStatistician
{
	struct NetPackStat
	{
		NetPackStat():Count(0),Bytes(0){}

		int64	Count;
		int64	Bytes;
	};

	typedef std::vector<NetPackStat> NetPackStatArray;

public:
	NetPackStatistician(int maxOpcode, const std::string& tag);
	virtual ~NetPackStatistician( void ) ;

	void Add(const NetPack& pack);
	void Add(const ByteBuffer& buf);

	virtual const char* GetOpCodeName( int opCode ) =0 ;

private:
	void Add( uint32 opCode, int packSize );
	void TryPrintLog();
	void PrintLog( bool isClean =false );
	void PrintStatLog( const char* opName, const NetPackStat& stat, int64 secs );

private:
	std::string         m_tag;
	uint64              m_tick;
	uint64              m_lastPrintTime;
	NetPackStatArray    m_array;
	NetPackStat         m_statAll;

public:
	uint64 GetPrintDiffCount( void ) {    return m_PrintDiffCount;    }
	void SetPrintDiffCount( uint64 count ) {    m_PrintDiffCount =count;    }

private:
	uint64              m_PrintDiffCount ;
};



