#include "aoipclient.h"

#include <QMessageBox>



int output(void *outputBuffer, void * /*inputBuffer*/, unsigned int nBufferFrames,
	double /*streamTime*/, RtAudioStreamStatus /*status*/, void *data)
{
	AoIPClient *client = (AoIPClient*)data;

	client->readFrames((MY_TYPE*)outputBuffer, nBufferFrames);

	return 0;
}

AoIPClient::AoIPClient(QWidget *parent)
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

	ui.editIp->setValidator(ipValidator);
	ui.editPort->setValue(AOIP_DEFAULT_RECV_PORT);
	m_isRefresh = false;

	m_fs = AOIP_DEFAULT_SAMPLE_RATE;
	m_channels = AOIP_DEFAULT_AUDIO_CHANNELS;
	m_bufferFrames = AOIP_DEFAULT_BUFFER_FRAMES;

	createFrameBuffer();

	if (!createOpusDecoder()) {
		QMessageBox::warning(this, "Error", "Failed to create opus decoder.");
	}

	refreshAudioDevices();
	
	if (!startAudioStream()) {
		QMessageBox::warning(this, "Error", "Failed to start audio stream.");
	}

	m_sess = NULL;

	m_thRecv = new AoipRecvThread(this);
	m_thRecv->start();
}

AoIPClient::~AoIPClient()
{
	if (m_thRecv->isRunning()) {
		m_thRecv->terminate();
		m_thRecv->wait();
	}
	stopAudioStream();
	destroyOpusDecoder();
	deleteFrameBuffer();
}

int AoIPClient::createOpusDecoder()
{
	int err;
	m_dec = opus_decoder_create(m_fs, m_channels, &err);
	if (err != OPUS_OK || m_dec == NULL)
		return 0;

	return 1;
}

void AoIPClient::destroyOpusDecoder()
{
	opus_decoder_destroy(m_dec);
}

int AoIPClient::refreshAudioDevices()
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
	}
	else {
		ui.cmbADevices->addItem("Default Device", QVariant(9999));
		for (unsigned int i = 0; i < devices; i++) {
			info = m_audio.getDeviceInfo(i);

			QString str_defaut = info.isDefaultInput ? " - Default Input" : info.isDefaultOutput ? " - Default Output" : "";

			ui.cmbADevices->addItem(QString::fromStdString(info.name) + str_defaut, QVariant(i));
		}
	}

	m_isRefresh = false;
	return ret;
}

int AoIPClient::startAudioStream()
{
	RtAudio::StreamParameters oParams;
	oParams.deviceId = ui.cmbADevices->currentData().toInt();
	oParams.nChannels = m_channels;
	oParams.firstChannel = 0;

	if (oParams.deviceId == 10000)
		return 0;

	if (oParams.deviceId == 9999)
		oParams.deviceId = m_audio.getDefaultOutputDevice();

	try {
		m_audio.openStream(&oParams, NULL, FORMAT, m_fs, &m_bufferFrames, &output, (void *)this);
		m_audio.startStream();
	}
	catch (RtAudioError& e) {
		return 0;
	}

	return 1;
}

int AoIPClient::stopAudioStream()
{
	if (m_audio.isStreamOpen()) m_audio.closeStream();
	return 1;
}

uint AoIPClient::getBufferFrames()
{
	return m_bufferFrames;
}



void AoIPClient::createFrameBuffer()
{
	m_maxFrames = AOIP_BUFFER_MAX_DURATION * m_fs;
	m_buffer = (MY_TYPE *)malloc(m_maxFrames * m_channels * sizeof(MY_TYPE));
	m_currentFrame = 0;
}

void AoIPClient::deleteFrameBuffer()
{
	if (m_buffer)
		free(m_buffer);
	m_buffer = NULL;
}

void AoIPClient::initFrameBuffer()
{
	lockBuffer();
	m_currentFrame = 0;
	unlockBuffer();
}

void AoIPClient::writeFrames(MY_TYPE* in, uint frames)
{
	if (m_buffer == NULL)
		return;

	if (!m_audio.isStreamRunning()) {
		initFrameBuffer();
		return;
	}

	lockBuffer();
	unsigned long offset = m_currentFrame * m_channels;
	unsigned long bytes = frames * m_channels * sizeof(MY_TYPE);
	memcpy(m_buffer + offset, in, bytes);

	if (m_currentFrame + frames > m_maxFrames) {
		memmove(m_buffer, m_buffer + m_bufferFrames * m_channels, m_maxFrames * m_channels * sizeof(MY_TYPE));
	} else {
		m_currentFrame += frames;
	}
	unlockBuffer();
}

void AoIPClient::readFrames(MY_TYPE* out, uint frames)
{
	lockBuffer();
	if (m_currentFrame == 0) {
		memset(out, 0, frames * m_channels * sizeof(MY_TYPE));
	} else if (m_currentFrame < frames) {
		memcpy(out, m_buffer, frames * m_channels * sizeof(MY_TYPE));
		m_currentFrame = 0;
	} else {
		memcpy(out, m_buffer, frames * m_channels * sizeof(MY_TYPE));
		memmove(m_buffer, m_buffer + frames * m_channels, m_maxFrames * m_channels * sizeof(MY_TYPE));
		m_currentFrame -= frames;
	}

	unlockBuffer();
}

void AoIPClient::lockBuffer()
{
	m_mutexBuffer.lock();
}

void AoIPClient::unlockBuffer()
{
	m_mutexBuffer.unlock();
}

void AoIPClient::on_btnReceive_clicked()
{
	if (m_sess == NULL) {
		QString srcAddr = ui.editIp->text();
		uint16_t portbase = ui.editPort->value();
		uint32_t srcip;

		if (srcAddr.isEmpty()) {
			QMessageBox::warning(this, "Error", "Please input the source IP address.");
			return;
		}

		srcip = inet_addr(srcAddr.toStdString().c_str());

		srcip = ntohl(srcip);
		
		m_mutexSession.lock();
		m_sess = new AoipRecvSession("sess_recv", srcip, portbase);
		m_sess->createSession();
		m_mutexSession.unlock();
		
		ui.btnReceive->setText("Stop");
		ui.editIp->setEnabled(FALSE);
		ui.editPort->setEnabled(FALSE);
	} else {
		m_mutexSession.lock();
		m_sess->destroySession();
		delete m_sess;
		m_sess = NULL;
		m_mutexSession.unlock();

		ui.btnReceive->setText("Receive");
		ui.editIp->setEnabled(TRUE);
		ui.editPort->setEnabled(TRUE);
	}
}

void AoIPClient::on_cmbADevices_currentIndexChanged(int)
{
	if (m_isRefresh)
		return;

	stopAudioStream();
	if (!startAudioStream()) {
		QMessageBox::warning(this, "Error", "Failed to start audio stream.");
	}
	initFrameBuffer();
}

AoipRecvThread::AoipRecvThread(AoIPClient *client)
{
	m_client = client;
}

AoipRecvThread::~AoipRecvThread()
{

}

void AoipRecvThread::run()
{
	while (TRUE) {
		if (m_client->m_sess == NULL || !m_client->m_sess->IsActive()) {
			Sleep(10);
			//RTPTime::Wait(RTPTime(0, 10));
			continue;
		}
		
		m_client->m_mutexSession.lock();
		AoipRecvSession *sess = m_client->m_sess;

		sess->BeginDataAccess();

		// check incoming packets
		if (sess->GotoFirstSourceWithData())
		{
			do
			{
				RTPPacket *pack;

				while ((pack = sess->GetNextPacket()) != NULL)
				{
					// You can examine the data here
					printf("Got packet !\n");

					uint bufferFrames = m_client->getBufferFrames();
					short *out = (short*)malloc(bufferFrames * 2 * sizeof(short));

					char* d = (char*)pack->GetPayloadData();
					int a = pack->GetPacketLength();

					opus_int32 out_samples = opus_decode(m_client->m_dec, pack->GetPayloadData(), pack->GetPayloadLength(), out, bufferFrames, 0);

					m_client->writeFrames(out, out_samples);

					free(out);
					// we don't longer need the packet, so
					// we'll delete it
					sess->DeletePacket(pack);
				}
			} while (sess->GotoNextSourceWithData());
		}

		sess->EndDataAccess();

#ifndef RTP_SUPPORT_THREAD
		sess->Poll();
#endif // RTP_SUPPORT_THREAD
		m_client->m_mutexSession.unlock();
		
		Sleep(10);
		//RTPTime::Wait(RTPTime(0, 10));
	}

}