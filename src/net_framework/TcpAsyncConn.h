#pragma once

#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/atomic.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "def/TypeDef.h"
#include "memory_buffer/NetPack.h"
#include "memory_buffer/ByteBuffer.h"
#include "memory_buffer/DoubleBuff.h"


class ITcpConnMgr ;
class INetPackHandler ;

class TcpAsyncConn : public boost::enable_shared_from_this<TcpAsyncConn>
{
	public:
		typedef boost::weak_ptr<TcpAsyncConn> WPtr;
		typedef boost::shared_ptr<TcpAsyncConn> Ptr;

	public:
		static TcpAsyncConn::Ptr Create( ITcpConnMgr *pMgr, boost::asio::io_service &io_service, uint32 id ) ;

		uint32 Id( void ) const {    return m_id ;    }

		int Port( void ) const {    return m_port ;   }

		const std::string& Ip( void ) const {   return m_ip ;   }

		boost::asio::ip::tcp::socket& Socket( void ) {   return m_socket ;   }

		bool GetIsGetPacket( void ) const {   return m_isGetPacket ;   }
		void SetIsGetPacket( bool isGet ) {   m_isGetPacket =isGet ;   }

		void Start( INetPackHandler *pPackHandler ) ;
		void SetPackHandler( INetPackHandler *pPackHandler ) {    m_pPackHandler =pPackHandler ;   }

		enum DisconnectMode
		{
			RemoveFromManager,
			KeepInManager,
		};

		void PostClose( DisconnectMode mode = RemoveFromManager ) ; 
		//void PostRecvPacket( void ) ;

		/*
		 * AsSendStackPacket() is Async .
		 *
		 * param : packet, must the stack memory .
		 * 
		 */ 
		bool AsSendStackPacket( NetPack &packet ) ;

		/*
		 * SendNewPacket() is Async .
		 *
		 * param : pPack, must the head memory .
		 * 
		 */ 
		bool AsSendNewPacket( NetPackPtr &pPack ) ;

		void SetConnectTimeMS( uint64 time ) {    m_connectTimeMS =time ;   }
		uint64 GetConnectTimeMS( void ) {    return m_connectTimeMS ;   }

		bool IsDisconnected( void ) const;
		void Disconnect( DisconnectMode mode = RemoveFromManager ) ;

	public:
		~TcpAsyncConn( void ) ;

	private:
		TcpAsyncConn( ITcpConnMgr *pMgr, boost::asio::io_service &io_service, uint32 id ) ;


		void AsyncReadHeader( void ) ;
		void AsyncReadBody( void ) ;
		void HandleReadHeader( const boost::system::error_code& error, size_t bytes_transferred ) ;
		void HandleReadBody( const boost::system::error_code& error, size_t bytes_transferred ) ;

		void AsyncSend( NetPackPtr &pPack ) ;
		void HandleWrite( const boost::system::error_code& error, size_t bytes_transferred, NetPackPtr pPack ) ;

		void CloseSocket( void ) ;
		bool DisconnectedOrHasError( const boost::system::error_code& error ) ;

	private:
		volatile boost::atomic_bool  m_isGetPacket ;
		uint32                       m_id;
		int                          m_port;
		uint64                       m_connectTimeMS ;
		std::string                  m_ip;

		ITcpConnMgr                 *m_pMgr;
		INetPackHandler             *m_pPackHandler;

		boost::asio::io_service&     m_ioSvc;
		boost::asio::ip::tcp::socket m_socket;

		NetPack                      m_recvHeader;
		NetPackPtr                   m_pRecvPack;

	public:
		//void IncreasePostRead( void ) {   ++m_postRead ;   }
		//void DecreasePostRead( void ) {   --m_postRead ;   }
		//volatile boost::atomic_int& GetPostRead( void ) {   return m_postRead;   }

		//void IncreasePostWrite( void ) {   ++m_postWrite ;   }
		//void DecreasePostWrite( void ) {   --m_postWrite ;   }
		//volatile boost::atomic_int& GetPostWrite( void ) {   return m_postWrite;   }

		void SetIsPostClose( bool bVal ) {   m_isPostClose =bVal ;    }
		volatile boost::atomic_bool& GetIsPostClose( void ) {    return m_isPostClose ;   }

		void SetIsFinishClose( bool bVal ) {   m_isFinisClose =bVal ;    }
		volatile boost::atomic_bool& GetIsFinishClose( void ) {    return m_isFinisClose ;   }

	private:
		volatile boost::atomic_bool  m_isPostClose ;
		volatile boost::atomic_bool  m_isFinisClose ;
		//volatile boost::atomic_int   m_postRead ;
		//volatile boost::atomic_int   m_postWrite ;
};

