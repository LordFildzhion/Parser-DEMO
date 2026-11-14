#pragma once

#include <QObject>
#include <QString>

#include <optional>

class QTextDocument;

enum class DocumentErrorCode
{
    Open,
    Read,
    Save,
    Write
};

struct DocumentError
{
    DocumentErrorCode code;
    QString path;
    QString message;
};

class EditorDocument final : public QObject
{
    Q_OBJECT

public:
    explicit EditorDocument(QTextDocument &textDocument, QObject *parent = nullptr);

    [[nodiscard]] const QString &filePath() const noexcept;
    [[nodiscard]] bool isModified() const;

    void createNew();
    void replaceWithUnsavedText(const QString &text);
    [[nodiscard]] std::optional<DocumentError> loadFromFile(const QString &path);
    [[nodiscard]] std::optional<DocumentError> saveToFile(const QString &path);

signals:
    void stateChanged();
    void contentsReplaced();

private:
    void replaceContents(const QString &text, QString path, bool modified);

    QTextDocument &textDocument_;
    QString filePath_;
};
