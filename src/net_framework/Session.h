
#ifndef SESSION_CENTER_H__
#define SESSION_CENTER_H__

#include <vector>

#include <boost/atomic.hpp>
#include <boost/smart_ptr.hpp>

#include "google/protobuf/message.h"

#include "NetTypeDef.h"
#include "NetFrameworkDefines.h"

#include "def/TypeDef.h"

namespace pb
{
	class CmsgPing ;
	class CmsgPong ;
	class Address;
}

class Server ;
class NetPack ;
class TcpAsyncConn ;

class Session : public boost::noncopyable, public INetPackHandler
{
public:
	Session( void ) ;
	virtual ~Session( void ) ;

	void Init( void ) ;
	void Clear( void ) ;

	virtual void Update( const uint64 &nowTimeMS, const int64 &diffMS );

	bool CheckRecvPack( TcpAsyncConn *pConn, NetPackPtr &pPack );

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// virtual
public:
	virtual void OnConnect( TcpAsyncConn *pConn ) ;
	virtual void OnRecv( TcpAsyncConn *pConn, NetPackPtr &pPack ) ;
	virtual void OnSend( TcpAsyncConn *pConn, NetPackPtr &pPack ) ;
	virtual void OnClose( uint32 sessionId ) ;


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// 
public:
	const char* GetSessionTypeName( void ) ;

	bool GetIsSetSessionType( void ) ;

	uint32 GetSessionId( void ) const {   return m_sessionId ;   }
	void SetSessionId( uint32 id ) {   m_sessionId =id ;   }

	Server* GetServer( void ) const {   return m_pServer ;   }
	void SetServer( Server *pServer ) { m_pServer =pServer ;   }

	uint64 GetPreRecvTime( void ) const {   return m_preRecvPacketTime ;    }
	void SetPreRecvTime( uint64 time ) {   m_preRecvPacketTime =time ;    }

	uint64 GetDelayTime( void ) const {   return m_delayTime ;   }
	void SetDelayTime( uint64 time ) {   m_delayTime =time ;    }

	uint64 GetUpdateTime( void ) { return m_updateTime; }
	void SetUpdateTime( uint64 time ) { m_updateTime = time; }

	uint32 GetClientSessionCount( void ) { return m_clientSessionCount; }
	void SetClientSessionCount( uint32 count ) { m_clientSessionCount = count; }

	int GetSessionType( void ) const {   return m_sessionType ;   }
	void SetSessionType( int type ) {   m_sessionType =type ;   }

	bool GetIsVerification( void ) const {   return m_isVerification ;   }
	void SetIsVerification( bool isVerify ) {   m_isVerification =isVerify ;   }

	bool GetIsInWorldThread( void ) const {   return m_isInWorldThread ;   }
	void SetIsInWorldThread( bool isInWorldThread ) {   m_isInWorldThread =isInWorldThread ;   }

	bool GetIsServerSession( void ) const {   return m_isServerSession ;   }
	void SetIsServerSession( bool isServer ) {   m_isServerSession =isServer;   }

	bool GetIsHadClosed( void ) const {   return m_isClosed ;   }
	void SetIsHadClosed( bool isClose ) {   m_isClosed =isClose ;   }

	bool GetIsNeedClose( void ) const {   return m_isNeedClose ;   }
	void SetIsNeedClose( bool isClosed ) {   m_isNeedClose =isClosed ;   }

	const AddressInfo& GetAddress( void )  const {   return m_address ;   }
	void SetAddress( const AddressInfo &address ) {   m_address =address ;   }

	const char* GetIpNear( void ) { return m_address.m_ipNear.c_str(); }
	const char* GetIpFar( void ) {   return m_address.m_ipFar.c_str() ;   }
	const char* GetPort( void ) {   return m_address.m_port.c_str() ;   }

	const SessionPairId& GetPairId( void ) const {    return m_pairId ;   }
	void SetPairId( const SessionPairId &pairId ) {   m_pairId =pairId ;   }

	ServerId GetServerId( void ) { return m_pairId.first; }
	ServerId GetRegionId( void ) { return m_pairId.second; }
	void SetPairId( ServerId regionId, ServerId serverId ) { m_pairId = SessionPairId( serverId, regionId ); }

	uint64 GetWaitVerfyTime( void ) {   return m_waitVerfyTimeMS ;   }
	void SetWaitVerfyTime( uint64 timeMS ) {   m_waitVerfyTimeMS =timeMS ;   }

	void ServerClose( void ) ;

	void WriteAddress( pb::Address *pAddress );

private:
	bool                  m_isClosed ;
	bool                  m_isNeedClose ;
	bool                  m_isServerSession ;
	bool                  m_isVerification ;
	bool                  m_isInWorldThread ;
	SessionId             m_sessionId ;
	SetssionType          m_sessionType ;
	Server               *m_pServer ;
	uint64                m_waitVerfyTimeMS ;
	uint64                m_delayTime ;
	uint64                m_updateTime;
	uint32                m_clientSessionCount;
	uint64                m_preRecvPacketTime ;

	SessionPairId         m_pairId ;
	AddressInfo           m_address ;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Packet
public:
	virtual void CloseSession( void ) ;
	virtual void Kick( int32 errorCode ) ;
	virtual bool IsValidRecvPacketType( const char type ) ;

	bool GetOnePacket( NetPackPtr &pPack ) {   return m_packetTable.Dequeue( pPack ) ;   }
	void PostInputPacket( NetPackPtr &pPack ) {   m_packetTable.Enqueue( pPack ) ;   }

	bool GetOneNextUpdatePacket(NetPackPtr &pPack) { return m_nextUpdatePacketTable.Dequeue(pPack); }
	void PostNextUpdatePacket(NetPackPtr &pPack) { m_nextUpdatePacketTable.Enqueue(pPack); }

	void SetConn( TcpAsyncConn *pConn ) ;

	virtual bool HandleNetPack( NetPackPtr &pPack, const uint64 &nowTimeMS, const int64 &diffMS ) =0;

	virtual bool Send( NetPack &packet ) const ;
	virtual bool Send( NetPackPtr &pPack ) const ;
	virtual bool Send( const int opCode, const ::google::protobuf::Message &msg ) const ;
	virtual bool Send( const int opCode, const int sessionId, const ::google::protobuf::Message &msg ) const ;
	virtual bool Send( const int opCode, const int sessionId, const char packetType, const ::google::protobuf::Message &msg ) ;

	bool SendMsg( const int opCode, const ::google::protobuf::Message *pMsg ) const { return Session::Send( opCode, *pMsg ); }
	bool SendSessionIdMsg( const int opCode, const int sessionId, const ::google::protobuf::Message *pMsg ) const { return Session::Send( opCode, sessionId, *pMsg ); }
	bool SendSessionIdPacketTypeMsg(  const int opCode, const int sessionId, const char packetType, const ::google::protobuf::Message *pMsg ) {   return Session::Send( opCode, sessionId, packetType, *pMsg ) ;   }

	void HandlePing( NetPack &packet, int pongOpCode, uint64 updateTimeMS, uint32 sessionCount ) ;
	void TakePingAndSetPongBaseInfo( const pb::CmsgPing &ping, pb::CmsgPong &pong, uint64 updateTimeMS, uint32 sessionCount ) ;

	static void SetMiniDelaySession( volatile uint32 &nowId, uint64 &nowVal, uint32 upId, uint64 upValue );

	bool GetIsCanSend( void ) const;
	bool GetConnectIsOk( void ) const;

private:
	DLockPacketQueue                m_packetTable;
	DLockPacketQueue                m_nextUpdatePacketTable ;

	volatile bool                   m_connIsOk;
	TcpAsyncConn* volatile          m_spConn ;


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// static 
public:
	static uint64 GetDelayNoticeLimitTimeMS( void ) {   return s_delayNoticLimitTimeMS ;   }
	static void SetDelayNoticeLimitTimeMS( uint64 time ) {   s_delayNoticLimitTimeMS =time ;   }

	static uint32 GetTakePacketLimit( void ) {   return s_takePacketLimit ;   }
	static void SetTakePacketLimit( uint32 limit ) {   s_takePacketLimit =limit ;   }

private:
	static uint32                     s_takePacketLimit ;
	static uint64                     s_delayNoticLimitTimeMS ;
};

#endif
