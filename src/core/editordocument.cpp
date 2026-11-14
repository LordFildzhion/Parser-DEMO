#include "editordocument.h"

#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QTextDocument>

#include <utility>

EditorDocument::EditorDocument(QTextDocument &textDocument, QObject *parent)
    : QObject(parent), textDocument_(textDocument)
{
    connect(&textDocument_, &QTextDocument::modificationChanged, this,
            &EditorDocument::stateChanged);
}

const QString &EditorDocument::filePath() const noexcept
{
    return filePath_;
}

bool EditorDocument::isModified() const
{
    return textDocument_.isModified();
}

void EditorDocument::createNew()
{
    replaceContents({}, {}, false);
}

void EditorDocument::replaceWithUnsavedText(const QString &text)
{
    replaceContents(text, {}, true);
}

std::optional<DocumentError> EditorDocument::loadFromFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return DocumentError{DocumentErrorCode::Open, path, file.errorString()};
    }

    const QByteArray bytes = file.readAll();
    if (file.error() != QFileDevice::NoError) {
        return DocumentError{DocumentErrorCode::Read, path, file.errorString()};
    }

    replaceContents(QString::fromUtf8(bytes), QFileInfo(path).absoluteFilePath(), false);
    return std::nullopt;
}

std::optional<DocumentError> EditorDocument::saveToFile(const QString &path)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return DocumentError{DocumentErrorCode::Save, path, file.errorString()};
    }

    const QByteArray bytes = textDocument_.toPlainText().toUtf8();
    if (file.write(bytes) != bytes.size() || !file.commit()) {
        return DocumentError{DocumentErrorCode::Write, path, file.errorString()};
    }

    filePath_ = QFileInfo(path).absoluteFilePath();
    textDocument_.setModified(false);
    emit stateChanged();
    return std::nullopt;
}

void EditorDocument::replaceContents(const QString &text, QString path, bool modified)
{
    filePath_ = std::move(path);
    textDocument_.setPlainText(text);
    textDocument_.setModified(modified);
    emit stateChanged();
    emit contentsReplaced();
}
