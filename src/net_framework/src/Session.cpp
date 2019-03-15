
#include "Session.h"

#include "ConsoleLog.h"
#include "OS.h"

#include "NetTypeDef.h"
#include "Server.h"

#include "def/MmoAssert.h"
#include "memory_buffer/NetPack.h"
#include "TcpAsyncConn.h"

#include "Opcode.pb.h"
#include "ServerOpcode.pb.h"
#include "CommomDef.pb.h"


uint32 Session::s_takePacketLimit       =10;
uint64 Session::s_delayNoticLimitTimeMS =10;


Session::Session( void )
{
	Init() ;
}

Session::~Session( void )
{
	ASSERT( m_spConn == NULL ) ;
	Clear() ;
}


void Session::Init( void )
{
	m_isClosed          =false ;
	m_isNeedClose       =false ;
	m_isServerSession   =false ;
	m_isVerification    =false ;
	m_isInWorldThread   =true ;
	m_sessionType       =PACKET_TYPE_END;
	m_sessionId         =0;
	m_pServer           =NULL;
	m_delayTime         =0;
	m_updateTime        =0;
	m_preRecvPacketTime =0;
	m_waitVerfyTimeMS   =0 ;
	m_spConn            =NULL;
	m_clientSessionCount = 0;
	m_connIsOk = false;
}

void Session::Clear( void )
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// virtual
void Session::OnConnect( TcpAsyncConn *pConn )
{
	NLOG( "Session::OnConnect() be invoked !!!!!!" ) ;
	ASSERT( false ) ;
}

bool Session::CheckRecvPack( TcpAsyncConn *pConn, NetPackPtr &pPack )
{
	if( !GetIsCanSend() )
	{
		ASSERT( GetIsHadClosed() );
		WLOG( "Session::OnRecv( id: %u) is get recv opCode (%s)%d packet by is post close, so discard!", GetSessionId(), pPack->GetOpcodeName(), pPack->GetOpcode() );
		return false ;
	}

	if( !IsValidRecvPacketType( pPack->GetPacketType() ) )
	{
		ELOG( "Session Id: %d, recv opCode (%s)%d packet type:%s not match seesion type %s, will be abandoned."
			, GetSessionId(), pPack->GetOpcodeName(), pPack->GetOpcode(), pPack->GetPacketTypeName(), GetSessionTypeName() );

		CloseSession();
		return false ;
	}

	return true;
}


void Session::OnRecv( TcpAsyncConn *pConn, NetPackPtr &pPack )
{
	if( CheckRecvPack( pConn, pPack ) )
	{
		GetServer()->NoticeRecv( pConn, pPack );
		PostInputPacket( pPack );
	}
}

void Session::OnSend( TcpAsyncConn *pConn, NetPackPtr &pPack )
{
	if( !pPack->IsClientPacket() )
	{
		pPack->SetPacketType( GetSessionType() ) ;
	}

	GetServer()->NoticeSend( pConn, pPack ) ;
}

void Session::OnClose( uint32 sessionId )
{
	m_spConn =NULL ;
	m_connIsOk = false;

	GetServer()->OnCloseSession( sessionId ) ;
}

void Session::ServerClose( void )
{
	m_spConn =NULL ;
	m_connIsOk = false;
}

void Session::WriteAddress( pb::Address *pAddress )
{
	pAddress->set_near_ip( m_address.m_ipNear );
	pAddress->set_far_ip( m_address.m_ipFar );
	pAddress->set_port( m_address.m_port );
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
const char* Session::GetSessionTypeName( void )
{
	return NetPack::GetPacketTypeName( GetSessionType() ) ;
}


bool Session::GetIsSetSessionType( void )
{
	return m_sessionType != PACKET_TYPE_END ;
}


bool Session::Send( NetPack &packet ) const
{
	ASSERT( packet.GetOpcode() > 0 );
	if( packet.GetPacketType() == PACKET_TYPE_EMPTY  )
	{
		packet.SetPacketType( GetServer()->GetServerType() ) ;
	}

	if( GetConnectIsOk() )
	{
		return m_spConn->AsSendStackPacket( packet ) ;
	}
	else
	{
		ELOG( "Session is %u send %s(%u) faild because m_spConn is empty!!", GetSessionId(), packet.GetOpcodeName(), packet.GetOpcode() ) ;
		return false;
	}
}

bool Session::Send( NetPackPtr &pPack ) const
{
	ASSERT( pPack->GetOpcode() > 0 );
	if( pPack->GetPacketType() == PACKET_TYPE_EMPTY  )
	{
		pPack->SetPacketType( GetServer()->GetServerType() ) ;
	}

	if( GetConnectIsOk() )
	{
		return m_spConn->AsSendNewPacket( pPack ) ;
	}
	else
	{
		ELOG( "Session is %u send %s(%u) faild because m_spConn is empty!!", GetSessionId(), pPack->GetOpcodeName(), pPack->GetOpcode() ) ;
		pPack =NULL ;
		return false;
	}
}

bool Session::Send( int opCode, const ::google::protobuf::Message &msg ) const
{
	NetPackPtr pPack = NetPack::MemPool().GetObj(); // + 2, beacuse pPack << msg will add a uint16 sign the msg size .
	pPack->SetOpCode( opCode );
	pPack->Reserve( msg.ByteSize() + 2 );
	*pPack << msg;
	return Session::Send( pPack );
}

bool Session::Send( const int opCode, const int sessionId, const ::google::protobuf::Message &msg ) const
{
	NetPackPtr pPack = NetPack::MemPool().GetObj(); // + 2, beacuse pPack << msg will add a uint16 sign the msg size .
	pPack->SetOpCode( opCode );
	pPack->Reserve( msg.ByteSize() + 2 );
	pPack->SetSessionId( sessionId ) ;
	*pPack << msg;
	return Session::Send( pPack );
}

bool Session::Send( const int opCode, const int sessionId, const char packetType, const ::google::protobuf::Message &msg )
{
	NetPackPtr pPack = NetPack::MemPool().GetObj(); // + 2, beacuse pPack << msg will add a uint16 sign the msg size .
	pPack->SetOpCode( opCode );
	pPack->Reserve( msg.ByteSize() + 2 );
	pPack->SetSessionId( sessionId ) ;
	pPack->SetPacketType( packetType ) ;
	*pPack << msg;
	return Session::Send( pPack );
}



void Session::CloseSession( void )
{
	//ASSERT( m_spConn ) ;
	ASSERT( GetIsHadClosed() == false ) ;

	SetIsHadClosed( true ) ;
	if( m_spConn != NULL )
	{
		m_spConn->PostClose() ;
		m_spConn =NULL ;
		m_connIsOk = false;
	}
}

void Session::Kick( int32 errorCode )
{
	pb::SmsgKick msg ;
	msg.set_type( static_cast<pb::KickErrorType>( errorCode ) ) ;
	Send( pb::pb_SmsgKick, msg ) ;

	SetIsNeedClose( true ) ;
}

bool Session::IsValidRecvPacketType( const char type )
{
	return GetSessionType() == type ;
}


void Session::SetConn( TcpAsyncConn *pConn )
{
	ASSERT( pConn ) ;
	m_spConn =pConn ;
	m_spConn->SetPackHandler( this ) ;
	m_connIsOk = true;

	// 9223372036854775807 * 2 : max uint64
	char port[20] ="" ;
	sprintf( port, "%u", m_spConn->Port() ) ;
	SetAddress( AddressInfo( port, m_spConn->Ip(), "" ) ) ;
}

void Session::Update( const uint64 &nowTimeMS, const int64 &diffMS )
{
	NetPackPtr pPack ;

	uint32 takeCount         =0 ;
	while( takeCount < GetTakePacketLimit() && GetOnePacket( pPack ) )
	{
		++takeCount ;

		SetPreRecvTime( nowTimeMS ) ;

		try
		{
			if( !HandleNetPack( pPack, nowTimeMS, diffMS ) )
			{
				ELOG( "%s session receive not handle packet from %s, session id:%u, %s(%d) will be close"
					, GetSessionTypeName(), pPack->GetPacketTypeName(), GetSessionId(), pPack->GetOpcodeName(), pPack->GetOpcode() );

				SetIsNeedClose( true );
			}
		}
		catch( std::exception &e )
		{
			ELOG( "%s session handle packet from %s, session id:%u, %s(%d) get error %s will be close"
				, GetSessionTypeName(), pPack->GetPacketTypeName(), GetSessionId(), pPack->GetOpcodeName(), pPack->GetOpcode(), e.what() );

			SetIsNeedClose( true );
		}
		catch( ... )
		{
			ELOG( "%s session handle packet from %s, session id:%u, %s(%d) get unknow error will be close"
				, GetSessionTypeName(), pPack->GetPacketTypeName(), GetSessionId(), pPack->GetOpcodeName(), pPack->GetOpcode() );

			SetIsNeedClose( true );
		}

	}

	while( GetOneNextUpdatePacket(pPack) )
	{
		PostInputPacket(pPack);
	}
}


void Session::HandlePing( NetPack &packet, int pongOpCode, uint64 updateTimeMS, uint32 sessionCount )
{
	pb::CmsgPing msg ;
	packet >> msg ;

	pb::CmsgPong retMsg ;
	TakePingAndSetPongBaseInfo( msg, retMsg, updateTimeMS, sessionCount ) ;

	Send( pongOpCode, retMsg ) ;
}

void Session::TakePingAndSetPongBaseInfo( const pb::CmsgPing &ping, pb::CmsgPong &pong, uint64 updateTimeMS, uint32 sessionCount )
{
	//ASSERT( ping.client_time() > 0 ) ;

	SetDelayTime( ping.delay() ) ;
	SetUpdateTime( ping.update_times() );
	SetClientSessionCount( ping.session_count() );

	pong.set_client_time( ping.client_time() ) ;
	pong.set_update_times( updateTimeMS );

	// Delay notice 
	if( GetDelayTime() > GetDelayNoticeLimitTimeMS() )
	{
		WLOG( "Id:%u, type:%s, delay %llu MS !!!!", GetSessionId(), GetSessionTypeName(), GetDelayTime() ) ;
	}
}

void Session::SetMiniDelaySession( volatile uint32 &nowId, uint64 &nowVal, uint32 upId, uint64 upValue )
{
	if( upValue == 0 )
	{
		return;
	}

	if( nowId == upId )
	{
		nowVal = upValue;
	}
	else if( nowId ==0 || upValue < nowVal )
	{
		nowId = upId;
		nowVal = upValue;
	}
}

bool Session::GetIsCanSend( void ) const
{
	return GetConnectIsOk() && GetIsHadClosed() == false ;
}

bool Session::GetConnectIsOk( void ) const
{
	return m_connIsOk && m_spConn != NULL;
}
