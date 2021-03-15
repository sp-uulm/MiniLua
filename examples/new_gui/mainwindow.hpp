#include <QGraphicsView>
#include <QPlainTextEdit>
#include <QTextEdit>

// namespace Ui {
//     class MainWindow;
// }

class MainWindow : public QWidget {
    Q_OBJECT // NOLINT

        QTextEdit* editor;
    QWidget* viz_box;
    QGraphicsView* viz;

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;
};
