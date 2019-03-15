
#include "NetPackStatistician.h"

NetPackStatistician::NetPackStatistician( int maxOpcode, const std::string& tag )
	:m_tag(tag)
	,m_tick(0)
{
	m_PrintDiffCount =20000 ;

	m_array.resize(maxOpcode);
	m_lastPrintTime = sOS.GetRealTime() ;
}


	
NetPackStatistician::~NetPackStatistician( void )
{
}

void NetPackStatistician::Add( const NetPack& pack )
{
	Add( pack.GetOpcode(), pack.DynamicTotalSize() );
}



void NetPackStatistician::Add( const ByteBuffer& bufIn )
{
	ByteBuffer& buf = const_cast<ByteBuffer&>(bufIn);
	size_t old_rpos =  buf.rpos();

	while (buf.size() - buf.rpos() >= 4 )
	{
		size_t packSize = buf.read<uint16>();
		size_t opCode = buf.read<uint16>();
		size_t rpos = buf.rpos() +packSize;

		if (opCode >= m_array.size() || rpos > buf.size())
		{
			// It's a HACKER
			break;
		}

		Add(opCode, packSize + NetPack::GetHeaderSize() );

		buf.rpos(rpos);
	}

	buf.rpos(old_rpos);
}



void NetPackStatistician::Add( uint32 opCode, int packSize )
{
	if (opCode >= m_array.size())
	{
		// Hacker
		return;
	}

	NetPackStat& stat = m_array[opCode];
	++stat.Count ;
	stat.Bytes += packSize;

	++m_statAll.Count ;
	m_statAll.Bytes += packSize;
	if( m_statAll.Bytes <= 0 )
	{
		m_statAll.Bytes -= packSize ;

		WLOG( "Packet status count is full will clean after print !" ) ;
		// Print and clear ;
		m_tick = 0;
		PrintLog( true );
	}

	TryPrintLog();
}




void NetPackStatistician::TryPrintLog()
{
	m_tick ++;
	if (m_tick < GetPrintDiffCount() )
	{
		return;
	}

	m_tick = 0;
	PrintLog();
}


void NetPackStatistician::PrintLog( bool isClean /*=false*/ )
{
	int64 passedTime = sOS.GetRealTime() - m_lastPrintTime;

	NLOG("========================================================================");
	NLOG("Net Pack Statistics : %s", m_tag.c_str());
	NLOG("%30s %23s %21s %12s %7s %5s",
		"Opcode", "All-Packs(%%)", "All-KB(%%)", "Bytes/p", "Packs/S", "KB/S");

	for (size_t opCode =0; opCode <m_array.size(); ++opCode )
	{
		NetPackStat& stat = m_array[opCode];
		if (stat.Count == 0)
		{
			continue;
		}

		const char * opName = GetOpCodeName( opCode ) ;
		PrintStatLog(opName, stat, passedTime);

		if( isClean )
		{
			stat.Count =stat.Bytes =0 ;
		}
	}

	PrintStatLog("ALL", m_statAll, passedTime);
	NLOG("========================================================================");

	if( isClean )
	{
		m_statAll.Count =m_statAll.Bytes =0 ;
	}
}


void NetPackStatistician::PrintStatLog( const char* opName, const NetPackStat& stat, int64 secs )
{
	NLOG("%30s %20lld(%6.2f) %10.2f(%6.2f) %8lld %7.2f %5.2f"
		,opName
		,stat.Count
		,(double)stat.Count*100.0/(double)m_statAll.Count
		,(double)stat.Bytes/1024.0
		,(double)stat.Bytes*100.0/(double)m_statAll.Bytes
		,stat.Bytes/stat.Count
		,(double)stat.Count/(double)secs
		,(double)stat.Bytes/(double)secs/1024.0
	);
}
