#include "MySqlDatabase.h"

#include <stdarg.h>

#include "ConsoleLog.h"
#include "def/MmoAssert.h"

#ifndef _LOG
#define _LOG NLOG
#endif


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CMySqlDatabase

CMySqlConnection::CMySqlConnection( void )
{
	SetIsBusy( false ) ;
	m_connection = NULL;
}

CMySqlConnection::~CMySqlConnection( void )
{
	Close();
}

bool CMySqlConnection::Reconnect( void )
{
	Close() ;
	return Connect( m_host.c_str(), m_port, m_user.c_str(), m_password.c_str(), m_database.c_str());
}

bool CMySqlConnection::Connect(const char* Hostname, unsigned int port, const char* Username, const char* Password, const char* DatabaseName)
{
	m_port      =port ;
	m_host      =Hostname ;
	m_database  =DatabaseName ;
	m_user      =Username;
	m_password  =Password;

	my_bool my_true = true;

	_LOG("Connecting to `%s`, database `%s`...\n", Hostname, DatabaseName);

	MYSQL* mysqlConn;
	mysqlConn = mysql_init( NULL );
	if( mysqlConn == NULL )
		return false;

	if( mysql_options( mysqlConn, MYSQL_SET_CHARSET_NAME, "utf8" ) )
	{
		_LOG( "MySQLDatabase: Could not set utf8 character set." );
	}

	if( mysql_options( mysqlConn, MYSQL_OPT_RECONNECT, &my_true ) )
	{
		_LOG( "MySQLDatabase: MYSQL_OPT_RECONNECT could not be set, connection drops may occur but will be counteracted." );
	}

	unsigned long flags = CLIENT_IGNORE_SIGPIPE | CLIENT_REMEMBER_OPTIONS | CLIENT_MULTI_STATEMENTS | CLIENT_MULTI_RESULTS;
	m_connection = mysql_real_connect( mysqlConn, Hostname, Username, Password, DatabaseName, port, NULL, flags );
	if( m_connection == NULL )
	{
		_LOG("MySQLDatabase: Connection failed due to: `%s`", mysql_error( mysqlConn ) );
		mysql_close(mysqlConn);
		return false;
	}

	return true;
}

void CMySqlConnection::Close( void )
{
	if (m_connection)
		mysql_close(m_connection);
	m_connection = NULL;
}

bool CMySqlConnection::CheckValid( void )
{
	return m_connection && (mysql_ping(m_connection) == 0);
}

bool CMySqlConnection::SelectDatabase(const char* pszDatabaseName)
{
	int nResult = m_connection ? mysql_select_db(m_connection, pszDatabaseName) : -1;
	return (nResult == 0);
}

bool CMySqlConnection::EscapeString( const std::string &Escape, std::string &result )
{
	if( Escape.empty() )
	{
		return true;
	}

	if( !m_connection )
	{
		return false ;
	}

	char a2[16384] = "";
	int count =mysql_real_escape_string( m_connection, a2, Escape.c_str(), (unsigned long)Escape.length() ) ;
	if( count > 0 )
	{
		result.assign( a2, count ) ;
	}
	//else
	//{
	//	result.assign( Escape ) ;
	//}

	return count > 0 ;
}

int CMySqlConnection::JustQuery( const char* strSql, unsigned int len )
{
	int result = mysql_real_query( m_connection, strSql, len );
	if( result != 0 )
	{
		const char* pError = GetLastError() ;
		ELOG( "Fail result %d, %s: %s", result, strSql, pError? pError : "Unknown Error!" );

		bool isReconnect =false ;
		while( !CheckValid() )
		{
			isReconnect =true ;
			if( Reconnect() )
			{
				SLOG( "Reconnect sucess !" ) ;
			}
		}

		if( isReconnect )
		{
			result = mysql_real_query( m_connection, strSql, len );
		}
		else if( HandleError( result ) )
		{
			result = mysql_real_query( m_connection, strSql, len );

			while( HandleError( result ) )
			{
				if( Reconnect() )
				{
					SLOG( "Reconnect sucess !" ) ;
					result = mysql_real_query( m_connection, strSql, len );
				}
			}
		}
	}
	else
	{
		SLOG( "%s", strSql );
	}

	return result ;
}


CMysqlRstPtr CMySqlConnection::QueryFormat(const char* szQueryString, ...)
{
	char sql[16384];
	va_list vlist;
	va_start(vlist, szQueryString);
	unsigned int len =vsnprintf(sql, 16384, szQueryString, vlist);
	va_end(vlist);

	if( len >= 16384 )
	{
		return CMysqlRstPtr() ;
	}

	return Query(sql, len);
}

int CMySqlConnection::Execute( const std::string &str )
{
	bool isSuc = JustQuery( str.c_str(), str.size() ) == 0 ;
	if ( !isSuc )
	{
		return -1;
	}

	return (int)mysql_affected_rows( m_connection );
}


int CMySqlConnection::Execute( const char* szQueryString, unsigned int len )
{
	bool isSuc = JustQuery( szQueryString, len ) == 0 ;
	if ( !isSuc )
	{
		return -1;
	}

	return (int)mysql_affected_rows( m_connection );
}


int CMySqlConnection::ExecuteFormat(const char* szQueryString, ...)
{
	char sql[16384];
	va_list vlist;
	va_start(vlist, szQueryString);
	unsigned int len =vsnprintf(sql, 16384, szQueryString, vlist);
	va_end(vlist);

	if( len >= 16384 )
	{
		return -1 ;
	}

	return Execute( sql, len );
}

void CMySqlConnection::ReleaseAllResult( void )
{
	do
	{
		MYSQL_RES * pRes =mysql_store_result( m_connection );
		
		mysql_free_result( pRes );

	} while( mysql_next_result( m_connection ) == 0 );
}


CMysqlRstPtr CMySqlConnection::Query( const char* szQueryString, unsigned int len )
{
	bool isSuc = JustQuery( szQueryString, len ) == 0 ;
	if(!isSuc)
	{
		return NULL;
	}

	MYSQL_RES * pRes = mysql_store_result( m_connection );
	uint32 uRows = (uint32)mysql_affected_rows( m_connection );
	uint32 uFields = (uint32)mysql_field_count( m_connection );

	CMysqlRstPtr pRst(new CMySqlRst( pRes, uFields, uRows ));
	pRst->NextRow();
	return pRst;
}

uint64 CMySqlConnection::GetInsertID( void )
{
	return mysql_insert_id(m_connection);
}

void CMySqlConnection::TransactionBegin( void )
{
	const std::string str( "START TRANSACTION" ) ;
	JustQuery( str.c_str(), str.size() );
}
void CMySqlConnection::Commit( void ) 
{
	const std::string str( "COMMIT" ) ;
	JustQuery( str.c_str(), str.size() );
}
void CMySqlConnection::Rollback( void )
{
	const std::string str( "ROLLBACK" ) ;
	JustQuery( str.c_str(), str.size() );
}

bool CMySqlConnection::Lock( const char* pTableName )
{
	char buff[1024];
	unsigned int len =sprintf(buff,"LOCK TABLES %s WRITE",pTableName);

	JustQuery( buff, len );

	return true;
}

bool CMySqlConnection::Unlock( )
{
	const std::string str( "UNLOCK TABLES" ) ;
	JustQuery( str.c_str(), str.size() );
	return true;
}


const char* CMySqlConnection::GetLastError( void )
{
	if ( !m_connection )
	{
		return "Invalid Connection";
	}

	if(mysql_errno(m_connection))
	{
		return mysql_error(m_connection);
	}
	else
	{
		return NULL;
	}
}

bool CMySqlConnection::SelectDatabase( const std::string &name )
{
	if( mysql_select_db( m_connection, name.c_str() ) )
	{
		ELOG( "SelectDatabase error: %s ;", GetLastError() ) ;
		if( mysql_select_db( m_connection, m_database.c_str() ) )
		{
			MASSERT( false, "Orign database can not use ??? " ) ;
		}
		return false ;
	}

	NLOG( "Change database %s .", name.c_str() ) ;
	m_database =name ;
	return true ;
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CMySqlDatabase

CMySqlDatabase::CMySqlDatabase( void )
{
	_index =0 ;
	_indexCount =0 ;
}

CMySqlDatabase::~CMySqlDatabase( void )
{
	int nCount = _connections.size();
	for ( int iConn = 0; iConn < nCount; iConn ++)
	{
		delete _connections.at(iConn);
	}
	_connections.clear();
	NLOG( "CMySqlDatabase::~CMySqlDatabase()" ) ;
}


CMySqlConnection* CMySqlDatabase::_NewConnection( void )
{
	CMySqlConnection * pConnection = new CMySqlConnection;
	if ( !pConnection->Connect(_host.c_str(),_port,_user.c_str(),_password.c_str(),_database.c_str()) )
	{
		delete pConnection;
		return NULL;
	}
	_connections.push_back(pConnection);
	return pConnection;
}

bool CMySqlDatabase::Connect(const char* Hostname, unsigned int port,
							 const char* Username, const char* Password, const char* DatabaseName, uint32 connCount,uint32 connCountMax)
{
	_host = Hostname;
	_port = port;
	_database = DatabaseName;
	_user = Username;
	_password = Password;
	_default_connection_count = connCount;
	_max_connection_count = connCountMax;

	{
		MutexGuard lock(_mutex);
		for (size_t i = 0; i < _default_connection_count; i ++)
		{
			if ( _NewConnection() == NULL )
			{
				return false;
			}
		}
		return true;
	}
}


bool CMySqlDatabase::ChangeDatabase( const std::string &name )
{
	std::vector< CMySqlConnection* >::iterator itr =_connections.begin() ;
	for( ; itr != _connections.end(); ++itr )
	{
		if( !(*itr)->SelectDatabase( name ) )
		{
			return false ;
		}
	}

	return true ;
}


CMySqlConnection* CMySqlDatabase::GetConnection( void )
{
	MutexGuard lock(_mutex);

	CMySqlConnection *pConn = NULL;

	while( true )
	{
		const size_t size = _connections.size() ;

		++_indexCount ;
		pConn =_connections[ ++_index % size ] ;
		if( !pConn->GetIsBusy() )
		{
			_indexCount =0 ;
			pConn->SetIsBusy( true ) ;

			return pConn ;
		}

		if( _indexCount >= size && size < _max_connection_count )
		{
			// ASSERT( false );  调试的时候加上，不是因为执行这里是错的，
			// 是因为调试的时候一般不会出现这个情况，不够用的情况下要注意是不是没有 GetConnection() 完没有 PutConnection()
			ASSERT( false ) ;

			WLOG( "Mysql current connect is not enough add new connect now!!!!!!!" ) ;
			pConn =_NewConnection() ;
			if( pConn != NULL )
			{
				_indexCount =0 ;
				pConn->SetIsBusy( true ) ;
				return pConn ;
			}
		}
	}
}

void CMySqlDatabase::PutConnection( CMySqlConnection*& pConn)
{
	pConn->SetIsBusy( false ) ;
}

const char* CMySqlDatabase::GetHostName( void )
{
	return _host.c_str();
}
const char* CMySqlDatabase::GetDatabaseName( void )
{
	return _database.c_str();
}

bool CMySqlDatabase::SyncExcute( std::string &sql, bool releaseMultiResult )
{
	bool isOk = false;

	CMySqlConnection *pConnect = GetConnection();
	if( pConnect != NULL )
	{
		isOk = pConnect->JustQuery( sql.c_str(), sql.size() ) == 0;
		if( isOk && releaseMultiResult )
		{
			pConnect->ReleaseAllResult();
		}
		PutConnection( pConnect );
	}

	return isOk;
}
