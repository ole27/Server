
#include "TcpAsyncClient.h"

#include "def/MmoAssert.h"
#include "NetFrameworkDefines.h"

using boost::asio::ip::tcp ;

TcpAsyncClient::TcpAsyncClient( boost::asio::io_service &io )
	: m_state( enuStopped )
	, m_ioSvc( io )
{
	m_spConn = TcpAsyncConn::Create(this, m_ioSvc, 1);
}

TcpAsyncClient::~TcpAsyncClient( void )
{
	if( IsStarting() || IsStarted() )
	{
		m_state =enuStopping ;
		m_spConn->PostClose() ;

		while( !IsStopped() && !m_ioSvc.stopped() )
		{
			WLOG( "TcpAsyncClient::~TcpAsyncClient( void ) waiting TcpAsyncClient::Remove() be invoke!" ) ;
			boost::this_thread::sleep( boost::posix_time::milliseconds(1) );
		}
	}
}

void TcpAsyncClient::Connect( const std::string &host, const std::string &port, INetPackHandler *pHandler )
{
	if( !IsStopped() )
	{
		ELOG( "Is start or wait start why connect agin ?????????????" ) ;
		return ;
	}

	ASSERT( m_spConn->IsDisconnected() ) ;

	try
	{
		tcp::resolver resolver( m_ioSvc );
		tcp::resolver::query query( host, port );
		tcp::resolver::iterator iterator = resolver.resolve( query );

		// Notice:
		// m_state =enuStarting ; must before async_connect() because HandleStart() may
		// runing at once ( that async_connect() had not return ).
		m_state =enuStarting ;
		m_spConn->SetIsPostClose( false ) ;
		m_spConn->SetIsFinishClose( false ) ;

		boost::asio::async_connect(
			m_spConn->Socket(),
			iterator,
			boost::bind( &TcpAsyncClient::HandleStart, this, pHandler, boost::asio::placeholders::error, _2 ) );
	}
	catch (boost::system::error_code &e)
	{
		ELOG("%s get error %d, %s", __FUNCTION__, e.value(), e.message().c_str());
	}
	catch( const std::exception &e )
	{
		ELOG( "%s get error %s", __FUNCTION__, e.what() ) ;
	}
	catch( ... )
	{
		ELOG( "%s get unkown error", __FUNCTION__ ) ;
	}
}

void TcpAsyncClient::HandleStart( INetPackHandler *pHandler, const boost::system::error_code &error, tcp::resolver::iterator i )
{
	if( error )
	{
		int value       =error.value() ;
		std::string msg =error.message() ;

		if( pHandler != NULL )
		{
			pHandler->OnClientConnectError( value, msg ) ;
		}
		else
		{
			ELOG( "TcpAsyncClient::HandleStart(), %s(%d), %s(%d) ", m_spConn->Ip().c_str(), m_spConn->Port(), msg.c_str(), value ) ;
		}

		m_spConn->Disconnect() ;
		return ;
	}

	if( !IsStarting() )
	{
		if( m_spConn != NULL )
		{
			m_spConn->Disconnect() ;
		}
		return ;
	}

	boost::asio::ip::tcp::resolver::iterator end;
	ASSERT( i != end ) ;

	m_state =enuStarted ;
	m_spConn->Start( pHandler ) ;
}


void TcpAsyncClient::Stop( void )
{
	if( IsStarting() || IsStarted() )
	{
		ASSERT( m_spConn ) ;
		m_state =enuStopping ;
		m_spConn->PostClose() ;
	}
}

void TcpAsyncClient::Remove( uint32 sessionId )
{
	ASSERT( m_spConn->Id() == sessionId ) ;
	m_state =enuStopped ;
}

void TcpAsyncClient::Send( NetPack &pack )
{
	m_spConn->AsSendStackPacket( pack ) ;
}

void TcpAsyncClient::Send( NetPackPtr &pPack )
{
	m_spConn->AsSendNewPacket( pPack ) ;
}

