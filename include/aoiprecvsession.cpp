#include "aoiprecvsession.h"

AoipRecvSession::AoipRecvSession(QString session_id, uint32_t srcIp, uint16_t srcPort)
{
	this->setObjectName(session_id);
	m_srcIp = srcIp;
	m_srcPort = srcPort;
}

AoipRecvSession::~AoipRecvSession()
{
	destroySession();
}

int AoipRecvSession::createSession()
{
	int status = 0;

	RTPUDPv4TransmissionParams transparams;
	RTPSessionParams sessparams;
	sessparams.SetOwnTimestampUnit(1.0 / 10.0);
	sessparams.SetAcceptOwnPackets(true);
	transparams.SetPortbase(m_srcPort);
	status = Create(sessparams, &transparams);
	if (status < 0)
		return 0;
	
	return 1;
}

int AoipRecvSession::destroySession()
{
	ClearDestinations();
	BYEDestroy(RTPTime(10, 0), 0, 0);
	return 1;
}

