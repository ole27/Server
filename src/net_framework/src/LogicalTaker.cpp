

#include "LogicalTaker.h"

#include "ConsoleLog.h"
#include "OS.h"
#include "Session.h"

#include "def/MmoAssert.h"
#include "utility/STLUtility.h"

#include "Opcode.pb.h"


LogicalTaker::LogicalTaker( void )
{
	m_sessionCount            =0 ;
	m_updateTimeMS            =0 ;
	m_preUpdateTimeMS         =0 ;
	m_notRecvPackLimitTimeMS  =5000 ;
	m_threadUpdateLimitTimeMS =100 ;
	m_threadUpdateNoticeTimeMS = 100;
	m_waitingStop = true ;
}


LogicalTaker::~LogicalTaker( void )
{
	Stop() ;

	//std::for_each( m_sessions.begin(), m_sessions.end(), DeleteSecondValue() ) ;
	for( SessionMap::iterator itr =m_sessions.begin(); itr != m_sessions.end(); ++itr )
	{
		itr->second->SetIsVerification( false ) ;
		delete itr->second ;
	}
	m_sessions.clear() ;

	Session *pSession =NULL ;
	while( m_pendingSession.Dequeue( pSession ) )
	{
		pSession->SetIsVerification( false ) ;
		delete pSession ;
		pSession =NULL ;
	}
}


void LogicalTaker::Start( size_t stackSize )
{
	m_logicalThread.Start( boost::bind( &LogicalTaker::LogicalThread, this ), stackSize ) ;
}

void LogicalTaker::Stop( void )
{
	m_logicalThread.Stop() ;
}

bool LogicalTaker::GetIsStart( void )
{
	return m_logicalThread.IsStart();
}


void LogicalTaker::StopNotWating( void )
{
	m_waitingStop = false;
	m_logicalThread.StopWithoutWaiting();
}

void LogicalTaker::LogicalThread( void )
{
	NLOG( "LogicalTaker::LogicalThread() start running ~" ) ;

	sOS.SRand() ;
	m_preUpdateTimeMS =sOS.TimeMS() ;

	while( m_logicalThread.IsStart() )
	{
		uint64 begTime = sOS.TimeMS() ;

		int64 twiceUpdateDiff =begTime - m_preUpdateTimeMS ;
		if( twiceUpdateDiff < 0 )
		{
			ELOG( "%s, twiceUpdateDiff %lld less than 0, now time is: %llu, pre time is: %llu ", __FUNCTION__, twiceUpdateDiff, begTime, m_preUpdateTimeMS );
			twiceUpdateDiff = 1;
		}

		sOS.SetLogicTimeMS( begTime );
		sOS.SetLogicTimeDiffMS( twiceUpdateDiff );

		m_preUpdateTimeMS     =begTime ;

		TakePendingSession() ;
		TakeCloseSession() ; // Tips: TakeCloseSession() must after TakePendingSession()

		BeforeSeesionUpdate( begTime, twiceUpdateDiff ) ;

		// Session update
		SessionMapItr itr ;
		SessionMapItr tmpItr ;

		for( itr =m_sessions.begin(); itr != m_sessions.end(); /*++itr*/ )
		{
			tmpItr =itr++ ;
			Session *pSession =tmpItr->second ;

			if( pSession->GetIsHadClosed() )
			{
				continue ;
			}

			if( pSession->GetIsNeedClose() )
			{
				NLOG( "LogicalThread initiative close session id: %u, type %s .", pSession->GetSessionId(), pSession->GetSessionTypeName() ) ;

				pSession->CloseSession() ;

				continue ;
			}


			pSession->Update( begTime, twiceUpdateDiff ) ;

			if( pSession->GetPreRecvTime() + m_notRecvPackLimitTimeMS < begTime )
			{
				int64 diffTime =begTime - pSession->GetPreRecvTime() ;
				WLOG( "LogicalThread, Will be closed id: %u, type:%s, %lld not packet recv more than %llu MS !!!!", pSession->GetSessionId(), pSession->GetSessionTypeName(), diffTime, m_notRecvPackLimitTimeMS ) ;
				pSession->CloseSession() ;
			}
		}

		// After session update ;
		AfterSessionUpdate( begTime, twiceUpdateDiff ) ;

		int64 updateUseTime =sOS.TimeMS() - begTime ;
		m_updateTimeMS = updateUseTime ;

		int64 diffTime = m_threadUpdateLimitTimeMS - m_updateTimeMS ;
		if( diffTime > 0 )
		{
			m_logicalThread.Sleep( static_cast<int>( diffTime ) ) ;
		}
		else if( m_updateTimeMS > m_threadUpdateNoticeTimeMS )
		{
			WLOG( "LogicalThread update time more than %llu ms use %llu ms", m_threadUpdateNoticeTimeMS, m_updateTimeMS ) ;
		}
	}

	m_logicalThread.SetIsStoped( true );
	NLOG( "LogicalThread is shoped!!!!" ) ;
}


void LogicalTaker::BeforeSeesionUpdate( uint64 nowTimeMS, int64 diffMS )
{
	ASSERT( false ) ;
}

void LogicalTaker::AfterSessionUpdate( uint64 nowTimeMS, int64 diffMS )
{
	ASSERT( false ) ;
}



void LogicalTaker::PutCloseSessionId( uint32 sessionId )
{
	m_closeloseQueue.Enqueue( sessionId ) ;
}

void LogicalTaker::TakeCloseSession( void )
{
	SessionId sessionId =0 ;
	while( m_closeloseQueue.Dequeue( sessionId ) )
	{
		SessionMapItr itr =m_sessions.find( sessionId ) ;
		if( itr != m_sessions.end() )
		{
			delete itr->second ;
			itr->second =NULL ;
			m_sessions.erase( itr ) ;
			m_sessionCount =m_sessions.size() ;
		}

	}
}

void LogicalTaker::PutSeesion( Session *pSession )
{
	m_pendingSession.Enqueue( pSession ) ;
}

void LogicalTaker::TakePendingSession( void )
{
	Session *pSession =NULL ;
	while( m_pendingSession.Dequeue( pSession ) )
	{
		m_sessions.insert( std::make_pair( pSession->GetSessionId(), pSession ) ) ;
		m_sessionCount =m_sessions.size() ;
	}
}

uint64 LogicalTaker::GetPerUpdateTime( void )
{
	return m_preUpdateTimeMS;
}

void LogicalTaker::SetPerUpdateTime( uint64 val )
{
	m_preUpdateTimeMS = val;
}


uint64 LogicalTaker::GetPerUpdateTimeLag( void )
{
	int64 diff = sOS.TimeMS() - m_preUpdateTimeMS;
	return diff < 0 ? ::abs( diff ) : diff ;
}


