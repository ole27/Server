#pragma once

#include <string>
#include <assert.h>
#include <boost/smart_ptr.hpp>

class NetPack;
class TcpAsyncConn ;

typedef boost::shared_ptr< NetPack > NetPackPtr;

class INetPackHandler
{
	public:
		virtual ~INetPackHandler( void ) {}

		virtual void OnConnect( TcpAsyncConn *pConn ) = 0;
		virtual void OnRecv( TcpAsyncConn *pConn, NetPackPtr &pPack ) = 0;
		virtual void OnSend( TcpAsyncConn *pConn, NetPackPtr &pPack ) = 0;
		virtual void OnClose( uint32 sessionId ) = 0;
		virtual void OnClientConnectError( int errValue, const std::string &errMsg ) { assert( false ) ; }
};





