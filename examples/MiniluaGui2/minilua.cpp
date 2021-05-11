#include "minilua.hpp"
#include "./ui_minilua.h"
#include "MiniLua/interpreter.hpp"
#include "MiniLua/values.hpp"
#include <qgraphicsscene.h>
#include <qnamespace.h>
#include <qpoint.h>
#include <string>
#include <unistd.h>

static auto str_to_color(const std::string& color_str) -> Qt::GlobalColor {
    if (color_str == "red") {
        return Qt::GlobalColor::red;
    } else if (color_str == "green") {
        return Qt::GlobalColor::green;
    } else if (color_str == "blue") {
        return Qt::GlobalColor::blue;
    } else if (color_str == "cyan") {
        return Qt::GlobalColor::cyan;
    } else if (color_str == "magenta") {
        return Qt::GlobalColor::magenta;
    } else if (color_str == "yellow") {
        return Qt::GlobalColor::yellow;
    } else {
        return Qt::GlobalColor::black;
    }
}

// class ForwardingOutStream
ForwardingOutStream::ForwardingOutStream(std::function<void(std::string)> callback)
    : callback(std::move(callback)) {}

auto ForwardingOutStream::xsputn(const char_type* s, std::streamsize count) -> std::streamsize {
    this->callback(std::string(s, count));
    return count;
}

// class Circle
MovableCircle::MovableCircle(minilua::Value x, minilua::Value y, double size, Qt::GlobalColor color)
    : QGraphicsEllipseItem(), lua_x(std::move(x)), lua_y(std::move(y)) {
    this->setFlags(
        QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable |
        QGraphicsItem::ItemSendsGeometryChanges);

    auto x_num = std::get<minilua::Number>(x).as_float();
    auto y_num = std::get<minilua::Number>(y).as_float();

    this->setPos(x_num, y_num);
    this->setRect(QRectF(0, 0, size, size));
    // border color
    this->setPen(QPen(Qt::black));
    // fill color
    this->setBrush(QBrush(color));
    this->setOpacity(0.8); // NOLINT
}
void MovableCircle::set_on_move(std::function<void(QPointF)> on_move) {
    this->on_move = std::move(on_move);
}
void MovableCircle::set_on_select(std::function<void(bool)> on_select) {
    this->on_select = std::move(on_select);
}
void MovableCircle::update_value_ranges(const minilua::RangeMap& range_map) {
    this->lua_x = this->lua_x.with_origin(this->lua_x.origin().with_updated_ranges(range_map));
    this->lua_y = this->lua_y.with_origin(this->lua_y.origin().with_updated_ranges(range_map));
}

auto MovableCircle::itemChange(GraphicsItemChange change, const QVariant& value) -> QVariant {
    if (change == ItemPositionHasChanged && scene() != nullptr) {
        auto new_pos = value.toPointF();
        this->on_move(new_pos);
    }
    if (change == ItemSelectedHasChanged && scene() != nullptr) {
        this->on_select(this->isSelected());
    }
    return QGraphicsEllipseItem::itemChange(change, value);
}

Minilua::Minilua(QMainWindow* parent)
    : QMainWindow(parent), ui(new Ui::Minilua), interpreter(),
      out_buf([this](auto str) { emit new_stdout(str); }),
      err_buf([this](auto str) { emit new_stderr(str); }), out_stream(&out_buf),
      err_stream(&err_buf) {
    ui->setupUi(this);
    ui->cancelButton->setVisible(false);
    ui->graphics->setScene(new QGraphicsScene());

    connect(this, &Minilua::new_stdout, this, &Minilua::writeTextToLog);
    connect(this, &Minilua::new_stderr, this, &Minilua::writeErrorToLog);
    connect(this, &Minilua::new_circle, this, &Minilua::create_circle);
    connect(this, &Minilua::circle_moved, this, &Minilua::apply_move_source_change);

    auto& env = this->interpreter.environment();
    env.set_stdout(&this->out_stream);
    env.set_stderr(&this->err_stream);
    env.add("addCircle", minilua::Value([this](const minilua::CallContext& ctx) {
                auto x = ctx.arguments().get(0);
                auto y = ctx.arguments().get(1);
                auto size = ctx.arguments().get(2);
                auto color = ctx.arguments().get(3);

                auto qt_color = Qt::GlobalColor::black;
                if (!color.is_nil()) {
                    auto color_str = std::get<minilua::String>(color).value;
                    qt_color = str_to_color(color_str);
                }

                emit new_circle(x, y, size, qt_color);
            }));
    env.add("sleep", minilua::Value([](const minilua::CallContext& ctx) {
                auto secs = std::get<minilua::Number>(ctx.arguments().get(0)).try_as_int();
                sleep(secs);
            }));
}

Minilua::~Minilua() { delete ui; }

void Minilua::clear_circles() {
    for (auto* circle : this->circles) {
        ui->graphics->scene()->removeItem(circle);
        delete circle;
    }
    this->circles.clear();
}

void Minilua::create_circle(
    minilua::Value x, minilua::Value y, minilua::Value size, Qt::GlobalColor color) {
    auto size_num = std::get<minilua::Number>(size).as_float();
    auto* circle = new MovableCircle(std::move(x), std::move(y), size_num, color);

    circle->set_on_move([this, circle](QPointF point) { emit circle_moved(circle, point); });

    circle->set_on_select([circle](bool selected) {
        if (selected) {
            std::cerr << "selected\n";
        }

        std::cerr << "circle: " << circle->pos().x() << ", " << circle->pos().y() << "\n";
    });

    std::cerr << "created circle: " << circle->pos().x() << ", " << circle->pos().y() << "\n";

    this->circles.push_back(circle);
    ui->graphics->scene()->addItem(circle);
}

void Minilua::apply_move_source_change(MovableCircle* circle, QPointF new_point) {
    auto new_x = new_point.x();
    auto new_y = new_point.y();

    auto source_change_x = circle->lua_x.force(new_x, "ui_drag").value();
    auto source_change_y = circle->lua_y.force(new_y, "ui_drag").value();

    auto source_change = minilua::SourceChangeCombination();
    source_change.add(source_change_x);
    source_change.add(source_change_y);

    auto source_changes = minilua::SourceChangeTree(source_change).collect_first_alternative();

    const auto range_map = this->interpreter.apply_source_changes(source_changes);

    // update ranges in origins of stored values
    // NOTE: We update all ranges so the byte offset of the other literals will
    // also be moved and then they will be correct when the user next moves a
    // different circle before re-executing the program
    for (auto* other_circle : this->circles) {
        other_circle->update_value_ranges(range_map);
    }

    // update editor text
    ui->inputField->setPlainText(QString::fromStdString(this->interpreter.source_code()));
}

void Minilua::writeTextToLog(const std::string& text) {
    ui->log->append(QString::fromStdString(text));
}
void Minilua::writeErrorToLog(const std::string& text) {
    ui->log->append(QString::fromStdString(text));
}

void Minilua::on_runButton_clicked() {
    ui->cancelButton->setVisible(true);
    std::string source_code = ui->inputField->toPlainText().toStdString();
    this->writeTextToLog("Application started");

    clear_circles();
    QFuture<void> future = QtConcurrent::run(&this->pool, [this]() { this->exec_interpreter(); });
}

void Minilua::on_cancelButton_released() {
    ui->cancelButton->setVisible(false);
    this->writeTextToLog("Application stopped");
}

void Minilua::exec_interpreter() {
    const auto parse_result = this->interpreter.parse(ui->inputField->toPlainText().toStdString());
    if (!parse_result) {
        for (const auto& e : parse_result.errors) {
            writeErrorToLog(e + "\n");
        }
    }

    try {
        const auto eval_result = this->interpreter.evaluate();
        std::stringstream ss;
        ss << "   RETURN VALUE: " << eval_result.value << "\n"
           << "   SOURCE CHANGES: " << eval_result.source_change << "\n";
        writeTextToLog(ss.str());
    } catch (const minilua::InterpreterException& e) {
        writeTextToLog(e.what());
    }
}
