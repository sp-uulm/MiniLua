#ifndef GUI_H
#define GUI_H

#include <QtGui>
#include <QtWidgets>
#include <memory>

namespace lua {
namespace rt {
struct SourceChange;
}
}

class DrawWidget : public QWidget {
    Q_OBJECT

    QPlainTextEdit *editor = nullptr;
    std::shared_ptr<struct _LuaChunk> parse_result;
    std::shared_ptr<lua::rt::SourceChange> current_source_changes;

public:
    DrawWidget(QWidget *parent, QPlainTextEdit *editor) : QWidget {parent}, editor {editor} {

        // textChanged is emitted whenever formatting is applied, therefore we use cursorPositionChanged
        connect(editor, &QPlainTextEdit::cursorPositionChanged,
                this, &DrawWidget::onTextChanged);

        editor->setFont(QFont("monospace"));
    }

    virtual ~DrawWidget() {

    }

    void paintEvent(QPaintEvent *event);

    void addSourceChanges(const std::shared_ptr<lua::rt::SourceChange>& change);
    void clearSourceChanges();

    void highlightSourceChanges(QPlainTextEdit* editor);

public slots:
    void onTextChanged();
};


#endif // GUI_H
