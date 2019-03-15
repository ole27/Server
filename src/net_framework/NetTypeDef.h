#ifndef NET_TYPE_DEF_H__
#define NET_TYPE_DEF_H__


#include <string>

#include "def/Typedef.h"
#include "memory_buffer/NetPack.h"
#include "lock_free/LockFreeQueue.h"

class Session;

typedef std::string                            Port;
typedef std::string                            IP;

struct AddressInfo
{
public:
	AddressInfo( void )
	{}

	AddressInfo( const Port &port, const IP &ipNear, const IP &ipFar ) : m_port( port ), m_ipNear( ipNear ), m_ipFar( ipFar )
	{}

	const char* GetClientUseCharIp( const char *pClientIp ) const;
	const std::string& GetClientUseIp( const std::string &clientIp ) const;

	void SetNearIp( const char* pStr ) { m_ipNear = pStr; }
	void SetFarIp( const char* pStr ) { m_ipFar = pStr; }
	void SetPort( const char* pStr ) { m_port = pStr; }

	bool IsSetting( void ) const
	{
		return (!m_ipNear.empty() || !m_ipFar.empty()) && !m_port.empty();
	}

public:
	IP   m_ipNear;
	IP   m_ipFar;
	Port m_port;

public:
	bool operator < ( const AddressInfo &a ) const
	{
		int ret = m_port.compare( a.m_port );
		if( ret == 0 )
		{
			ret = m_ipNear.compare( a.m_ipNear );
			if( ret == 0 )
			{
				ret = m_ipFar.compare( a.m_ipFar );
			}
		}

		return ret < 0;
	}
};

typedef unsigned int                                   ServerId;
typedef unsigned int                                   RegionId;
typedef std::pair< ServerId, RegionId >                SessionPairId;

typedef unsigned int                                   SessionId;
typedef unsigned int                                   ServerClinetId;
typedef unsigned int                                   SetssionType;

typedef std::vector< SessionId >                       SessionIdVec;


typedef std::set< AddressInfo >                         AddressInfoSet;
typedef std::set< SessionPairId >                       SessionPairIdSet;



typedef ServerMap< SessionId, Session* >                SessionMap;
typedef SessionMap::iterator                            SessionMapItr;

typedef ServerMap< ServerId, Session* >                 SessionByServerIdMap;
typedef SessionByServerIdMap::iterator                  SessionByServerIdMapItr;

// Multi thread queue
typedef ServerQueue< Session* >                      MLockSessionQueue;
typedef ServerQueue< Session* >                      DLockSessionQueue;

typedef ServerQueue< SessionId >                     MLockSessionIdQueue;
typedef ServerQueue< SessionId >                     DLockSessionIdQueue;

typedef CustomQueue< NetPackPtr >                  DLockPacketQueue;

typedef CustomQueue< std::pair< uint32, uint32 > >   MPairIdLockQueue;

enum ServerSessionRegisterResult
{
	euSERVER_SESSION_REGISTER_OK           = 0,
	euSERVER_SESSION_REGISTER_FAIL_ID      = 1,
	euSERVER_SESSION_REGISTER_FAIL_ADDRESS = 2,
	euSERVER_SESSION_REGISTER_JUST_CAN_ONE = 3,
};


class ServerClient;

typedef boost::shared_ptr<ServerClient>             ServerClientPtr;
typedef std::map< AddressInfo, ServerClientPtr >    ServerClientTable;

#endif


