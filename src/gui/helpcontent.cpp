#include "helpcontent.h"

#include <QCoreApplication>

namespace {
    QString htmlDocument(const QString &heading, const QString &body)
    {
        return QStringLiteral("<html><head><meta charset='utf-8'></head><body>"
                            "<h2>%1</h2>%2</body></html>")
            .arg(heading.toHtmlEscaped(), body);
    }
}

namespace HelpContent {
    const std::array<TextSectionInfo, 7> &sections()
    {
        static const std::array<TextSectionInfo, 7> catalog{{
            {TextSection::Task, QStringLiteral("Постановка задачи")},
            {TextSection::Grammar, QStringLiteral("Грамматика")},
            {TextSection::GrammarClassification, QStringLiteral("Классификация грамматики")},
            {TextSection::AnalysisMethod, QStringLiteral("Метод анализа")},
            {TextSection::Example, QStringLiteral("Тестовый пример")},
            {TextSection::Bibliography, QStringLiteral("Список литературы")},
            {TextSection::SourceCode, QStringLiteral("Исходный код программы")},
        }};

        return catalog;
    }

    QString sectionTitle(TextSection section)
    {
        for (const auto &entry : sections()) {
            if (entry.id == section) {
                return entry.title;
            }
        }
        return {};
    }

    QString sectionHtml(TextSection section)
    {
        QString body;
        switch (section) {
        case TextSection::Task:
            body = QStringLiteral("<p>Разработать синтаксический анализатор для конструкции "
                                  "<code>if (условие) { действие; } else { действие; }</code> "
                                  "языка C/C++.</p><p>На данном этапе реализуется графический "
                                  "текстовый редактор. Синтаксический анализ будет подключён позже.</p>");
            break;

        case TextSection::Grammar:
        case TextSection::GrammarClassification:
        case TextSection::AnalysisMethod:
            body = QStringLiteral("<p>Этот раздел будет заполнен в ходе разработки "
                                "контекстно-свободной грамматики и синтаксического анализатора.</p>"
                                "<p>Пока завершён только первый этап — графический интерфейс.</p>");
            break;

        case TextSection::Example:
            return {};

        case TextSection::Bibliography:
            body = QStringLiteral("<p>Шорников Ю. В. <i>Теория языков программирования: "
                                  "проектирование и реализация</i>. — Новосибирск: Изд-во НГТУ, 2022.</p>");
            break;

        case TextSection::SourceCode:
            body = QStringLiteral("<p>Исходный код интерфейса находится в каталоге "
                                  "<code>src/gui</code>, точка входа — <code>src/main.cpp</code>.</p>"
                                  "<p>Исходники включены в архив проекта.</p>");
            break;
        }

        return htmlDocument(sectionTitle(section), body);
    }

    QString exampleText()
    {
        return QStringLiteral(
                            "if (a > 0)\n"
                            "{\n"
                            "    x = 1;\n"
                            "}\n"
                            "else\n"
                            "{\n"
                            "    x = 0;\n"
                            "}\n");
    }

    QString userGuideHtml()
    {
        const QString body = QStringLiteral(R"(
            <p>Приложение является текстовым редактором для последующего подключения
            синтаксического анализатора конструкции <code>if...else</code>.</p>
            <h3>Файл</h3>
            <p><b>Создать (Ctrl+N)</b> — новый документ;<br>
            <b>Открыть (Ctrl+O)</b> — открыть текстовый файл;<br>
            <b>Сохранить (Ctrl+S)</b> — сохранить документ;<br>
            <b>Сохранить как (Ctrl+Shift+S)</b> — сохранить под новым именем;<br>
            <b>Выход</b> — закрыть редактор.</p>
            <h3>Правка</h3>
            <p>Отменить, повторить, вырезать, копировать, вставить,
            удалить выделенный фрагмент или символ, выделить весь текст.</p>
            <h3>Текст</h3>
            <p>Окна сведений о постановке задачи, грамматике, методе анализа,
            примерах и исходном коде. Пункт «Тестовый пример» позволяет
            загрузить демонстрационный текст в редактор.</p>
            <h3>Пуск (F5)</h3>
            <p>Зарезервирован для запуска синтаксического анализатора.
            На текущем этапе выводит сообщение о том, что парсер пока не подключён.</p>
            <h3>Дополнительно</h3>
            <p>Перетаскивание локального файла в редактор открывает его.
            Нижнюю область результатов можно изменять, перетаскивая разделитель.
            Файлы открываются и сохраняются в UTF-8.</p>
        )");

        return htmlDocument(QStringLiteral("Руководство пользователя"), body);
    }

    QString aboutHtml()
    {
        return QStringLiteral("<b>Analyzer %1</b><br><br>"
                              "Графический текстовый редактор на C++17 и Qt 6 Widgets.<br>"
                              "Текущая версия: GUI без подключённого парсера.")
            .arg(QCoreApplication::applicationVersion().toHtmlEscaped());
    }

}
