#pragma once


#ifndef WIN32
#include <netinet/in.h>
#include <arpa/inet.h>
#else
#include <winsock2.h>
#endif // WIN32

#include <QThread>
#include <QList>
#include <QMutex>

#include "rtpsession.h"
#include "rtpsessionparams.h"
#include "rtpipv4address.h"
#include "rtpudpv4transmitter.h"
#include "rtppacket.h"

#include "RtAudio.h"
#include "rtaudio_c.h"

#include "opus.h"

typedef signed short MY_TYPE;
#define FORMAT RTAUDIO_SINT16

#define AOIP_DEFAULT_SEND_PORT		20216
#define AOIP_DEFAULT_RECV_PORT		20216

#define MAX_PACKET (512)

#define AOIP_DEFAULT_SAMPLE_RATE	48000
#define AOIP_DEFAULT_AUDIO_CHANNELS	2	
#define AOIP_DEFAULT_BUFFER_FRAMES	960

#define AOIP_BUFFER_MAX_DURATION	5