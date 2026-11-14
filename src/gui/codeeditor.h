#pragma once

#include <QPlainTextEdit>

class QDragEnterEvent;
class QDropEvent;
class QEvent;
class QPaintEvent;
class QResizeEvent;
class QWidget;

class CodeEditor final : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit CodeEditor(QWidget *parent = nullptr);

    [[nodiscard]] bool goToPosition(int line, int column);

public slots:
    void deleteSelectionOrNextCharacter();

signals:
    void fileOpenRequested(const QString &path);

protected:
    void changeEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    class LineNumberArea;

    void paintLineNumberArea(QPaintEvent *event);
    void updateLineNumberAreaWidth();
    void updateLineNumberAreaGeometry();
    void updateLineNumberArea(const QRect &rect, int dy);
    void updateTabStopDistance();
    void highlightCurrentLine();
    [[nodiscard]] int lineNumberAreaWidth() const;

    LineNumberArea *const lineNumberArea_;
};
