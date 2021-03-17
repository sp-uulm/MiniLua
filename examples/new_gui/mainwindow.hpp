#include <QGraphicsView>
#include <QTextEdit>
#include <QThreadPool>

#include <MiniLua/MiniLua.hpp>
#include <functional>
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

class MainWindow : public QWidget {
    Q_OBJECT; // NOLINT

    QTextEdit* editor;
    QTextEdit* log;
    QWidget* viz_box;
    QGraphicsView* viz;
    QGraphicsEllipseItem* circle;

    minilua::Interpreter interpreter;

    ForwardingOutStream out_buf;
    ForwardingOutStream err_buf;

    std::ostream out_stream;
    std::ostream err_stream;

    QThreadPool pool;

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void handle_run_button();
    void insert_stdout(std::string str);
    void insert_stderr(std::string str);
    void set_circle(double x, double y, double size);

signals:
    void new_stdout(std::string str);
    void new_stderr(std::string str);
    void new_circle(double x, double y, double size);

private: // NOLINT
    void set_text(std::string str);
    void exec_interpreter();
};
