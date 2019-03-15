#pragma once

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/smart_ptr.hpp>

#include "TcpAsyncConn.h"
#include "NetFrameworkImplDetail.h"

using boost::asio::ip::tcp ;

enum ClientState
{
	enuStopped       =0,
	enuStarting      =1,
	enuStarted       =2,
	enuStopping      =3,
} ;

class TcpAsyncClient : public ITcpConnMgr
{
public:
	TcpAsyncClient( boost::asio::io_service &io );
	virtual ~TcpAsyncClient( void );

public:
	virtual void Remove( uint32 sessionId ) ;
	virtual boost::asio::io_service& GetIoService( void ) {   return m_ioSvc ;   }

public:
	void Connect( const std::string &host, const std::string &port, INetPackHandler *pHandler ) ;
	void Stop( void ) ;

	void Send( NetPack &pack ) ;
	void Send( NetPackPtr &pPack ) ; 
	//void Send (const ByteBuffer& buf);

	bool IsStarted( void ) const {    return m_state == enuStarted ;    }
	bool IsStarting( void ) const {    return m_state == enuStarting ;    }
	bool IsStopping( void ) const {    return m_state == enuStopping ;    }
	bool IsStopped( void ) const {    return m_state == enuStopped ;    }
	
	TcpAsyncConn::Ptr& Conn( void )
	{
		return m_spConn;
	}

private:
	void HandleStart( INetPackHandler *pHandler, const boost::system::error_code &error, tcp::resolver::iterator i ) ;

private:
	volatile char                       m_state ;
	boost::asio::io_service            &m_ioSvc ;
	TcpAsyncConn::Ptr                   m_spConn ;
};

