#ifndef MINILUA_HPP
#define MINILUA_HPP

#include <QMainWindow>
#include <QTextBrowser>

//#include <MiniLua/MiniLua.hpp>

QT_BEGIN_NAMESPACE
namespace Ui {
class Minilua;
}
QT_END_NAMESPACE

class Minilua : public QMainWindow {
    Q_OBJECT

public:
    Minilua(QWidget* parent = nullptr);
    ~Minilua();

private slots:
    void on_runButton_clicked();

    void on_cancelButton_released();

private:
    Ui::Minilua* ui;
    void writeTextToLogger(std::string text);
    void exec_interpreter();
};

class LoggerStream : public QTextEdit {
public:
    LoggerStream& operator<<(const QString& s) {
        append(s);
        return *this;
    }
};

#endif // MINILUA_HPP
