#pragma once

#include "common.h"

using namespace jrtplib;

class AoipSendSession: public RTPSession, QObject
{
public:
	AoipSendSession();
	~AoipSendSession();

	int		createSession();
	int		destroySession();
	int		sendPacket(unsigned char *data, int len);

	int		addDestination(uint32_t destIp, uint16_t destPort);
	int		deleteDestination(uint32_t destIp, uint16_t destPort);

private:
};

