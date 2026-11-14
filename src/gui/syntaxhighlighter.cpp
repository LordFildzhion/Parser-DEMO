#include "syntaxhighlighter.h"

#include <QColor>
#include <QFont>
#include <QTextDocument>

namespace {
QTextCharFormat textFormat(const QColor &color, bool bold = false)
{
    QTextCharFormat format;
    format.setForeground(color);
    if (bold) {
        format.setFontWeight(QFont::Bold);
    }
    return format;
}
}

SyntaxHighlighter::SyntaxHighlighter(QTextDocument *document)
    : QSyntaxHighlighter(document),
      rules_{{
          {QRegularExpression(QStringLiteral(R"(\b(if|else)\b)")),
           textFormat(QColor("#2357a8"), true)},
          {QRegularExpression(QStringLiteral(R"(\b[0-9]+(?:\.[0-9]+)?\b)")),
           textFormat(QColor("#ad5b00"))},
          {QRegularExpression(QStringLiteral(R"((==|!=|<=|>=|&&|\|\||[=<>+*/%!-]))")),
           textFormat(QColor("#7c3aed"))},
          {QRegularExpression(QStringLiteral(R"("(?:\\.|[^"\\])*")")),
           textFormat(QColor("#12805c"))},
          {QRegularExpression(QStringLiteral(R"(//[^\n]*)")), textFormat(QColor("#748296"))},
      }}
{
}

void SyntaxHighlighter::highlightBlock(const QString &text)
{
    for (const auto &rule : rules_) {
        auto matches = rule.pattern.globalMatch(text);
        while (matches.hasNext()) {
            const auto match = matches.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}
