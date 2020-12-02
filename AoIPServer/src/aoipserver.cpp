#include "aoipserver.h"

#include <QMessageBox>

// Platform-dependent sleep routines.
#if defined( WIN32 )
#include <windows.h>
#define SLEEP( milliseconds ) Sleep( (DWORD) milliseconds ) 
#else // Unix variants
#include <unistd.h>
#define SLEEP( milliseconds ) usleep( (unsigned long) (milliseconds * 1000.0) )
#endif

typedef signed short MY_TYPE;
#define FORMAT RTAUDIO_SINT16

// Interleaved buffers
int input(void * /*outputBuffer*/, void *inputBuffer, unsigned int nBufferFrames,
	double /*streamTime*/, RtAudioStreamStatus /*status*/, void *data)
{
	AoIPServer *server = (AoIPServer *)data;

	if (!server->m_sess->IsActive())
		return 0;
	
	unsigned int frames = nBufferFrames;
	unsigned long bufferBytes = nBufferFrames * server->getChannelCount() * sizeof(MY_TYPE);

	unsigned char packet[MAX_PACKET + 32];
	int len;
	len = opus_encode(server->m_enc, (short*)inputBuffer, nBufferFrames, packet, MAX_PACKET);
	
	server->lockSession();
	server->m_sess->sendPacket(packet, len);	
	server->unlockSession();
	
	return 0;
}

AoIPServer::AoIPServer(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);

	setWindowFlags(Qt::WindowCloseButtonHint | Qt::WindowMinimizeButtonHint);

	QString ipRange = "(?:[0-1]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])";
	// You may want to use QRegularExpression for new code with Qt 5 (not mandatory).
	QRegExp ipRegex("^" + ipRange
		+ "\\." + ipRange
		+ "\\." + ipRange
		+ "\\." + ipRange + "$");
	QRegExpValidator *ipValidator = new QRegExpValidator(ipRegex, this);

	ui.editIp1->setValidator(ipValidator);
	ui.editIp2->setValidator(ipValidator);
	ui.editIp3->setValidator(ipValidator);
	ui.editIp4->setValidator(ipValidator);
	ui.editIp5->setValidator(ipValidator);

	//ui.editIp1->setText("10.10.10.140");

	ui.editPort1->setValue(AOIP_DEFAULT_SEND_PORT);
	ui.editPort2->setValue(AOIP_DEFAULT_SEND_PORT);
	ui.editPort3->setValue(AOIP_DEFAULT_SEND_PORT);
	ui.editPort4->setValue(AOIP_DEFAULT_SEND_PORT);
	ui.editPort5->setValue(AOIP_DEFAULT_SEND_PORT);

	m_isRefresh = false;

	m_sess = new AoipSendSession();

	if (!m_sess->createSession()) {
		QMessageBox::warning(this, "Error", "Failed to create rtp session.");
	}

	m_fs = AOIP_DEFAULT_SAMPLE_RATE;
	m_channels = AOIP_DEFAULT_AUDIO_CHANNELS;
	m_bufferFrames = AOIP_DEFAULT_BUFFER_FRAMES;

	if (!createOpusEncoder()) {
		QMessageBox::warning(this, "Error", "Failed to create opus encoder.");
	}

	refreshAudioDevices();

	if (!startAudioStream()) {
		QMessageBox::warning(this, "Error", "Failed to start audio stream.");
	}
}


AoIPServer::~AoIPServer()
{
	stopAudioStream();
	destroyOpusEncoder();
	m_sess->destroySession();
}

int AoIPServer::createOpusEncoder()
{
	int err;
	m_enc = opus_encoder_create(m_fs, m_channels, OPUS_APPLICATION_AUDIO, &err);
	if (err != OPUS_OK || m_enc == NULL)
		return 0;

	//if (opus_encoder_ctl(m_enc, OPUS_SET_BANDWIDTH(OPUS_AUTO)) != OPUS_OK) return 0;
	//if (opus_encoder_ctl(m_enc, OPUS_SET_FORCE_MODE(-2)) != OPUS_BAD_ARG) return 0;

	return 1;
}

void AoIPServer::destroyOpusEncoder()
{
	opus_encoder_destroy(m_enc);
}

int AoIPServer::refreshAudioDevices()
{
	RtAudio::DeviceInfo info;
	int ret = 1;
	m_isRefresh = true;

	ui.cmbADevices->clear();

	unsigned int devices = m_audio.getDeviceCount();
	std::cout << "\nFound " << devices << " device(s) ...\n";

	if (devices == 0) {
		QMessageBox::warning(this, "Error", "No audio devices found!");
		ui.cmbADevices->addItem("No Device", QVariant(10000));
		ret = 0;
	} else {
		ui.cmbADevices->addItem("Default Device", QVariant(9999));
		for (unsigned int i = 0; i < devices; i++) {
			info = m_audio.getDeviceInfo(i);

			QString str_defaut = info.isDefaultInput ? " - Default Input" : info.isDefaultOutput ? " - Default Output" : "";

			ui.cmbADevices->addItem(QString::fromStdString(info.name) + str_defaut, QVariant(i));
		}
		ret = devices;
	}

	m_isRefresh = false;
	return ret;
}

void AoIPServer::on_btnSend1_clicked()
{
	if (ui.btnSend1->text() == "Stop") {
		QString destAddr = ui.editIp1->text();
		uint16_t destPort = ui.editPort1->value();
		
		if (!removeConnection(destAddr, destPort)) {
			QMessageBox::warning(this, "Error", "Failed to remove connection.");
			return;
		}

		ui.editIp1->setEnabled(TRUE);
		ui.editPort1->setEnabled(TRUE);
		ui.btnSend1->setText("Start");
	} else {
		QString destAddr = ui.editIp1->text();
		uint16_t destPort = ui.editPort1->value();

		if (destAddr.isEmpty()) {
			QMessageBox::warning(this, "Error", "Please input the destination IP address.");
			return;
		}

		if (!addConnection(destAddr, destPort)) {
			QMessageBox::warning(this, "Error", "Failed to add connection.");
			return;
		}

		ui.editIp1->setEnabled(FALSE);
		ui.editPort1->setEnabled(FALSE);
		ui.btnSend1->setText("Stop");
	}
}

void AoIPServer::on_btnSend2_clicked()
{
	if (ui.btnSend2->text() == "Stop") {
		QString destAddr = ui.editIp2->text();
		uint16_t destPort = ui.editPort2->value();

		if (!removeConnection(destAddr, destPort)) {
			QMessageBox::warning(this, "Error", "Failed to remove connection.");
			return;
		}

		ui.editIp2->setEnabled(TRUE);
		ui.editPort2->setEnabled(TRUE);
		ui.btnSend2->setText("Start");
	}
	else {
		QString destAddr = ui.editIp2->text();
		uint16_t destPort = ui.editPort2->value();

		if (destAddr.isEmpty()) {
			QMessageBox::warning(this, "Error", "Please input the destination IP address.");
			return;
		}

		if (!addConnection(destAddr, destPort)) {
			QMessageBox::warning(this, "Error", "Failed to add connection.");
			return;
		}

		ui.editIp2->setEnabled(FALSE);
		ui.editPort2->setEnabled(FALSE);
		ui.btnSend2->setText("Stop");
	}
}

void AoIPServer::on_btnSend3_clicked()
{
	if (ui.btnSend3->text() == "Stop") {
		QString destAddr = ui.editIp3->text();
		uint16_t destPort = ui.editPort3->value();

		if (!removeConnection(destAddr, destPort)) {
			QMessageBox::warning(this, "Error", "Failed to remove connection.");
			return;
		}

		ui.editIp3->setEnabled(TRUE);
		ui.editPort3->setEnabled(TRUE);
		ui.btnSend3->setText("Start");
	}
	else {
		QString destAddr = ui.editIp3->text();
		uint16_t destPort = ui.editPort3->value();

		if (destAddr.isEmpty()) {
			QMessageBox::warning(this, "Error", "Please input the destination IP address.");
			return;
		}

		if (!addConnection(destAddr, destPort)) {
			QMessageBox::warning(this, "Error", "Failed to add connection.");
			return;
		}

		ui.editIp3->setEnabled(FALSE);
		ui.editPort3->setEnabled(FALSE);
		ui.btnSend3->setText("Stop");
	}
}

void AoIPServer::on_btnSend4_clicked()
{
	if (ui.btnSend4->text() == "Stop") {
		QString destAddr = ui.editIp4->text();
		uint16_t destPort = ui.editPort4->value();

		if (!removeConnection(destAddr, destPort)) {
			QMessageBox::warning(this, "Error", "Failed to remove connection.");
			return;
		}

		ui.editIp4->setEnabled(TRUE);
		ui.editPort4->setEnabled(TRUE);
		ui.btnSend4->setText("Start");
	}
	else {
		QString destAddr = ui.editIp4->text();
		uint16_t destPort = ui.editPort4->value();

		if (destAddr.isEmpty()) {
			QMessageBox::warning(this, "Error", "Please input the destination IP address.");
			return;
		}

		if (!addConnection(destAddr, destPort)) {
			QMessageBox::warning(this, "Error", "Failed to add connection.");
			return;
		}

		ui.editIp4->setEnabled(FALSE);
		ui.editPort4->setEnabled(FALSE);
		ui.btnSend4->setText("Stop");
	}
}

void AoIPServer::on_btnSend5_clicked()
{
	if (ui.btnSend5->text() == "Stop") {
		QString destAddr = ui.editIp5->text();
		uint16_t destPort = ui.editPort5->value();

		if (!removeConnection(destAddr, destPort)) {
			QMessageBox::warning(this, "Error", "Failed to remove connection.");
			return;
		}

		ui.editIp5->setEnabled(TRUE);
		ui.editPort5->setEnabled(TRUE);
		ui.btnSend5->setText("Start");
	}
	else {
		QString destAddr = ui.editIp5->text();
		uint16_t destPort = ui.editPort5->value();

		if (destAddr.isEmpty()) {
			QMessageBox::warning(this, "Error", "Please input the destination IP address.");
			return;
		}

		if (!addConnection(destAddr, destPort)) {
			QMessageBox::warning(this, "Error", "Failed to add connection.");
			return;
		}

		ui.editIp5->setEnabled(FALSE);
		ui.editPort5->setEnabled(FALSE);
		ui.btnSend5->setText("Stop");
	}
}

void AoIPServer::on_cmbADevices_currentIndexChanged(int index)
{
	if (m_isRefresh)
		return;

	//QMessageBox::information(this, "Info", "1");
	stopAudioStream();
	//QMessageBox::information(this, "Info", "2");
	if (!startAudioStream()) {
		QMessageBox::warning(this, "Error", "Failed to start audio stream.");
	}
	//QMessageBox::information(this, "Info", "3");
}

int AoIPServer::startAudioStream()
{
	RtAudio::StreamParameters iParams;
	iParams.deviceId = ui.cmbADevices->currentData().toInt();
	iParams.nChannels = m_channels;
	iParams.firstChannel = 0;

	if (iParams.deviceId == 10000)
		return 0;

	if (iParams.deviceId == 9999)
		iParams.deviceId = m_audio.getDefaultOutputDevice();

	try {
		m_audio.openStream(NULL, &iParams, FORMAT, m_fs, &m_bufferFrames, &input, (void *)this);
		m_audio.startStream();
	}
	catch (RtAudioError& e) {
		return 0;
	}

	return 1;
}

int AoIPServer::stopAudioStream()
{
	if (m_audio.isStreamOpen()) 
		m_audio.closeStream();
	return 1;
}

int AoIPServer::getChannelCount()
{
	return m_channels;
}

int AoIPServer::addConnection(QString addr, uint16_t port)
{
	int ret = 1;

	uint32_t destip = inet_addr(addr.toStdString().c_str());
	destip = ntohl(destip);

	m_mutexSession.lock();
	ret = m_sess->addDestination(destip, port);
	m_mutexSession.unlock();

	return ret;
}

int AoIPServer::removeConnection(QString addr, uint16_t port)
{
	int ret = 1;

	uint32_t destip = inet_addr(addr.toStdString().c_str());
	destip = ntohl(destip);

	m_mutexSession.lock();
	ret = m_sess->deleteDestination(destip, port);
	m_mutexSession.unlock();

	return ret;

}

void AoIPServer::lockSession()
{
	m_mutexSession.lock();
}

void AoIPServer::unlockSession()
{
	m_mutexSession.unlock();
}
