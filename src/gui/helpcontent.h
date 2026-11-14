#pragma once

#include <QString>

#include <array>

enum class TextSection
{
    Task,
    Grammar,
    GrammarClassification,
    AnalysisMethod,
    Example,
    Bibliography,
    SourceCode
};

struct TextSectionInfo
{
    TextSection id;
    QString title;
};

namespace HelpContent {

[[nodiscard]] const std::array<TextSectionInfo, 7> &sections();
[[nodiscard]] QString sectionTitle(TextSection section);

// Example is an editor command, so it has no informational HTML page.
[[nodiscard]] QString sectionHtml(TextSection section);
[[nodiscard]] QString exampleText();
[[nodiscard]] QString userGuideHtml();
[[nodiscard]] QString aboutHtml();

} // namespace HelpContent
