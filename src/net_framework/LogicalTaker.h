
#ifndef LOGICAL_THREAD_H__
#define LOGICAL_THREAD_H__

#include <map>

#include "Thread.h"
#include "NetTypeDef.h"

#include "def/TypeDef.h"
#include "lock_free/LockFreeQueue.h"

class Session ;

class LogicalTaker
{
public:
	LogicalTaker( void ) ;
	virtual ~LogicalTaker( void ) ;

	void Start( size_t stackSize ) ;
	void Stop( void );
	bool GetIsStart( void );
	void StopNotWating( void ) ;

	void LogicalThread( void ) ;

	virtual void BeforeSeesionUpdate( uint64 nowTimeMS, int64 diffMS ) ;
	virtual void AfterSessionUpdate( uint64 nowTimeMS, int64 diffMS ) ;

	void PutSeesion( Session *pSession ) ;
	void PutCloseSessionId( uint32 sessionId ) ;

	void TakeCloseSession( void ) ;
	void TakePendingSession( void ) ;

	size_t GetSessionCount( void ) {   return m_sessionCount ;   }
	uint64 GetUpdateUseTimeMS( void ) {   return m_updateTimeMS ;   }

	// 上次更新距今的时间间隔
	uint64 GetPerUpdateTime( void );
	void SetPerUpdateTime( uint64 val );
	uint64 GetPerUpdateTimeLag( void );

	void SetNotRecvPackLimitTimeMS( uint64 time ) {  m_notRecvPackLimitTimeMS =time ;   }
	void SetThreadUpdateLimitTimeMS( uint64 time ) { m_threadUpdateLimitTimeMS = time; }
	void SetThreadUpdateNoticeTimeMS( uint64 time ) { m_threadUpdateNoticeTimeMS =time ;}

	virtual Session* GetSession( uint32 sessionId )
	{
		SessionMapItr itr =m_sessions.find( sessionId ) ;
		return ( itr != m_sessions.end() ? itr->second : NULL ) ;
	}

	bool GetIsWaitingStop( void ) { return m_waitingStop; }
	void SetIsWaitingStop( bool val ) { m_waitingStop = val; }

protected:
	volatile bool        m_waitingStop;
	CThread              m_logicalThread ;

	volatile size_t      m_sessionCount ;
	volatile uint64      m_updateTimeMS ;
	volatile uint64      m_preUpdateTimeMS ;
	uint64               m_notRecvPackLimitTimeMS ;
	uint64               m_threadUpdateLimitTimeMS;
	uint64               m_threadUpdateNoticeTimeMS ;

	SessionMap           m_sessions ;

	DLockSessionQueue    m_pendingSession ;
	DLockSessionIdQueue  m_closeloseQueue ;
} ;


#endif
