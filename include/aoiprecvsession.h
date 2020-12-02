#pragma once

#include "common.h"

using namespace jrtplib;

class AoipRecvSession: public RTPSession, QObject
{
public:
	AoipRecvSession(QString session_id, uint32_t srcIp, uint16_t srcPort);
	~AoipRecvSession();

	int		createSession();
	int		destroySession();

private:
	uint32_t	m_srcIp;
	uint16_t	m_srcPort;
};

