#include "minilua.hpp"
#include "./ui_minilua.h"

Minilua::Minilua(QMainWindow* parent) : QMainWindow(parent), ui(new Ui::Minilua) {
    ui->setupUi(this);
    ui->cancelButton->setVisible(false);
}

Minilua::~Minilua() { delete ui; }

void Minilua::writeTextToLogger(std::string text) { ui->log->append(QString::fromStdString(text)); }

void Minilua::on_runButton_clicked() {
    ui->cancelButton->setVisible(true);
    std::string source_code = ui->inputField->toPlainText().toStdString();
    this->writeTextToLogger("Application started");
}

void Minilua::on_cancelButton_released() {
    ui->cancelButton->setVisible(false);
    this->writeTextToLogger("Application stopped");
}

void Minilua::exec_interpreter() { auto parse_result = 0; }
