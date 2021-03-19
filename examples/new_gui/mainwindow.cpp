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
setCircle(0, 0, 100)
)";

// class ForwardingOutStream
ForwardingOutStream::ForwardingOutStream(std::function<void(std::string)> callback)
    : callback(std::move(callback)) {}

auto ForwardingOutStream::xsputn(const char_type* s, std::streamsize count) -> std::streamsize {
    this->callback(std::string(s, count));
    return count;
}

// class Circle
MovableCircle::MovableCircle() : QGraphicsEllipseItem() {
    this->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
}
void MovableCircle::set_on_move(std::function<void(QPointF)> on_move) {
    this->on_move = std::move(on_move);
}
void MovableCircle::set_on_select(std::function<void(bool)> on_select) {
    this->on_select = std::move(on_select);
}

auto MovableCircle::itemChange(GraphicsItemChange change, const QVariant& value) -> QVariant {
    if (change == ItemSelectedHasChanged && scene() != nullptr) {
        this->on_select(this->isSelected());
    }
    return QGraphicsEllipseItem::itemChange(change, value);
}
void MovableCircle::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    this->on_move(event->scenePos());
    QGraphicsEllipseItem::mouseMoveEvent(event);
}

// class MainWindow
MainWindow::MainWindow(QWidget* parent)
    : QWidget(parent), interpreter(), out_buf([this](auto str) { emit new_stdout(str); }),
      err_buf([this](auto str) { emit new_stderr(str); }), out_stream(&out_buf),
      err_stream(&err_buf) {

    connect(this, &MainWindow::new_stdout, this, &MainWindow::insert_stdout);
    connect(this, &MainWindow::new_stderr, this, &MainWindow::insert_stderr);
    connect(this, &MainWindow::new_circle, this, &MainWindow::set_circle);

    auto& env = this->interpreter.environment();
    env.set_stdout(&this->out_stream);
    env.set_stderr(&this->err_stream);
    env.add("setCircle", minilua::Value([this](const minilua::CallContext& ctx) {
                auto x = ctx.arguments().get(0);
                auto y = ctx.arguments().get(1);
                auto size = ctx.arguments().get(2);

                this->circle_x = x;
                this->circle_y = y;

                auto x_num = std::get<minilua::Number>(x).as_float();
                auto y_num = std::get<minilua::Number>(y).as_float();
                auto size_num = std::get<minilua::Number>(size).as_float();

                emit new_circle(x_num, y_num, size_num);
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
    run_button->setFixedWidth(100);
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
    // scene->addEllipse(QRectF(0, 0, 100, 100), QPen(Qt::black), QBrush(Qt::green));
    this->circle = new MovableCircle();
    scene->addItem(this->circle);
    this->circle->set_on_move([this](QPointF point) {
        std::cerr << "pos: " << point.x() << ", " << point.y() << "\n";
        std::cerr << "value: " << this->circle_x << ", " << this->circle_y << "\n";

        // TODO move into signal

        auto source_change_x = this->circle_x.force(point.x(), "ui_drag").value();
        auto source_change_y = this->circle_y.force(point.y(), "ui_drag").value();

        auto source_change = minilua::SourceChangeCombination();
        source_change.add(source_change_x);
        source_change.add(source_change_y);

        auto source_changes = minilua::SourceChangeTree(source_change).collect_first_alternative();

        const auto range_map = this->interpreter.apply_source_changes(source_changes);

        // update ranges in origins of stored values
        this->circle_x =
            this->circle_x.with_origin(this->circle_x.origin().with_updated_ranges(range_map));
        this->circle_y =
            this->circle_y.with_origin(this->circle_y.origin().with_updated_ranges(range_map));

        // update editor text
        this->editor->setPlainText(QString::fromStdString(this->interpreter.source_code()));
    });
    this->circle->set_on_select([this](bool selected) {
        if (selected) {
            std::cerr << "selected\n";
        }

        std::cerr << "circle: " << this->circle->pos().x() << ", " << this->circle->pos().y()
                  << "\n";
        std::cerr << "circle: " << this->circle->scenePos().x() << ", "
                  << this->circle->scenePos().y() << "\n";
    });
    this->circle->setRect(QRectF(0, 0, 100, 100));
    // border color
    this->circle->setPen(QPen(Qt::black));
    // fill color
    this->circle->setBrush(QBrush(Qt::green));

    std::cerr << "circle: " << this->circle->pos().x() << ", " << this->circle->pos().y() << "\n";
    std::cerr << "circle: " << this->circle->scenePos().x() << ", " << this->circle->scenePos().y()
              << "\n";

    this->viz = new QGraphicsView(scene);
    main_layout->addWidget(this->viz);
    this->viz->setSizePolicy(
        QSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding));

    // Log
    this->log = new QTextEdit();
    base_box->addWidget(log);
    log->setReadOnly(true);
    log->setPlainText("Log\n");
    log->setSizePolicy(QSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding));
}

MainWindow::~MainWindow() {
    delete this->editor;
    delete this->viz;
}

void MainWindow::handle_run_button() {
    this->err_stream << "Running program!\n" << std::flush;

    QFuture<void> future = QtConcurrent::run(&this->pool, [this]() { this->exec_interpreter(); });
}

void MainWindow::exec_interpreter() {
    const auto parse_result = this->interpreter.parse(this->editor->toPlainText().toStdString());
    if (!parse_result) {
        this->err_stream << "== FAILED TO PARSE: ==\n";
        for (const auto& error : parse_result.errors) {
            this->err_stream << error << "\n";
        }
        this->err_stream << std::flush;
        return;
    }

    try {
        const auto eval_result = this->interpreter.evaluate();
        this->err_stream << "== SUCCESSFULLY EXECUTED ==\n"
                         << "RETURN VALUE: " << eval_result.value << "\n"
                         << "SOURCE CHANGES: " << eval_result.source_change << "\n"
                         << std::flush;
    } catch (const minilua::InterpreterException& e) {
        this->err_stream << "== FAILED TO EXECUTE PROGRAM: ==\n" << e.what() << "\n" << std::flush;
    }
}

void MainWindow::set_circle(double x, double y, double size) {
    this->circle->setPos(x, y);
    this->circle->setRect(QRectF(0, 0, size, size));
}

void MainWindow::set_text(std::string str) { this->editor->setText(QString(str.c_str())); }

void MainWindow::insert_stdout(std::string str) {
    this->log->moveCursor(QTextCursor::End);
    this->log->insertPlainText(QString(str.c_str()));
    this->log->moveCursor(QTextCursor::End);
}
void MainWindow::insert_stderr(std::string str) {
    this->log->moveCursor(QTextCursor::End);
    this->log->insertHtml("<font color=\"red\">" + QString(str.c_str()) + "</font>");
    if (*(str.end() - 1) == '\n') {
        this->log->insertPlainText(QString('\n'));
    }
    this->log->moveCursor(QTextCursor::End);
}
