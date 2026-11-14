#pragma once

#include "core/diagnostic.h"

#include <QList>
#include <QWidget>

class QTableWidget;

class DiagnosticsPanel final : public QWidget
{
    Q_OBJECT

public:
    explicit DiagnosticsPanel(QWidget *parent = nullptr);

    void setDiagnostics(const QList<Diagnostic> &diagnostics);
    void showInformation(const QString &message);
    void clear();

signals:
    void diagnosticActivated(int line, int column);

private:
    void activateDiagnostic(int row);

    QTableWidget *table_ = nullptr;
    QList<Diagnostic> diagnostics_;
};
