#include <fstream>
#include <string>

#include "ServerClient.h"
#include "ConsoleLog.h"
#include "OS.h"
#include "IoServicePool.h"
#include "Server.h"

#include "def/MmoAssert.h"
#include "memory_buffer/NetPack.h"

#include "Opcode.pb.h"
#include "ServerOpcode.pb.h"
#include "InterServer.pb.h"


ServerClinetId ServerClient::s_id =0;
uint64 ServerClient::s_delayNoticLimitTimeMS =10;


ServerClient::ServerClient( void )
{
	m_isUse = true;
	m_id = ++s_id;
	m_connectCount =0 ;

	SetCanSend( false ) ; // 不让继续执行Send操作

	m_pTcpClient.reset( new TcpAsyncClient( sIoPool.GetServerIoService() ) ) ;

	SetDelayTime( 0 ) ;
	SetUpdateTime( 0 );
	SetDstSessionCount( 0 );

	SetMaxTryConnectCount( ~0 ) ; // 默认设置重连无数次，子类根据需求改变

	SetClientType( PACKET_TYPE_END ) ;
	SetDstServerType( PACKET_TYPE_END ) ;
}


ServerClient::~ServerClient( void )
{
}

void ServerClient::Stop( void )
{
	m_pTcpClient->Stop();
}


void ServerClient::TakePacket( const uint64 &nowTimeMS, const int64 &diffMS )
{
	// Tips: whether had connect or not, the packet must be taker
	NetPackPtr pPack =NULL ;
	while( GetOnePacket( pPack ) )
	{
		try
		{
			if( !HandleNetPack( pPack, nowTimeMS, diffMS ) )
			{
				ELOG( "Client type %s receive not handle packet from %s, opName= [ %s ( %d ) ]"
					, GetClientName(), pPack->GetPacketTypeName(), pPack->GetOpcodeName(), pPack->GetOpcode() );
			}
		}
		catch( std::exception &e )
		{
			ELOG( "Client type %s handle packet from %s, opName= [ %s ( %d ) ], get error %s"
				, GetClientName(), pPack->GetPacketTypeName(), pPack->GetOpcodeName(), pPack->GetOpcode(), e.what() );
		}
		catch( ... )
		{
			ELOG( "Client type %s handle packet from %s, opName= [ %s ( %d ) ], get unknow error %s"
				, GetClientName(), pPack->GetPacketTypeName(), pPack->GetOpcodeName(), pPack->GetOpcode() );
		}
	}
}


void ServerClient::Update( const uint64 &nowTimeMS, const int64 &diffMS )
{
	// Tips: whether had connect or not, the packet must be taker
	TakePacket( nowTimeMS, diffMS ) ;

	if( !IsSettingAddress() || m_pTcpClient->IsStopping() || m_pTcpClient->IsStarting() )
	{
		return ;
	}
	
	ClientUpdate( nowTimeMS, diffMS );

	if( m_pTcpClient->IsStopped() )
	{
		m_reConnectWatch.Update( diffMS ) ;
		if( !m_reConnectWatch.Done() )
		{
			return ;
		}

		m_reConnectWatch.Reset() ;
		ReConnect() ;
		return ;
	}

	m_pingWatch.Update( diffMS ) ;
	if( m_pingWatch.Done() )
	{
		SendPing() ;
		m_pingWatch.Reset() ;
	}
}

void ServerClient::OnConnect( TcpAsyncConn *pConn )
{
	SetCanSend( true ) ;
	ASSERT( pConn->Ip() == GetIP() ) ;

	m_connectCount =0 ;

	NLOG( "%s had connected %s server, (%s:%s), session id: %u", GetClientName(), GetServerName(), GetIP(), GetPort(), pConn->Id() ) ;
	ClientConnect() ;
}

void ServerClient::OnRecv(TcpAsyncConn *pConn, NetPackPtr &pPack)
{
	if( pConn == NULL )
	{
		return;
	}

	const uint32 id = pConn->Id();
	const char *pIp = pConn->Ip().c_str();
	const int port = pConn->Port();

	const uint64 accountId = pPack->GetAccountId();
	const uint64 playerId = pPack->GetPlayerId();
	const uint32 packetSessionId = pPack->GetSessionId();

	if( NetPack::IsNoticeRecvAndSend( *pPack ) )
	{
		if( playerId != 0 || accountId != 0 )
		{
			RecvLOG( "%s to %s id:%u(%s:%d) %s(%u) psId: %u acc:%llu plr:%llu."
				, GetServerName(), GetClientName(), id, pIp, port, pPack->GetOpcodeName(), pPack->GetOpcode(), packetSessionId, accountId, playerId );
		}
		else
		{
			RecvLOG( "%s to %s id:%u(%s:%d) %s(%u) psId: %u.", GetServerName(), GetClientName(), id, pIp, port, pPack->GetOpcodeName(), pPack->GetOpcode(), packetSessionId );
		}
	}

	const char packType =pPack->GetPacketType() ;
	if( !IsValidRecvPacketType( packType ) )
	{
		ELOG( "%s client id:%u(%s:%d) receive packet type is %s not the %s.", GetClientName(), id, pIp, port, pPack->GetPacketTypeName(), GetServerName() ) ;
		return ;
	}

	m_packetTable.Enqueue( pPack ) ;
}

bool ServerClient::IsValidSendPacketType( const char type )
{
	return type == GetClientType() ;
}

bool ServerClient::IsValidRecvPacketType( const char type )
{
	return type == GetDstServerType() || type == PACKET_TYPE_SERVER_HTTP_ADDRESS;
}

void ServerClient::OnSend( TcpAsyncConn *pConn, NetPackPtr &pPack )
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
		// only gate wiill send client packet
		const char packetType = pPack->GetPacketType();
		ASSERT( IsValidSendPacketType( packetType ) ) ;

		const uint64 accountId = pPack->GetAccountId();
		const uint64 playerId = pPack->GetPlayerId();
		const uint32 packetSessionId = pPack->GetSessionId();

		if( playerId != 0 || accountId != 0 )
		{
			SendLOG( "%s to %s id:%u(%s:%d) %s(%u) psId: %u acc:%llu plr:%llu."
				, GetClientName(), GetServerName(), id, pIp, port, pPack->GetOpcodeName(), pPack->GetOpcode(), packetSessionId, accountId, playerId );
		}
		else
		{
			SendLOG( "%s to %s id:%u(%s:%d) %s(%u) psId: %u.", GetClientName(), GetServerName(), id, pIp, port, pPack->GetOpcodeName(), pPack->GetOpcode(), packetSessionId );
		}
	}
}

void ServerClient::OnClose(uint32 sessionId)
{
	SetCanSend( false ) ; // 不让继续执行Send操作
	ClientDisconnect() ;

	NLOG( "%s disconnect %s server(%s:%s), session id: %u", GetClientName(), GetServerName(), GetIP(), GetPort(), sessionId ) ;
}

bool ServerClient::Send( NetPack &packet )
{
	ASSERT( packet.GetOpcode() > 0 );
	if( packet.GetPacketType() == PACKET_TYPE_EMPTY )
	{
		packet.SetPacketType( GetClientType() ) ;
	}

	if( !CanSend() )
	{
		ELOG( "send packetet %s(%d) to %s faild !", packet.GetOpcodeName(),  packet.GetOpcode(), GetServerName() ) ;
		return false ;
	}

	m_pTcpClient->Send( packet ) ;
	return true ;
}

bool ServerClient::Send( NetPackPtr &pPack )
{
	ASSERT( pPack->GetOpcode() > 0 );
	if( pPack->GetPacketType() == PACKET_TYPE_EMPTY )
	{
		pPack->SetPacketType( GetClientType() ) ;
	}

	if( !CanSend() )
	{
		ELOG( "send packet %s(%d) to %s faild !", pPack->GetOpcodeName(),  pPack->GetOpcode(), GetServerName() ) ;
		return false ;
	}

	m_pTcpClient->Send( pPack ) ;
	return true ;
}

bool ServerClient::Send( const int opCode, const ::google::protobuf::Message &msg )
{
	NetPackPtr pPack = NetPack::MemPool().GetObj(); // + 2, beacuse pPack << msg will add a uint16 sign the msg size .
	pPack->SetOpCode( opCode );
	pPack->Reserve( msg.ByteSize() + 2 );
	*pPack << msg;
	return ServerClient::Send( pPack );
}

bool ServerClient::Send( const int opCode, const int sessionId, const ::google::protobuf::Message &msg )
{
	NetPackPtr pPack = NetPack::MemPool().GetObj(); // + 2, beacuse pPack << msg will add a uint16 sign the msg size .
	pPack->SetOpCode( opCode );
	pPack->Reserve( msg.ByteSize() + 2 );
	pPack->SetSessionId( sessionId ) ;
	*pPack << msg;
	return ServerClient::Send( pPack );
}

bool ServerClient::Send( const int opCode, const int sessionId, const char packetType, const ::google::protobuf::Message &msg )
{
	NetPackPtr pPack = NetPack::MemPool().GetObj(); // + 2, beacuse pPack << msg will add a uint16 sign the msg size .
	pPack->SetOpCode( opCode );
	pPack->Reserve( msg.ByteSize() + 2 );
	pPack->SetSessionId( sessionId ) ;
	pPack->SetPacketType( packetType ) ;
	*pPack << msg;
	return ServerClient::Send( pPack );
}

void ServerClient::ReConnect( void )
{
	std::string error ;
	if( !Connect( m_host, m_port, error ) )
	{
		NLOG( "%s connect %s server, %s:%s fail because %s !" ,GetClientName(), GetServerName(), GetIP(), GetPort(), error.c_str() ) ;
	}
}

void ServerClient::OnClientConnectError( int errValue, const std::string &errMsg )
{
	NLOG( "%s connect %s server, %s:%s fail because %s(%d) !" ,GetClientName(), GetServerName(), GetIP(), GetPort(), errMsg.c_str(), errValue ) ;
}


void ServerClient::SetDstInfo( const std::string &host, const std::string &port )
{
	m_port =port ;
	m_host =host ;
}


bool ServerClient::Connect(const std::string& host, const std::string &port, std::string& errMsg)
{
	ASSERT( m_pTcpClient->IsStopped() ) ;

	SetDstInfo( host, port ) ;
	
	if( m_connectCount < m_maxTryConnectCount )
	{
		++m_connectCount ;
		m_pTcpClient->Connect( host, port, this );
		return true ;
	}
	else
	{
		errMsg ="Max connect count limit" ;
		return false ;
	}
}


void ServerClient::SetClientType( char type )
{
	m_clientType =type ;
	m_clientName =NetPack::GetPacketTypeName( GetClientType() ) ;
}


void ServerClient::SetDstServerType( char type )
{
	m_dstServetType =type ;
	m_dstServerName =NetPack::GetPacketTypeName( GetDstServerType() ) ;
}



void ServerClient::SendPing( int pingOpCode, uint64 updateTimeMS, int32 sessionCount )
{
	pb::CmsgPing msg ;
	SetPingBaseInfo( msg, updateTimeMS, sessionCount ) ;
	Send( pingOpCode, msg  ) ;
}

void ServerClient::SetPingBaseInfo( pb::CmsgPing &msg, uint64 updateTimeMS, int32 sessionCount )
{
	msg.set_client_time( sOS.GetRealTime() );
	msg.set_delay( GetDelayTime() ) ;
	msg.set_update_times( updateTimeMS );

	MASSERT( sessionCount >= 0, "SetPingBaseInfo, sessionCount less than 0." );
	msg.set_session_count( sessionCount );
}

void ServerClient::HandlePong( NetPack &packet, const uint64 &nowTimeMS, const int64 &diffMS )
{
	pb::CmsgPong msg ;
	packet >> msg ;
	TakePongBaseInfo( msg ) ;
}

void ServerClient::TakePongBaseInfo( const pb::CmsgPong &msg )
{
	int64 delayTime =sOS.GetRealTime() - msg.client_time() ;

	if( delayTime < 0 )
	{
		ELOG( "%s delay time is %lld less than 0, cpu is ok ??", __FUNCTION__, delayTime );
		delayTime = 1;
	}

	SetDelayTime( delayTime ) ;
	SetUpdateTime( msg.update_times() );
	SetDstSessionCount( msg.session_count() );

	// Delay notice 
	if( GetDelayTime() > GetDelayNoticeLimitTimeMS() )
	{
		WLOG( "%s with %s(%s:%s), delay %llu MS !!!!", GetClientName(), GetServerName(), GetIP(), GetPort(), GetDelayTime() ) ;
	}
}


void ServerClient::WriteRegisterInfo( Server &server, pb::RegisterInfo &msg )
{
	msg.set_region_id( server.GetRegionId() );
	msg.set_server_id( server.GetServerId() );

	pb::Address *pAddress = msg.mutable_address();

	pAddress->set_near_ip( server.GetNearIP() );
	pAddress->set_far_ip( server.GetFarIP() );
	pAddress->set_port( server.GetPort() );
}
