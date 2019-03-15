#ifndef NET_PACK_CLIENT_COUNT__ 
#define NET_PACK_CLIENT_COUNT__

#include "NetPackStatistician.h"

class NetPackClientCount : public NetPackStatistician
{
	public:
		NetPackClientCount( int maxOpcode, const std::string& tag ) ;
		virtual ~NetPackClientCount( void ) ;

	public:
		virtual const char* GetOpCodeName( int opCode ) ;
} ;

#endif
