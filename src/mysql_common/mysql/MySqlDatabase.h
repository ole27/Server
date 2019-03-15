#ifndef MYSQL_DATA_BASE_H__
#define MYSQL_DATA_BASE_H__

#include <string>

#include <boost/thread/mutex.hpp>

#include "ConsoleLog.h"
#include "MySqlRst.h"

#include "def/TypeDef.h"
#include "lock_free/Mutex.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CMySqlDatabase
class CMySqlConnection
{
public:
	CMySqlConnection( void );
	virtual ~CMySqlConnection( void );

	virtual bool Reconnect( void ) ;
	virtual bool Connect( const char* Hostname, unsigned int port, const char* Username, const char* Password, const char* DatabaseName );
	virtual bool SelectDatabase(const char* pszDatabaseName);
	virtual void Close( void ) ;
	virtual bool CheckValid( void );

	virtual bool EscapeString( const std::string &Escape, std::string &result );

	virtual CMysqlRstPtr Query( const char* QueyString, unsigned int len );
	virtual CMysqlRstPtr QueryFormat( const char* QueryString, ... );

	virtual int Execute( const std::string &str );
	virtual int Execute( const char* QueryString, unsigned int len );
	virtual int ExecuteFormat(const char* QueryString, ... );

	virtual void ReleaseAllResult( void ); // 在批量sql语句操作后需要调用

	virtual uint64 GetInsertID( void );

	virtual void TransactionBegin( void );
	virtual void Commit( void ) ;
	virtual void Rollback( void );

	virtual bool Lock( const char* pTableName );
	virtual bool Unlock( );

	const char* GetLastError( void );
	//const std::string GetHostName( void );
	//const std::string GetDatabaseName( void );

	bool SelectDatabase( const std::string &name ) ;

	int JustQuery( const char* strSql, unsigned int len );

	bool GetIsBusy( void ) {   return m_isBusy ;   }
	void SetIsBusy( bool isBusy ) {   m_isBusy =isBusy ;   }

	unsigned long long GetEffectedRows( void )
	{
		return mysql_affected_rows( m_connection );
	}

	bool HandleError( uint32 ErrorNumber )
	{
		// Handle errors that should cause a reconnect to the MySQLDatabase.
		switch(ErrorNumber)
		{
		case CR_SERVER_GONE_ERROR:     // Mysql server has gone away
		case CR_OUT_OF_MEMORY:         // Client ran out of memory
		case CR_SERVER_LOST:           // Lost connection to sql server during query
		case CR_SERVER_LOST_EXTENDED:  // Lost connection to sql server - system error
		case CR_UNKNOWN_ERROR:         // Unknown MySQL error
			{
				return true ;
			}break;
		}

		return false;
	}

protected:
	CMysqlRstPtr _FetchRecordset( void );

private:
	volatile bool   m_isBusy ;
	MYSQL*          m_connection;

	short                            m_port;
	std::string                      m_host;
	std::string                      m_database;
	std::string                      m_user;
	std::string                      m_password;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CMySqlDatabase
/*
Update!!!!!
CMySqlDatabase not a singlton now.
call DatabaseMgr::init to init database, 
example:
	DatabaseMgr::newInstance( void );
	sDBMgr.Init("beiwks3717",3306,"root","123","mmo_db","mmo_char");
*/
class CMySqlDatabase
{
public:
	CMySqlDatabase( void );
	virtual ~CMySqlDatabase( void );

	const char* GetHostName( void );
	const char* GetDatabaseName( void );

	bool Connect(const char* Hostname, unsigned int port,
		const char* Username, const char* Password, const char* DatabaseName, uint32 connCount = 1,uint32 connCountMax = 5);

	bool ChangeDatabase( const std::string &name ) ;

	CMySqlConnection* GetConnection( void );
	void              PutConnection( CMySqlConnection*& pConn);

	virtual void AddAsyncQuery( std::string &sql ) = 0;
	virtual void AddAsyncQuery( const std::string &sql ) = 0;

	bool SyncExcute( std::string &sql, bool releaseMultiResult );

private:
	CMySqlConnection* _NewConnection( void );

private:
	Mutex                            _mutex;
	short                            _port;
	std::string                      _host;
	std::string                      _database;
	std::string                      _user;
	std::string                      _password;

	size_t	                         _max_connection_count;
	size_t	                         _default_connection_count;
	size_t                           _index ;
	size_t                           _indexCount ;
	std::vector< CMySqlConnection* > _connections;
};

#endif
//#define sMysql (&g_con)
