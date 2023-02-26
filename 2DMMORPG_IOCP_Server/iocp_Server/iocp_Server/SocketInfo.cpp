#include "SocketInfo.h"

SocketInfo::SocketInfo()
{
	m_recv_over.wsabuf[0].len = MAX_BUFFER;
	m_recv_over.wsabuf[0].buf = m_recv_over.buffer;
	m_recv_over.event_type = EV_RECV;
}

SocketInfo::~SocketInfo()
{
}
