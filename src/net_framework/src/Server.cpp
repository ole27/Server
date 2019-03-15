
#include "Server.h"

#include "OS.h"
#include "SConfig.h"
#include "ConsoleLog.h"
#include "Session.h"
#include "TcpAsyncServer.h"
#include "IoServicePool.h"
#include "ServerClient.h"

#include "def/MmoAssert.h"
#include "utility/STLUtility.h"
#include "TcpAsyncConn.h"

#include "Opcode.pb.h"
#include "ServerOpcode.pb.h"


Server::Server(void)
	//:m_inClientNetStat( pb::pb_OpcodeMsgCount, "Client Input" )
	// ,m_outClientNetStat( pb::pb_OpcodeMsgCount, "Client Output" )
	// ,m_inServerNetStat( pb::SERVER_OPCODE_COUNT, "Server Input" )
	// ,m_outServerNetStat( pb::SERVER_OPCODE_COUNT, "Server Output" )
{
	m_serverType               =0 ;
	m_takeNewSessionLimit      =100 ;
	m_updateTimeMS             =0 ;
	m_preUpdateTimeMS          =0 ;

	m_verfyLimitTimeMS         =5000 ;
	m_notRecvPackLimitTimeMS   =5000 ;
	m_threadUpdateLimitTimeMS  =100 ;
	m_threadUpdateNoticeTimeMS = 100;

	m_sessions.clear() ;
	m_updateSessionMap.clear() ;

	m_isNeedReloadConfig =false ;
	m_isListening = false;

	m_pTcpSvr.reset( new TcpAsyncServer() );

	m_serverId = 0;
	m_regionId = 0;
	m_platformId = 0;

	m_isRuning = false;
}

Server::~Server(void)
{
	Stop() ;

	ClearAllSession();
}

void Server::ClearAllSession( void )
{
	//std::for_each( m_updateSessonMap.begin(), m_updateSessonMap.end(), DeleteSecondValue() ) ;
	for( SessionMap::iterator itr = m_updateSessionMap.begin(); itr != m_updateSessionMap.end(); ++itr )
	{
		itr->second->SetIsVerification( false );
		delete itr->second;
	}

	Session *pSession = NULL;
	while( m_newSession.Dequeue( pSession ) )
	{
		pSession->SetIsVerification( false );
		delete pSession;
		pSession = NULL;
	}

	m_updateSessionMap.clear();
}

void Server::OnConnect( TcpAsyncConn *pConn )
{
	ASSERT( pConn != NULL ) ;
	NLOG("Session(%d), IP:%s(%d), connected", pConn->Id(), pConn->Ip().c_str(), pConn->Port() ) ;

	OnOpenSession( pConn ) ;
}

void Server::NoticeRecv( TcpAsyncConn *pConn, NetPackPtr &pPack )
{
	if( pConn == NULL )
	{
		return;
	}

	const uint32 id = pConn->Id();
	const char *pIp = pConn->Ip().c_str();
	const int port = pConn->Port();

	if( NetPack::IsNoticeRecvAndSend( *pPack ) )
	{
		const char *pPackName = pPack->GetPacketTypeName();
		const char *pServerName = GetServerTypeName();
		const char *pOpName = pPack->GetOpcodeName();
		const int opVal = pPack->GetOpcode();

		const uint64 accountId = pPack->GetAccountId();
		const uint64 playerId = pPack->GetPlayerId();
		const uint32 packetSessionId = pPack->GetSessionId();
		if( playerId != 0 || accountId != 0 )
		{
			RecvLOG( "%s to %s id:%u(%s:%d) %s(%d) psId:%u acc:%llu plr:%llu."
				, pPackName, pServerName, id, pIp, port, pOpName, opVal, packetSessionId, accountId, playerId );
		}
		else
		{
			RecvLOG( "%s to %s id:%u(%s:%d) %s(%d) psId:%u.", pPackName, pServerName, id, pIp, port, pOpName, opVal, packetSessionId );
		}
		//m_inServerNetStat.Add(*pPack);
	}

}

void Server::OnRecv( TcpAsyncConn *pConn, NetPackPtr &pPack)
{
	NoticeRecv( pConn, pPack ) ;
	PostInputPacket( pConn, pPack ) ;
}

void Server::NoticeSend( TcpAsyncConn *pConn, NetPackPtr &pPack )
{
	if( pConn == NULL )
	{
		return;
	}

	const uint32 id = pConn->Id();
	const char *pIp = pConn->Ip().c_str();
	const int port = pConn->Port();

	if( NetPack::IsNoticeRecvAndSend( *pPack ) )
	{
		const char *pPackName = pPack->GetPacketTypeName();
		const char *pServerName = GetServerTypeName();
		const char *pOpName = pPack->GetOpcodeName();
		const int opVal = pPack->GetOpcode();

		const uint64 accountId = pPack->GetAccountId();
		const uint64 playerId = pPack->GetPlayerId();
		const uint32 packetSessionId = pPack->GetSessionId();
		if( playerId != 0 || accountId != 0 )
		{
			SendLOG( "%s to %s id:%u(%s:%d) %s(%d) psId:%u acc:%llu plr:%llu."
				, pServerName, pPackName, id, pIp, port, pOpName, opVal, packetSessionId, accountId, playerId );
		}
		else
		{
			SendLOG( "%s to %s id:%u(%s:%d) %s(%d) psId:%u.", pServerName, pPackName, id, pIp, port, pOpName, opVal, packetSessionId );
		}
		//m_outServerNetStat.Add(pack);

	}
}

void Server::OnSend( TcpAsyncConn *pConn, NetPackPtr &pPack)
{
	ASSERT( false ) ; // will not send by server, just by each session

	NoticeSend( pConn, pPack ) ;
}

void Server::OnClose( uint32 sessionId )
{
	// Notice:
	// Just when the session connect but had not recv first packet
	// that had not invode Session::SetConn() so the TcpAsyncConn::SetPackHandler()
	// not be invoked to reset use session as handler the packet, close event will come here .
	// 
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TcpServer 

const std::string& Server::GetNearIP( void )
{
	return m_pTcpSvr->GetNearIP();
}

const std::string& Server::GetFarIP( void )
{
	return m_pTcpSvr->GetFarIP();
}


const std::string& Server::GetPort( void )
{
	return m_pTcpSvr->GetPort() ;
}

const AddressInfo* Server::GetAddressInfo( void )
{
	return m_pTcpSvr->GetAddressInfo();
}

void Server::SetServerTypeName( char type )
{
	m_serverTypeName =NetPack::GetPacketTypeName( type ) ;
}


void Server::SetPackPrintDiffCount( uint64 count )
{
	//m_inClientNetStat.SetPrintDiffCount( count ) ;
	//m_outClientNetStat.SetPrintDiffCount( count ) ;
	//m_inServerNetStat.SetPrintDiffCount( count ) ;
	//m_outServerNetStat.SetPrintDiffCount( count ) ;
}

bool Server::StartListenPort( bool isFixPoint, const std::string &port )
{
	if( m_isListening )
	{
		return true;
	}

	// Start listen
	std::string nearIp = Config().GetString( "Self.fix_near_ip", "" );
	std::string farIp = Config().GetString( "Self.fix_far_ip", "" );

	if( !m_pTcpSvr->StartListen( port, nearIp, farIp ) )
	{
		if( isFixPoint )
		{
			return false;
		}

		std::stringstream sstr;
		int beginPort = ::atoi( port.c_str() );

		do
		{
			sstr.str( "" );
			sstr << ++beginPort;
			NLOG( "Starting network service try listen port:%d ....", beginPort );

		} while( !m_pTcpSvr->StartListen( sstr.str(), nearIp, farIp ) );
	}

	SetJustConnectLimintTimeMS( Config().GetInt( "Self.wait_first_packet_limit_time_ms", 3000 )  ) ;

	m_pTcpSvr->Start( this );
	m_isListening = true;

	return true ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Thread
void Server::Start( void )
{
#ifdef WIN32
	char title[ MAX_PATH ] = "";
	int titleLen = GetConsoleTitle( &title[ 0 ], MAX_PATH );

	std::string newTitle( &title[ 0 ], titleLen );
	newTitle.append( "_" );
	newTitle.append( GetPort() );
	SetConsoleTitle( newTitle.c_str() );
#endif

	sLog.SetNewFileFistLog( GetPort() );

	OnBeforeStart() ;

	size_t stackSize = Config().GetInt( "StackMemory.world_thread", 1048576 ) ;
	m_worldThread.Start( boost::bind( &Server::WorldThread, this ), stackSize ) ;

	//ASSERT( m_pLogicalTaker ) ;
	if( m_pLogicalTaker )
	{
		stackSize = Config().GetInt( "StackMemory.logic_thread", 1048576 );
		m_pLogicalTaker->Start( stackSize ) ;
	}
}

void Server::OnBeforeStart( void )
{
	// 东八区时间差
	sOS.AddCheatTime( 8 * 60 * 60 );
}


void Server::Stop( void )
{
	m_isListening = false;

	m_worldThread.Stop() ;

	if( m_pLogicalTaker )
	{
		if( m_pLogicalTaker->GetIsWaitingStop() )
		{
			m_pLogicalTaker->Stop();
		}
		else
		{
			m_pLogicalTaker->StopNotWating();
		}
	}

	if (m_pTcpSvr.get())
	{
		m_pTcpSvr->Stop();
		
		while( m_pTcpSvr->GetIsOpen() )
		{
			// wait ..........
		}
	}

	OnAfterStop();
}


void Server::OnAfterStop( void )
{
	for( SessionMapItr itr =m_sessions.begin(); itr != m_sessions.end(); ++itr )
	{
		Session *pSession =itr->second ;
		pSession->ServerClose() ;
	}
}


void Server::WorldThread( void )
{
	NLOG( "Server::WorldThread() start running ~" ) ;

	sOS.SRand() ;
	m_preUpdateTimeMS =sOS.TimeMS() ;

	while( m_worldThread.IsStart() )
	{
		uint64 begTime = sOS.TimeMS() ;

		int64 twiceUpdateDiff =begTime - m_preUpdateTimeMS ;
		if( twiceUpdateDiff < 0 )
		{
			ELOG( "%s, twiceUpdateDiff %lld less than 0, now time is: %llu, pre time is: %llu ", __FUNCTION__, twiceUpdateDiff, begTime, m_preUpdateTimeMS );
			twiceUpdateDiff = 1;
		}

		m_preUpdateTimeMS     =begTime ;

		// Open and close sesssion .
		TakeNewSession() ;
		TakeCloseSession() ;

		BeforeSeesionUpdate( begTime, twiceUpdateDiff ) ;

		// Session update
		SessionMapItr itr ;
		SessionMapItr tmpItr ;

		for( itr =m_updateSessionMap.begin(); itr != m_updateSessionMap.end(); /*++itr*/ )
		{
			tmpItr =itr++ ;
			Session *pSession =tmpItr->second ;

			if( pSession->GetIsHadClosed() )
			{
				continue ;
			}

			if( pSession->GetIsNeedClose() )
			{
				NLOG( "WorldThread initiative close session %d, type %s .", pSession->GetSessionId(), pSession->GetSessionTypeName() ) ;

				pSession->CloseSession() ;

				continue ;
			}

			pSession->Update( begTime, twiceUpdateDiff ) ;

			OnSessonUpdate( pSession ) ;

			if( pSession->GetPreRecvTime() + m_notRecvPackLimitTimeMS < begTime )
			{
				int64 diffTime =begTime - pSession->GetPreRecvTime() ;
				WLOG( "WorldThread, Will be closed id:%d, type:%s,%lld not packet recv more than %lld MS !!!!", pSession->GetSessionId(), pSession->GetSessionTypeName(), diffTime, m_notRecvPackLimitTimeMS ) ;
				pSession->CloseSession() ;
			}
			else if( !pSession->GetIsVerification() )
			{
				if( pSession->GetWaitVerfyTime() + m_verfyLimitTimeMS < begTime )
				{
					NLOG( "WorldThread, More than %llu MS not verfy initiative close session %u, type %s .", m_verfyLimitTimeMS, pSession->GetSessionId(), pSession->GetSessionTypeName() ) ;
					pSession->CloseSession() ;
				}
			}

		}

		// After session update ;
		AfterSessionUpdate( begTime, twiceUpdateDiff ) ;

		int64 updateUseTime =sOS.TimeMS() - begTime ;
		if( updateUseTime < 0 )
		{
			ELOG( "%s, sOS.TimeMS() - begTime =%lld is error", __FUNCTION__, updateUseTime );
			updateUseTime = 1;
		}

		m_updateTimeMS = updateUseTime ;

		int64 diffTime = m_threadUpdateLimitTimeMS - m_updateTimeMS ;
		if( diffTime > 0 )
		{
			m_worldThread.Sleep( static_cast<int>( diffTime ) ) ;
		}
		else if( m_updateTimeMS > m_threadUpdateNoticeTimeMS )
		{
			WLOG( "WorldThread update time more than %llu ms use %llu ms", m_threadUpdateNoticeTimeMS, m_updateTimeMS ) ;
		}
	}

	m_worldThread.SetIsStoped( true );
	WLOG( "WorldThread is shoped!!!!" ) ;

}


void Server::BeforeSeesionUpdate( uint64 nowTimeMS, int64 diffMS )
{
	ASSERT( false ) ;
}

void Server::AfterSessionUpdate( uint64 nowTimeMS, int64 diffMS )
{
	ASSERT( false ) ;
}


bool Server::ReLoadGameConfig( void )
{
	return InitGameConf( m_config.GetPath(), m_config.GetConfigFile(), m_config.GetConfigFile() ) ;
}


bool Server::InitGameConf( const std::string &filePath, const std::string &strConfigFile, const std::string &strDefaultConfigFile )
{
	if( !m_config.LoadOrCopyFormDefaultFile( filePath, strConfigFile, strDefaultConfigFile ) )
	{
		return false ;
	}

	NetPack::ClientOpCodeNoticeInit() ;
	NetPack::ServerOpCodeNoticeInit() ;

	SetVerfyLimitTime( Config().GetInt( "Self.verfy_limit_time_ms", 5000 )  ) ;

	SetThreadUpdateLimitTimeMS( Config().GetInt( "Self.thread_update_limit_time_ms", 100 ) ) ;
	SetNotRecvPackLimitTimeMS( Config().GetInt( "Self.not_recv_packet_limit_time_ms", 3000 ) );
	SetThreadUpdateNoticeTimeMS( Config().GetInt( "Self.thread_update_notice_time_ms", 100 ) ) ;
	if( GetLogicalTaker() )
	{
		GetLogicalTaker()->SetThreadUpdateLimitTimeMS( GetThreadUpdateLimitTimeMS() );
		GetLogicalTaker()->SetNotRecvPackLimitTimeMS( GetNotRecvPackLimitTimeMS() );
		GetLogicalTaker()->SetThreadUpdateNoticeTimeMS( GetThreadUpdateNoticeTimeMS() );
	}

	Session::SetTakePacketLimit( Config().GetInt( "Self.session_take_packet_limit", 10 ) ) ;

	Session::SetDelayNoticeLimitTimeMS( Config().GetInt( "Self.delay_notice_limie_time_ms", 10 ) ) ;
	ServerClient::SetDelayNoticeLimitTimeMS( Config().GetInt( "Self.delay_notice_limie_time_ms", 10 ) ) ;

	SetIsNeedReloadConfig( false ) ;
	return true ;
}

bool Server::GetIsFixListenPort( void )
{
	return sConfig.GetInt( "Server.fix_listen_port", 1 ) != 0 ;
}

void Server::SettingMemCache( void )
{
}

void Server::ShowMemCacheCount( void )
{
	NLOG( "Show memory cache ( number, size(K) )" );
	for( ObjPoolMgr::ObjPoolTableItr itr = sObjPool.m_memPools.begin(); itr != sObjPool.m_memPools.end(); ++itr )
	{
		ObjPoolBase *pPool = itr->second;
		ILOG( "%s, use(%u:%0.3f), not use(%u, %0.1f)", itr->first.c_str(), pPool->GetHadUseCount(), pPool->GetHadUseByteSize() / 1024.0f, pPool->GetCacheCount(), pPool->GetCacheByteSize() / 1024.0f );
	}

	ILOG( "All session count: %u", GetAllSessionSize() );
}


void Server::ReleaseMemCache( void )
{
	NLOG( "Release memory cache..............." );
	for( ObjPoolMgr::ObjPoolTableItr itr = sObjPool.m_memPools.begin(); itr != sObjPool.m_memPools.end(); ++itr )
	{
		if( itr->first.find( "Logger" ) != std::string::npos )
		{
			continue;
		}

		ObjPoolBase *pPool = itr->second;
		pPool->ClearObj( 0, true );
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session

void Server::OnOpenSession( TcpAsyncConn *pConn )
{
	ASSERT( pConn != NULL ) ;
}

void Server::TakeNewSession( void )
{
	Session *pSession =NULL ;

	uint64 nowTimeMS =sOS.TimeMS() ;

	while( m_newSession.Dequeue( pSession ) )
	{
		pSession->SetWaitVerfyTime( nowTimeMS ) ;

		SessionId sessionId =pSession->GetSessionId() ;
		bool sessionIsInsert =m_sessions.insert( std::make_pair( sessionId, pSession ) ).second ;
		if( !sessionIsInsert )
		{
			ELOG( "session id %u is exist in session map", sessionId ) ;
			ASSERT( false ) ;

			continue ;
		}

		m_updateSessionMap.insert( std::make_pair( pSession->GetSessionId(), pSession ) ) ;
	}
}


void Server::OnCloseSession( int sessionId )
{
	m_closeQueue.Enqueue( sessionId ) ;
}


bool Server::AddToLogicalTacker( Session *pSession )
{
	if( !pSession->GetIsInWorldThread() )
	{
		return false ;
	}

	pSession->SetIsInWorldThread( false ) ;
	m_pLogicalTaker->PutSeesion( pSession ) ;

	return true ;

}


bool Server::ChangeToLogicalTacker( Session *pSession )
{
	if( !pSession->GetIsInWorldThread() )
	{
		return false ;
	}

	pSession->SetIsInWorldThread( false ) ;
	m_updateSessionMap.erase( pSession->GetSessionId() ) ;
	m_pLogicalTaker->PutSeesion( pSession ) ;

	return true ;
}


bool Server::AddSession( Session *pSession )
{
	pSession->SetIsVerification( false ) ;
	pSession->SetIsInWorldThread( true ) ;

	switch( pSession->GetSessionType() )
	{
	case PACKET_TYPE_SERVER_LOGIN:
	case PACKET_TYPE_SERVER_GAME:
	case PACKET_TYPE_SERVER_DBPROXY:
	case PACKET_TYPE_SERVER_CENTER:
	case PACKET_TYPE_SERVER_GATE:
	case PACKET_TYPE_SERVER_EVENT:
	case PACKET_TYPE_SERVER_ROBOT:
	case PACKET_TYPE_SERVER_HTTP_ADDRESS:
	case PACKET_TYPE_SERVER_AID:
		{
			pSession->SetIsServerSession( true ) ;
		} break ;

	case PACKET_TYPE_CLIENT:
	case PACKET_TYPE_SERVER_WEB:
		{
			pSession->SetIsServerSession( false ) ;
		} break ;

	default:
		ASSERT( false ) ;
		return false;
	}

	pSession->SetPreRecvTime( sOS.TimeMS() ) ;

	m_newSession.Enqueue( pSession ) ;

	return true;
}


void Server::TakeCloseSession( void )
{
	uint32 sessionId =0 ;
	while( m_closeQueue.Dequeue( sessionId ) )
	{
		if( m_sessions.erase( sessionId ) <= 0 )
		{
			WLOG( "Server::TakeCloseSession() can not find sessiion %d in m_sessions ", sessionId ) ;
			continue ;
		}

		SessionMapItr itr =m_updateSessionMap.find( sessionId ) ;
		if( itr != m_updateSessionMap.end() )
		{
			delete itr->second ;
			itr->second =NULL ;

			m_updateSessionMap.erase( itr ) ;
		}
		else
		{
			assert( m_pLogicalTaker != NULL ) ;
			m_pLogicalTaker->PutCloseSessionId( sessionId ) ;
		}
	}
}



void Server::PostInputPacket( TcpAsyncConn *pConn, NetPackPtr pPack )
{
	ASSERT( pConn != NULL ) ;
	// Notice:
	// just first packet will come here, after first packet will change to session self recv packet .
	//

	pConn->SetIsGetPacket( true ) ;

	uint16 maxOpCode =pPack->IsClientPacket() ? static_cast<uint16>( pb::pb_OpcodeMsgCount ) : static_cast<uint16>( pb::SERVER_OPCODE_COUNT ) ;
	if( pPack->GetOpcode() >= maxOpCode )
	{
		ELOG( "Unhandled opCode:%d, more than max opcde ", pPack->GetOpcode(), maxOpCode );
		return;
	}

	int type =pPack->GetPacketType() ;
	Session *pSession =CreateSesion( type ) ;
	if( pSession == NULL )
	{
		// the connect will be close by m_mtJustConnectList .
		ELOG( "Receive error not accept session type, will close it, somebody is attacking ?????? " ) ;
		return ;
	}

	pSession->PostInputPacket( pPack ) ;

	// Set the conn to session, and change the packet handler .
	pSession->SetConn( pConn ) ;

	pSession->SetServer( this ) ;
	pSession->SetSessionId( pConn->Id() ) ;
	pSession->SetSessionType( type ) ;

	if( !AddSession( pSession ) )
	{
		delete pSession ;
		pSession =NULL ;
		ELOG( "%s add session faild ", __FUNCTION__ );
		return ;
	}
}

void Server::SetJustConnectLimintTimeMS( uint64 time )
{
	ASSERT( m_pTcpSvr ) ;
	m_pTcpSvr->SetJustConnectLimintTimeMS( time ) ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Common data
Session* Server::GetFromSessionMap( SessionMap &sessionMap, SessionId id )
{
	SessionMap::iterator itr =sessionMap.find( id ) ;
	return ( itr != sessionMap.end() ? itr->second : NULL ) ;
}

bool Server::AddSeesionToSeerionMap( SessionMap &sessionMap, Session *pSession )
{
	bool isInsert =sessionMap.insert( std::make_pair( pSession->GetSessionId(), pSession ) ).second ;
	ASSERT( isInsert ) ;

	return isInsert ;
}

bool Server::RemoveSessionFromSessionMap( SessionMap &sessionMap, Session *pSession )
{
	int rmCount =sessionMap.erase( pSession->GetSessionId() ) ;
	ASSERT( rmCount == 1 ) ;

	return rmCount > 0 ;
}

void Server::BroadcastBySessionMap( SessionMap &sessionMap, NetPack &packet )
{
	for( SessionMapItr itr =sessionMap.begin(); itr != sessionMap.end(); ++itr )
	{
		NetPack tmpPacket( packet ) ;
		itr->second->Send( tmpPacket ) ;
	}
}

void Server::BroadcastBySessionMap( SessionMap &sessionMap, const int opCode, const ::google::protobuf::Message &msg )
{
	for( SessionMapItr itr =sessionMap.begin(); itr != sessionMap.end(); ++itr )
	{
		itr->second->Send( opCode, msg ) ;
	}
}

void Server::BroadcastBySessionMap( SessionMap &sessionMap, const int opCode, const int sessionId, const ::google::protobuf::Message &msg )
{
	for( SessionMapItr itr =sessionMap.begin(); itr != sessionMap.end(); ++itr )
	{
		itr->second->Send( opCode, sessionId, msg ) ;
	}
}

void Server::BroadcastBySessionMap( SessionMap &sessionMap, const int opCode, const int sessionId, const int packetType, const ::google::protobuf::Message &msg )
{
	for( SessionMapItr itr =sessionMap.begin(); itr != sessionMap.end(); ++itr )
	{
		itr->second->Send( opCode, sessionId, packetType, msg ) ;
	}

}
