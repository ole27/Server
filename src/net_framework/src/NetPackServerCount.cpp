#include "NetPackServerCount.h"

#include "ServerOpcode.pb.h"

NetPackServerCount::NetPackServerCount( int maxOpcode, const std::string& tag ): NetPackStatistician( maxOpcode, tag )
{
}

NetPackServerCount::~NetPackServerCount( void )
{
}

const char* NetPackServerCount::GetOpCodeName( int opCode )
{
	return pb::ServerOpcode_Name( static_cast<pb::ServerOpcode>( opCode ) ).c_str() ;
}

