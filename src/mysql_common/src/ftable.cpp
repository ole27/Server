#include "ftable.h"
#include <vector>




CFTable::CFTable()
{
	m_file = NULL;
	memset( (void*)&m_header,0,sizeof(m_header) );
}

CFTable::~CFTable()
{

}

bool CFTable::OpenFile(const char* pszFileName)
{
	m_file = fopen(pszFileName, "rb");
	if ( !m_file )
	{
		return false;
	}
	bool isOk = ReadHeader();
	if ( !isOk )
	{
		return false;
	}
	
	fseek(m_file,0,SEEK_END);
	m_nFileSize = ftell(m_file);

	isOk = ReadFormat();
	if ( !isOk )
	{
		return false;
	}

	if( m_header.nRows == 0 )
		return true;

	isOk = ReadIndex();
	if ( !isOk )
	{
		return false;
	}
	return true;
}

bool CFTable::IsFileOpen() const
{
	return (m_file != NULL);
}

bool CFTable::Close()
{
	if ( m_file )
	{
		fclose( m_file );
	}
	//memset( (void*)&m_header,0,sizeof(m_header) );
	m_file = NULL;
	return true;
}

bool CFTable::ReadHeader()
{
	fseek(m_file,0,SEEK_SET);
	int nLen = fread(&m_header,sizeof(m_header),1,m_file);
	if ( nLen <= 0 )
	{
		return false;
	}
	return true;
}

bool CFTable::ReadIndex()
{
	std::vector<uint32> buf(m_header.nRows,0);
	fseek(m_file,m_header.nOffsetIndex,SEEK_SET);
	fread(&buf[0],sizeof(uint32),m_header.nRows,m_file);

	for(uint32 iIdx = 0; iIdx < m_header.nRows; iIdx++ )
	{
		m_index.insert( std::make_pair( buf[iIdx],iIdx ) );
	}
	return true;
}

bool CFTable::ReadFormat()
{
	m_strFormat.resize( m_header.nCols + 1);
	fseek(m_file,m_header.nOffsetFormat,SEEK_SET);
	fread(&m_strFormat[0],sizeof(char),m_header.nCols+1,m_file);
	return true;
}

bool CFTable::LoadEntryBySN(uint32 entrySN,char* buf)  const
{
	uint32 nOffset = m_header.nOffsetEntry + entrySN * m_header.nEntrySize;
	
	fseek(m_file,nOffset,SEEK_SET);
	fread(buf,m_header.nEntrySize,1,m_file);
	return true;
}

bool CFTable::LoadEntryById(uint32 entryIdx,char* buf) const
{
	IndexTable::const_iterator iter = m_index.find(entryIdx);
	if ( iter == m_index.end() )
	{
		return false;
	}

	uint32 nEntrySerialNumber = iter->second;
	return LoadEntryBySN( nEntrySerialNumber,buf );
}


bool CFTable::ReadString(uint32 nOffset, uint16 nLen,std::string& strOut) const
{
	if ( nOffset+nLen > m_nFileSize )
		return false;

	if ( nLen == 0 ) return true;

	strOut.resize(nLen);
	fseek(m_file,nOffset  ,SEEK_SET);
	fread(&strOut[0],nLen,1,m_file);	
	return true;
}

bool CFTable::CreateFTable(const char* pszFileName, uint32 nRows, uint32 nCols, uint32 nEntrySize, const char* pszFormat)
{
	Close();
	m_file = fopen(pszFileName,"wb");
	if ( !m_file )
		return false;

	uint32 offset = 0;
	FTableHeader& header = m_header;
	strcpy( (char*)header.FLAG,TABLE_STR_FLAG);
	header.nVersion			= TABLE_VERSION;
	header.nRows			= nRows;
	header.nCols			= nCols;

	offset += sizeof(header);
	header.nOffsetIndex		= offset;

	offset += sizeof(uint32) * nRows;
	header.nOffsetFormat	= offset;

	offset += sizeof(char) * nCols + 1;
	header.nOffsetEntry		= offset;
	header.nEntrySize		= nEntrySize;

	offset += nEntrySize * nRows;
	header.nOffsetStrTable	= offset;
	
	fwrite(&header,sizeof(header),1,m_file );

	fseek(m_file,header.nOffsetFormat,SEEK_SET);
	fwrite(pszFormat,sizeof(char),nCols+1,m_file);
	return true;
}

bool CFTable::BeginWriteEntry()
{
	m_index.clear();
	m_iCurEntry		= 0;
	m_iCurString	= m_header.nOffsetStrTable;
	return true;
}

uint32 CFTable::WriteEntry(uint32 nId, const char* pEntry)
{
	uint32 nEntryOffset = m_header.nOffsetEntry+ m_iCurEntry * m_header.nEntrySize;
	fseek(m_file,nEntryOffset,SEEK_SET);
	fwrite(pEntry, m_header.nEntrySize, 1, m_file);
	m_index.insert( std::make_pair(nId,m_iCurEntry) );
	m_iCurEntry++;
	return m_iCurEntry;
}

bool CFTable::EndWriteEntry()
{
	uint32 nEntryCount = m_iCurEntry;
	std::vector< uint32 > buf(nEntryCount,0);

	IndexTable::iterator iter = m_index.begin();
	while ( iter != m_index.end() )
	{
		if ( iter->second < nEntryCount ) 
		{
			buf[iter->second] = iter->first;
		}
		else{
			return false;
		}
		iter++;
	}
	fflush(m_file);
	if ( !m_index.empty() )
	{
		fseek(m_file, m_header.nOffsetIndex, SEEK_SET);
		fwrite( &buf[0], sizeof(uint32)*nEntryCount, 1, m_file );
	}
	
	m_index.clear();
	m_iCurEntry = 0;
	return true;
}

uint32 CFTable::WriteString(const char* pszString, uint32 nLen)
{
	if ( !m_file )
		return 0;

	fseek(m_file, m_iCurString, SEEK_SET);
	fwrite(pszString, sizeof(char), nLen+1, m_file);
	uint32 offset = m_iCurString ;
	m_iCurString += (nLen + 1);
	return offset;
}

