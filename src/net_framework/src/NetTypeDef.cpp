#include "NetTypeDef.h"

const char* AddressInfo::GetClientUseCharIp( const char *pClientIp ) const
{
	return GetClientUseIp( pClientIp ).c_str();
}

const std::string& AddressInfo::GetClientUseIp( const std::string &clientIp ) const
{
	if( m_ipNear.empty() )
	{
		return m_ipFar;
	}

	if( m_ipFar.empty() )
	{
		return m_ipNear;
	}

	size_t cPos =clientIp.find_last_of( '.' );
	if( cPos == std::string::npos )
	{
		return m_ipFar;
	}

	size_t nPos = m_ipNear.find_last_of( '.' );
	if( cPos == std::string::npos )
	{
		return m_ipFar;
	}

	if( cPos == nPos )
	{
		for( size_t i = 0; i < cPos; ++i )
		{
			if( clientIp[ i ] != m_ipNear[ i ] )
			{
				return m_ipFar;
			}
		}

		return m_ipNear;
	}
	else
	{
		return m_ipFar;
	}
}


