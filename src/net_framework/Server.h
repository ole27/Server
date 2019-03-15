#pragma once

#include <conio.h>
#include <boost/smart_ptr.hpp>

#include "config/ConfigMgr.h"
#include "LogicalTaker.h"

#include "Thread.h"
#include "NetTypeDef.h"
#include "NetFrameworkDefines.h"

#include "NetPackClientCount.h"
#include "NetPackServerCount.h"

#include "google/protobuf/message.h"

class NetPack;
class Session ;
class TcpAsyncConn ;
class TcpAsyncServer ;

#ifdef WIN32
#define kbhit _kbhit
#endif

class Server : public INetPackHandler
{
public:
	Server( void ) ;
	virtual ~Server( void ) ;

	void ClearAllSession( void );

public:
	virtual void OnConnect( TcpAsyncConn *pConn ) ;
	virtual void OnRecv( TcpAsyncConn *pConn, NetPackPtr &pPack ) ;
	virtual void OnSend( TcpAsyncConn *pConn, NetPackPtr &pPack ) ;
	virtual void OnClose( uint32 sessionId ) ;

public:
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// TcpServer 
	bool StartListenPort( bool isFixPoint, const std::string &port ) ;

	const std::string& GetNearIP( void );
	const std::string& GetFarIP( void ) ;
	const std::string& GetPort( void ) ;
	const AddressInfo* GetAddressInfo( void );

	const char* GetCharNearIP( void ) { return GetNearIP().c_str(); }
	const char* GetCharFarIP( void ) { return GetFarIP().c_str(); }
	const char* GetCharPort( void ) { return GetPort().c_str(); }

	char GetServerType( void ) {   return m_serverType ;   }
	void SetServerType( char type ) {   m_serverType =type ;   SetServerTypeName( m_serverType ) ;   }

	void SetServerTypeName( char type ) ;
	const char* GetServerTypeName( void ) {   return m_serverTypeName.c_str() ;   }

	void SetPackPrintDiffCount( uint64 count ) ;


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Thread
public:
	void Start( void ) ;
	virtual void OnBeforeStart( void );

	void Stop( void ) ;
	virtual void OnAfterStop( void ) ;

	void WorldThread( void ) ;
	
	virtual void BeforeSeesionUpdate( uint64 nowTimeMS, int64 diffMS ) ;
	virtual void AfterSessionUpdate( uint64 nowTimeMS, int64 diffMS ) ;
	virtual void OnSessonUpdate( Session *pSession ) {}

	boost::shared_ptr<LogicalTaker>& GetLogicalTaker( void ) {   return m_pLogicalTaker ;   }
	void SetLogicalTaker( LogicalTaker *pLogicalTaker ) {   m_pLogicalTaker.reset( pLogicalTaker ) ;   }

private:
	CThread                         m_worldThread ;
	boost::shared_ptr<LogicalTaker> m_pLogicalTaker ;


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Config
public:
	const ConfigMgr& Config() { return m_config; }

	ConfigMgr& MutableConfig() { return m_config; }

	bool ReLoadGameConfig( void ) ;

	virtual bool InitGameConf( const std::string &filePath, const std::string &strConfigFile, const std::string &strDefaultConfigFile ) ;

	bool GetIsNeedReloadConfig( void ) {    return m_isNeedReloadConfig ;    }
	void SetIsNeedReloadConfig( bool val ) {    m_isNeedReloadConfig =val ;    }

	bool GetIsFixListenPort( void );

	ServerId GetServerId( void ) { return m_serverId; }
	void SetServerId( ServerId id ) { m_serverId = id; }

	ServerId GetRegionId( void ) { return m_regionId; }
	void SetRegionId( ServerId id ) { m_regionId = id; }

	virtual void SettingMemCache( void );
	virtual void ShowMemCacheCount( void );
	static void ReleaseMemCache( void );

	void SetIsRun( bool val ) { m_isRuning = val; }
	bool GetIsRun( void ) { return m_isRuning ; }

	uint32 GetPlatformId( void ) { return m_platformId; }
	void SetPlatformId( uint32 id ) { m_platformId =id; }

private:
	bool        m_isNeedReloadConfig ;
	ConfigMgr   m_config;
	ServerId    m_serverId;
	ServerId    m_regionId;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Packet
public:
	void PostInputPacket( TcpAsyncConn *pConn, NetPackPtr pPack ) ;

	void NoticeRecv( TcpAsyncConn *pConn, NetPackPtr &pPack );
	void NoticeSend( TcpAsyncConn *pConn, NetPackPtr &pPack ) ;


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Session
public:
	size_t GetAllSessionSize( void )
	{
		return m_sessions.size() ;
	}

	size_t GetUpdateSessionSize( void )
	{
		return m_updateSessionMap.size() ;
	}

	uint64 GetUpdateUseTimeMS( void ) {   return m_updateTimeMS ;   }

	void OnOpenSession( TcpAsyncConn *pConn ) ;
	void OnCloseSession( int sessionId ) ;

	virtual Session* CreateSesion( int type ) =0 ;

	bool AddToLogicalTacker( Session *pSession ) ;
	virtual bool ChangeToLogicalTacker( Session *pSession ) ;

	bool AddSession( Session *pSession ) ;

	Session* GetSession( SessionId sessionId )
	{
		SessionMapItr itr =m_sessions.find( sessionId ) ;
		return ( itr != m_sessions.end() ? itr->second : NULL ) ;
	}

	void SetVerfyLimitTime( uint64 time ) {   m_verfyLimitTimeMS =time ;   }

	void SetThreadUpdateLimitTimeMS( uint64 time ) {   m_threadUpdateLimitTimeMS =time ;   }
	uint64 GetThreadUpdateLimitTimeMS( void ) { return m_threadUpdateLimitTimeMS; }

	void SetNotRecvPackLimitTimeMS( uint64 time ) { m_notRecvPackLimitTimeMS = time; }
	uint64 GetNotRecvPackLimitTimeMS( void ) {   return m_notRecvPackLimitTimeMS ;   }

	void SetThreadUpdateNoticeTimeMS( uint64 time ) { m_threadUpdateNoticeTimeMS = time; }
	uint64 GetThreadUpdateNoticeTimeMS( void ) { return m_threadUpdateNoticeTimeMS; }

	void SetJustConnectLimintTimeMS( uint64 time ) ; 

	void BroadcastToUpdateSession( NetPack &packet )
	{
		BroadcastBySessionMap( m_updateSessionMap, packet ) ;
	}

	template< class Fun >
	void ForAllSession( const Fun &fun )
	{
		for( SessionMapItr itr = m_sessions.begin(); itr != m_sessions.end(); ++itr )
		{
			fun( itr->second );
		}
	}
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Common data
protected:
	Session* GetFromSessionMap( SessionMap &sessionMap, SessionId id ) ;
	bool AddSeesionToSeerionMap( SessionMap &sessionMap, Session *pSession ) ;
	bool RemoveSessionFromSessionMap( SessionMap &sessionMap, Session *pSession ) ;

	void BroadcastBySessionMap( SessionMap &sessionMap, NetPack &packet ) ;
	void BroadcastBySessionMap( SessionMap &sessionMap, const int opCode, const ::google::protobuf::Message &msg ) ;
	void BroadcastBySessionMap( SessionMap &sessionMap, const int opCode, const int sessionId, const ::google::protobuf::Message &msg ) ;
	void BroadcastBySessionMap( SessionMap &sessionMap, const int opCode, const int sessionId, const int packetType, const ::google::protobuf::Message &msg ) ;

private:
	void TakeNewSession( void ) ;
	void TakeCloseSession( void ) ;

private:
	bool                                  m_isListening;
	char                                  m_serverType ;
	uint32                                m_takeNewSessionLimit ;
	volatile uint64                       m_updateTimeMS ;
	uint64                                m_preUpdateTimeMS ;
	uint64                                m_verfyLimitTimeMS ;
	uint64                                m_notRecvPackLimitTimeMS ;
	uint64                                m_threadUpdateLimitTimeMS;
	uint64                                m_threadUpdateNoticeTimeMS;
	uint32                                m_platformId;

	std::string                           m_serverTypeName ;

	SessionMap                            m_sessions ;
	SessionMap                            m_updateSessionMap ;

	MLockSessionQueue                     m_newSession ;
	MLockSessionIdQueue                   m_closeQueue ;

	boost::scoped_ptr<TcpAsyncServer>     m_pTcpSvr ;

	volatile bool                         m_isRuning;
	//NetPackClientCount                    m_inClientNetStat ;
	//NetPackClientCount                    m_outClientNetStat ;

	//NetPackServerCount                    m_inServerNetStat ;
	//NetPackServerCount                    m_outServerNetStat ;
};

