#include "aoipclient.h"
#include <QtWidgets/QApplication>

#include "common.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	int ret;

#ifdef WIN32
	WSADATA dat;
	WSAStartup(MAKEWORD(2, 2), &dat);
#endif // WIN32

	AoIPClient w;
	w.show();
	ret = a.exec();

#ifdef WIN32
	WSACleanup();
#endif // WIN32

	return ret;
}
