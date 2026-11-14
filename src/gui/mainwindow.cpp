#include "mainwindow.h"

#include "codeeditor.h"
#include "core/editordocument.h"
#include "diagnosticspanel.h"
#include "helpcontent.h"

#include <QAction>
#include <QCloseEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QIcon>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QSplitter>
#include <QStatusBar>
#include <QStyle>
#include <QTextBrowser>
#include <QTextCursor>
#include <QTextDocument>
#include <QToolBar>
#include <QVBoxLayout>

namespace {
constexpr int NotificationDuration = 5000;
constexpr int FileNotificationDuration = 6000;

QIcon themeIcon(const QString &name, const QIcon &fallback)
{
    return QIcon::fromTheme(name, fallback);
}

QIcon letterIcon(const QString &letter)
{
    QPixmap pixmap(24, 24);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QColor("#475569"));
    painter.drawRoundedRect(QRect(2, 2, 20, 20), 4, 4);
    QFont font = painter.font();
    font.setBold(true);
    font.setPointSize(10);
    painter.setFont(font);
    painter.drawText(pixmap.rect(), Qt::AlignCenter, letter);
    return QIcon(pixmap);
}

QAction *makeMenuAction(QMenu &menu, const QString &text, const QKeySequence &shortcut = {}, const QIcon &icon = {})
    {
        auto *action = menu.addAction(icon, text);
        action->setShortcut(shortcut);
        return action;
    }
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setMinimumSize(740, 500);
    resize(1060, 750);

    createCentralWidgets();
    document_ = new EditorDocument(*editor_->document(), editor_->document());
    createMenusAndToolbars();
    connectSignals();

    updateWindowTitle();
    updateCursorPosition();
    statusBar()->showMessage(QStringLiteral("Готово. Откройте файл или введите текст."), FileNotificationDuration);
}

void MainWindow::createCentralWidgets()
{
    auto *splitter = new QSplitter(Qt::Vertical, this);
    splitter->setChildrenCollapsible(false);

    editor_ = new CodeEditor(splitter);
    diagnosticsPanel_ = new DiagnosticsPanel(splitter);

    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({530, 190});
    setCentralWidget(splitter);

    cursorPositionLabel_ = new QLabel(this);
    auto *encodingLabel = new QLabel(QStringLiteral("UTF-8  |  C/C++"), this);
    statusBar()->addPermanentWidget(cursorPositionLabel_);
    statusBar()->addPermanentWidget(encodingLabel);
}

void MainWindow::createMenusAndToolbars()
{
    auto *toolbar = addToolBar(QStringLiteral("Панель инструментов"));
    toolbar->setObjectName(QStringLiteral("MainToolBar"));
    toolbar->setMovable(false);
    toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);

    createFileMenu(*toolbar);
    toolbar->addSeparator();
    createEditMenu(*toolbar);
    toolbar->addSeparator();
    createTextMenu();
    createRunMenu(*toolbar);
    toolbar->addSeparator();
    createHelpMenu(*toolbar);
}

void MainWindow::createFileMenu(QToolBar &toolbar)
{
    auto *menu = menuBar()->addMenu(QStringLiteral("&Файл"));
    auto *newAction = makeMenuAction(*menu, QStringLiteral("Создать"), QKeySequence::New, themeIcon(QStringLiteral("document-new"), style()->standardIcon(QStyle::SP_FileIcon)));
    connect(newAction, &QAction::triggered, this, &MainWindow::newDocument);

    auto *openAction = makeMenuAction(*menu, QStringLiteral("Открыть…"), QKeySequence::Open, themeIcon(QStringLiteral("document-open"), style()->standardIcon(QStyle::SP_DirOpenIcon)));
    connect(openAction, &QAction::triggered, this, &MainWindow::openDocument);

    auto *saveAction = makeMenuAction(*menu, QStringLiteral("Сохранить"), QKeySequence::Save, themeIcon(QStringLiteral("document-save"), style()->standardIcon(QStyle::SP_DialogSaveButton)));
    connect(saveAction, &QAction::triggered, this, [this] { (void)saveDocument(); });

    auto *saveAsAction = makeMenuAction(*menu, QStringLiteral("Сохранить как…"), QKeySequence::SaveAs);
    connect(saveAsAction, &QAction::triggered, this, [this] { (void)saveDocumentAs(); });

    menu->addSeparator();
    auto *exitAction = makeMenuAction(*menu, QStringLiteral("Выход"), QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    toolbar.addActions({newAction, openAction, saveAction});
}

void MainWindow::createEditMenu(QToolBar &toolbar)
{
    auto *menu = menuBar()->addMenu(QStringLiteral("&Правка"));
    auto *undoAction = makeMenuAction(*menu, QStringLiteral("Отменить"), QKeySequence::Undo, themeIcon(QStringLiteral("edit-undo"), style()->standardIcon(QStyle::SP_ArrowBack)));
    undoAction->setEnabled(false);
    connect(undoAction, &QAction::triggered, editor_, &CodeEditor::undo);
    connect(editor_, &CodeEditor::undoAvailable, undoAction, &QAction::setEnabled);

    auto *redoAction = makeMenuAction(*menu, QStringLiteral("Повторить"), QKeySequence::Redo, themeIcon(QStringLiteral("edit-redo"), style()->standardIcon(QStyle::SP_ArrowForward)));
    redoAction->setEnabled(false);
    connect(redoAction, &QAction::triggered, editor_, &CodeEditor::redo);
    connect(editor_, &CodeEditor::redoAvailable, redoAction, &QAction::setEnabled);
    menu->addSeparator();

    auto *cutAction = makeMenuAction(*menu, QStringLiteral("Вырезать"), QKeySequence::Cut, themeIcon(QStringLiteral("edit-cut"), letterIcon(QStringLiteral("X"))));
    cutAction->setEnabled(false);
    connect(cutAction, &QAction::triggered, editor_, &CodeEditor::cut);
    connect(editor_, &CodeEditor::copyAvailable, cutAction, &QAction::setEnabled);

    auto *copyAction = makeMenuAction(*menu, QStringLiteral("Копировать"), QKeySequence::Copy, themeIcon(QStringLiteral("edit-copy"), letterIcon(QStringLiteral("C"))));
    copyAction->setEnabled(false);
    connect(copyAction, &QAction::triggered, editor_, &CodeEditor::copy);
    connect(editor_, &CodeEditor::copyAvailable, copyAction, &QAction::setEnabled);

    auto *pasteAction = makeMenuAction(*menu, QStringLiteral("Вставить"), QKeySequence::Paste, themeIcon(QStringLiteral("edit-paste"), letterIcon(QStringLiteral("V"))));
    connect(pasteAction, &QAction::triggered, editor_, &CodeEditor::paste);

    auto *deleteAction = makeMenuAction(*menu, QStringLiteral("Удалить"), QKeySequence(Qt::Key_Delete));
    connect(deleteAction, &QAction::triggered, editor_, &CodeEditor::deleteSelectionOrNextCharacter);

    menu->addSeparator();
    auto *selectAllAction = makeMenuAction(*menu, QStringLiteral("Выделить всё"), QKeySequence::SelectAll);
    connect(selectAllAction, &QAction::triggered, editor_, &CodeEditor::selectAll);

    toolbar.addActions({undoAction, redoAction});
    toolbar.addSeparator();
    toolbar.addActions({cutAction, copyAction, pasteAction});
}

void MainWindow::createTextMenu()
{
    auto *menu = menuBar()->addMenu(QStringLiteral("&Текст"));
    for (const auto &section : HelpContent::sections()) {
        auto *action = menu->addAction(section.title);
        connect(action, &QAction::triggered, this, [this, id = section.id] { showTextSection(id); });
    }
}

void MainWindow::createRunMenu(QToolBar &toolbar)
{
    auto *menu = menuBar()->addMenu(QStringLiteral("Пуск"));
    auto *runAction = makeMenuAction(*menu, QStringLiteral("Запустить синтаксический анализ"), QKeySequence(Qt::Key_F5), themeIcon(QStringLiteral("media-playback-start"), style()->standardIcon(QStyle::SP_MediaPlay)));
    connect(runAction, &QAction::triggered, this, &MainWindow::startAnalysis);
    toolbar.addAction(runAction);
}

void MainWindow::createHelpMenu(QToolBar &toolbar)
{
    auto *menu = menuBar()->addMenu(QStringLiteral("&Справка"));
    auto *helpAction = makeMenuAction(*menu, QStringLiteral("Вызов справки"), QKeySequence::HelpContents, themeIcon(QStringLiteral("help-contents"), style()->standardIcon(QStyle::SP_DialogHelpButton)));
    connect(helpAction, &QAction::triggered, this, &MainWindow::showHelp);

    auto *aboutAction = makeMenuAction(*menu, QStringLiteral("О программе"), {}, themeIcon(QStringLiteral("help-about"), style()->standardIcon(QStyle::SP_MessageBoxInformation)));
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAbout);
    toolbar.addActions({helpAction, aboutAction});
}

void MainWindow::connectSignals()
{
    connect(document_, &EditorDocument::stateChanged, this, &MainWindow::updateWindowTitle);
    connect(document_, &EditorDocument::contentsReplaced, diagnosticsPanel_, &DiagnosticsPanel::clear);
    connect(document_, &EditorDocument::contentsReplaced, editor_, [editor = editor_] { (void)editor->goToPosition(1, 1); });
    connect(editor_, &CodeEditor::fileOpenRequested, this, &MainWindow::openDocumentAt);
    connect(editor_, &CodeEditor::cursorPositionChanged, this, &MainWindow::updateCursorPosition);
    connect(diagnosticsPanel_, &DiagnosticsPanel::diagnosticActivated, editor_, [this](int line, int column) { (void)editor_->goToPosition(line, column); });
}

void MainWindow::updateWindowTitle()
{
    const QString filename = document_->filePath().isEmpty() ? QStringLiteral("Безымянный") : QFileInfo(document_->filePath()).fileName();
    setWindowTitle(QStringLiteral("%1[*] — Analyzer").arg(filename));
    setWindowModified(document_->isModified());
}

void MainWindow::updateCursorPosition()
{
    const QTextCursor cursor = editor_->textCursor();
    cursorPositionLabel_->setText(QStringLiteral("Строка %1, столбец %2").arg(cursor.blockNumber() + 1).arg(cursor.positionInBlock() + 1));
}

bool MainWindow::confirmDiscardOrSave()
{
    if (!document_->isModified()) {
        return true;
    }

    const auto choice = QMessageBox::warning(this, QStringLiteral("Несохранённые изменения"), QStringLiteral("Документ был изменён. Сохранить изменения?"), QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);

    if (choice == QMessageBox::Save) {
        return saveDocument();
    }

    return choice == QMessageBox::Discard;
}

void MainWindow::newDocument()
{
    if (!confirmDiscardOrSave()) {
        return;
    }

    document_->createNew();
    editor_->setFocus();
    statusBar()->showMessage(QStringLiteral("Новый документ создан."), 4000);
}

void MainWindow::openDocument()
{
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Открыть документ"), {}, QStringLiteral("Исходные файлы (*.cpp *.cc *.c *.h *.hpp);;Текстовые файлы (*.txt);;Все файлы (*)"));
    if (!path.isEmpty()) {
        openDocumentAt(path);
    }
}

void MainWindow::openDocumentAt(const QString &path)
{
    if (!confirmDiscardOrSave()) {
        return;
    }

    if (const auto error = document_->loadFromFile(path)) {
        showDocumentError(*error);
        return;
    }

    editor_->setFocus();
    statusBar()->showMessage(QStringLiteral("Файл открыт: %1").arg(document_->filePath()), FileNotificationDuration);
}

bool MainWindow::saveDocument()
{
    return document_->filePath().isEmpty() ? saveDocumentAs() : saveDocumentAt(document_->filePath());
}

bool MainWindow::saveDocumentAs()
{
    QString path = QFileDialog::getSaveFileName(this, QStringLiteral("Сохранить документ"), document_->filePath().isEmpty() ? QStringLiteral("untitled.cpp") : document_->filePath(), QStringLiteral("Исходный код C++ (*.cpp);;Текстовые файлы (*.txt);;Все файлы (*)"));
    
    if (path.isEmpty()) {
        return false;
    }
    
    if (QFileInfo(path).suffix().isEmpty()) {
        path += QStringLiteral(".cpp");
    }
    
    return saveDocumentAt(path);
}

bool MainWindow::saveDocumentAt(const QString &path)
{
    if (const auto error = document_->saveToFile(path)) {
        showDocumentError(*error);
        return false;
    }

    statusBar()->showMessage(QStringLiteral("Файл сохранён: %1").arg(document_->filePath()), FileNotificationDuration);
    return true;
}

void MainWindow::showDocumentError(const DocumentError &error)
{
    QString title;
    switch (error.code) {
    case DocumentErrorCode::Open:
        title = QStringLiteral("Не удалось открыть файл");
        break;

    case DocumentErrorCode::Read:
        title = QStringLiteral("Ошибка чтения");
        break;

    case DocumentErrorCode::Save:
        title = QStringLiteral("Ошибка сохранения");
        break;

    case DocumentErrorCode::Write:
        title = QStringLiteral("Ошибка записи");
        break;
    }

    QMessageBox::critical(this, title, QStringLiteral("%1\n\n%2").arg(error.path, error.message));
}

void MainWindow::showDiagnostics(const QList<Diagnostic> &diagnostics)
{
    diagnosticsPanel_->setDiagnostics(diagnostics);
    statusBar()->showMessage(diagnostics.isEmpty() ? QStringLiteral("Ошибки не обнаружены.") : QStringLiteral("Диагностических сообщений: %1").arg(diagnostics.size()), NotificationDuration);
}

void MainWindow::insertExample()
{
    if (!confirmDiscardOrSave()) {
        return;
    }

    document_->replaceWithUnsavedText(HelpContent::exampleText());
    editor_->setFocus();
    statusBar()->showMessage(QStringLiteral("Тестовый пример вставлен. Сохраните его при необходимости."), NotificationDuration);
}

void MainWindow::startAnalysis()
{
    diagnosticsPanel_->showInformation(QStringLiteral("Синтаксический анализатор ещё не подключён. Интерфейс готов к интеграции."));
    statusBar()->showMessage(QStringLiteral("GUI-этап: парсер будет реализован отдельно."), 7000);
}

void MainWindow::showReadOnlyDialog(const QString &title, const QString &html)
{
    QDialog dialog(this);
    dialog.setWindowTitle(title);
    dialog.resize(660, 460);

    auto *layout = new QVBoxLayout(&dialog);
    auto *browser = new QTextBrowser(&dialog);
    browser->setHtml(html);
    layout->addWidget(browser);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);
    dialog.exec();
}

void MainWindow::showTextSection(TextSection section)
{
    if (section == TextSection::Example) {
        const auto reply = QMessageBox::question(this, HelpContent::sectionTitle(section),
            QStringLiteral("Вставить демонстрационный пример if...else в редактор?\n"
                           "Несохранённые изменения будет предложено сохранить."),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

        if (reply == QMessageBox::Yes) {
            insertExample();
        }

        return;
    }

    showReadOnlyDialog(HelpContent::sectionTitle(section), HelpContent::sectionHtml(section));
}

void MainWindow::showHelp()
{
    showReadOnlyDialog(QStringLiteral("Руководство пользователя"), HelpContent::userGuideHtml());
}

void MainWindow::showAbout()
{
    QMessageBox::about(this, QStringLiteral("О программе"), HelpContent::aboutHtml());
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (confirmDiscardOrSave()) {
        event->accept();
    } else {
        event->ignore();
    }
}
