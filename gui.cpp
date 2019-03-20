#include "include/luaparser.h"
#include "include/luainterpreter.h"
#include "include/gui.h"

using namespace std;

void DrawWidget::paintEvent(QPaintEvent *event) {
    QPainter painter;
    painter.begin(this);
    painter.fillRect(event->rect(), Qt::white);

    if (parse_result) {
        lua::rt::Environment env;
        lua::rt::ASTEvaluator eval;

        env.populate_stdlib();

        env.t[string {"line"}] = make_shared<lua::rt::cfunction>([&painter](const lua::rt::vallist& args) -> lua::rt::vallist {
            if (args.size() != 4) {
                return {lua::rt::nil(), string {"invalid number of arguments"}};
            }

            for (int i = 0; i < 4; ++i) {
                if (!holds_alternative<double>(args[i])) {
                    return {lua::rt::nil(), string {"invalid type of argument "} + to_string(i+1) + " (number expected)"};
                }
            }

            painter.drawLine(get<double>(args[0]), get<double>(args[1]), get<double>(args[2]), get<double>(args[3]));

            return {};
        });

        env.t[string {"force"}] = make_shared<lua::rt::cfunction>([this](const lua::rt::vallist& args) -> lua::rt::vallist {
            if (args.size() != 2) {
                return {lua::rt::nil(), string {"wrong number of arguments (expected 2)"}};
            }

            cout << "force " << args[0] << " to be " << args[1] << endl;

            auto source_changes = args[0].forceValue(args[1]);

            if (!source_changes) {
                cout << "could not force value, source location not available" << endl;
                return {};
            }

            cout << (*source_changes)->to_string() << endl;
            addSourceChanges(*source_changes);

            return {};
        });

        clearSourceChanges();

        if (auto eval_result = parse_result->accept(eval, env); holds_alternative<string>(eval_result)) {
            cerr << "Error: " << get<string>(eval_result) << endl;
        }
    }

    painter.end();
    highlightSourceChanges(editor);
}

void DrawWidget::addSourceChanges(const shared_ptr<lua::rt::SourceChange>& change) {
    if (!current_source_changes)
        current_source_changes = make_shared<lua::rt::SourceChangeAnd>();
    dynamic_pointer_cast<lua::rt::SourceChangeAnd>(current_source_changes)->changes.push_back(change);
}

void DrawWidget::clearSourceChanges() {
    current_source_changes.reset();
}

void highlight_changes(const shared_ptr<lua::rt::SourceChange>& change, QTextCursor& cursor) {
    if (auto p = dynamic_pointer_cast<lua::rt::SourceAssignment>(change); p) {
        QTextCharFormat fmt;
        fmt.setBackground(Qt::red);

        cursor.setPosition(p->token.pos, QTextCursor::MoveAnchor);
        cursor.setPosition(p->token.pos+p->token.length, QTextCursor::KeepAnchor);
        cursor.setCharFormat(fmt);
    }

    if (auto p = dynamic_pointer_cast<lua::rt::SourceChangeAnd>(change); p) {
        for (const auto& child : p->changes)
            highlight_changes(child, cursor);
    }

    if (auto p = dynamic_pointer_cast<lua::rt::SourceChangeOr>(change); p) {
        if (!p->alternatives.empty())
            highlight_changes(p->alternatives[0], cursor);
    }
}

void DrawWidget::highlightSourceChanges(QPlainTextEdit *editor) {
    QTextCharFormat fmt;

    QTextCursor cursor(editor->document());

    cursor.setPosition(0, QTextCursor::MoveAnchor);
    cursor.setPosition(editor->document()->toPlainText().length(), QTextCursor::KeepAnchor);
    cursor.setCharFormat(fmt);


    highlight_changes(current_source_changes, cursor);
}

void DrawWidget::onTextChanged() {
    LuaParser parser;
    const auto result = parser.parse(editor->toPlainText().toStdString());

    if (holds_alternative<string>(result)) {
        cerr << "Error: " << get<string>(result) << endl;
        parse_result.reset();
    } else {
        parse_result = get<LuaChunk>(result);
        repaint();
    }
}

auto main(int argc, char *argv[]) -> int {

    QApplication app(argc, argv);
    setlocale(LC_ALL, "C");

    QWidget window;
    window.resize(1280, 720);

    QGridLayout* layout = new QGridLayout(&window);
    QPlainTextEdit* editor = new QPlainTextEdit(&window);

    DrawWidget* draw_area = new DrawWidget(&window, editor);

    draw_area->setMinimumWidth(500);

    editor->setPlainText("print(1.5)\n"
                         "i=1+1.5;\n"
                         "force(-i, 3)\n"
                         "line(0, 0, 200, 200)\n"
                         "force(1, 2)");

    window.setLayout(layout);
    layout->addWidget(editor, 0, 0);
    layout->addWidget(draw_area, 0, 1);

    window.show();
    window.setWindowTitle(
        QApplication::translate("toplevel", "QMiniLua"));
    return app.exec();
}
