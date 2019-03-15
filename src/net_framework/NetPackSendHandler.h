#pragma once

template<typename TNetPack>
class NetPackSendHandler
{
public:
	NetPackSendHandler(boost::function<void(const TNetPack&)> fun, const TNetPack& pack)
		:m_fun(fun),m_pPack(new TNetPack(pack))
	{
	}

	NetPackSendHandler(const NetPackSendHandler& other)
		:m_fun(other.m_fun),m_pPack(other.m_pPack)
	{
	}

	void operator()()
	{
		m_fun(*m_pPack);
	}

private:
	boost::function<void(const TNetPack&)>	m_fun;
	mutable std::auto_ptr<TNetPack>			m_pPack;
};
