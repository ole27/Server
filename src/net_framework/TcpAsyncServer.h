#pragma once

#include <string>
#include <boost/thread.hpp>
#include <boost/smart_ptr.hpp>

#include "TcpAsyncConn.h"
#include "NetFrameworkImplDetail.h"
#include "NetTypeDef.h"

class TcpAsyncServer : public ITcpConnMgr
{
	public:
		typedef ServerMap< uint32, TcpAsyncConn::Ptr > ConnectMap ;
		typedef ConnectMap::iterator ConnectMapItr ;

		typedef ServerMap< uint32, TcpAsyncConn::WPtr > ConnectWMap ;
		typedef ConnectWMap::iterator               ConnectWMapItr ;

		
		typedef std::multimap< time_t, TcpAsyncConn::Ptr > MConnectMap ;
		typedef MConnectMap::iterator MConnectMapItr ;

public:
			TcpAsyncServer( void );
			virtual ~TcpAsyncServer( void );

	public:
			virtual void Remove( uint32 sessionId ) ;
			//virtual void HadRecvPacket( int sessionId ) ;
			virtual boost::asio::io_service& GetIoService( void ) {   return m_ioSvc ;   }

	public:
			void Start( INetPackHandler *pPackHandler );
			bool StartListen( const std::string &port, const std::string &nearIP = "", const std::string &farIP = "" );
			void Stop( void );
			bool GetIsOpen( void ) {   return m_isOpen ;   }

			std::string &GetPort( void ) {   return m_address.m_port;   }
			const std::string& GetNearIP( void ) { return m_address.m_ipNear; }
			const std::string& GetFarIP( void ) {   return m_address.m_ipFar;   }
			const AddressInfo* GetAddressInfo( void ) { return &m_address; }

			void SetJustConnectLimintTimeMS( uint64 time ) {   m_justConnectLimitTimeMS  =time ;   }

	private:
			void StartAccept( void );
			void HandleAccept( TcpAsyncConn::Ptr pNewConn, const boost::system::error_code& error );
			void SyncStop( void );

			void CheckWaitFirstPacketSessions( uint64 nowTimeMS ) ;

	private:
			volatile bool                      m_isOpen;
			uint32                             m_idSeed;
			INetPackHandler                    *m_pPackHandler;
			boost::asio::io_service            &m_ioSvc ;
			AddressInfo                         m_address;
			boost::asio::ip::tcp::acceptor      m_acceptor;

			uint64                              m_justConnectLimitTimeMS ;

			ConnectMap                          m_conns ;
			ConnectMap                          m_justConnectConns ;
			MConnectMap                         m_waiForCloseConns ;
};

