#include "aoipsendsession.h"

AoipSendSession::AoipSendSession()
{

}

AoipSendSession::~AoipSendSession()
{
	destroySession();
}

int AoipSendSession::sendPacket(unsigned char* data, int len)
{
	int status = 0;

	status = SendPacket((void *)data, len, 0, false, 20);

	if (status < 0)
		return -1;

	return status;
}

int AoipSendSession::addDestination(uint32_t destIp, uint16_t destPort)
{
	int status = 0;

	RTPIPv4Address addr(destIp, destPort);
	status = AddDestination(addr);
	if (status < 0)
		return 0;

	return 1;
}

int AoipSendSession::deleteDestination(uint32_t destIp, uint16_t destPort)
{
	int status = 0;

	RTPIPv4Address addr(destIp, destPort);
	status = DeleteDestination(addr);
	if (status < 0)
		return 0;

	return 1;
}

int AoipSendSession::createSession()
{
	int status = 0;

	RTPSessionParams sessparams;
	sessparams.SetOwnTimestampUnit(1.0 / 10.0);
	sessparams.SetAcceptOwnPackets(false);

	status = Create(sessparams);
	if (status < 0)
		return 0;

	/*
	RTPIPv4Address addr(m_destIp, m_destPort);
	status = AddDestination(addr);
	if (status < 0)
		return 0;
	*/

	return 1;
}

int AoipSendSession::destroySession()
{
	BYEDestroy(RTPTime(10, 0), 0, 0);
	return 1;
}

