
#ifndef MYSQL_SINGLETION_H__
#define MYSQL_SINGLETION_H__

#include <string>

#include "ConsoleLog.h"
#include "OS.h"
#include "Thread.h"
#include "Singleton.h"
#include "SimpleSingleton.h"
#include "lock_free/LockFreeQueue.h"

#include "structDef.h"
#include "def/MmoAssert.h"
#include "mysql/MySqlDatabase.h"

class MysSqlWorkThread: public CMySqlDatabase
{
public:
	MysSqlWorkThread( void )
	{
	}

	virtual ~MysSqlWorkThread( void )
	{
		_workThread.Stop();

		DeleteAllPtrInLFQueue( _strBuff );
		DeleteAllPtrInLFQueue( _queryStr );
		NLOG( "~MySqlSingletion( void )" );
	}

	void Start( size_t stackSize )
	{
		_workThread.Start( boost::bind( &MysSqlWorkThread::WorldThread, this ), stackSize );
	}

	void WorldThread( void )
	{
		while( _workThread.IsStart() )
		{
			int  count = 0;
			uint64 begTime = sOS.TimeMS();

			std::string *pStr;
			while( _queryStr.Dequeue( pStr ) )
			{
				++count;
				CMySqlConnection *pConnect = GetConnection();
				if( pConnect != NULL )
				{
					int result = pConnect->JustQuery( pStr->c_str(), pStr->size() );
					ASSERT( result == 0 );
					PutConnection( pConnect );
				}

				_strBuff.Enqueue( pStr );
			}

			uint64 endTime = sOS.TimeMS();

			int64 diffTime = endTime - begTime;
			if( count > 0 && diffTime > count * 100 )
			{
				WLOG( "Sql thread taker %d query use %lld ms", count, diffTime );
			}
			else
			{
				_workThread.Sleep( 1 );
			}
		}

		NLOG( "MySql thread will stop, release all memory.......!" );
	}

	virtual void AddAsyncQuery( std::string &sql )
	{
		std::string *pStr = NULL;
		if( !_strBuff.Dequeue( pStr ) )
		{
			pStr = new std::string();
		}

		pStr->swap( sql );
		_queryStr.Enqueue( pStr );
	}


	virtual void AddAsyncQuery( const std::string &sql )
	{
		std::string *pStr = NULL;
		if( !_strBuff.Dequeue( pStr ) )
		{
			pStr = new std::string();
		}

		pStr->assign( sql );
		_queryStr.Enqueue( pStr );
	}

public:
	typedef ServerQueue< std::string* > QueryBufferTable;

private:
	CThread           _workThread;
	QueryBufferTable  _strBuff;
	QueryBufferTable  _queryStr;
};

// µ¥ÀýÀà
class MySqlSingletion : public Singleton<MySqlSingletion>, public MysSqlWorkThread
{
private:
	friend class Singleton<MySqlSingletion> ;

	MySqlSingletion( void )
	{
	}
public:

	virtual ~MySqlSingletion( void )
	{
	}


} ;


#define sDatabase (MySqlSingletion::Instance())

#endif

