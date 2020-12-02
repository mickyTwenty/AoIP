#pragma once

#include <QtWidgets/QDialog>
#include "ui_aoipclient.h"

#include "common.h"

#include "aoiprecvsession.h"

using namespace jrtplib;

class AoIPClient : public QDialog
{
	Q_OBJECT

public:
	AoIPClient(QWidget *parent = Q_NULLPTR);
	~AoIPClient();

	friend class AoipRecvThread;

	int			createOpusDecoder();
	void		destroyOpusDecoder();

	int			refreshAudioDevices();
	int			startAudioStream();
	int			stopAudioStream();

	uint		getBufferFrames();

	void		createFrameBuffer();
	void		initFrameBuffer();
	void		deleteFrameBuffer();
	void		writeFrames(MY_TYPE* in, uint frames);
	void		readFrames(MY_TYPE* out, uint frames);
	void		lockBuffer();
	void		unlockBuffer();

private slots:
	void		on_btnReceive_clicked();
	void		on_cmbADevices_currentIndexChanged(int);

public:
	AoipRecvSession				*m_sess;
	QMutex						m_mutexSession;

	OpusDecoder					*m_dec;

private:
	AoipRecvThread				*m_thRecv;

	RtAudio						m_audio;
	uint						m_bufferFrames;
	uint						m_fs;
	uint						m_channels;
	bool						m_isRefresh;

	MY_TYPE						*m_buffer;
	uint						m_maxFrames;
	uint						m_currentFrame;
	QMutex						m_mutexBuffer;

private:
	Ui::AoIPClientClass ui;
};

class AoipRecvThread : public QThread
{
public:
	AoipRecvThread(AoIPClient *server);
	~AoipRecvThread();

protected:
	void		run();

private:
	AoIPClient * m_client;

};