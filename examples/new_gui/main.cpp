#include <QApplication>
#include <QBoxLayout>
#include <QPlainTextEdit>
#include <QTextEdit>

#include "mainwindow.hpp"

// Layout:
// +------------------------+
// |        |               |
// | Editor | Visualization |
// |        |               |
// +------------------------+
// | Log                    |
// +------------------------+

auto main(int argc, char* argv[]) -> int {
    QApplication app(argc, argv);

    MainWindow window;
    window.show();

    return app.exec(); // NOLINT
}
