#include "mainwindow.hpp"
#include "MiniLua/source_change.hpp"
#include <QBoxLayout>
#include <QGraphicsEllipseItem>
#include <QGraphicsSceneEvent>
#include <QMimeData>
#include <QPushButton>
#include <QTimer>
#include <QtConcurrent/QtConcurrent>
#include <qnamespace.h>
#include <unistd.h>
#include <utility>

const static char* const INITIAL_TEXT = R"(
addCircle(0, 0, 100, "red")
addCircle(50, 50, 100, "blue")
)";

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
void MovableCircle::update_value_ranges(
    const std::unordered_map<minilua::Range, minilua::Range>& range_map) {
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

// class MainWindow
MainWindow::MainWindow(QWidget* parent)
    : QWidget(parent), interpreter(), out_buf([this](auto str) { emit new_stdout(str); }),
      err_buf([this](auto str) { emit new_stderr(str); }), out_stream(&out_buf),
      err_stream(&err_buf) {

    connect(this, &MainWindow::new_stdout, this, &MainWindow::insert_stdout);
    connect(this, &MainWindow::new_stderr, this, &MainWindow::insert_stderr);
    connect(this, &MainWindow::new_circle, this, &MainWindow::create_circle);
    connect(this, &MainWindow::circle_moved, this, &MainWindow::apply_move_source_change);

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

    auto* base_box = new QVBoxLayout(this);
    this->setLayout(base_box);

    // Util area
    auto* util_area = new QWidget();
    base_box->addWidget(util_area);
    auto* util_layout = new QHBoxLayout();
    util_area->setLayout(util_layout);
    util_layout->setAlignment(Qt::Alignment(Qt::AlignLeft));

    auto* run_button = new QPushButton();
    util_layout->addWidget(run_button);
    run_button->setText("Run");
    run_button->setFixedWidth(100); // NOLINT
    connect(run_button, &QPushButton::clicked, this, &MainWindow::handle_run_button);

    // Main Area
    auto* main_area = new QWidget();
    base_box->addWidget(main_area, 1);
    auto* main_layout = new QHBoxLayout();
    main_area->setLayout(main_layout);

    // Editor
    this->editor = new QTextEdit();
    main_layout->addWidget(this->editor);
    this->editor->setPlainText(INITIAL_TEXT);
    this->editor->setSizePolicy(
        QSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding));

    // Visualization
    auto* scene = new QGraphicsScene();
    this->viz = new QGraphicsView(scene);
    main_layout->addWidget(this->viz);
    this->viz->setSizePolicy(
        QSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding));
    // set the scene size so it does not move around on screen
    scene->setSceneRect(-250, -250, 500, 500); // NOLINT

    auto* zero_text = scene->addSimpleText("0");
    zero_text->setPos(0, 0);
    // always keep on top
    zero_text->setZValue(1000); // NOLINT

    // axis lines
    scene->addLine(-250, 0, 250, 0); // NOLINT
    scene->addLine(0, -250, 0, 250); // NOLINT

    // Log
    this->log = new QTextEdit();
    base_box->addWidget(log);
    log->setReadOnly(true);
    log->setSizePolicy(QSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding));
}

MainWindow::~MainWindow() {
    delete this->editor;
    delete this->viz;
}

void MainWindow::handle_run_button() {
    this->insert_log("== Running program! ==\n", "green");

    clear_circles();
    QFuture<void> future = QtConcurrent::run(&this->pool, [this]() { this->exec_interpreter(); });
}

void MainWindow::clear_circles() {
    for (auto* circle : this->circles) {
        this->viz->scene()->removeItem(circle);
        delete circle;
    }
    this->circles.clear();
}

void MainWindow::exec_interpreter() {
    const auto parse_result = this->interpreter.parse(this->editor->toPlainText().toStdString());
    if (!parse_result) {
        this->insert_log("== Running program! ==\n", "red");
        for (const auto& error : parse_result.errors) {
            this->insert_log(" " + error + "\n", "red");
        }
        return;
    }

    try {
        const auto eval_result = this->interpreter.evaluate();
        std::stringstream ss;
        ss << "== SUCCESSFULLY EXECUTED ==\n"
           << "   RETURN VALUE: " << eval_result.value << "\n"
           << "   SOURCE CHANGES: " << eval_result.source_change << "\n";
        this->insert_log(ss.str(), "green");
    } catch (const minilua::InterpreterException& e) {
        this->insert_log("== FAILED TO EXECUTE PROGRAM: ==\n"s + e.what() + "\n", "green");
    }
}

void MainWindow::create_circle(
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
    this->viz->scene()->addItem(circle);
}
void MainWindow::apply_move_source_change(MovableCircle* circle, QPointF new_point) {
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
    circle->update_value_ranges(range_map);

    // update editor text
    this->editor->setPlainText(QString::fromStdString(this->interpreter.source_code()));
}

void MainWindow::set_text(std::string str) { this->editor->setText(QString(str.c_str())); }

void MainWindow::insert_log(std::string str, std::optional<std::string> color) {
    this->log->moveCursor(QTextCursor::End);
    if (!color) {
        this->log->insertPlainText(QString(str.c_str()));
    } else {
        this->log->insertHtml(
            "<font color=\"" + QString::fromStdString(*color) + "\">" +
            QString(str.c_str()).replace('\n', "<br>") + "</font>");
        if (*(str.end() - 1) == '\n') {
            this->log->insertPlainText(QString('\n'));
        }
    }
    this->log->moveCursor(QTextCursor::End);
}
void MainWindow::insert_stdout(std::string str) { this->insert_log(std::move(str)); }
void MainWindow::insert_stderr(std::string str) { this->insert_log(std::move(str), "red"); }
