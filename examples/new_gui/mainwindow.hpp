#include <QGraphicsEllipseItem>
#include <QGraphicsView>
#include <QTextEdit>
#include <QThreadPool>

#include <MiniLua/MiniLua.hpp>
#include <functional>
#include <optional>
#include <sstream>

// namespace Ui {
//     class MainWindow;
// }

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

public:
    minilua::Value lua_x; // NOLINT
    minilua::Value lua_y; // NOLINT

    explicit MovableCircle(minilua::Value x, minilua::Value y, double size, Qt::GlobalColor color);

    void set_on_move(std::function<void(QPointF)> on_move);
    void set_on_select(std::function<void(bool)> on_select);

    void update_value_ranges(const std::unordered_map<minilua::Range, minilua::Range>& range_map);

protected:
    auto itemChange(GraphicsItemChange change, const QVariant& value) -> QVariant override;
};

class MainWindow : public QWidget {
    Q_OBJECT; // NOLINT

    QTextEdit* editor;
    QTextEdit* log;
    QWidget* viz_box;
    QGraphicsView* viz;

    std::vector<MovableCircle*> circles;

    minilua::Interpreter interpreter;

    ForwardingOutStream out_buf;
    ForwardingOutStream err_buf;

    std::ostream out_stream;
    std::ostream err_stream;

    QThreadPool pool;

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    void insert_log(std::string str, std::optional<std::string> color = std::nullopt);
    void clear_circles();

private slots:
    void handle_run_button();
    void insert_stdout(std::string str);
    void insert_stderr(std::string str);

    void
    create_circle(minilua::Value x, minilua::Value y, minilua::Value size, Qt::GlobalColor color);
    void apply_move_source_change(MovableCircle* circle, QPointF new_point);

signals:
    void new_stdout(std::string str);
    void new_stderr(std::string str);
    void new_circle(minilua::Value x, minilua::Value y, minilua::Value size, Qt::GlobalColor color);
    void circle_moved(MovableCircle* circle, QPointF new_point);

private: // NOLINT
    void set_text(std::string str);
    void exec_interpreter();
};
