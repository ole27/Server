#pragma once

#include <sstream>
#include <boost/numeric/conversion/cast.hpp>

#include "FileTableCache.h"
#include "mysql/MySqlDatabase.h"
#include "mysql/MySqlRst.h"

#include "def/MmoAssert.h"
#include "utility/Utility.h"

#include "MySqlSingleton.h"


template<class T>
class CMysqlTableCache : public FileTableCache< T >
{
protected:
	static CMySqlDatabase*		m_pDB;

private:
	static CMySqlConnection*	m_pLockConn;

protected:
	bool LoadFieldNames();

public:
	typedef typename T::IdType           IdType ;
	typedef CTableCache< T >             CacheBase;
	typedef typename CacheBase::K        K ;

	//interface
public:
	static void SetDB( CMySqlDatabase* pDatabase )
	{
		m_pDB = pDatabase;
	}

	static CMySqlDatabase* GetDB( void )
	{
		MASSERT( m_pDB != NULL, "Invaliad database" );
		return m_pDB;
	}

	//Use GetConnection/FinishConn pairs
	static CMySqlConnection* GetConnection()
	{
		if ( !m_pLockConn )
		{
			if (GetDB())
			{
				return GetDB()->GetConnection();
			}
			else
			{
				return NULL;
			}
		}
		return m_pLockConn;
	}

	static bool FinishConn(CMySqlConnection*& pConn)
	{
		if ( pConn != m_pLockConn)
		{
			GetDB()->PutConnection(pConn);
		}
		else
		{
			pConn = NULL;
		}

		return true;
	}

public:

	//load from recordset
	/*
	 * ���ض�Ӧ������
	 *
	 * @param : szSource : ָ��sql��䣬�����ָ����Ĭ�ϼ����������Ӧ���������ݡ�
	 * @param : pAddVec  : ָ������Ի�ȡ���ж�ȡ�����ݡ�
	 *
	 * @return : �Ƿ�ɹ�
	 */
	virtual bool SqlLoadAndAddData( const char* szSource = NULL, std::vector<T*> *pAddVec =NULL, LOAD_FLAG flagLoad = FlagNone,MULTILANG curLanguage = LANG_EN)
	{
		bool bResult =false ;

		//MutexGuard lock((this->m_mutex));

		if ( this->GetNumRows() > 0 && !(flagLoad&FlagReload)  )
		{
			return bResult;
		}

		if (flagLoad&FlagCleanup)
		{
			this->Cleanup();
		}

		CMySqlConnection* pConn = GetConnection();
		if ( !pConn )
		{
			ASSERT( false ) ;
			return bResult;
		}

		string strTablename ;// Tips: ��ʱ��ӱ����ʱ�򷽱㶨λ���� By CaiBingjie 2014-12-9
		CMysqlRstPtr pRst;
		if ( szSource != NULL)
		{
			pRst = pConn->Query( szSource, strlen( szSource ) );
		}
		else
		{
			strTablename =T::GetTableName() ;
			pRst = pConn->QueryFormat("SELECT * FROM %s", T::GetTableName());
		}

		if ( !CheckRecordset (pRst, pConn) )
		{
			FinishConn(pConn);
			return bResult;
		}

		this->m_nCols = pRst->GetFieldCount();

		//load field names
		//this->m_strColNames.clear();
		//for (uint32 iCol = 0; iCol < this->m_nCols; iCol++)
		//{
		//	this->m_strColNames.push_back( pRst->GetFieldName(iCol) );
		//}

		T object;

		if ( pRst->GetRowCount() > 0)
		{
			if( pAddVec != NULL )
			{
				pAddVec->reserve( pRst->GetRowCount() ) ;
			}

			bResult =true ;

			Field* pField;
			do 
			{
				pField = pRst->Fetch();

				// TODO: By CaiBingjie �Ż���ʾ��
				//       �������ʹ���� CTableCache< T >�ڲ���һ��map< dbUniqueId, T > memMap ����
				//       Ȼ��ʹ�� map< Key, T* >, ��νƽʱʹ�ã���ô����Ͳ���ÿ��һ��������һ���ˣ�
				//       T object ������  memMap ���ء�Ȼ��ָ��Ÿ� CTableCache< T >::AddEntry() ȥ��ӡ�
				//T object;

				ReadFieldEntry(pField,&object,this->m_nCols );
				T *pAddObj =CTableCache< T >::AddEntry(&object);
				if( pAddVec != NULL )
				{
					pAddVec->push_back( pAddObj ) ;
				}

				this->m_nRows ++;

				//if (this->m_nRows > MAX_ITEM_LIMIT )
				//{
				//	break;
				//}

			}while (pRst->NextRow());
		}

		//pRst->Delete();
		FinishConn(pConn);

		printf("=====> LoadAndAddData mysqlTableCache = [%s]\n", T::GetTableName());

		CMysqlTableCache< T >::PrepareData(0);

		return bResult;
	}


	virtual bool DumpToMysql(const char* szDest = NULL )
	{
		if ( T::HasIndex() )
		{
			typename CacheBase::MapItr iter = this->m_mapTable.begin();
			while ( iter != this->m_mapTable.end())
			{
				uint64 insertID = WriteEntryToDB( &(iter->second), true );
				if ( insertID == 0)
				{
					MASSERT(false,"Add Record Faild!");
				}
				iter++;
			}
		}
		else
		{
			int nCount = this->m_vecTable.size();
			for ( int iEntry = 0 ; iEntry < nCount; iEntry++)
			{
				uint64 insertID = WriteEntryToDB( &this->m_vecTable[iEntry], true );
				if ( insertID == 0)
				{
					MASSERT(false,"Add Record Faild!");
				}
			}
		}

		return true;
	}

	/*
	 * ������ڴ� �� ��������ݿ�Ĳ���, �������һ����¼���ҽ���¼���ؽ��ڴ�( �磺�½��˺� )
	 *
	 * @param : pEntry : ���һ�����ݵĶ�Ӧ�ṹ��ָ��
	 * @param : isSync : �Ƿ�ͬ��������Ĭ�� false Ϊ��ͬ������
	 *
	 * @return : ��������ݱ���ĵ�ַָ��
	 */
	virtual T* SaveAndAddEntry(T* pEntry, bool isSync =false )
	{
		if( isSync )
		{
			uint64 insertId =WriteEntryToDB(pEntry, isSync) ;
			if( pEntry->id == 0 && insertId != 0 )
			{
				pEntry->id =insertId ;
			}
			else
			{
				ASSERT( pEntry->id == insertId ) ;
				if( pEntry->id != insertId || insertId == 0 )
				{
					return NULL ;
				}
			}

			return this->AddEntry( pEntry ) ;
		}
		else
		{
			T *pRet =this->AddEntry( pEntry ) ;
			if( pRet == NULL )
			{
				return pRet ;
			}
			
			AsyncWriteEntryToDB( pEntry ) ;

			return pRet ;
		}
	}

	/*
	 * �������ݱ�������ݿ�Ĳ���( ����ֻ�½���ɫ�����ǽ�ɫ��������Ϸ�����Բ���Ҫ���� )
	 *
	 * @param : pEntry : ���һ�����ݵĶ�Ӧ�ṹ��ָ��
	 * @param : isSync : �Ƿ�ͬ��������Ĭ�� false Ϊ��ͬ������
	 *
	 * @return : ��������ݱ���ĵ�ַָ��
	 */
	virtual uint64 WriteEntry(T* pEntry, bool isSync =false )
	{
		if( pEntry == NULL || pEntry->id == 0 )
		{
			ASSERT( false ) ;
			return 0 ;
		}

		return WriteEntryToDB(pEntry, isSync);
	}


	// ͬ��
	virtual bool DeleteEntrySync( T* pEntry )
	{
		return DeleteEntryFromDB( pEntry->id, true ) > 0 ;
	}

	// �첽
	virtual void DeleteEntryAsync( T* pEntry )
	{
		DeleteEntryFromDB( pEntry->id, false ) ;
	}

	/*
	 * ���ڴ����Ƴ����� �� ͬʱ ɾ�����ݿ����������
	 *
	 * @param : pEntry : ���һ�����ݵĶ�Ӧ�ṹ��ָ��
	 * @param : isSync : �Ƿ�ͬ��������Ĭ�� false Ϊ��ͬ������
	 *
	 * @return : �Ƿ�ɹ�
	 */
	virtual bool RemoveAndDeleteEntry( T* pEntry, bool isSync =false )
	{
		if( !T::HasIndex() || pEntry == NULL )
		{
			return false;
		}

		bool isOk = DeleteEntryFromDB( pEntry->id, isSync ) > 0 ;
		if( isSync && !isOk )
		{
			return false;
		}

		return this->RemoveKeyEntry( pEntry );
	}

	/*
	 * ���ڴ����Ƴ����� �� ͬʱ����һ������( ��Ϸһ�㲻���������Ϊ�����ߵ�ʱ����뻺���Ѿ�������һ�Σ����Դ��ڴ��Ƴ��Ѿ�����Ҫ�ٱ��� )
	 *
	 * @param : pEntry : ���һ�����ݵĶ�Ӧ�ṹ��ָ��
	 * @param : isSync : �Ƿ�ͬ��������Ĭ�� false Ϊ��ͬ������
	 *
	 * @return : �Ƿ�ɹ�
	 */
	virtual bool RemoveAndSaveData( T *pEntry, bool isSync =false )
	{
		bool isOk = this->WriteEntry( pEntry, isSync ) > 0 ;
		if( isSync && !isOk )
		{
			return isOk ;
		}

		return this->RemoveEntryByKey( pEntry->GetKey() ) ;
	}


	//!!not thread safe, don't use in multithread.
	virtual bool BeginWrite()
	{
		m_pLockConn = GetConnection();
		if ( !m_pLockConn )
			return false;

		m_pLockConn->Lock( T::GetTableName() );
		return true;
	}

	//!!not thread safe, don't use in multithread.
	virtual bool EndWrite()
	{
		m_pLockConn->Unlock();
		GetDB()->PutConnection(m_pLockConn);
		return true;
	}


	//virtual bool ReadEntry(typename T::IdType &id, T* pEntry) const
	//{
	//	return ReadEntryFromDB(id, pEntry);
	//}



	//static functions
public:
	static uint64 GetMaxFieldValue( const char* pszFieldName )
	{
		CMySqlConnection* pConn = GetConnection();
		if ( !pConn )
			return false;

		CMysqlRstPtr pRst = pConn->QueryFormat( "select max(`%s`) from `%s` ;", pszFieldName, T::GetTableName() );
		if ( pRst && pRst->GetRowCount() > 0 && pRst->GetFieldCount() > 0)
		{
			Field* pField = pRst->Fetch();
			uint64 MaxID = pField->GetUInt64();
			//pRst.Delete();
			FinishConn(pConn);
			return MaxID;
		}

		//ASSERT( false );
		FinishConn(pConn);
		return 0;
	}

	static uint64 QueryMaxID()
	{
		return GetMaxFieldValue( T::GetDbIncreaseIdName() );
	}

	static IdType& GetMaxId( void )
	{
		static IdType s_maxId =0 ;
		return s_maxId ;
	}

	static void SetMaxId( const IdType &id )
	{
		GetMaxId() =id ;
	}

	static const IdType& IncreaseMaxId( void )
	{
		return ++GetMaxId() ;
	}

	static void InitMaxId(  void )
	{
		GetMaxId() =boost::numeric_cast<IdType>( QueryMaxID() );
	}

	static bool QueryIsTure( const char* pszFieldName, ... )
	{
		char buffer[512];
		va_list vlist;
		va_start(vlist, pszFieldName);
		unsigned int retValut =vsnprintf(buffer, 512, pszFieldName, vlist);
		va_end(vlist);

		if( retValut > 512 )
		{
			ASSERT( false ) ;
			return false ;
		}

		bool isTrue =false ;

		CMySqlConnection* pConn = GetConnection();
		if ( !pConn )
		{
			ASSERT( false ) ;
			return isTrue;
		}

		CMysqlRstPtr pRst= pConn->Query( buffer, retValut ) ;
		if ( pRst && pRst->GetRowCount() > 0 && pRst->GetFieldCount() > 0)
		{
			isTrue =true ;
		}
		FinishConn(pConn);

		return isTrue;
	}


	/*
	read a entry from database,return is success
	*/
	static bool ReadEntryFromDB(typename T::IdType &id, T* pEntry)
	{
		CMySqlConnection* pConn = GetConnection();
		if ( !pConn )
			return false;

		CMysqlRstPtr pRst = pConn->QueryFormat( "SELECT * FROM %s where %s = %llu LIMIT 1; ", T::GetTableName(), T::GetDbIncreaseIdName(), id );
		if ( !CheckRecordset (pRst, pConn) )
		{
			FinishConn(pConn);
			return false;
		}

		bool ret = false;
		if ( pRst->GetRowCount() > 0)
		{
			Field* pField = pRst->Fetch();
			ReadFieldEntry(pField,pEntry, pRst->GetFieldCount() );
			ret = true;
		}

		FinishConn(pConn);
		return ret;
	}

	/*
	read a entry from database,return is success
	*/
	static bool ReadEntryByLogicalKeyFromDB(typename T::IdType &id, T* pEntry)
	{
		CMySqlConnection* pConn = GetConnection();
		if (!pConn)
			return false;

		CMysqlRstPtr pRst = pConn->QueryFormat("SELECT * FROM %s where %s = %llu LIMIT 1; ", T::GetTableName(), T::GetLogicalKeyName(), id);
		if (!CheckRecordset(pRst, pConn))
		{
			FinishConn(pConn);
			return false;
		}

		bool ret = false;
		if (pRst->GetRowCount() > 0)
		{
			Field* pField = pRst->Fetch();
			ReadFieldEntry(pField, pEntry, pRst->GetFieldCount());
			ret = true;
		}

		FinishConn(pConn);
		return ret;
	}

	/*
	insert a entry to database,return is success
	*/
	static uint64 WriteEntryToDB(const T* pEntry, bool isSync )
	{
		uint64 insertId = 0;
		if( pEntry == NULL )
		{
			ASSERT( false ) ;
			return insertId ;
		}

		if( isSync )
		{
			CMySqlConnection* pConn = GetConnection();
			if ( !pConn )
			{
				ASSERT( false ) ;
				return insertId;
			}

			std::stringstream ss ;
			EntryToSql( ss, pEntry, pConn );

			int iCount = pConn->Execute( ss.rdbuf()->str() );
			if ( iCount > 0)
			{
				insertId = pConn->GetInsertID();
			}

			FinishConn(pConn);
		}
		else
		{
			std::stringstream ss ;
			EntryToSql( ss, pEntry );
			GetDB()->AddAsyncQuery( ss.rdbuf()->str() ) ;
		}

		return insertId;
	}

	static bool AsyncWriteEntryToDB( T* pEntry )
	{
		if( pEntry == NULL || pEntry->id == 0)
		{
			ASSERT( false ) ;
			return false ;
		}

		std::stringstream ss ;
		EntryToSql( ss, pEntry );
		GetDB()->AddAsyncQuery( ss.rdbuf()->str() ) ;

		return true ;
	}

	/*
	Delete a entry from database, reutrn count of deleted items.
	*/
	static uint32 DeleteEntryFromDB( uint64 id, bool isSync )
	{
		MASSERT( id >= 0 ,"Key should >= 0" );

		std::string sql ;
        Utility::FormatString( sql, "DELETE FROM %s WHERE %s = %llu", T::GetTableName(), T::GetDbIncreaseIdName(), id ) ;

		uint32 value =0 ;
		if( isSync )
		{
			CMySqlConnection* pConn = GetConnection();
			if ( !pConn )
			{
				return 0;
			}

			value =pConn->Execute( sql.c_str(), sql.size() ) ;
			FinishConn(pConn);
		}
		else
		{
			GetDB()->AddAsyncQuery( sql ) ;
		}

		return value ;
	}

	//protected:

	static bool CheckRecordset( const CMysqlRstPtr& pRst, CMySqlConnection *pConn )
	{
		if ( !pRst && pConn != NULL )
		{
			const char* pszError =pConn->GetLastError() ;
			if ( pszError )
			{
				MASSERT(false,"Mysql Error" );
			}

			return false;
		}

		uint32 nCols = pRst->GetFieldCount();
		uint32 nFormatLen = strlen( T::GetFormat() ) ; 
		if ( nCols != nFormatLen )
		{
			printf ("Table:'%s' column not match, in db is %d, in src is %d\n", T::GetTableName(), nCols, nFormatLen);
			ASSERT(false);
			return false;
		}

		//==================================================
		uint32 nRealSize = sizeof(T); 
		uint32 nSizeFormat = CTableCache<T>::SizeByFormat() +T::ExtraSize(); 
		if ( nRealSize != nSizeFormat )
		{
			MASSERT(false, "nRealSize define not match ncolumns, ignore to continue...");
			return false;
		}

		return true;
	}

	static void ReadFieldEntry(Field * fields, T * Allocated,uint32 nCols, bool reload = false )
	{
		const char * p = T::GetFormat();
		char * structpointer = (char*)Allocated;
		uint32 offset = 0;
		Field * f = fields;

		for(uint32 iCol = 0; p[iCol] != 0 && iCol < nCols; ++iCol, ++f)
		{
			char format = p[iCol];
			switch(format)
			{
			case 'u':	// Unsigned integer
				*(uint32*)&structpointer[offset] = f->GetUInt32();
				offset += sizeof(uint32);
				break;

			case 'i':	// Signed integer
				*(int32*)&structpointer[offset] = f->GetInt32();
				offset += sizeof(int32);
				break;

			case 'I':	// Signed integer 64
				*(int64*)&structpointer[offset] = f->GetInt64();
				offset += sizeof(int64);
				break;

			case 'U':	// Unsigned integer 64
				*(uint64*)&structpointer[offset] = f->GetUInt64();
				offset += sizeof(uint64);
				break;

			case 's':	// std::string
			case 'S':
				*(std::string*)&structpointer[offset] = f->GetString();
				offset += sizeof(std::string);
				break;

			case 'b':
			case 'B':
				{
					std::string &outStr =*(std::string*)&structpointer[offset] ;
					f->GetBlobString( outStr, '|' ) ;
					offset += sizeof(std::string) ;
				}
				break ;

			case 'x':	// Skip
				//MASSERT(false, "No Skip In Develop Step");
				break;

			case 'f':	// Float
				*(float*)&structpointer[offset] = f->GetFloat();
				offset += sizeof(float);
				break;

			case 'c':	// Char
				*(uint8*)&structpointer[offset] = f->GetUInt8();
				offset += sizeof(uint8);
				break;

			case 'h':	// Short
				*(uint16*)&structpointer[offset] = f->GetUInt16();
				offset += sizeof(uint16);
				break;

			case 't':	// time
				*(int64*)&structpointer[offset] = f->GetTimeT();
				offset += sizeof(int64);
				break;

			default:	// unknown
				printf("%s:unknown field type in string: %d\n", T::GetTableName(), format);
				break;
			}
		}
	};

	static void EntryToSql( std::stringstream &ss, const T* pEntry, CMySqlConnection *pConn =NULL )
	{
		//ss.str( "" ) ;

		ss<<"REPLACE INTO ";
		ss<< T::GetTableName();
		ss<<" value (";
		const char * p = T::GetFormat();
		char * structpointer = (char*)pEntry;
		uint32 offset = 0;
		for(; *p != 0; ++p)
		{
			switch(*p)
			{
			case 'u':	// Unsigned integer
				ss <<  *(uint32*)&structpointer[offset];
				offset += sizeof(uint32);
				break;

			case 'i':	// Signed integer
				ss << *(int32*)&structpointer[offset];
				offset += sizeof(int32);
				break;

			case 'I':	// Signed integer 64
				ss << *(int64*)&structpointer[offset];
				offset += sizeof(int64);
				break;

			case 'U':	// Unsigned integer 64
				ss << *(uint64*)&structpointer[offset];
				offset += sizeof(uint64);
				break;

			case 's':	// Null-terminated string
				ss << "\"" << (*(std::string*)&structpointer[offset]) << "\"";
				offset += sizeof(std::string);
				break;

			case 'S':	// Null-terminated string
				{
					std::string &in = *(std::string*)&structpointer[ offset ];
					if( in.empty() )
					{
						ss << "\"\"";
					}
					else
					{
						bool isRelease = false;
						if( pConn == NULL )
						{
							pConn = GetConnection();
							isRelease = pConn != NULL ;
						}

						std::string &in = *(std::string*)&structpointer[ offset ];
						std::string out;
						if( pConn != NULL && pConn->EscapeString( in, out ) )
						{
							ss << "\"" << out << "\"";
						}
						else
						{
							ss << "\"" << in << "\"";
							printf( "Table :'%s', S field: %s, escape fail~~~", T::GetTableName(), in.c_str() );
						}

						if( pConn != NULL && isRelease )
						{
							FinishConn( pConn );
						}
					}
					offset += sizeof(std::string);
				} break;

			case 'b':
				{
					std::string &in = *(std::string*)&structpointer[ offset ];
					ss << "\"" << in.size() << '|' << in << "\"";
					offset += sizeof( std::string );
				} break;

			case 'B':	// Null-terminated string
				{
					std::string &in = *(std::string*)&structpointer[ offset ];
					if( in.empty() )
					{
						ss << "\"\"";
					}
					else
					{
						bool isRelease = false;
						if( pConn == NULL )
						{
							pConn = GetConnection();
							isRelease = pConn != NULL;
						}

						std::string out;
						if( pConn != NULL && pConn->EscapeString( in, out ) )
						{
							ss << "\"" << out.size() << '|' << out << "\"";
						}
						else
						{
							ss << "\"" << in.size() << '|' << in << "\"";
							printf( "Table :'%s', B field: %s, escape fail~~~", T::GetTableName(), in.c_str() );
						}

						if( pConn != NULL && isRelease )
						{
							FinishConn( pConn );
						}
					}
					offset += sizeof( std::string );
				} break;

			case 'x':	// Skip
				//MASSERT(false,"No Skip In Develop Step");
				break;

			case 'f':	// Float
				ss << *(float*)&structpointer[offset];
				offset += sizeof(float);
				break;

			case 'c':	// Char
				ss << (uint32)(*(uint8*)&structpointer[offset]);
				offset += sizeof(uint8);
				break;

			case 'h':	// Short
				ss << (uint32)(*(uint16*)&structpointer[offset]);
				offset += sizeof(uint16);
				break;

			case 't':	// time 
				{
					string strtime( "1970-01-01 00:00:00" );
					TimeT2String(*(int64*)&structpointer[offset], strtime, false);
					ss << "\"" << strtime.c_str() << "\"";
					offset += sizeof(int64);
				}
				break;

			default:	// unknown
				printf("%s:unknown field type in string: %d\n", T::GetTableName(), *p);
				break;
			}
			if ( *(p+1) != 0 )
			{
				ss << ",";
			}else{
				ss << ")";
			}
		}
	}
};

template<class T> CMySqlDatabase*   CMysqlTableCache<T>::m_pDB = NULL;
template<class T> CMySqlConnection*	CMysqlTableCache<T>::m_pLockConn = NULL;
