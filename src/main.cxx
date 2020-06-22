#include <qapplication.h>
#include "MainWindow.h"

int main(int argc, char* argv[])
{
	QApplication app(argc, argv);

	MainWindow* theMainWindow = new MainWindow(0, argc, argv);
	theMainWindow->show();

	return app.exec();
}
