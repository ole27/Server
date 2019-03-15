#ifndef __DATA_FTABLE_H__
#define __DATA_FTABLE_H__

#pragma once
////////////////////////////////////////////////////////////////////
/*
File Format:
	Header
	indexs
	col Formats
	fixed entrys 
	std::string table
*/
///////////////////////////////////////////////////////////////////

#include <stdio.h>


#include <map>
#include <string>

#include "def/TypeDef.h"


class CFTable
{
public:
	//struct Header
	//{
	//	uint8		FLAG[8];
	//	uint32		nVersion;
	//	uint32		nRows;
	//	uint32		nCols;
	//	uint32		nOffsetIndex;
	//	uint32		nOffsetFormat;
	//	uint32		nOffsetEntry;
	//	uint32		nEntrySize;
	//	uint32		nOffsetStrTable;
	//	uint32		nLengthStrTable;
	//};

public:
	typedef ServerMap< uint32, uint32 > IndexTable;

private:
	mutable FILE* m_file;
	FTableHeader m_header;
	uint32	m_nFileSize;

public:
	CFTable();
	~CFTable();
	bool OpenFile(const char* pszFileName);
	bool Close();
	bool IsFileOpen() const;
	
	const FTableHeader&	GetHeader() const { 
		return m_header;
	}
	const std::string&	GetFormat() const {
		return m_strFormat; 
	}
	//reader
protected:
	bool ReadHeader();
	bool ReadIndex();
	bool ReadFormat();
	
public:
	bool LoadEntryBySN(uint32 entrySN,char* buf) const;
	bool LoadEntryById(uint32 entryId,char* buf) const;
	bool ReadString(uint32 nOffset, uint16 nLen,std::string& strOut) const;
	
	//writer
private:
	//id,serial number
	IndexTable                  m_index;
	std::string					m_strFormat;
	uint32						m_iCurEntry;
	uint32						m_iCurString;
public:
	bool CreateFTable(const char* pszFileName, uint32 nRows, uint32 nCols, uint32 nEntrySize, const char* pszFormat);
	bool BeginWriteEntry();
	uint32 WriteEntry(uint32 nId, const char* pEntry);
	bool EndWriteEntry();
	uint32 WriteString(const char* pszString, uint32 nLen);
};

#endif // __DATA_FTABLE_H__
