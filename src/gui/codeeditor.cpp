#include "codeeditor.h"

#include "syntaxhighlighter.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QEvent>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QMimeData>
#include <QPaintEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextEdit>
#include <QTextFormat>
#include <QUrl>

namespace {
    constexpr int editorFontPointSize = 11;
    constexpr int tabWidthInSpaces = 4;
    constexpr int lineNumberPadding = 14;
    constexpr int lineNumberRightPadding = 7;

    QString localFilePath(const QMimeData &mimeData)
    {
        const auto urls = mimeData.urls();
        if (urls.size() == 1 && urls.first().isLocalFile()) {
            return urls.first().toLocalFile();
        }
        return {};
    }
}

class CodeEditor::LineNumberArea final : public QWidget
{
public:
    explicit LineNumberArea(CodeEditor &editor) : QWidget(&editor), editor_(editor) {}

protected:
    void paintEvent(QPaintEvent *event) override
    {
        editor_.paintLineNumberArea(event);
    }

private:
    CodeEditor &editor_;
};

CodeEditor::CodeEditor(QWidget *parent)
    : QPlainTextEdit(parent), lineNumberArea_(new LineNumberArea(*this))
{
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    mono.setPointSize(editorFontPointSize);
    setFont(mono);
    updateTabStopDistance();
    setLineWrapMode(QPlainTextEdit::NoWrap);
    setPlaceholderText(QStringLiteral("Введите конструкцию if ... else ..."));
    setAcceptDrops(true);
    new SyntaxHighlighter(document());

    connect(this, &QPlainTextEdit::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest, this, &CodeEditor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &CodeEditor::highlightCurrentLine);

    updateLineNumberAreaWidth();
    highlightCurrentLine();
}

bool CodeEditor::goToPosition(int line, int column)
{
    if (line <= 0 || column <= 0) {
        return false;
    }

    const QTextBlock block = document()->findBlockByNumber(line - 1);
    if (!block.isValid()) {
        return false;
    }

    QTextCursor cursor(document());
    const int offset = qMin(column - 1, qMax(0, block.length() - 1));
    cursor.setPosition(block.position() + offset);
    setTextCursor(cursor);
    centerCursor();
    setFocus();
    return true;
}

void CodeEditor::deleteSelectionOrNextCharacter()
{
    QTextCursor cursor = textCursor();
    cursor.deleteChar();
    setTextCursor(cursor);
    setFocus();
}

int CodeEditor::lineNumberAreaWidth() const
{
    int digits = 1;
    int count = qMax(1, blockCount());
    while (count >= 10) {
        count /= 10;
        digits++;
    }

    return lineNumberPadding + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}

void CodeEditor::updateLineNumberAreaWidth()
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
    updateLineNumberAreaGeometry();
}

void CodeEditor::updateLineNumberAreaGeometry()
{
    const QRect area = contentsRect();
    lineNumberArea_->setGeometry(area.left(), area.top(), lineNumberAreaWidth(), area.height());
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy != 0) {
        lineNumberArea_->scroll(0, dy);
    } else {
        lineNumberArea_->update(0, rect.y(), lineNumberArea_->width(), rect.height());
    }

    if (rect.contains(viewport()->rect())) {
        updateLineNumberAreaWidth();
    }
}

void CodeEditor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);
    updateLineNumberAreaGeometry();
}

void CodeEditor::changeEvent(QEvent *event)
{
    QPlainTextEdit::changeEvent(event);
    if (event->type() == QEvent::FontChange) {
        updateTabStopDistance();
        updateLineNumberAreaWidth();
    }
}

void CodeEditor::updateTabStopDistance()
{
    setTabStopDistance(fontMetrics().horizontalAdvance(QLatin1Char(' ')) * tabWidthInSpaces);
}

void CodeEditor::paintLineNumberArea(QPaintEvent *event)
{
    QPainter painter(lineNumberArea_);
    const QColor background = palette().alternateBase().color();
    painter.fillRect(event->rect(), background);

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QColor numberColor = palette().color(QPalette::Disabled, QPalette::Text);
            if (blockNumber == textCursor().blockNumber()) {
                numberColor = palette().color(QPalette::Text);
            }
            painter.setPen(numberColor);
            painter.drawText(0, top, lineNumberArea_->width() - lineNumberRightPadding, fontMetrics().height(), Qt::AlignRight, QString::number(blockNumber + 1));
        }

        block = block.next();
        top = bottom;
        if (!block.isValid()) {
            break;
        }

        bottom = top + qRound(blockBoundingRect(block).height());
        blockNumber++;
    }
}

void CodeEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> selections;
    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(palette().alternateBase());
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        selections.append(selection);
    }

    setExtraSelections(selections);
    lineNumberArea_->update();
}

void CodeEditor::dragEnterEvent(QDragEnterEvent *event)
{
    if (!localFilePath(*event->mimeData()).isEmpty()) {
        event->acceptProposedAction();
        return;
    }

    QPlainTextEdit::dragEnterEvent(event);
}

void CodeEditor::dropEvent(QDropEvent *event)
{
    const QString path = localFilePath(*event->mimeData());
    if (!path.isEmpty()) {
        emit fileOpenRequested(path);
        event->acceptProposedAction();
        return;
    }

    QPlainTextEdit::dropEvent(event);
}
