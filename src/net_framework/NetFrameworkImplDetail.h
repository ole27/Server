#pragma once

#include <boost/asio.hpp>
#include <boost/smart_ptr.hpp>

class TcpAsyncConn ;

class ITcpConnMgr
{
	public:
		virtual ~ITcpConnMgr() {}

		virtual void Remove( uint32 sessionId ) = 0;
		virtual void HadRecvPacket( uint32 sessionId ) {}

		virtual boost::asio::io_service& GetIoService( void ) = 0;
};

