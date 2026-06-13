#include <QApplication>

#include "IggyQtShellWindow.hpp"

int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	iggy::qt_shell::IggyQtShellWindow window;
	window.show();
	return app.exec();
}
