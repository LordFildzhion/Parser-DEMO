#pragma once

#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>

#include <array>

class QTextDocument;

class SyntaxHighlighter final : public QSyntaxHighlighter
{
public:
    explicit SyntaxHighlighter(QTextDocument *document);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct Rule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    std::array<Rule, 5> rules_;
};
