#ifndef NET_PACK_SERVER_COUNT__ 
#define NET_PACK_SERVER_COUNT__

#include "NetPackStatistician.h"

class NetPackServerCount : public NetPackStatistician
{
	public:
		NetPackServerCount( int maxOpcode, const std::string& tag ) ;
		virtual ~NetPackServerCount( void ) ;

	public:
		virtual const char* GetOpCodeName( int opCode ) ;
} ;

#endif
