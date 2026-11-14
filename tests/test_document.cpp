#include "core/editordocument.h"

#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QTextCursor>
#include <QTextDocument>

class DocumentTest final : public QObject
{
    Q_OBJECT

private slots:
    void utf8SurvivesSaveAndReload();
    void failedLoadPreservesUnsavedDocumentAndUndo();
    void failedSaveAsPreservesPathAndOriginalFile();
    void saveAsSwitchesPathWithoutChangingOriginal();
    void newDocumentResetsState();
    void replacingWithUnsavedTextDetachesFile();
    void successfulLoadClearsPreviousUndoHistory();
    void editingNotifiesDocumentState();
};

void DocumentTest::utf8SurvivesSaveAndReload()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("пример.cpp"));
    const QString source =
        QStringLiteral("// Проверка UTF-8: ё, 漢字, 🙂\nif (a > 0) { x = 1; }\n");
    QTextDocument text;
    EditorDocument document(text);
    document.replaceWithUnsavedText(source);

    QVERIFY(!document.saveToFile(path).has_value());
    QVERIFY(!document.isModified());
    QCOMPARE(document.filePath(), QFileInfo(path).absoluteFilePath());
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), source.toUtf8());
    file.close();

    document.replaceWithUnsavedText(QStringLiteral("temporary changes"));
    QVERIFY(!document.loadFromFile(path).has_value());
    QCOMPARE(text.toPlainText(), source);
    QVERIFY(!document.isModified());
}

void DocumentTest::failedLoadPreservesUnsavedDocumentAndUndo()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("original.cpp"));
    QTextDocument text;
    EditorDocument document(text);
    document.replaceWithUnsavedText(QStringLiteral("original"));
    QVERIFY(!document.saveToFile(path).has_value());
    QTextCursor cursor(&text);
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(QStringLiteral(" changed"));
    QVERIFY(text.isUndoAvailable());
    QSignalSpy replacements(&document, &EditorDocument::contentsReplaced);

    const auto error = document.loadFromFile(directory.filePath(QStringLiteral("missing.cpp")));
    QVERIFY(error.has_value());
    QCOMPARE(error->code, DocumentErrorCode::Open);
    QVERIFY(!error->message.isEmpty());
    QCOMPARE(document.filePath(), path);
    QCOMPARE(text.toPlainText(), QStringLiteral("original changed"));
    QVERIFY(document.isModified());
    QCOMPARE(replacements.count(), 0);
    text.undo();
    QCOMPARE(text.toPlainText(), QStringLiteral("original"));
}

void DocumentTest::failedSaveAsPreservesPathAndOriginalFile()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("original.cpp"));
    QTextDocument text;
    EditorDocument document(text);
    document.replaceWithUnsavedText(QStringLiteral("original"));
    QVERIFY(!document.saveToFile(path).has_value());
    QTextCursor cursor(&text);
    cursor.insertText(QStringLiteral("unsaved "));

    const QString invalidPath = directory.filePath(QStringLiteral("missing/subdirectory/copy.cpp"));
    const auto error = document.saveToFile(invalidPath);
    QVERIFY(error.has_value());
    QCOMPARE(error->code, DocumentErrorCode::Save);
    QCOMPARE(error->path, invalidPath);
    QVERIFY(!error->message.isEmpty());
    QCOMPARE(document.filePath(), path);
    QCOMPARE(text.toPlainText(), QStringLiteral("unsaved original"));
    QVERIFY(document.isModified());
    QVERIFY(!QFile::exists(invalidPath));
    QFile original(path);
    QVERIFY(original.open(QIODevice::ReadOnly));
    QCOMPARE(original.readAll(), QByteArray("original"));
}

void DocumentTest::saveAsSwitchesPathWithoutChangingOriginal()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString originalPath = directory.filePath(QStringLiteral("original.cpp"));
    const QString copyPath = directory.filePath(QStringLiteral("copy.cpp"));
    QTextDocument text;
    EditorDocument document(text);
    document.replaceWithUnsavedText(QStringLiteral("original"));
    QVERIFY(!document.saveToFile(originalPath).has_value());
    QTextCursor cursor(&text);
    cursor.select(QTextCursor::Document);
    cursor.insertText(QStringLiteral("copy"));

    QVERIFY(!document.saveToFile(copyPath).has_value());
    QCOMPARE(document.filePath(), copyPath);
    QVERIFY(!document.isModified());
    QFile original(originalPath);
    QVERIFY(original.open(QIODevice::ReadOnly));
    QCOMPARE(original.readAll(), QByteArray("original"));
    QFile copy(copyPath);
    QVERIFY(copy.open(QIODevice::ReadOnly));
    QCOMPARE(copy.readAll(), QByteArray("copy"));
}

void DocumentTest::newDocumentResetsState()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QTextDocument text;
    EditorDocument document(text);
    document.replaceWithUnsavedText(QStringLiteral("existing file"));
    QVERIFY(!document.saveToFile(directory.filePath(QStringLiteral("existing.cpp"))).has_value());
    QTextCursor(&text).insertText(QStringLiteral("changed "));
    QSignalSpy replacements(&document, &EditorDocument::contentsReplaced);

    document.createNew();
    QVERIFY(text.isEmpty());
    QVERIFY(document.filePath().isEmpty());
    QVERIFY(!document.isModified());
    QVERIFY(!text.isUndoAvailable());
    QCOMPARE(replacements.count(), 1);
}

void DocumentTest::replacingWithUnsavedTextDetachesFile()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("original.cpp"));
    QTextDocument text;
    EditorDocument document(text);
    document.replaceWithUnsavedText(QStringLiteral("original"));
    QVERIFY(!document.saveToFile(path).has_value());

    document.replaceWithUnsavedText(QStringLiteral("if (a) { x = 1; }"));
    QVERIFY(document.filePath().isEmpty());
    QVERIFY(document.isModified());
    QCOMPARE(text.toPlainText(), QStringLiteral("if (a) { x = 1; }"));
    QFile original(path);
    QVERIFY(original.open(QIODevice::ReadOnly));
    QCOMPARE(original.readAll(), QByteArray("original"));
}

void DocumentTest::successfulLoadClearsPreviousUndoHistory()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("saved.cpp"));
    QTextDocument text;
    EditorDocument document(text);
    document.replaceWithUnsavedText(QStringLiteral("saved content"));
    QVERIFY(!document.saveToFile(path).has_value());
    QTextCursor(&text).insertText(QStringLiteral("unsaved "));
    QVERIFY(text.isUndoAvailable());

    QVERIFY(!document.loadFromFile(path).has_value());
    QCOMPARE(text.toPlainText(), QStringLiteral("saved content"));
    QVERIFY(!document.isModified());
    QVERIFY(!text.isUndoAvailable());
    QVERIFY(!text.isRedoAvailable());
}

void DocumentTest::editingNotifiesDocumentState()
{
    QTextDocument text;
    EditorDocument document(text);
    document.createNew();
    QSignalSpy changes(&document, &EditorDocument::stateChanged);

    QTextCursor(&text).insertText(QStringLiteral("change"));
    QVERIFY(document.isModified());
    QVERIFY(!changes.isEmpty());
    text.undo();
    QVERIFY(!document.isModified());
}

QTEST_MAIN(DocumentTest)
#include "test_document.moc"
