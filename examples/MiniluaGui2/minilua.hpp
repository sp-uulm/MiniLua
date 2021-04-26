#ifndef MINILUA_GUI_HPP
#define MINILUA_GUI_HPP

#include <QFuture>
#include <QFutureWatcher>
#include <QGraphicsEllipseItem>
#include <QMainWindow>
#include <QTextBrowser>
#include <QThreadPool>
#include <QtConcurrent/QtConcurrent>

#include <functional>
#include <qfuturewatcher.h>
#include <qgraphicssceneevent.h>
#include <sstream>

#include <MiniLua/MiniLua.hpp>

QT_BEGIN_NAMESPACE
namespace Ui {
class Minilua;
}
QT_END_NAMESPACE

class ForwardingOutStream : public std::streambuf {
    std::function<void(std::string)> callback;

public:
    ForwardingOutStream(std::function<void(std::string)> callback);

protected:
    auto xsputn(const char_type* s, std::streamsize count) -> std::streamsize override;
};

class MovableCircle : public QGraphicsEllipseItem {
    std::function<void(QPointF)> on_move;
    std::function<void(bool)> on_select;
    std::function<void()> on_mouse_released;

public:
    minilua::Value lua_x; // NOLINT
    minilua::Value lua_y; // NOLINT

    explicit MovableCircle(minilua::Value x, minilua::Value y, double size, Qt::GlobalColor color);

    void set_on_move(std::function<void(QPointF)> on_move);
    void set_on_select(std::function<void(bool)> on_select);
    void set_on_mouse_released(std::function<void()> on_mouse_released);

    void update_value_ranges(const minilua::RangeMap& range_map);

protected:
    auto itemChange(GraphicsItemChange change, const QVariant& value) -> QVariant override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
};

class Minilua : public QMainWindow {
    Q_OBJECT // NOLINT

        public : Minilua(QMainWindow* parent = nullptr);
    ~Minilua() override;

private slots:
    void on_runButton_clicked();
    void on_cancelButton_released();

    void hide_cancel_button();

    void writeTextToLog(const std::string& text);
    void writeErrorToLog(const std::string& text);

    void
    create_circle(minilua::Value x, minilua::Value y, minilua::Value size, Qt::GlobalColor color);
    void apply_move_source_change(MovableCircle* circle, QPointF new_point);

signals:
    void new_stdout(std::string str);
    void new_stderr(std::string str);
    void new_circle(minilua::Value x, minilua::Value y, minilua::Value size, Qt::GlobalColor color);
    void circle_moved(MovableCircle* circle, QPointF new_point);
    void mouse_released();

private:
    Ui::Minilua* ui;

    minilua::Interpreter interpreter;

    std::vector<MovableCircle*> circles;

    ForwardingOutStream out_buf;
    ForwardingOutStream err_buf;

    std::ostream out_stream;
    std::ostream err_stream;

    QThreadPool pool;
    QFuture<void> future;
    QFutureWatcher<void> watcher;

    void exec_interpreter();
    void clear_circles();
};
#endif // MINILUA_HPP
