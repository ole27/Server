/*
A cache of entry table.
*/
#ifndef __DATA_TABLECACHE_H__
#define __DATA_TABLECACHE_H__

#include <map>
#include <vector>

#include "def/TypeDef.h"
#include "def/MmoAssert.h"

#include "lock_free/Mutex.h"
#include "memory_buffer/ByteBuffer.h"

#include "multi_language/multi_lang_def.h"

//#define MAX_ITEM_LIMIT 1000
enum LOAD_FLAG
{
	FlagNone			= 0x0000,
	FlagReload			= 0x0001,
	FlagCleanup			= 0x0002,
	FlagCleanupReload	= FlagCleanup|FlagReload,
};

template<class T>
class CTableCache
{
public:
	typedef typename T::Key K ;
	typedef std::map< K, T > CacheMapTable;

protected:
	uint32 m_nRows;
	uint32 m_nCols;
	std::vector< T >	m_vecTable;
	CacheMapTable       m_mapTable;

protected:
	Mutex m_mutex;

public:
	CTableCache()
	{
		m_nRows = 0;
		m_nCols = strlen(T::GetFormat());
	}

	virtual ~CTableCache()
	{
		Cleanup();
	}

	///////////////////////////////////////////////////////////////
	// Common
public:
	typedef typename CacheMapTable::iterator       MapItr;
	typedef typename CacheMapTable::const_iterator MapCItr;

	typedef typename std::vector< T >::iterator       VecItr;
	typedef typename std::vector< T >::const_iterator VecICtr;

public:
	size_t Size( void )
	{
		return T::HasIndex() ? m_mapTable.size() : m_vecTable.size() ;
	}

	void Cleanup()
	{
		m_nCols = strlen(T::GetFormat());
		m_nRows = 0;
		m_vecTable.clear();
		m_mapTable.clear();
	}

	virtual uint32 GetNumRows() const
	{
		return m_nRows;
	}

	//Add an entry to table.if has key,the old one will be replaced.
	/*
	 * 添加如内存操作, 用在添加一条记录
	 *
	 * @param : pEntry : 表格一行数据的对应结构体指针
	 *
	 * @return : 加入后，数据保存的地址指针
	 */
	virtual T* AddEntry( T* pEntry)
	{
		T *pRet =NULL ;
		if ( T::HasIndex() )
		{
			std::pair< MapItr, bool> retPair =m_mapTable.insert( std::make_pair( pEntry->GetKey(), *pEntry ) ) ;
			ASSERT( retPair.second ) ;

			pRet =&( retPair.first->second ) ;
		}
		else
		{
			m_vecTable.push_back(*pEntry);
			pRet =&m_vecTable.back() ;
		}

		return pRet;
	}


	///////////////////////////////////////////////////////////////
	// Has key use
public:
	inline MapItr MapBegin( void )
	{
		return m_mapTable.begin() ;
	}

	inline MapItr MapEnd()
	{
		return m_mapTable.end() ;
	}

	inline MapItr FindByKey( const K &key )
	{
		return T::HasIndex() ?  m_mapTable.find( key ) : m_mapTable.end() ;
	}

	//just for table with key
	inline bool CanFindByKey( const K &key )
	{
		return FindByKey( key ) != m_mapTable.end() ;
	}

	//for in game,quick get entry
	virtual T* GetEntryByKey( const K &key )
	{
		if( !T::HasIndex() )
		{
			return NULL ;
		}

		MapItr itr =FindByKey( key ) ;

		return itr != m_mapTable.end() ? &itr->second : NULL ;
	}


	/*
	 * 仅仅从内存中移除数据
	 *
	 * @param : key : 表格一行数据的对应结构体的Key
	 *
	 * @return : 是否成功
	 */
	virtual bool RemoveEntryByKey( const K &key )
	{
		if( !T::HasIndex() )
		{
			return false ;
		}

		size_t eraseCount =m_mapTable.erase( key ) ;
		return eraseCount > 0 ;
	}

	/*
	 * 仅仅从内存中移除数据
	 *
	 * @param : pEntry : 表格一行数据的对应结构体指针
	 *
	 * @return : 是否成功
	 */
	bool RemoveKeyEntry( const T* pEntry )
	{
		if( pEntry == NULL )
		{
			return false ;
		}

		return RemoveEntryByKey( pEntry->GetKey() ) ;
	}

	MapItr GetLower( const K &key )
	{
		return m_mapTable.lower_bound( key ) ;
	}

	MapItr GetUpper( const K &key )
	{
		return m_mapTable.upper_bound( key ) ;
	}

	void GetRange( const K &minKey, const K &maxKey, std::vector< T* > &vec )
	{
		MapItr beg =GetLower( minKey ) ;
		MapItr end =GetUpper( maxKey ) ;

		size_t dz = std::distance( beg, end ) ;
		vec.reserve( dz ) ;

		for( MapItr cur =beg; cur != end; ++cur )
		{
			T *pTable =&cur->second ;
			vec.push_back( pTable ) ;
		}
	}

	///////////////////////////////////////////////////////////////
	// Not use key
public:
	inline VecItr VecBegin( void )
	{
		return m_vecTable.begin();
	}
	
	inline VecItr VecEnd( void )
	{
		return m_vecTable.end();
	}

	virtual T* GetEntryBySN (int sn )
	{
		if( T::HasIndex() )
		{
			if( m_mapTable.empty() || sn < 0 || sn >=  (int)(m_mapTable.size() ) )
			{
				return NULL ;
			}

			MapItr iter = m_mapTable.begin() ;
			std::advance( iter, sn ) ;
			return &iter->second ;
		}
		else
		{
			if( sn < 0 || sn >= (int)m_vecTable.size() )
			{
				return NULL ;
			}

			return &m_vecTable[sn] ;
		}

		return NULL ;
	};

	//remove an Entry from the table
	virtual bool RemoveEntryBySN( int sn )
	{
		if( T::HasIndex() )
		{
			if (m_mapTable.empty() || sn < 0 || sn >=  (int)(m_mapTable.size() ) )
			{
				return false ;
			}

			MapItr iter = m_mapTable.begin() ;
			std::advance( iter, sn ) ;
			m_mapTable.erase( iter ) ;
		}
		else
		{
			if( sn < 0 || sn >=  (int)m_vecTable.size() )
			{
				return false ;
			}

			m_vecTable.erase( m_vecTable.begin() + sn ) ;
		}

		return true ;
	}

	//interface 
public:
	virtual bool FileLoadAndAddData(const char* szSource = NULL, std::vector<T*> *pAddVec =NULL, LOAD_FLAG flagLoad = FlagNone ,MULTILANG curLanguage = LANG_EN)
	{
		ASSERT(false);
		return false;
	}

	virtual bool SqlLoadAndAddData(const char* szSource = NULL, std::vector<T*> *pAddVec =NULL, LOAD_FLAG flagLoad = FlagNone ,MULTILANG curLanguage = LANG_EN)
	{
		ASSERT(false);
		return false;
	};

	virtual bool RemoveAndSaveData( T *pEntry, bool isSync =false )
	{
		ASSERT(false);
		return false ;
	};

	bool PrepareData(int iCurLanguage)
	{
		return true;
	};

	virtual bool DumpToMysql(const char* szDest = NULL )
	{
		ASSERT(false);
		return false;
	}

	virtual bool DumpToFile(const char* szDest = NULL )
	{
		ASSERT(false);
		return false;
	}

	//virtual bool ReadEntry( typename T::IdType &id, T* pEntry) const
	//{
	//	ASSERT(false);
	//	return false;
	//}

	virtual uint64 WriteEntry(T* pEntry, bool isSync =false )
	{
		ASSERT(false);
		return false;
	}

	virtual T* SaveAndAddEntry(T* pEntry, bool isSync =false )
	{
		ASSERT(false&&"just for mysql");
		return NULL;
	}

	virtual bool DeleteEntrySync( T* pEntry )
	{
		ASSERT(false);
		return false;
	}

	virtual void DeleteEntryAsync( T* pEntry )
	{
		ASSERT(false);
	}

	virtual bool RemoveAndDeleteEntry( T* pEntry, bool isSync =false )
	{
		ASSERT(false&&"just for mysql");
		return false;
	}

	virtual bool BeginWrite()
	{
		return true;
	}

	virtual bool EndWrite()
	{
		return true;
	}

	static bool UnpackEntry(ByteBuffer* pBuff, T* pEntry)
	{
		const char * pFormat =  T::GetFormat();
		char * structpointer = (char*)pEntry;

		//uint32 offset = 0;
		for(; *pFormat != 0; ++pFormat)
		{
			switch(*pFormat)
			{
			case 'u':	// Unsigned integer
				*pBuff>>(*(uint32*)structpointer);
				structpointer += sizeof(uint32);
				break;

			case 'i':	// Signed integer
				*pBuff>>(*(int32*)structpointer);
				structpointer += sizeof(int32);
				break;

			case 'l':	// Signed integer 64
				*pBuff>>(*(int64*)structpointer);
				structpointer += sizeof(int64);
				break;

			case 'b':	// Unsigned integer 64
				*pBuff>>(*(uint64*)structpointer);
				structpointer += sizeof(uint64);
				break;

			case 'f':	// Float
				*pBuff>>(*(int32*)structpointer);
				structpointer += sizeof(float);
				break;

			case 'c':	// Char
				*pBuff>>(*(char*)structpointer);
				structpointer += sizeof(char);
				break;

			case 'h':	// Short
				*pBuff>>(*(short*)structpointer);
				structpointer += sizeof(short);
				break;

			case 's':	// std::string
			case 'S':
				*pBuff>>(*(std::string*)structpointer);
				structpointer += sizeof(std::string);
				break;

			case 't':	// time
				*pBuff>>(*(int64*)structpointer);
				structpointer += sizeof(int64);
				break;

			default:	// unknown
				printf("%s:unknown field type in string: %d\n", T::GetTableName(), *pFormat);
				break;
			}
		}
		return true;
	}

	static bool PackEntry(const T* pEntry, ByteBuffer* pBuff)
	{ 
		const char * pFormat = T::GetFormat();
		char * structpointer = (char*)pEntry;
		uint32 offset = 0;
		for(; *pFormat != 0; ++pFormat)
		{
			switch(*pFormat)
			{
			case 'u':	// Unsigned integer
				*pBuff <<  *(uint32*)&structpointer[offset];
				offset += sizeof(uint32);
				break;

			case 'i':	// Signed integer
				*pBuff << *(int32*)&structpointer[offset];
				offset += sizeof(int32);
				break;

			case 'l':	// Signed integer 64
				*pBuff << *(int64*)&structpointer[offset];
				offset += sizeof(int64);
				break;

			case 'b':	// Unsigned integer 64
				*pBuff << *(uint64*)&structpointer[offset];
				offset += sizeof(uint64);
				break;

			case 's':	// Null-terminated string
			case 'S':
				{
					*pBuff << *(std::string*)&structpointer[offset];
					offset += sizeof(std::string);
				}
				break;

			case 'f':	// Float
				*pBuff << *(float*)&structpointer[offset];
				offset += sizeof(float);
				break;

			case 'c':	// Char
				*pBuff << *(uint8*)&structpointer[offset];
				offset += sizeof(uint8);
				break;

			case 'h':	// Short
				*pBuff << *(uint16*)&structpointer[offset];
				offset += sizeof(uint16);
				break;

			case 't':	// time
				*pBuff << *(int64*)&structpointer[offset];
				offset += sizeof(int64);
				break;

            case 'x':	// ignore
                break;

			default:	// unknown
				printf("%s:unknown field type in string: %d\n", T::GetTableName(), *pFormat);
				break;
			}
		}
		return true;
	}

	static bool cmpentry(const T* pEntry, const T* pEntry2)
	{ 
		const char * pFormat = T::GetFormat();
		char * structpointer = (char*)pEntry;
		char * structpointer2 = (char*)pEntry2;
		uint32 offset = 0;
		for(; *pFormat != 0; ++pFormat)
		{
			switch(*pFormat)
			{
			case 'u':	// Unsigned integer
				if(*(uint32*)&structpointer[offset] != *(uint32*)&structpointer2[offset])
					return false;
				offset += sizeof(uint32);
				break;

			case 'i':	// Signed integer
				if(*(int32*)&structpointer[offset] != *(int32*)&structpointer2[offset])
					return false;
				offset += sizeof(int32);
				break;

			case 'l':	// Signed integer 64
				if(*(int64*)&structpointer[offset] != *(int64*)&structpointer2[offset])
					return false;
				offset += sizeof(int64);
				break;

			case 'b':	// Unsigned integer 64
				if(*(uint64*)&structpointer[offset] != *(uint64*)&structpointer2[offset])
					return false;
				offset += sizeof(uint64);
				break;

			case 's':	// Null-terminated string
			case 'S':
				{
					if(*(std::string*)&structpointer[offset] != *(std::string*)&structpointer2[offset])
						return false;
					offset += sizeof(std::string);
				}
				break;

			case 'f':	// Float
				if(*(float*)&structpointer[offset] != *(float*)&structpointer2[offset])
					return false;
				offset += sizeof(float);
				break;

			case 'c':	// Char
				if(*(uint8*)&structpointer[offset] != *(uint8*)&structpointer2[offset])
					return false;
				offset += sizeof(uint8);
				break;

			case 'h':	// Short
				if(*(uint16*)&structpointer[offset] != *(uint16*)&structpointer2[offset])
					return false;
				offset += sizeof(uint16);
				break;

			case 't':	// time
				if(*(int64*)&structpointer[offset] != *(int64*)&structpointer2[offset])
					return false;
				offset += sizeof(int64);
				break;

            case 'x':	// ignore
                break;

			default:	// unknown
				printf("%s:unknown field type in string: %d\n", T::GetTableName(), *pFormat);
				break;
			}
		}
		return true;
	}

	static uint32 SizeByFormat()
	{ 
		const char * pFormat = T::GetFormat();
		uint32 offset = 0;
		for(; *pFormat != 0; ++pFormat)
		{
			switch(*pFormat)
			{
			case 'u':	// Unsigned integer
				offset += sizeof(uint32);
				break;

			case 'i':	// Signed integer
				offset += sizeof(int32);
				break;

			case 'I':	// Signed integer 64
				offset += sizeof(int64);
				break;

			case 'U':	// Unsigned integer 64
				offset += sizeof(uint64);
				break;

			case 's':	// Null-terminated string
			case 'S':
			case 'b':	// Null-terminated string
			case 'B':
				offset += sizeof(std::string);
				break;

			case 'f':	// Float
				offset += sizeof(float);
				break;

			case 'c':	// Char
				offset += sizeof(uint8);
				break;

			case 'h':	// Short
				offset += sizeof(uint16);
				break;

			case 't':	// time
				offset += sizeof(int64);
				break;

            case 'x':	// ignore
                break;

			default:	// unknown
				printf("%s:unknown field type in string: %d\n", T::GetTableName(), *pFormat);
				break;
			}
		}
		return offset;
	}
};

#endif // __DATA_TABLECACHE_H__
