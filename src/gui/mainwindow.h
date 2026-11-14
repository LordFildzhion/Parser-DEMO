#pragma once

#include "core/diagnostic.h"

#include <QList>
#include <QMainWindow>

class CodeEditor;
class DiagnosticsPanel;
class EditorDocument;
class QLabel;
class QCloseEvent;
class QToolBar;
struct DocumentError;
enum class TextSection;

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    void showDiagnostics(const QList<Diagnostic> &diagnostics);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void createCentralWidgets();
    void createMenusAndToolbars();
    void createFileMenu(QToolBar &toolbar);
    void createEditMenu(QToolBar &toolbar);
    void createTextMenu();
    void createRunMenu(QToolBar &toolbar);
    void createHelpMenu(QToolBar &toolbar);
    void connectSignals();
    void updateWindowTitle();
    void updateCursorPosition();
    [[nodiscard]] bool confirmDiscardOrSave();
    void newDocument();
    void openDocument();
    void openDocumentAt(const QString &path);
    [[nodiscard]] bool saveDocument();
    [[nodiscard]] bool saveDocumentAs();
    [[nodiscard]] bool saveDocumentAt(const QString &path);
    void showDocumentError(const DocumentError &error);
    void insertExample();
    void startAnalysis();
    void showTextSection(TextSection section);
    void showHelp();
    void showAbout();
    void showReadOnlyDialog(const QString &title, const QString &html);

    // QObject parents own these objects; the pointers are observers.
    CodeEditor *editor_ = nullptr;
    DiagnosticsPanel *diagnosticsPanel_ = nullptr;
    EditorDocument *document_ = nullptr;
    QLabel *cursorPositionLabel_ = nullptr;
};
