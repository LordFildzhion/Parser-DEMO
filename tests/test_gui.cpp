#include "gui/codeeditor.h"
#include "gui/diagnosticspanel.h"
#include "gui/mainwindow.h"

#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QMimeData>
#include <QScrollBar>
#include <QSignalSpy>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QTest>
#include <QTextCursor>
#include <QTextDocument>
#include <QTimer>
#include <QUrl>

namespace {

QAction *findAction(const MainWindow &window, const QString &text)
{
    const auto actions = window.findChildren<QAction *>();
    for (auto *action : actions) {
        if (action->text() == text) {
            return action;
        }
    }
    return nullptr;
}

void showWindow(QWidget &widget)
{
    widget.show();
    widget.activateWindow();
    QApplication::processEvents();
}

void answerNextMessageBox(QWidget &context, QMessageBox::StandardButton response, bool &answered)
{
    QTimer::singleShot(0, &context, [response, &answered] {
        auto *messageBox = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
        if (messageBox == nullptr) {
            return;
        }

        if (auto *button = messageBox->button(response)) {
            answered = true;
            button->click();
        } else {
            messageBox->reject();
        }
    });
}

} // namespace

class GuiTest final : public QObject
{
    Q_OBJECT

private slots:
    void navigationUsesOneBasedCoordinates();
    void navigationClampsColumnsToLineEnd();
    void invalidNavigationPreservesCursor_data();
    void invalidNavigationPreservesCursor();
    void deleteRemovesSelectionOrNextCharacter();
    void localFileDropRequestsOpeningWithoutInsertingText();
    void diagnosticClickUsesPositionAndIgnoresUnpositionedRows();
    void informationReplacesDiagnosticsAndCannotActivatePosition();
    void mainWindowF5ShowsAnalyzerInformation();
    void loadingFileAndInsertingExampleResetCursorAndScrolling();
    void cancellingNewDocumentPreservesUnsavedText();
    void discardingChangesForNewDocumentResetsDocumentAndResults();
};

void GuiTest::navigationUsesOneBasedCoordinates()
{
    CodeEditor editor;
    editor.setPlainText(QStringLiteral("first\nabc\nlast"));

    QVERIFY(editor.goToPosition(2, 3));
    QCOMPARE(editor.textCursor().blockNumber(), 1);
    QCOMPARE(editor.textCursor().positionInBlock(), 2);

    QVERIFY(editor.goToPosition(1, 1));
    QCOMPARE(editor.textCursor().position(), 0);
}

void GuiTest::navigationClampsColumnsToLineEnd()
{
    CodeEditor editor;
    editor.setPlainText(QStringLiteral("first\nabc\n\nlast"));

    QVERIFY(editor.goToPosition(2, 999));
    QCOMPARE(editor.textCursor().blockNumber(), 1);
    QCOMPARE(editor.textCursor().positionInBlock(), 3);

    QVERIFY(editor.goToPosition(3, 999));
    QCOMPARE(editor.textCursor().blockNumber(), 2);
    QCOMPARE(editor.textCursor().positionInBlock(), 0);
}

void GuiTest::invalidNavigationPreservesCursor_data()
{
    QTest::addColumn<int>("line");
    QTest::addColumn<int>("column");

    QTest::newRow("zero line") << 0 << 1;
    QTest::newRow("negative line") << -1 << 1;
    QTest::newRow("zero column") << 1 << 0;
    QTest::newRow("negative column") << 1 << -1;
    QTest::newRow("missing line") << 99 << 1;
}

void GuiTest::invalidNavigationPreservesCursor()
{
    QFETCH(int, line);
    QFETCH(int, column);

    CodeEditor editor;
    editor.setPlainText(QStringLiteral("first\nsecond"));
    QTextCursor cursor = editor.textCursor();
    cursor.setPosition(2);
    cursor.setPosition(4, QTextCursor::KeepAnchor);
    editor.setTextCursor(cursor);

    QVERIFY(!editor.goToPosition(line, column));
    QCOMPARE(editor.textCursor().position(), cursor.position());
    QCOMPARE(editor.textCursor().anchor(), cursor.anchor());
}

void GuiTest::deleteRemovesSelectionOrNextCharacter()
{
    CodeEditor editor;
    editor.setPlainText(QStringLiteral("alpha beta"));
    QTextCursor cursor = editor.textCursor();
    cursor.setPosition(6);
    cursor.setPosition(10, QTextCursor::KeepAnchor);
    editor.setTextCursor(cursor);

    editor.deleteSelectionOrNextCharacter();
    QCOMPARE(editor.toPlainText(), QStringLiteral("alpha "));
    QCOMPARE(editor.textCursor().position(), 6);
    QVERIFY(!editor.textCursor().hasSelection());

    cursor = editor.textCursor();
    cursor.setPosition(2);
    editor.setTextCursor(cursor);
    editor.deleteSelectionOrNextCharacter();
    QCOMPARE(editor.toPlainText(), QStringLiteral("alha "));
    QCOMPARE(editor.textCursor().position(), 2);

    cursor = editor.textCursor();
    cursor.movePosition(QTextCursor::End);
    editor.setTextCursor(cursor);
    editor.deleteSelectionOrNextCharacter();
    QCOMPARE(editor.toPlainText(), QStringLiteral("alha "));
}

void GuiTest::localFileDropRequestsOpeningWithoutInsertingText()
{
    QTemporaryFile file;
    QVERIFY(file.open());

    CodeEditor editor;
    editor.setPlainText(QStringLiteral("existing text"));
    showWindow(editor);

    QSignalSpy requests(&editor, &CodeEditor::fileOpenRequested);
    QVERIFY(requests.isValid());

    QMimeData mimeData;
    mimeData.setUrls({QUrl::fromLocalFile(file.fileName())});
    const QPoint position(10, 10);
    // The QPoint overload also supports the minimum supported Qt 6.2.
    QT_WARNING_PUSH
    QT_WARNING_DISABLE_DEPRECATED
    QDragEnterEvent enter(position, Qt::CopyAction, &mimeData, Qt::LeftButton, Qt::NoModifier);
    QT_WARNING_POP
    QApplication::sendEvent(editor.viewport(), &enter);
    QVERIFY(enter.isAccepted());

    QDropEvent drop(QPointF(position), Qt::CopyAction, &mimeData, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(editor.viewport(), &drop);
    QVERIFY(drop.isAccepted());
    QCOMPARE(requests.count(), 1);
    QCOMPARE(requests.at(0).at(0).toString(), file.fileName());
    QCOMPARE(editor.toPlainText(), QStringLiteral("existing text"));
}

void GuiTest::diagnosticClickUsesPositionAndIgnoresUnpositionedRows()
{
    DiagnosticsPanel panel;
    panel.resize(640, 240);
    panel.setDiagnostics(
        {{2, 7, QStringLiteral("problem")}, {0, 1, QStringLiteral("unpositioned message")}});
    showWindow(panel);

    auto *table = panel.findChild<QTableWidget *>();
    QVERIFY(table != nullptr);
    QSignalSpy positions(&panel, &DiagnosticsPanel::diagnosticActivated);
    QVERIFY(positions.isValid());

    QTest::mouseClick(table->viewport(), Qt::LeftButton, Qt::NoModifier,
                      table->visualItemRect(table->item(0, 2)).center());
    QCOMPARE(positions.count(), 1);
    QCOMPARE(positions.at(0).at(0).toInt(), 2);
    QCOMPARE(positions.at(0).at(1).toInt(), 7);

    QTest::mouseClick(table->viewport(), Qt::LeftButton, Qt::NoModifier,
                      table->visualItemRect(table->item(1, 2)).center());
    QCOMPARE(positions.count(), 1);
}

void GuiTest::informationReplacesDiagnosticsAndCannotActivatePosition()
{
    DiagnosticsPanel panel;
    panel.resize(640, 240);
    panel.setDiagnostics({{2, 7, QStringLiteral("old diagnostic")}});
    showWindow(panel);

    auto *table = panel.findChild<QTableWidget *>();
    QVERIFY(table != nullptr);
    QSignalSpy positions(&panel, &DiagnosticsPanel::diagnosticActivated);

    panel.showInformation(QStringLiteral("analysis unavailable"));
    QCOMPARE(table->rowCount(), 1);
    QCOMPARE(table->item(0, 0)->text(), QStringLiteral("—"));
    QCOMPARE(table->item(0, 1)->text(), QStringLiteral("—"));
    QCOMPARE(table->item(0, 2)->text(), QStringLiteral("analysis unavailable"));

    QTest::mouseClick(table->viewport(), Qt::LeftButton, Qt::NoModifier,
                      table->visualItemRect(table->item(0, 2)).center());
    QCOMPARE(positions.count(), 0);

    panel.clear();
    QCOMPARE(table->rowCount(), 0);
}

void GuiTest::mainWindowF5ShowsAnalyzerInformation()
{
    MainWindow window;
    showWindow(window);
    auto *editor = window.findChild<CodeEditor *>();
    auto *table = window.findChild<QTableWidget *>();
    QVERIFY(editor != nullptr);
    QVERIFY(table != nullptr);
    editor->setFocus();
    QApplication::processEvents();

    QTest::keyClick(editor, Qt::Key_F5);
    QCOMPARE(table->rowCount(), 1);
    QCOMPARE(table->item(0, 0)->text(), QStringLiteral("—"));
    QCOMPARE(table->item(0, 1)->text(), QStringLiteral("—"));
    QVERIFY(table->item(0, 2)->text().contains(QStringLiteral("ещё не подключён")));
    QVERIFY(!editor->document()->isModified());
}

void GuiTest::loadingFileAndInsertingExampleResetCursorAndScrolling()
{
    QTemporaryFile file;
    QVERIFY(file.open());
    const QByteArray contents = (QByteArray(200, 'a') + '\n').repeated(120);
    QCOMPARE(file.write(contents), qint64(contents.size()));
    file.close();

    MainWindow window;
    showWindow(window);
    auto *editor = window.findChild<CodeEditor *>();
    auto *table = window.findChild<QTableWidget *>();
    auto *exampleAction = findAction(window, QStringLiteral("Тестовый пример"));
    QVERIFY(editor != nullptr);
    QVERIFY(table != nullptr);
    QVERIFY(exampleAction != nullptr);

    editor->fileOpenRequested(file.fileName());
    QCOMPARE(editor->textCursor().position(), 0);
    QCOMPARE(editor->textCursor().blockNumber(), 0);
    QCOMPARE(editor->horizontalScrollBar()->value(), 0);
    QCOMPARE(editor->verticalScrollBar()->value(), 0);

    QTextCursor cursor = editor->textCursor();
    cursor.movePosition(QTextCursor::End);
    editor->setTextCursor(cursor);
    QApplication::processEvents();
    QVERIFY(editor->horizontalScrollBar()->maximum() > 0);
    QVERIFY(editor->verticalScrollBar()->maximum() > 0);
    editor->horizontalScrollBar()->setValue(editor->horizontalScrollBar()->maximum());
    editor->verticalScrollBar()->setValue(editor->verticalScrollBar()->maximum());

    editor->fileOpenRequested(file.fileName());
    QCOMPARE(editor->textCursor().position(), 0);
    QCOMPARE(editor->textCursor().blockNumber(), 0);
    QCOMPARE(editor->horizontalScrollBar()->value(), 0);
    QCOMPARE(editor->verticalScrollBar()->value(), 0);
    QVERIFY(!editor->document()->isModified());

    window.showDiagnostics({{1, 1, QStringLiteral("old diagnostic")}});
    bool answered = false;
    answerNextMessageBox(window, QMessageBox::Yes, answered);
    exampleAction->trigger();

    QVERIFY(answered);
    QVERIFY(editor->toPlainText().startsWith(QStringLiteral("if (a > 0)")));
    QVERIFY(editor->document()->isModified());
    QVERIFY(window.isWindowModified());
    QVERIFY(window.windowTitle().startsWith(QStringLiteral("Безымянный")));
    QCOMPARE(editor->textCursor().position(), 0);
    QCOMPARE(editor->textCursor().blockNumber(), 0);
    QCOMPARE(editor->horizontalScrollBar()->value(), 0);
    QCOMPARE(editor->verticalScrollBar()->value(), 0);
    QCOMPARE(table->rowCount(), 0);
}

void GuiTest::cancellingNewDocumentPreservesUnsavedText()
{
    MainWindow window;
    showWindow(window);
    auto *editor = window.findChild<CodeEditor *>();
    auto *createAction = findAction(window, QStringLiteral("Создать"));
    QVERIFY(editor != nullptr);
    QVERIFY(createAction != nullptr);

    editor->setFocus();
    QTest::keyClicks(editor, "unsaved text");
    QVERIFY(editor->document()->isModified());
    const QString previousTitle = window.windowTitle();

    bool answered = false;
    answerNextMessageBox(window, QMessageBox::Cancel, answered);
    createAction->trigger();

    QVERIFY(answered);
    QCOMPARE(editor->toPlainText(), QStringLiteral("unsaved text"));
    QVERIFY(editor->document()->isModified());
    QCOMPARE(window.windowTitle(), previousTitle);
}

void GuiTest::discardingChangesForNewDocumentResetsDocumentAndResults()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("original.cpp"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write("original text"), qint64(13));
    file.close();

    MainWindow window;
    showWindow(window);
    auto *editor = window.findChild<CodeEditor *>();
    auto *table = window.findChild<QTableWidget *>();
    auto *createAction = findAction(window, QStringLiteral("Создать"));
    QVERIFY(editor != nullptr);
    QVERIFY(table != nullptr);
    QVERIFY(createAction != nullptr);

    editor->fileOpenRequested(path);
    QCOMPARE(editor->toPlainText(), QStringLiteral("original text"));
    QVERIFY(window.windowTitle().startsWith(QFileInfo(path).fileName()));
    QVERIFY(!editor->document()->isModified());
    editor->setFocus();
    QTest::keyClicks(editor, "changed ");
    QVERIFY(editor->document()->isModified());
    window.showDiagnostics({{1, 1, QStringLiteral("old diagnostic")}});
    QCOMPARE(table->rowCount(), 1);

    bool answered = false;
    answerNextMessageBox(window, QMessageBox::Discard, answered);
    createAction->trigger();

    QVERIFY(answered);
    QCOMPARE(editor->toPlainText(), QString());
    QVERIFY(!editor->document()->isModified());
    QVERIFY(!window.isWindowModified());
    QVERIFY(window.windowTitle().startsWith(QStringLiteral("Безымянный")));
    QCOMPARE(table->rowCount(), 0);

    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), QByteArray("original text"));
}

QTEST_MAIN(GuiTest)
#include "test_gui.moc"
