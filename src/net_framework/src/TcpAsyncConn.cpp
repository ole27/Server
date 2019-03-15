#include "TcpAsyncConn.h"

#include "OS.h"
#include "ConsoleLog.h"

#include "def/MmoAssert.h"

#include "../TcpAsyncServer.h"
#include "../NetFrameworkDefines.h"
#include "NetFrameworkImplDetail.h"

using namespace boost::asio::ip;

TcpAsyncConn::~TcpAsyncConn( void )
{
}


TcpAsyncConn::TcpAsyncConn( ITcpConnMgr *pMgr, boost::asio::io_service& io_service, uint32 id )
	:m_isGetPacket( false )
	,m_id( id )
	,m_pMgr( pMgr ) 
	,m_pPackHandler( NULL )
	,m_ioSvc( io_service )
	,m_socket( io_service )
	,m_recvHeader( 0, HEADER_SIZE )
	 ,m_pRecvPack( NULL )
{
	m_port =0 ;
	m_connectTimeMS =0 ;
	m_recvHeader.Resize( HEADER_SIZE );

	m_isPostClose =false ;
	m_isFinisClose =false ;
	//m_postRead =0;
	//m_postWrite =0;
}

TcpAsyncConn::Ptr TcpAsyncConn::Create( ITcpConnMgr *pMgr, boost::asio::io_service &io_service, uint32 id )
{
	return Ptr( new TcpAsyncConn( pMgr, io_service, id ) ) ;
}


void TcpAsyncConn::Start( INetPackHandler *pPackHandler )
{
	if( NULL == pPackHandler )
	{
		ELOG( "TcpAsyncConn::Start() pPacketHandler is NULL !!!!!" ) ;
		ASSERT( false ) ;
		return ;
	}

	m_pPackHandler = pPackHandler ;

	// 这里之前有个报错，在远端刚连接端口后，导致无法获取得到remote_endpoint而报错,tan 2015-8-19
	{
		boost::system::error_code ec ;
		boost::asio::ip::tcp::endpoint endpoint = m_socket.remote_endpoint(ec) ;
		if( DisconnectedOrHasError(ec) )
		{
			return ;
		}

		m_ip   =endpoint.address().to_string() ;
		m_port =endpoint.port() ;
	}

	{
		boost::asio::ip::tcp::no_delay option( true );
		boost::system::error_code ec;
		m_socket.set_option( option, ec );
		if( ec )
		{
			ELOG( "Set socket not delay faild( %s, %d ) ~~!!!! ", ec.message().c_str(), ec.value() );
		}
	}

	m_pPackHandler->OnConnect( this );

	AsyncReadHeader() ;
}

void TcpAsyncConn::PostClose( DisconnectMode mode /*= RemoveFromManager*/ )
{
	if( GetIsPostClose() || GetIsFinishClose() )
	{
		WLOG( "TcpAsyncConn::PostClose() can not duplicate post close!" ) ;
		return ;
	}

	SetIsPostClose( true ) ;
	m_socket.get_io_service().post( boost::bind( &TcpAsyncConn::Disconnect, this, mode ) ) ;
}

//void TcpAsyncConn::PostRecvPacket( void )
//{
//	m_pMgr->GetIoService().post( boost::bind( &ITcpConnMgr::HadRecvPacket, m_pMgr, Id() ) ) ;
//}


void TcpAsyncConn::AsyncReadHeader( void )
{
	if( GetIsPostClose() )
	{
		WLOG( "TcpAsyncConn::AsyncReadHeader() had post close can not post read packet." ) ;
		return ;
	}

	//IncreasePostRead() ;

	boost::asio::async_read
		(
		 m_socket, 
		 boost::asio::buffer( m_recvHeader.HeaderRawBuffer(), NetPack::GetHeaderSize() ),
		 boost::bind
		 (
		  &TcpAsyncConn::HandleReadHeader, 
		  this,
		  boost::asio::placeholders::error,
		  boost::asio::placeholders::bytes_transferred
		 )
		);
}

void TcpAsyncConn::HandleReadHeader(const boost::system::error_code& error, size_t bytes_transferred)
{
	if (DisconnectedOrHasError(error))
	{
		//DecreasePostRead() ;
		return;
	}

	ASSERT( bytes_transferred == NetPack::GetHeaderSize() ) ;

	char   type       =m_recvHeader.GetPacketType() ;
	uint16 len        =m_recvHeader.GetBodySize() ;
	uint16 opCode     =m_recvHeader.GetOpcode() ;
	uint32 sessionId  =m_recvHeader.GetSessionId() ;
	if( type < 0 || type >= PACKET_TYPE_END )
	{
		ELOG( "%s, will colse it, packet type is %d is error, right range is %d ~ %d", __FUNCTION__, type, PACKET_TYPE_EMPTY, PACKET_TYPE_END );
		Disconnect();
		return;
	}

	if( opCode == 0 )
	{
		ELOG( "%s, packet opCode is %d is error, the connect should be closed!!!", __FUNCTION__, opCode ) ;
		Disconnect();
		return;
	}

	m_pRecvPack = NetPack::MemPool().GetObj(); // + 2, beacuse pPack << msg will add a uint16 sign the msg size .
	m_pRecvPack->SetPacketType( type );
	m_pRecvPack->SetBodySize( len );
	m_pRecvPack->SetOpCode( opCode );
	m_pRecvPack->SetSessionId( sessionId );
	
	if( len >= ByteBuffer::s_maxInputPacketSize )
	{
		ELOG( "%s, client session packet len %d max than %d bytes, will be close", __FUNCTION__, len, ByteBuffer::s_maxInputPacketSize );
		Disconnect();
		return;
	}

	if( type == PACKET_TYPE_EMPTY && sessionId != 0 )
	{
		ELOG( "%s, client session packet session id is %d, not 0 error, will be close", __FUNCTION__, sessionId );
		Disconnect();
		return;
	}

	ASSERT( len == m_pRecvPack->GetBodySize() ) ;

	if (m_pRecvPack->GetBodySize() > 0)
	{
		m_pRecvPack->Resize( len + NetPack::GetHeaderSize() ) ;
		AsyncReadBody();
	}
	else
	{
		if (m_pPackHandler)
		{
			m_pPackHandler->OnRecv( this, m_pRecvPack );
		}

		//DecreasePostRead() ;
		AsyncReadHeader();
	}
}

void TcpAsyncConn::AsyncReadBody()
{
	if( GetIsPostClose() )
	{
		WLOG( "TcpAsyncConn::AsyncReadBody() had post close can not post read packet." ) ;
		return ;
	}

	boost::asio::async_read
		(
		 m_socket, 
		 boost::asio::buffer(m_pRecvPack->BodyRawBuffer(), m_pRecvPack->GetBodySize()),
		 boost::bind
		 (
		  &TcpAsyncConn::HandleReadBody, 
		  this,
		  boost::asio::placeholders::error,
		  boost::asio::placeholders::bytes_transferred
		 )
		);
}

void TcpAsyncConn::HandleReadBody(const boost::system::error_code& error, size_t bytes_transferred)
{
	if (DisconnectedOrHasError(error))
	{
		//DecreasePostRead() ;
		return;
	}
	else
	{
		//DecreasePostRead() ;
	}

	ASSERT(bytes_transferred == m_pRecvPack->GetBodySize());

	if( m_pPackHandler )
	{
		m_pPackHandler->OnRecv( this, m_pRecvPack ) ;
		m_pRecvPack.reset();
	}

	AsyncReadHeader();
}

bool TcpAsyncConn::AsSendNewPacket( NetPackPtr &pPack )
{
	ASSERT( pPack != NULL ) ;
	if( IsDisconnected() || m_pPackHandler == NULL || GetIsPostClose() )
	{
		pPack =NULL ;

		WLOG( "TcpAsyncConn::AsSendNewPacket() had post close can not post send packet." ) ;
		return false;
	}

	//IncreasePostWrite() ;

	ASSERT( pPack->GetOpcode() != 0 ) ;
	m_socket.get_io_service().post( boost::bind( &TcpAsyncConn::AsyncSend, this, pPack ) ) ;
	return true;
}



bool TcpAsyncConn::AsSendStackPacket( NetPack &packet )
{
	NetPackPtr pPack = NetPack::MemPool().GetObj();
	pPack->Swap( packet ) ;
	return AsSendNewPacket( pPack ) ;
}


void TcpAsyncConn::AsyncSend( NetPackPtr &pPack )
{
	if( m_pPackHandler == NULL || GetIsPostClose() )
	{
		pPack =NULL ;

		WLOG( "TcpAsyncConn::AsyncSend() had post close can not post send packet." ) ;
		//DecreasePostWrite() ;
		return ;
	}

	pPack->SetDynamicBodySize() ;

	boost::asio::async_write
		(
		 m_socket, 
		 boost::asio::buffer( pPack->HeaderRawBuffer(), pPack->DynamicTotalSize() ),
		 boost::bind
		 (
		  &TcpAsyncConn::HandleWrite, 
		  this,
		  boost::asio::placeholders::error,
		  boost::asio::placeholders::bytes_transferred,
		  pPack
		 )
		);
}

void TcpAsyncConn::HandleWrite(const boost::system::error_code& error, size_t bytes_transferred, NetPackPtr pPack )
{
	if( m_pPackHandler )
	{
		if( !DisconnectedOrHasError(error) )
		{
			ASSERT( bytes_transferred == pPack->DynamicTotalSize() ) ;
			m_pPackHandler->OnSend( this, pPack ) ;
		}
	}

	pPack =NULL ;

	//DecreasePostWrite() ;
}


void TcpAsyncConn::CloseSocket()
{
	if (m_socket.is_open())
	{
		boost::system::error_code error;
		m_socket.shutdown(tcp::socket::shutdown_both, error);
		m_socket.close(error);

		if( error )
		{
			int value =error.value() ;
			std::string msg =error.message() ;
			ELOG( "CloseSocket(), %d, %s ", value, msg.c_str() ) ;
		}
	}
}

bool TcpAsyncConn::IsDisconnected() const
{
	return !m_socket.is_open() ;
}

void TcpAsyncConn::Disconnect( DisconnectMode mode /*= RemoveFromManager*/ )
{
	if( GetIsFinishClose() )
	{
		return ;
	}

	SetIsPostClose( true ) ;

	if( IsDisconnected() )
	{
		if( m_pPackHandler != NULL )
		{
			m_pPackHandler->OnClose(m_id);
			m_pPackHandler = NULL;
		}

		return;
	}

	if( m_pPackHandler != NULL )
	{
		m_pPackHandler->OnClose(m_id);
		m_pPackHandler = NULL;
	}

	CloseSocket();

	if( mode == RemoveFromManager )
	{
		m_pMgr->GetIoService().post( boost::bind( &ITcpConnMgr::Remove, m_pMgr, Id() ) ) ;
	}

	ASSERT( GetIsFinishClose() == false ) ;
	SetIsFinishClose( true ) ;
}

bool TcpAsyncConn::DisconnectedOrHasError(const boost::system::error_code& error)
{
	if( GetIsFinishClose() )
	{
		return true ;
	}

	if (IsDisconnected())
	{
		SetIsFinishClose( true ) ;
		return true;
	}

	if (error)
	{
		int value =error.value() ;
		std::string msg =error.message() ;
		ELOG( "DisconnectedOrHasError(), %d, %s, id: %u ", value, msg.c_str(), Id() ) ;
		Disconnect();
		return true;
	}

	return false;
}

