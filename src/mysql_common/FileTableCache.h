#ifndef __DATA_FILETABLECACHE_H__
#define __DATA_FILETABLECACHE_H__

#include <map>
#include <vector>
#include <streambuf>

#include "TableCache.h"
#include "ftable/ftable.h"

#include "utility/Cast.h"
#include "def/TypeDef.h"
#include "memory_buffer/ByteBuffer.h"


extern bool s_IsCacheFileTable;

template<class T>
class FileTableCache: public CTableCache< T >
{
protected:
	CFTable  m_file;

public:
	typedef CTableCache< T >      CacheBase;
	typedef typename CacheBase::K K ;

public:
	FileTableCache()
	{
	}

	virtual ~FileTableCache()
	{
	}

	void SetCacheData(bool bCache)
	{
	}

	bool FileClose()
	{
		if( !m_file.IsFileOpen() )
			return false;

		return m_file.Close();
	}

	virtual bool FileLoadAndAddData(const char * szSource = NULL, std::vector<T*> *pAddVec =NULL, LOAD_FLAG flagLoad = FlagNone,MULTILANG curLanguage = LANG_EN)
	{
		//MutexGuard lock((this->m_mutex));

		if( this->GetNumRows() > 0 && !(flagLoad&FlagReload) )
		{
			return true;
		}

		if( flagLoad & FlagCleanup )
		{
			this->Cleanup();
		}

		std::string strName;
		if( szSource == NULL || strlen(szSource) == 0 )
		{
			strName = "tables/";
			const char * tableName = T::GetTableName();

			strName += tableName;

			strName += ".bytes";

		}else
		{
			strName = szSource;
		}

		bool isOk = m_file.OpenFile( strName.c_str() ) ;
		if ( !isOk )
		{
			char temp[128];
			memset(temp,0,128);
			sprintf(temp,"Can't open file! %s",strName.c_str());
			MASSERT( false, temp );
			return false;
		}

		const FTableHeader& header = m_file.GetHeader();
		
		const char* pszFormat = m_file.GetFormat().c_str();
		if( strcmp( pszFormat, T::GetFormat()) != 0 )
		{
			//table format doesn't same;
			m_file.Close();
			return false;
		}

		if ( strlen(pszFormat) != header.nCols )
		{
			//table format doesn't match the format string
			m_file.Close();
			return false;
		}

		FileTableCache::m_nCols = header.nCols;

		ByteBuffer buf;
		for ( uint32 iEntry = 0; iEntry <header.nRows; iEntry++)
		{
			buf.resize( header.nEntrySize );
			m_file.LoadEntryBySN(iEntry,(char*)buf.buffer());
			T entry;
			BufferToEntry( buf, &entry );
			CTableCache< T >::AddEntry(&entry);
			FileTableCache::m_nRows++;
		}

		if ( FileTableCache::m_nRows != header.nRows )
		{
			assert(false);
			m_file.Close();
			return false;
		}

		m_file.Close();

		FileTableCache<T>::PrepareData(0);

		return true;
	};

	virtual bool DumpToFile(const char* szSource = NULL )
	{
		std::string strName;
		if( szSource == NULL || strlen(szSource) == 0)
		{
			strName = "tables/";
			strName += T::GetTableName();
			strName += ".bytes";
		}else{
			strName = szSource;
		}

		printf ("%s\n", strName.c_str());

		if ( !T::HasIndex() )
		{
			return _dumpUnindexed(strName);
		}
		else
		{
			return _dumpIndexed(strName);
		}
	}

	//!untest
	//virtual bool ReadEntry( typename T::IdType &id, T* pEntry ) const
	//{
	//	ByteBuffer buf;
	//	const FTableHeader& header = m_file.GetHeader();
	//	if ( !header.nEntrySize )
	//	{
	//		return false;
	//	}

	//	buf.resize( header.nEntrySize );
	//	bool isFound = m_file.LoadEntryById(id,(char*)buf.buffer());;//m_file.LoadEntryBySN(nEntryID,(char*)buf.buffer());
	//	if ( !isFound )
	//	{
	//		return false;
	//	}

	//	BufferToEntry( buf, pEntry );
	//	return true;
	//}

	virtual bool ReadEntryBySN( uint32 nEntrySN, T* pEntry ) const
	{
		ByteBuffer buf;
		const FTableHeader& header = m_file.GetHeader();
		if ( !header.nEntrySize )
		{
			return false;
		}

		buf.resize( header.nEntrySize );
		bool isFound = m_file.LoadEntryBySN(nEntrySN,(char*)buf.buffer());
		if ( !isFound )
		{
			return false;
		}
		BufferToEntry( buf, pEntry );
		return true;
	}

	virtual uint64 WriteEntry(uint32 nEntryID, const T* pEntry)
	{
		assert(false &&".TBL can't write a single entry");
		return 0;
	}

	//interface 
protected:

	bool _dumpIndexed(const std::string& strName)
	{
		if( m_file.IsFileOpen() )
		{
			m_file.Close();
		}

		int strFind = strName.find("String_") ;
		bool convertString = strFind >= 0;
		
		char outPutFileNameRoot[256];
		strcpy(outPutFileNameRoot, strName.c_str());
		if(convertString)
		{
			char * p =strchr(outPutFileNameRoot, '.');
			if(p)
				*p = 0;
		}
		
		char outPutFileName[256];

		uint32 nEntrySize = 0;
		ByteBuffer buf;
		if ( !FileTableCache::m_mapTable.empty() )
		{
			EntryToBuffer( &FileTableCache::m_mapTable.begin()->second, buf, convertString, 1);
			nEntrySize = buf.size();
		}

		int loopTimes = convertString ? LANG_MAX : 1;

		for(int i=0; i<loopTimes; i++)
		{

			bool isOk = false;
			if(convertString)
			{
				sprintf(outPutFileName, "%s_%s.bytes", outPutFileNameRoot, s_LangISO6391Str[i]);
				isOk = m_file.CreateFTable(outPutFileName, FileTableCache::m_nRows, 2, nEntrySize, "us");
			}
			else
				isOk = m_file.CreateFTable(strName.c_str(), FileTableCache::m_nRows, FileTableCache::m_nCols, nEntrySize, T::GetFormat() );

			assert(isOk);
			
			buf.clear();
			m_file.BeginWriteEntry();
			typename CacheBase::MapItr iter = FileTableCache::m_mapTable.begin();
			while ( iter != FileTableCache::m_mapTable.end())
			{
				EntryToBuffer( &iter->second,buf,convertString, i+1);
				ASSERT( false ) ;
				//m_file.WriteEntry( iter->second.GetKey(),(const char*)buf.contents());	
				buf.clear();
				iter++;
			}
			m_file.EndWriteEntry();
			m_file.Close();
		}
		return true;
	}

	bool _dumpUnindexed(const std::string& strName)
	{	
	/*	if ( FileTableCache::m_vecTable.empty() )
		{
			return false;
		}*/
		if( m_file.IsFileOpen() )
		{
			m_file.Close();
		}
		int strFind = strName.find("String_") ;
		bool convertString = strFind >= 0;

		ByteBuffer buf;
		uint32 nEntrySize = 0;
		if ( !FileTableCache::m_vecTable.empty() )
		{
			EntryToBuffer( &FileTableCache::m_vecTable[0],buf,convertString);
			nEntrySize = buf.size();
		}
		
		bool isOk = m_file.CreateFTable(strName.c_str(), FileTableCache::m_nRows, FileTableCache::m_nCols, nEntrySize, T::GetFormat() );
		assert(isOk);
		
		buf.clear();
		m_file.BeginWriteEntry();
		uint32 nCount =  FileTableCache::m_vecTable.size();
		for ( uint32 iEntry = 0; iEntry < nCount; iEntry++ )
		{
			EntryToBuffer( &FileTableCache::m_vecTable[iEntry],buf,convertString);
			ASSERT( false ) ;
			//m_file.WriteEntry( FileTableCache::m_vecTable[iEntry].GetKey(),(const char*)buf.contents());	
			buf.clear();
		}
		m_file.EndWriteEntry();
		m_file.Close();
		return true;
	}

	bool BufferToEntry(ByteBuffer& Buff, T* pEntry) const
	{
		const char * pFormat =  T::GetFormat();
		char * structpointer = (char*)pEntry;
		//uint32 offset = 0;
		for(; *pFormat != 0; ++pFormat)
		{
			switch(*pFormat)
			{
			case 'u':	// Unsigned integer
				Buff>>(*(uint32*)structpointer);
				structpointer += sizeof(uint32);
				break;

			case 'i':	// Signed integer
				Buff>>(*(int32*)structpointer);
				structpointer += sizeof(int32);				
				break;

			case 'l':	// Signed integer 64
				Buff>>(*(int64*)structpointer);
				structpointer += sizeof(int64);				
				break;

			case 'b':	// Unsigned integer 64
				Buff>>(*(uint64*)structpointer);
				structpointer += sizeof(uint64);				
				break;

			case 'f':	// Float
				Buff>>(*(int32*)structpointer);
				structpointer += sizeof(float);		
				break;

			case 'c':	// Char
				Buff>>(*(char*)structpointer);
				structpointer += sizeof(char);		
				break;

			case 'h':	// Short
				Buff>>(*(short*)structpointer);
				structpointer += sizeof(short);		
				break;
			case 's':	// std::string
				uint32	nOffset;
				short	nLen;
				Buff>>nOffset>>nLen;
				m_file.ReadString(nOffset,nLen,*(std::string*)structpointer);
				structpointer += sizeof(std::string);
				break;
			case 't':
				Buff>>(*(int64*)structpointer);
				structpointer += sizeof(int64);
				break;

            case 'x':	// Skip
                //MASSERT(false,"No Skip In Develop Step");
                break;

			default:	// unknown
				printf("%s:unknown field type in string: %d\n", T::GetTableName(), *pFormat);
				break;
			}
		}
		return true;
	}

	bool EntryToBuffer( const T* pEntry, ByteBuffer& buf, bool convertString = false, int stringIndex = 1)
	{
		const char * pFormat = T::GetFormat();
		char * structpointer = (char*)pEntry;
		uint32 offset = 0;
		int index = 0;
		std::string* strEN =NULL;
		for(; *pFormat != 0; ++pFormat, index++)
		{
			switch(*pFormat)
			{
			case 'u':	// Unsigned integer
				buf <<  *(uint32*)&structpointer[offset];
				offset += sizeof(uint32);
				break;

			case 'i':	// Signed integer
				buf << *(int32*)&structpointer[offset];
				offset += sizeof(int32);
				break;

			case 'l':	// Signed integer 64
				buf << (*(int64*)&structpointer[offset]);
				offset += sizeof(int64);				
				break;

			case 'b':	// Unsigned integer 64
				buf << (*(uint64*)&structpointer[offset]);
				offset += sizeof(uint64);				
				break;

			case 's':	// Null-terminated string
				{
					if(convertString ) //only convert specific index string (only 1 language)
					{
						if(index < stringIndex)
						{
							if(index == 1)
								strEN = (std::string*)&structpointer[offset];
							offset += sizeof(std::string);
							continue;
						}
						if(index > stringIndex)
							return true;
					}
					std::string* str = (std::string*)&structpointer[offset];
					if(str->length()==0 && stringIndex!= 1) //Export English Text for String Waiting Translation
						str = strEN;
						

					if(convertString && str->length() > 0)
					{
						int outLen = 0;
						const char * retStr = filterString(str->c_str(), outLen, index == 2);
						uint32 nOffset = m_file.WriteString( retStr, outLen );
						buf << nOffset << (short)outLen;
					}
					else
					{
						uint32 nOffset = m_file.WriteString( str->c_str(), str->length() );
						buf << nOffset << (short)str->length() ;
					}

					offset += sizeof(std::string);
				}
				break;

			case 'f':	// Float
				buf << *(float*)&structpointer[offset];
				offset += sizeof(float);
				break;

			case 'c':	// Char
				buf << *(uint8*)&structpointer[offset];
				offset += sizeof(uint8);
				break;

			case 'h':	// Short
				buf << *(uint16*)&structpointer[offset];
				offset += sizeof(uint16);
				break;

            case 't':	// time
                buf << *(int64*)&structpointer[offset];
                offset += sizeof(int64);
                break;

            case 'x':	// Skip
                //MASSERT(false,"No Skip In Develop Step");
                break;

			default:	// unknown
				printf("%s:unknown field type in string: %d\n", T::GetTableName(), *pFormat);
				break;
			}
		}
		return true;
	}

};

#endif // __DATA_FILETABLECACHE_H__
