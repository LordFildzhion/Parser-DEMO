#include "diagnosticspanel.h"

#include <QFont>
#include <QHeaderView>
#include <QLabel>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace {
    constexpr int PanelMargin = 8;
    constexpr int PanelSpacing = 6;
    constexpr int PositionColumnWidth = 85;
    constexpr int LineColumn = 0;
    constexpr int PositionColumn = 1;
    constexpr int MessageColumn = 2;
    constexpr int ColumnCount = 3;
}

DiagnosticsPanel::DiagnosticsPanel(QWidget *parent)
    : QWidget(parent), table_(new QTableWidget(0, ColumnCount, this))
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(PanelMargin, PanelMargin, PanelMargin, PanelMargin);
    layout->setSpacing(PanelSpacing);

    auto *title = new QLabel(QStringLiteral("Результаты анализа (подключение парсера — следующий этап)"), this);
    QFont titleFont = title->font();
    titleFont.setBold(true);
    title->setFont(titleFont);
    layout->addWidget(title);

    table_->setHorizontalHeaderLabels({QStringLiteral("Строка"), QStringLiteral("Столбец"), QStringLiteral("Сообщение")});
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setAlternatingRowColors(true);
    table_->verticalHeader()->setVisible(false);
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->setColumnWidth(LineColumn, PositionColumnWidth);
    table_->setColumnWidth(PositionColumn, PositionColumnWidth);
    layout->addWidget(table_);

    connect(table_, &QTableWidget::cellClicked, this, [this](int row, int) { activateDiagnostic(row); });
}

void DiagnosticsPanel::setDiagnostics(const QList<Diagnostic> &diagnostics)
{
    diagnostics_ = diagnostics;
    table_->setRowCount(static_cast<int>(diagnostics_.size()));

    for (int row = 0; row < table_->rowCount(); row++) {
        const Diagnostic &diagnostic = diagnostics_.at(row);
        table_->setItem(row, LineColumn, new QTableWidgetItem(QString::number(diagnostic.line)));
        table_->setItem(row, PositionColumn, new QTableWidgetItem(QString::number(diagnostic.column)));
        table_->setItem(row, MessageColumn, new QTableWidgetItem(diagnostic.message));
    }
}

void DiagnosticsPanel::showInformation(const QString &message)
{
    clear();
    table_->setRowCount(1);
    table_->setItem(0, LineColumn, new QTableWidgetItem(QStringLiteral("—")));
    table_->setItem(0, PositionColumn, new QTableWidgetItem(QStringLiteral("—")));
    table_->setItem(0, MessageColumn, new QTableWidgetItem(message));
}

void DiagnosticsPanel::clear()
{
    diagnostics_.clear();
    table_->setRowCount(0);
}

void DiagnosticsPanel::activateDiagnostic(int row)
{
    if (row < 0 || row >= diagnostics_.size()) {
        return;
    }

    const Diagnostic &diagnostic = diagnostics_.at(row);
    if (diagnostic.hasPosition()) {
        emit diagnosticActivated(diagnostic.line, diagnostic.column);
    }
}
