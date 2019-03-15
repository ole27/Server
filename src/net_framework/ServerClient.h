
#ifndef SERVER_CLIENT_H__
#define SERVER_CLIENT_H__

#include <string>

#include <boost/atomic.hpp>
#include <boost/smart_ptr.hpp>

#include "StopWatch.h"

#include "NetTypeDef.h"
#include "TcpAsyncClient.h"
#include "NetFrameworkDefines.h"

#include "def/TypeDef.h"
#include "google/protobuf/message.h"

class NetPack ;
class Server;

namespace pb
{
	class CmsgPing ;
	class CmsgPong ;
	class RegisterInfo;
}

class ServerClient : public INetPackHandler
{
public:
	ServerClient ( void ) ;
	virtual ~ServerClient( void ) ;

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// virtual 
private:
	virtual void OnConnect( TcpAsyncConn *pConn ) ;
	virtual void OnRecv( TcpAsyncConn *pConn, NetPackPtr &pPack ) ;
	virtual void OnSend( TcpAsyncConn *pConn, NetPackPtr &pPack ) ;
	virtual void OnClose( uint32 sessionId ) ;
	virtual void OnClientConnectError( int errValue, const std::string &errMsg ) ;
	virtual void TakePacket( const uint64 &nowTimeMS, const int64 &diffMS ) ;

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// 
public:
	void Stop( void ) ;
	void Update( const uint64 &nowTimeMS, const int64 &diffMS ) ;

	bool GetOnePacket( NetPackPtr &pPack ) {   return m_packetTable.Dequeue( pPack ) ;   }

	virtual void ClientConnect( void ) {}
	virtual void ClientDisconnect( void ) {}
	virtual void ClientUpdate( uint64 nowTimeMS, int64 diffMS ) =0 ;
	virtual bool HandleNetPack( NetPackPtr &pPack, const uint64 &nowTimeMS, const int64 &diffMS ) =0 ;
	virtual bool IsValidSendPacketType( const char type ) ;
	virtual bool IsValidRecvPacketType( const char type ) ;

	void ReConnect( void )  ;

	void SetDstInfo( const std::string &host, const std::string &port ) ;

	const char* GetIP( void ) {   return m_host.c_str() ;   }
	const char* GetPort( void ) {   return m_port.c_str() ;   }

	bool IsSettingAddress( void ) {   return !m_host.empty() && !m_port.empty() ;   }

	bool Connect( const std::string& host, const std::string &port, std::string& errMsg ) ;

	bool Send( NetPack &packet ) ;
	
	virtual bool Send( NetPackPtr &pPack ) ;
	virtual bool SendPacketPtr( NetPackPtr &pPack ) {   return Send( pPack ) ;    }
	virtual bool Send( const int opCode, const ::google::protobuf::Message &msg ) ;
	virtual bool Send( const int opCode, const int sessionId, const ::google::protobuf::Message &msg ) ;
	virtual bool Send( const int opCode, const int sessionId, const char packetType, const ::google::protobuf::Message &msg ) ;

	bool SendMsg( const int opCode, const ::google::protobuf::Message *pMsg )
	{
		return ServerClient::Send( opCode, *pMsg );
	}

	bool SendSessionIdMsg( const int opCode, const int sessionId, const ::google::protobuf::Message *pMsg )
	{
		return ServerClient::Send( opCode, sessionId, *pMsg );
	}

	bool SendSessionIdPacketTypeMsg(  const int opCode, const int sessionId, const char packetType, const ::google::protobuf::Message *pMsg )
	{
		return ServerClient::Send( opCode, sessionId, packetType, *pMsg ) ;
	}

	inline bool CanSend( void ) {    return m_canSend ;    }
	inline void SetCanSend( bool canSend )
	{
		m_canSend =canSend ;
	}

	uint64 GetDelayTime( void ) {   return m_delayTime ;   }
	void SetDelayTime( uint64 time ) {   m_delayTime =time ;    }

	uint64 GetUpdateTime( void ) { return m_updateTime; }
	void SetUpdateTime( uint64 time ) { m_updateTime = time; }

	uint32 GetDstSessionCount( void ) { return m_dstSessionCount; }
	void SetDstSessionCount( uint32 count ) { m_dstSessionCount = count; }

	char GetClientType( void ) {   return m_clientType ;   }
	void SetClientType( char type ) ;

	char GetDstServerType( void ) {   return m_dstServetType ;   }
	void SetDstServerType( char type ) ;

	uint32 GetConnectCount( void ) {   return m_connectCount ;   }

	uint32 GetMaxTryConnectCount( void ) {   return m_maxTryConnectCount ;   }
	void SetMaxTryConnectCount( int count ) {   m_maxTryConnectCount =count ;   }

	const char* GetClientName( void ) {   return m_clientName.c_str() ;   }
	const char* GetServerName( void ) {   return m_dstServerName.c_str() ;   }

	void SetPingDiffMS( uint64 time ) {   m_pingWatch.SetMax( time ) ;  m_pingWatch.SetDone() ;   }

	void SetConnectNow( void ) {   m_reConnectWatch.SetDone() ;   }
	void SetReConnectDiffMS( uint64 time ) {   m_reConnectWatch.SetMax( time ) ;   }

	void WriteRegisterInfo( Server &server, pb::RegisterInfo &msg );

	ServerClinetId GetId( void ) { return m_id; }

	bool GetIsUse( void ) { return m_isUse; }
	void SetIsUse( bool val ) { m_isUse = val; }

private:
	bool                              m_isUse;
	char                              m_clientType ;
	char                              m_dstServetType ;

	std::string                       m_clientName ;
	std::string                       m_dstServerName ;

	std::string                       m_port ;
	std::string                       m_host ;
	uint64                            m_delayTime ;
	uint64                            m_updateTime;
	uint32                            m_dstSessionCount ;

	uint32                            m_connectCount ;
	uint32                            m_maxTryConnectCount ;

	boost::scoped_ptr<TcpAsyncClient> m_pTcpClient;
	volatile bool                     m_canSend ;

	StopWatch                         m_pingWatch ;
	StopWatch                         m_reConnectWatch ;

	ServerClinetId                    m_id;
	static ServerClinetId             s_id;

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Packet
public:
	virtual void SendPing( void ) =0 ;

	void SendPing( int pingOpCode, uint64 updateTimeMS, int32 sessionCount ) ;
	void SetPingBaseInfo( pb::CmsgPing &msg, uint64 updateTimeMS, int32 sessionCount ) ;

	void HandlePong( NetPack &packet, const uint64 &nowTimeMS, const int64 &diffMS ) ;
	void TakePongBaseInfo( const pb::CmsgPong &msg ) ;

private:
	DLockPacketQueue          m_packetTable ;


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// static 
public:
	static uint64 GetDelayNoticeLimitTimeMS( void ) {   return s_delayNoticLimitTimeMS ;   }
	static void SetDelayNoticeLimitTimeMS( uint64 time ) {   s_delayNoticLimitTimeMS =time ;   }

private:
	static uint64                     s_delayNoticLimitTimeMS ;

};


#endif
