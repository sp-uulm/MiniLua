#ifndef MINILUA_GUI_HPP
#define MINILUA_GUI_HPP

#include <QMainWindow>
#include <QTextBrowser>

#include <MiniLua/MiniLua.hpp>

QT_BEGIN_NAMESPACE
namespace Ui {
class Minilua;
}
QT_END_NAMESPACE

class Minilua : public QMainWindow {
    Q_OBJECT

public:
    Minilua(QMainWindow* parent = nullptr);
    ~Minilua();

private slots:
    void on_runButton_clicked();
    void on_cancelButton_released();

private:
    Ui::Minilua* ui;
    minilua::Interpreter interpreter;

    void exec_interpreter();
    void writeTextToLogger(std::string text);
};
#endif // MINILUA_HPP
