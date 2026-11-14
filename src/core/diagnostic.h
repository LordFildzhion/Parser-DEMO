#pragma once

#include <QString>

struct Diagnostic
{
    int line = 0;
    int column = 0;
    QString message;

    [[nodiscard]] bool hasPosition() const noexcept
    {
        return line > 0 && column > 0;
    }
};
