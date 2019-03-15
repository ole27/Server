
#include "NetPackClientCount.h"

#include "Opcode.pb.h"

NetPackClientCount::NetPackClientCount( int maxOpcode, const std::string& tag ): NetPackStatistician( maxOpcode, tag )
{
}

NetPackClientCount::~NetPackClientCount( void )
{
}

const char* NetPackClientCount::GetOpCodeName( int opCode )
{
	return pb::Opcode_Name( static_cast<pb::Opcode>( opCode ) ).c_str() ;
}

