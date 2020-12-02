#pragma once

#include <QtWidgets/QDialog>
#include "ui_aoipserver.h"

#include "common.h"

#include "aoipsendsession.h"

typedef QMap<QString, AoipSendSession*>		AoipSendSessionList;

using namespace jrtplib;

class AoIPServer : public QDialog
{
	Q_OBJECT

public:
	AoIPServer(QWidget *parent = Q_NULLPTR);
	~AoIPServer();

	friend class AoipSendThread;

	int			createOpusEncoder();
	void		destroyOpusEncoder();

	int			refreshAudioDevices();
	int			startAudioStream();
	int			stopAudioStream();
	int			getChannelCount();

	int			addConnection(QString addr, uint16_t port);
	int			removeConnection(QString addr, uint16_t port);

	void		lockSession();
	void		unlockSession();

private slots:
	void		on_btnSend1_clicked();
	void		on_btnSend2_clicked();
	void		on_btnSend3_clicked();
	void		on_btnSend4_clicked();
	void		on_btnSend5_clicked();

	void		on_cmbADevices_currentIndexChanged(int index);

public:
	AoipSendSessionList		m_listSendSessions;
	AoipSendSession			*m_sess;
	OpusEncoder				*m_enc;

private:
	RtAudio					m_audio;
	uint					m_bufferFrames;
	uint					m_fs;
	uint					m_channels;
	bool					m_isRefresh;

	QMutex					m_mutexSession;
private:
	Ui::AoIPServerClass ui;
};
