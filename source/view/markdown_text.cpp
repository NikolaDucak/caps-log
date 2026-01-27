#include "markdown_text.hpp"

#include <algorithm>
#include <ftxui/dom/elements.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace caps_log::view {

const MarkdownTheme &getDefaultMarkdownTheme() {
    // Postponed initialization of the default theme, so that prior to its first use,
    // it is possible to set the collor support with ftxui::Terminal::SetColorSupport.
    // as Color constructors may depend on it.
    static MarkdownTheme defaultTheme{};
    return defaultTheme;
}

// ---------- Helpers ----------
constexpr bool isWhitespaceChar(char characterCh) {
    return characterCh == ' ' || characterCh == '\t';
}

inline bool startsWith(std::string_view inputView, std::string_view prefixView) {
    if (inputView.size() < prefixView.size()) {
        return false;
    }
    return inputView.substr(0, prefixView.size()) == prefixView;
}

inline std::size_t runOf(std::string_view inputView, char targetChar, std::size_t startPos = 0) {
    std::size_t indexPos = startPos;
    while (indexPos < inputView.size() && inputView[indexPos] == targetChar) {
        ++indexPos;
    }
    return indexPos - startPos;
}

inline std::size_t digitsAt(std::string_view inputView, std::size_t startPos = 0) {
    std::size_t indexPos = startPos;
    while (indexPos < inputView.size() &&
           (std::isdigit(static_cast<unsigned char>(inputView[indexPos])) != 0)) {
        ++indexPos;
    }
    return indexPos - startPos;
}

inline bool isHorizontalRule(std::string_view lineView) {
    int symbolCount = 0;
    char symbolChar = 0;
    for (char characterCh : lineView) {
        if (isWhitespaceChar(characterCh)) {
            continue;
        }
        if (characterCh == '-' || characterCh == '*' || characterCh == '_') {
            if (symbolChar == 0) {
                symbolChar = characterCh;
            }
            if (characterCh != symbolChar) {
                return false;
            }
            ++symbolCount;
        } else {
            return false;
        }
    }
    return symbolCount >= 3;
}

inline bool caseInsensitiveEqual(char firstChar, char secondChar) {
    auto toLower = [](char valueChar) {
        return static_cast<char>(std::tolower(static_cast<unsigned char>(valueChar)));
    };
    return toLower(firstChar) == toLower(secondChar);
}

// ---------- Inline tokenizer that preserves delimiters ----------
struct InlineStyle {
    bool isBold = false;
    bool isItalic = false;
    bool isStrike = false;
    bool isCode = false;
    bool isLink = false;
};

struct HighlightSpan {
    std::string_view textView;
    InlineStyle styleState;
};

inline void applyInlineStyle(ftxui::Element &elementRef, const InlineStyle &styleState,
                             const MarkdownTheme &theme) {
    using namespace ftxui;
    if (styleState.isCode) {
        elementRef = elementRef | color(theme.codeFg);
    }
    if (styleState.isBold) {
        elementRef = elementRef | bold;
    }
    if (styleState.isItalic) {
        elementRef = elementRef | italic;
    }
    if (styleState.isStrike) {
        elementRef = elementRef | strikethrough;
    }
    if (styleState.isLink) {
        elementRef = elementRef | color(Color::Blue) | underlined;
    }
}

inline void emitPlainSpan(std::vector<HighlightSpan> &outputSpans, std::string_view textView) {
    if (!textView.empty()) {
        outputSpans.push_back({textView, {}});
    }
}

// Add this next to emitPlainSpan:
inline void emitStyledSpan(std::vector<HighlightSpan> &outputSpans, std::string_view textView,
                           const InlineStyle &styleState) {
    if (!textView.empty()) {
        outputSpans.push_back({textView, styleState});
    }
}

// Tokenizes while **keeping** markdown delimiters as plain text.
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
inline std::vector<HighlightSpan> tokenizeInline(std::string_view lineView) {
    std::vector<HighlightSpan> outputSpans;
    // NOLINTNEXTLINE(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
    outputSpans.reserve(std::max<size_t>(4, lineView.size() / 8));

    InlineStyle currentStyle{};
    size_t indexPos = 0;
    size_t segmentStart = 0;

    auto flushTo = [&](size_t endPos) {
        if (endPos > segmentStart) {
            emitStyledSpan(outputSpans, lineView.substr(segmentStart, endPos - segmentStart),
                           currentStyle);
        }
        segmentStart = endPos;
    };

    while (indexPos < lineView.size()) {
        // Code span: `...`
        if (lineView[indexPos] == '`') {
            flushTo(indexPos);
            emitPlainSpan(outputSpans, lineView.substr(indexPos, 1)); // opening backtick
            ++indexPos;
            segmentStart = indexPos;

            size_t closingPos = lineView.find('`', indexPos);
            if (closingPos == std::string_view::npos) {
                // unmatched backtick; keep scanning as raw text
                continue;
            }

            InlineStyle codeStyle = currentStyle;
            codeStyle.isCode = true;
            if (closingPos > indexPos) {
                emitStyledSpan(outputSpans, lineView.substr(indexPos, closingPos - indexPos),
                               codeStyle);
            }

            emitPlainSpan(outputSpans, lineView.substr(closingPos, 1)); // closing backtick
            indexPos = closingPos + 1;
            segmentStart = indexPos;
            continue;
        }

        // Strike: ~~text~~
        if (indexPos + 1 < lineView.size() && lineView[indexPos] == '~' &&
            lineView[indexPos + 1] == '~') {
            flushTo(indexPos);
            emitPlainSpan(outputSpans, lineView.substr(indexPos, 2)); // keep delimiters visible
            currentStyle.isStrike = !currentStyle.isStrike;
            indexPos += 2;
            segmentStart = indexPos;
            continue;
        }

        // Bold: ** or __
        if (indexPos + 1 < lineView.size() &&
            ((lineView[indexPos] == '*' && lineView[indexPos + 1] == '*') ||
             (lineView[indexPos] == '_' && lineView[indexPos + 1] == '_'))) {
            flushTo(indexPos);
            emitPlainSpan(outputSpans, lineView.substr(indexPos, 2)); // keep delimiters visible
            currentStyle.isBold = !currentStyle.isBold;
            indexPos += 2;
            segmentStart = indexPos;
            continue;
        }

        // Italic: * or _ (single; bold case handled above)
        if (lineView[indexPos] == '*' || lineView[indexPos] == '_') {
            flushTo(indexPos);
            emitPlainSpan(outputSpans, lineView.substr(indexPos, 1)); // keep delimiter visible
            currentStyle.isItalic = !currentStyle.isItalic;
            ++indexPos;
            segmentStart = indexPos;
            continue;
        }

        // Link: [text](url) — delimiters raw; text + url styled as link
        if (lineView[indexPos] == '[') {
            size_t rightBracketPos = lineView.find(']', indexPos + 1);
            if (rightBracketPos != std::string_view::npos &&
                rightBracketPos + 1 < lineView.size() && lineView[rightBracketPos + 1] == '(') {
                size_t rightParenPos = lineView.find(')', rightBracketPos + 2);
                if (rightParenPos != std::string_view::npos) {
                    flushTo(indexPos);

                    emitPlainSpan(outputSpans, lineView.substr(indexPos, 1)); // '['
                    if (rightBracketPos > indexPos + 1) {
                        InlineStyle linkStyle = currentStyle;
                        linkStyle.isLink = true;
                        emitStyledSpan(
                            outputSpans,
                            lineView.substr(indexPos + 1, rightBracketPos - (indexPos + 1)),
                            linkStyle);
                    }
                    emitPlainSpan(outputSpans, lineView.substr(rightBracketPos, 2)); // "]("

                    if (rightParenPos > rightBracketPos + 2) {
                        InlineStyle linkStyle = currentStyle;
                        linkStyle.isLink = true;
                        emitStyledSpan(outputSpans,
                                       lineView.substr(rightBracketPos + 2,
                                                       rightParenPos - (rightBracketPos + 2)),
                                       linkStyle);
                    }
                    emitPlainSpan(outputSpans, lineView.substr(rightParenPos, 1)); // ')'

                    indexPos = rightParenPos + 1;
                    segmentStart = indexPos;
                    continue;
                }
            }
        }

        // Autolink: http(s)://...
        if (startsWith(lineView.substr(indexPos), "http://") ||
            startsWith(lineView.substr(indexPos), "https://")) {
            flushTo(indexPos);
            size_t endPos = indexPos;
            while (endPos < lineView.size() &&
                   (std::isspace(static_cast<unsigned char>(lineView[endPos])) == 0)) {
                ++endPos;
            }
            InlineStyle linkStyle = currentStyle;
            linkStyle.isLink = true;
            emitStyledSpan(outputSpans, lineView.substr(indexPos, endPos - indexPos), linkStyle);
            indexPos = endPos;
            segmentStart = indexPos;
            continue;
        }

        ++indexPos;
    }

    flushTo(indexPos);
    return outputSpans;
}

inline ftxui::Element renderInline(std::string_view lineView, const MarkdownTheme &theme) {
    using namespace ftxui;

    auto spanList = tokenizeInline(lineView);
    std::vector<Element> elementList;
    elementList.reserve(spanList.size());

    for (auto &spanEntry : spanList) {
        Element elementItem = text(std::string(spanEntry.textView));
        applyInlineStyle(elementItem, spanEntry.styleState, theme);
        elementList.push_back(std::move(elementItem));
    }

    if (elementList.empty()) {
        return text("");
    }
    return hbox(std::move(elementList));
}

// ---------- Block parsing (preserves raw prefixes/fences) ----------
struct ParseState {
    bool inFence = false;
    char fenceChar = '`';
};

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
inline ftxui::Element decorateLine(std::string_view lineView, ParseState &parseState,
                                   const MarkdownTheme &theme) {
    using namespace ftxui;

    if (!lineView.empty() && lineView.back() == '\r') {
        lineView.remove_suffix(1);
    }

    // Fenced code lines: show raw fence line; toggle state
    if (startsWith(lineView, "```") || startsWith(lineView, "~~~")) {
        char fenceDetect = lineView[0];
        if (!parseState.inFence) {
            parseState.inFence = true;
            parseState.fenceChar = fenceDetect;
        } else {
            if (parseState.fenceChar == fenceDetect) {
                parseState.inFence = false;
            }
        }
        return text(std::string(lineView)) | dim | color(Color::GrayLight);
    }

    // Inside fence: style whole raw line as code
    if (parseState.inFence) {
        return text(std::string(lineView)) | color(theme.codeFg);
    }

    // Horizontal rule: keep raw characters, just dim them
    if (isHorizontalRule(lineView)) {
        return text(std::string(lineView)) | dim;
    }

    // Headers: keep '#' raw; colorize entire line with per-level green shade
    if (!lineView.empty() && lineView.front() == '#') {
        int headerLevel = static_cast<int>(runOf(lineView, '#'));
        auto afterHashesPos = static_cast<std::size_t>(headerLevel);
        if (afterHashesPos < lineView.size() && lineView[afterHashesPos] == ' ') {
            auto headerElement = renderInline(lineView, theme);
            if (headerLevel <= 2) {
                headerElement = headerElement | bold;
            }
            return headerElement | color(theme.headerColorForLevel(headerLevel));
        }
    }

    // Blockquotes: keep all '>' raw; color prefix; inline for remainder
    if (!lineView.empty() && lineView.front() == '>') {
        std::size_t indexPos = 0;
        while (indexPos < lineView.size()) {
            if (lineView[indexPos] == '>') {
                ++indexPos;
                if (indexPos < lineView.size() && lineView[indexPos] == ' ') {
                    ++indexPos;
                }
            } else {
                break;
            }
        }
        auto prefixElement = text(std::string(lineView.substr(0, indexPos))) | color(theme.quote);
        auto restElement = renderInline(lineView.substr(indexPos), theme);
        return hbox({prefixElement, restElement});
    }

    // Unordered lists: keep bullet raw and colored
    {
        std::size_t indexPos = 0;
        int indentSpaces = 0;
        while (indexPos < lineView.size() && isWhitespaceChar(lineView[indexPos])) {
            if (lineView[indexPos] == ' ') {
                ++indentSpaces;
            }
            ++indexPos;
        }
        if (indexPos < lineView.size() &&
            (lineView[indexPos] == '-' || lineView[indexPos] == '*' || lineView[indexPos] == '+') &&
            indexPos + 1 < lineView.size() && lineView[indexPos + 1] == ' ') {
            auto padElement = text(std::string(lineView.substr(0, indexPos)));
            auto bulletElement =
                text(std::string(lineView.substr(indexPos, 2))) | color(theme.list) | bold;
            auto restElement = renderInline(lineView.substr(indexPos + 2), theme);
            return hbox({padElement, bulletElement, restElement});
        }
    }

    // Ordered lists: keep "NN. " raw and colored
    {
        std::size_t indexPos = 0;
        int indentSpaces = 0;
        while (indexPos < lineView.size() && isWhitespaceChar(lineView[indexPos])) {
            if (lineView[indexPos] == ' ') {
                ++indentSpaces;
            }
            ++indexPos;
        }
        std::size_t digitCount = digitsAt(lineView, indexPos);
        if (digitCount > 0 && indexPos + digitCount < lineView.size() &&
            lineView[indexPos + digitCount] == '.' && indexPos + digitCount + 1 < lineView.size() &&
            lineView[indexPos + digitCount + 1] == ' ') {
            auto padElement = text(std::string(lineView.substr(0, indexPos)));
            auto bulletElement = text(std::string(lineView.substr(indexPos, digitCount + 2))) |
                                 color(theme.list) | bold;
            auto restElement = renderInline(lineView.substr(indexPos + digitCount + 2), theme);
            return hbox({padElement, bulletElement, restElement});
        }
    }

    // Paragraph: inline highlighting only (all raw preserved)
    return renderInline(lineView, theme);
}

ftxui::Elements markdown(std::string_view documentView, const MarkdownTheme &theme) {
    using namespace ftxui;

    Elements elementList;
    elementList.reserve(static_cast<std::size_t>(std::ranges::count(documentView, '\n')) + 1);

    ParseState parseState{};
    std::size_t scanPosition = 0;

    while (scanPosition <= documentView.size()) {
        std::size_t newlinePosition = documentView.find('\n', scanPosition);
        std::string_view lineView =
            (newlinePosition == std::string_view::npos)
                ? documentView.substr(scanPosition)
                : documentView.substr(scanPosition, newlinePosition - scanPosition);

        elementList.push_back(decorateLine(lineView, parseState, theme));

        if (newlinePosition == std::string_view::npos) {
            break;
        }
        scanPosition = newlinePosition + 1;
    }

    return elementList;
}

} // namespace caps_log::view
