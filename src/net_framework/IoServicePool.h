#ifndef IO_SERVICE_POOL_H__
#define IO_SERVICE_POOL_H__

#include <vector>

#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread.hpp>
#include <boost/atomic.hpp>

#include "Singleton.h"

/// A pool of io_service objects.
class IoServicePool : private boost::noncopyable, public Singleton< IoServicePool >
{
	public:
		typedef boost::asio::io_service*                            IoServicePtr ; 
		typedef boost::shared_ptr< boost::asio::signal_set >        SpSign ;
		typedef std::map< IoServicePtr, SpSign >                    SignMap ;
		typedef boost::shared_ptr< boost::asio::io_service >        SpIoServicePtr;
		typedef boost::shared_ptr< boost::asio::io_service::work >  SpWorkPtr ;
		typedef boost::shared_ptr< boost::thread >                  SpThread ;

		//typedef boost::boost::atomic<int>                           SocketCount ;
		//struct SocketCuntInfo
		//{
		//	SocketCount    count ;
		//	SpIoServicePtr spIoService ;
		//} ;
		//typedef std::map< boost::theread::id, SocketCuntInfo >      SocketCountMap ;
		//typedef SocketCountMap::iterator                            SocketCountMapItr ;
	public:
		/// Construct the io_service pool.
		explicit IoServicePool( void ) ;

		~IoServicePool( void ) ;

		/// Run all io_service objects in the pool.
		void Run( int ioServiceSize, size_t stackSize ) ;

		/// Stop all io_service objects in the pool.
		void Stop( void ) ;

		/// Get an io_service to use.
		boost::asio::io_service& GetIoService( void ) ;

		//boost::asio::io_service& GetIoServiceByCount( void ) ;
		//void AddUseCount( boost::asio::io_service &io ) ;
		//void ReduceUseCount( boost::asio::io_service &io ) ;

		// 
		boost::asio::io_service& GetServerIoService( void ) {   return *m_spServerIoService ;   }

		boost::thread::id GetServerIoThreadId( void ) {   return m_spServerIoThread->get_id() ;   } 

		void AddIoSign( IoServicePtr ptr, const std::vector<int> &signsTable ) ;
		void HandlerSigns( SpSign spSign, const boost::system::error_code& error, int signal_number ) ;

		void GetAllThreadId( std::vector< boost::thread::id > &vec ) ;
		void GetNetPacketThreadId( std::vector< boost::thread::id > &vec ) ;


	private:
		/// Server use io_service
		SpIoServicePtr                                    m_spServerIoService ;
		SpWorkPtr                                         m_spServerWork ;
		SpThread                                          m_spServerIoThread ;


		/// The next io_service to use for a connection.
		boost::atomic<size_t>                             m_nextIoServiceIndex ;

		/// The thread per io_service.
		int                                               m_oneIoServiceThreadCount ;

		/// The work that keeps the io_services running.
		std::vector< SpWorkPtr >                          m_work ;

		/// The pool of io_services.
		std::vector< SpIoServicePtr >                     m_ioServices ;

		// count socket for each io_service
		//SocketCountMap                                  m_socketCount ;

		/// The all network packet thread .
		std::vector< SpThread >                           m_threads ; // tips: not inclue list io_service thread 

		/// The signs
		SignMap                                           m_signs ;

};

#define sIoPool (IoServicePool::Instance()) 



void OnSignal( int s );

void HookSignals( void );

void UnhookSignals( void );

#endif
