#include "mainwindow.hpp"
#include <QBoxLayout>
#include <QGraphicsEllipseItem>
#include <QGraphicsSceneEvent>
#include <QMimeData>

const static char* const INITIAL_TEXT = R"(
x = 0
y = 0
drawCircle(x, y, 100)
)";

// class MainWindow
MainWindow::MainWindow(QWidget* parent) : QWidget(parent) {
    auto* base_box = new QVBoxLayout(this);
    this->setLayout(base_box);

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
    QGraphicsEllipseItem* item =
        scene->addEllipse(QRectF(0, 0, 100, 100), QPen(Qt::black), QBrush(Qt::green));

    this->viz = new QGraphicsView(scene);
    main_layout->addWidget(this->viz);
    this->viz->setSizePolicy(
        QSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding));

    // Log
    auto* log = new QPlainTextEdit();
    base_box->addWidget(log);
    log->setReadOnly(true);
    log->setPlainText("Log");
    log->setSizePolicy(QSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding));
}

MainWindow::~MainWindow() {
    delete this->editor;
    delete this->viz;
}
