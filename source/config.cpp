#include "config.hpp"
#include "log/log_repository_crypto_applier.hpp"
#include "utils/string.hpp"
#include "view/view.hpp"
#include <algorithm>
#include <boost/program_options/config.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <cctype>
#include <filesystem>
#include <fmt/format.h>
#include <ftxui/screen/color.hpp>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace caps_log {
using boost::program_options::variables_map;

namespace {

inline std::vector<int> parseInts(std::string_view str) {
    std::vector<int> out;
    std::string tmp;
    tmp.reserve(str.size());
    for (char chr : str) {
        if (chr == ',' || (std::isspace(static_cast<unsigned char>(chr)) != 0)) {
            if (!tmp.empty()) {
                try {
                    out.push_back(std::stoi(tmp));
                } catch (const std::exception &) {
                    throw ConfigParsingException{"Invalid numeric value in color: " + tmp};
                }
                tmp.clear();
            }
        } else {
            tmp.push_back(chr);
        }
    }
    if (!tmp.empty()) {
        try {
            out.push_back(std::stoi(tmp));
        } catch (const std::exception &) {
            throw ConfigParsingException{"Invalid numeric value in color: " + tmp};
        }
    }
    return out;
}

inline ftxui::Color parseHexStrict(std::string_view str) {
    static constexpr auto kMaxHexColorLength = 7; // including '#'
    if (str.size() != kMaxHexColorLength || str[0] != '#') {
        throw ConfigParsingException{"Invalid hex color format: " + std::string(str)};
    }
    auto hex = [&](char chr) -> int {
        if (chr >= '0' && chr <= '9') {
            return chr - '0';
        }
        chr = static_cast<char>(std::tolower(static_cast<unsigned char>(chr)));
        if (chr >= 'a' && chr <= 'f') {
            // NOLINTNEXTLINE(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
            return 10 + (chr - 'a');
        }
        return -1;
    };
    // NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
    int red1 = hex(str[1]);
    int red2 = hex(str[2]);
    int green1 = hex(str[3]);
    int green2 = hex(str[4]);
    int blue1 = hex(str[5]);
    int blue2 = hex(str[6]);
    // NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
    if (red1 < 0 || red2 < 0 || green1 < 0 || green2 < 0 || blue1 < 0 || blue2 < 0) {
        throw ConfigParsingException{"Invalid hex color value: " + std::string(str)};
    }
    return ftxui::Color::RGB((red1 << 4) | red2, (green1 << 4) | green2, (blue1 << 4) | blue2);
}

inline ftxui::Color parseAnsi16Strict(std::string_view str) {
    static const std::unordered_map<std::string, int> kNames = {
        {"black", 0},        {"red", 1},
        {"green", 2},        {"yellow", 3},
        {"blue", 4},         {"magenta", 5},
        {"cyan", 6},         {"white", 7},
        {"brightblack", 8},  {"brightred", 9},
        {"brightgreen", 10}, {"brightyellow", 11},
        {"brightblue", 12},  {"brightmagenta", 13},
        {"brightcyan", 14},  {"brightwhite", 15},
    };

    std::string val = utils::lowercase(utils::trim(std::string(str)));
    if (val.empty()) {
        throw ConfigParsingException{"ansi16 color value is empty"};
    }

    bool allDigits =
        std::all_of(val.begin(), val.end(), [](unsigned char chr) { return std::isdigit(chr); });
    if (allDigits) {
        static constexpr auto kMaxAnsi16Value = 15;
        int num = 0;
        try {
            num = std::stoi(val);
        } catch (const std::exception &) {
            throw ConfigParsingException{"Invalid ansi16 numeric value: " + val};
        }
        if (num < 0 || num > kMaxAnsi16Value) {
            throw ConfigParsingException{"ansi16 value out of range: " + val};
        }
        return static_cast<ftxui::Color::Palette16>(num);
    }

    auto it = kNames.find(val);
    if (it == kNames.end()) {
        throw ConfigParsingException{"Unknown ansi16 color name: " + val};
    }
    return static_cast<ftxui::Color::Palette16>(it->second);
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
inline ftxui::Color parseColorStrict(std::string_view input) {
    static constexpr auto kMaxColorValue = 255;
    std::string str = utils::trim(std::string(input));
    if (str.empty()) {
        throw ConfigParsingException{"Color value is empty"};
    }

    if (str[0] == '#') {
        return parseHexStrict(str);
    }

    std::string lower = utils::lowercase(str);
    auto lparen = lower.find('(');
    auto rparen = lower.rfind(')');
    if (lparen == std::string::npos || rparen == std::string::npos || rparen <= lparen + 1 ||
        rparen != lower.size() - 1) {
        throw ConfigParsingException{"Invalid color format: " + str};
    }

    std::string kind = lower.substr(0, lparen);
    std::string payload = lower.substr(lparen + 1, rparen - lparen - 1);

    if (kind == "rgb" || kind == "rgba") {
        auto vals = parseInts(payload);
        const bool isRgba = kind == "rgba";
        const size_t expected = isRgba ? 4U : 3U;
        if (vals.size() != expected) {
            throw ConfigParsingException{fmt::format("{} color expects {} values: {}",
                                                     isRgba ? "RGBA" : "RGB", expected, str)};
        }
        int red = vals[0];
        int green = vals[1];
        int blue = vals[2];
        if (red < 0 || red > kMaxColorValue || green < 0 || green > kMaxColorValue || blue < 0 ||
            blue > kMaxColorValue) {
            throw ConfigParsingException{
                fmt::format("{} values out of range: {}", isRgba ? "RGBA" : "RGB", str)};
        }
        if (isRgba) {
            int alpha = vals[3];
            if (alpha < 0 || alpha > kMaxColorValue) {
                throw ConfigParsingException{"RGBA alpha out of range: " + str};
            }
        }
        return ftxui::Color::RGB(red, green, blue);
    }

    if (kind == "ansi256") {
        auto vals = parseInts(payload);
        if (vals.size() != 1) {
            throw ConfigParsingException{"ansi256 expects 1 value: " + str};
        }
        int num = vals[0];
        if (num < 0 || num > kMaxColorValue) {
            throw ConfigParsingException{"ansi256 value out of range: " + str};
        }
        return ftxui::Color::Palette256(num);
    }

    if (kind == "ansi16") {
        return parseAnsi16Strict(payload);
    }

    throw ConfigParsingException{"Unknown color format: " + str};
}

inline ftxui::Color parseColorOrDefault(const std::string &key, std::string_view input) {
    std::string trimmed = utils::trim(std::string(input));
    if (trimmed.empty() || utils::lowercase(trimmed) == "default") {
        return ftxui::Color::Default;
    }
    try {
        return parseColorStrict(trimmed);
    } catch (const ConfigParsingException &e) {
        throw ConfigParsingException{fmt::format("Invalid {} ({}): {}", key, trimmed, e.what())};
    }
}

struct TextStyle {
    ftxui::Color fgcolor = ftxui::Color::Default;
    ftxui::Color bgcolor = ftxui::Color::Default;
    bool italic = false;
    bool bold = false;
    bool underlined = false;
    bool dim = false;
    bool inverted = false;
    bool strikethrough = false;
    bool blink = false;
};

ftxui::Decorator textStyleDecorator(const TextStyle &style) {
    return [style](ftxui::Element element) {
        if (style.fgcolor != ftxui::Color::Default) {
            element = element | ftxui::color(style.fgcolor);
        }
        if (style.bgcolor != ftxui::Color::Default) {
            element = element | ftxui::bgcolor(style.bgcolor);
        }
        if (style.italic) {
            element = element | ftxui::italic;
        }
        if (style.bold) {
            element = element | ftxui::bold;
        }
        if (style.underlined) {
            element = element | ftxui::underlined;
        }
        if (style.dim) {
            element = element | ftxui::dim;
        }
        if (style.inverted) {
            element = element | ftxui::inverted;
        }
        if (style.strikethrough) {
            element = element | ftxui::strikethrough;
        }
        if (style.blink) {
            element = element | ftxui::blink;
        }
        return element;
    };
}

struct Theme {
    TextStyle logDateStyle;
    TextStyle weekendDateStyle;
    TextStyle eventDateStyle;
    TextStyle todaysDateStyle;
};

view::FtxuiTheme parseFtxuiThemeFromPTree(const boost::property_tree::ptree &ptree,
                                          const std::string &baseKey,
                                          const view::FtxuiTheme &baseTheme) {
    view::FtxuiTheme theme = baseTheme;

    const auto makePath = [](const std::string &key) {
        return boost::property_tree::ptree::path_type(key, '/');
    };

    const auto parseTextStyle = [&](const std::string &sectionKey) -> TextStyle {
        TextStyle style;
        std::string fgColorStr =
            ptree.get<std::string>(makePath(sectionKey + "/fgcolor"), "Default");
        std::string bgColorStr =
            ptree.get<std::string>(makePath(sectionKey + "/bgcolor"), "Default");

        style.fgcolor = parseColorOrDefault(sectionKey + ".fgcolor", fgColorStr);
        style.bgcolor = parseColorOrDefault(sectionKey + ".bgcolor", bgColorStr);

        style.italic = ptree.get<bool>(makePath(sectionKey + "/italic"), false);
        style.bold = ptree.get<bool>(makePath(sectionKey + "/bold"), false);
        style.underlined = ptree.get<bool>(makePath(sectionKey + "/underlined"), false);
        style.dim = ptree.get<bool>(makePath(sectionKey + "/dim"), false);
        style.inverted = ptree.get<bool>(makePath(sectionKey + "/inverted"), false);
        style.strikethrough = ptree.get<bool>(makePath(sectionKey + "/strikethrough"), false);
        style.blink = ptree.get<bool>(makePath(sectionKey + "/blink"), false);

        return style;
    };

    const auto maybeApplyStyle = [&](const std::string &sectionKey, ftxui::Decorator &decorator) {
        if (!ptree.get_child_optional(makePath(sectionKey))) {
            return;
        }
        decorator = textStyleDecorator(parseTextStyle(sectionKey));
    };

    // accepted border styles correspond to ftxui border styles:   LIGHT, DASHED, HEAVY, DOUBLE,
    // ROUNDED, EMPTY,

    const auto maybeApplyBorderStyle = [&](const std::vector<std::string> &keys,
                                           ftxui::BorderStyle &destination) {
        static const std::unordered_map<std::string, ftxui::BorderStyle> kBorderStyles = {
            {"light", ftxui::BorderStyle::LIGHT},     {"dashed", ftxui::BorderStyle::DASHED},
            {"heavy", ftxui::BorderStyle::HEAVY},     {"double", ftxui::BorderStyle::DOUBLE},
            {"rounded", ftxui::BorderStyle::ROUNDED}, {"empty", ftxui::BorderStyle::EMPTY},
        };
        std::string value;
        std::string keyUsed;
        for (const auto &key : keys) {
            if (const auto rawValue = ptree.get_optional<std::string>(makePath(key))) {
                value = utils::lowercase(utils::trim(rawValue.value()));
                keyUsed = key;
                break;
            }
        }
        if (value.empty()) {
            return;
        }
        auto it = kBorderStyles.find(value);
        if (it == kBorderStyles.end()) {
            throw ConfigParsingException{"Invalid border style for " + keyUsed + ": " + value};
        }
        destination = it->second;
    };

    maybeApplyStyle(baseKey + "empty-date", theme.emptyDateDecorator);
    maybeApplyStyle(baseKey + "log-date", theme.logDateDecorator);
    maybeApplyStyle(baseKey + "highlighted-date", theme.highlightedDateDecorator);
    maybeApplyStyle(baseKey + "weekend-date", theme.weekendDateDecorator);
    maybeApplyStyle(baseKey + "event-date", theme.eventDateDecorator);
    maybeApplyStyle(baseKey + "todays-date", theme.todaysDateDecorator);

    const auto baseSectionKey = (not baseKey.empty() && baseKey.back() == '.')
                                    ? baseKey.substr(0, baseKey.size() - 1)
                                    : baseKey;

    maybeApplyBorderStyle({baseSectionKey + "/calendar-border", baseKey + "calendar-border/border"},
                          theme.calendarBorder);
    maybeApplyBorderStyle(
        {baseSectionKey + "/calendar-month-border", baseKey + "calendar-month-border/border"},
        theme.calendarMonthBorder);
    maybeApplyBorderStyle({baseSectionKey + "/tags-menu.border", baseKey + "tags-menu/border"},
                          theme.tagsMenuConfig.border);
    maybeApplyBorderStyle(
        {baseSectionKey + "/sections-menu.border", baseKey + "sections-menu/border"},
        theme.sectionsMenuConfig.border);
    maybeApplyBorderStyle({baseSectionKey + "/events-list.border", baseKey + "events-list/border"},
                          theme.eventsListConfig.border);
    maybeApplyBorderStyle(
        {baseSectionKey + "/log-entry-preview.border", baseKey + "log-entry-preview/border"},
        theme.logEntryPreviewConfig.border);

    return theme;
}

view::MarkdownTheme parseMarkdownThemeFromPTree(const boost::property_tree::ptree &ptree,
                                                const std::string &baseSectionKey,
                                                const view::MarkdownTheme &baseTheme) {
    view::MarkdownTheme theme = baseTheme;

    const auto makePath = [](const std::string &key) {
        return boost::property_tree::ptree::path_type(key, '/');
    };

    const auto maybeApplyColor = [&](const std::string &key, ftxui::Color &destination) {
        const auto path = baseSectionKey + "/" + key;
        if (const auto value = ptree.get_optional<std::string>(makePath(path))) {
            destination = parseColorOrDefault(baseSectionKey + "." + key, value.value());
        }
    };

    for (std::size_t i = 0; i < theme.headerShades.size(); ++i) {
        maybeApplyColor("header" + std::to_string(i + 1), theme.headerShades.at(i));
    }
    maybeApplyColor("list", theme.list);
    maybeApplyColor("quote", theme.quote);
    maybeApplyColor("code-fg", theme.codeFg);

    return theme;
}

view::ScratchpadTheme parseScratchpadThemeFromPTree(const boost::property_tree::ptree &ptree,
                                                    const std::string &baseKey,
                                                    const view::ScratchpadTheme &baseTheme) {
    view::ScratchpadTheme theme = baseTheme;

    const auto makePath = [](const std::string &key) {
        return boost::property_tree::ptree::path_type(key, '/');
    };

    const auto maybeApplyBorderStyle = [&](const std::vector<std::string> &keys,
                                           ftxui::BorderStyle &destination) {
        static const std::unordered_map<std::string, ftxui::BorderStyle> kBorderStyles = {
            {"light", ftxui::BorderStyle::LIGHT},     {"dashed", ftxui::BorderStyle::DASHED},
            {"heavy", ftxui::BorderStyle::HEAVY},     {"double", ftxui::BorderStyle::DOUBLE},
            {"rounded", ftxui::BorderStyle::ROUNDED}, {"empty", ftxui::BorderStyle::EMPTY},
        };
        std::string value;
        std::string keyUsed;
        for (const auto &key : keys) {
            if (const auto rawValue = ptree.get_optional<std::string>(makePath(key))) {
                value = utils::lowercase(utils::trim(rawValue.value()));
                keyUsed = key;
                break;
            }
        }
        if (value.empty()) {
            return;
        }
        auto it = kBorderStyles.find(value);
        if (it == kBorderStyles.end()) {
            throw ConfigParsingException{"Invalid border style for " + keyUsed + ": " + value};
        }
        destination = it->second;
    };

    const auto baseSectionKey = (not baseKey.empty() && baseKey.back() == '.')
                                    ? baseKey.substr(0, baseKey.size() - 1)
                                    : baseKey;

    maybeApplyBorderStyle({baseSectionKey + "/menu.border", baseKey + "menu/border"},
                          theme.menuConfig.border);
    maybeApplyBorderStyle({baseSectionKey + "/preview.border", baseKey + "preview/border"},
                          theme.previewConfig.border);

    theme.previewConfig.markdownTheme = parseMarkdownThemeFromPTree(
        ptree, baseSectionKey + ".preview.markdown-theme", theme.previewConfig.markdownTheme);

    return theme;
}

std::filesystem::path expandTilde(const std::filesystem::path &input) {
    std::string str = input.string();

    if (!str.empty() && str[0] == '~') {
        const char *home = std::getenv("HOME");
        if (home == nullptr) {
#ifdef _WIN32
            home = std::getenv("USERPROFILE");
#endif
        }
        if (home != nullptr) {
            // drop `~/` (note: must drop '/', otherwise it is an absolute path and the path
            // concatenation wont work)
            return std::filesystem::path(home) / str.substr(2);
        }
    }
    return input;
}

std::optional<std::chrono::month_day> parseDate(const std::string &date_str) {
    int day = 0;
    int month = 0;
    char dot1 = 0;
    char dot2 = 0;

    std::istringstream iss(date_str);
    static constexpr auto kMaxMonths = 12;
    static constexpr auto kMaxDays = 31;
    if (iss >> std::setw(2) >> day >> dot1 >> std::setw(2) >> month >> dot2 && dot1 == '.' &&
        dot2 == '.' && day >= 1 && day <= kMaxDays && month >= 1 && month <= kMaxMonths) {
        return std::chrono::month_day{std::chrono::month(month), std::chrono::day(day)};
    }
    return std::nullopt; // Return empty optional if parsing fails
}

view::CalendarEvents parseCalendarEvents(const boost::property_tree::ptree &ptree) {
    // CalendarEvents structure to store all events
    view::CalendarEvents calendarEvents;

    // Iterate over each section in the property tree
    for (const auto &section : ptree) {
        const std::string &key = section.first; // e.g., "calendar-events.birthdays.0"
        const boost::property_tree::ptree &data = section.second;

        // Check if the key starts with "calendar-events."
        if (key.rfind("calendar-events.", 0) != 0) {
            // Not a calendar event section, skip it
            continue;
        }

        // Split the key to extract event type
        std::stringstream sstream(key);
        std::string item;
        std::vector<std::string> tokens;
        while (std::getline(sstream, item, '.')) {
            tokens.push_back(item);
        }

        if (tokens.size() != 3) {
            throw ConfigParsingException("Invalid calendar event key: " + key);
        }

        // Extract the category (e.g., "birthdays", "holidays", "anniversaries")
        std::string category = tokens[1];

        // Get the name and date from the section
        auto name = data.get<std::string>("name");
        auto dateStr = data.get<std::string>("date");

        // Parse the date string into day and month
        int day{};
        int month{};
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
        if (sscanf(dateStr.c_str(), "%02d.%02d.", &day, &month) != 2) {
            throw ConfigParsingException("Invalid date format: " + dateStr);
        }

        // Create a month_day object
        std::chrono::month_day date{std::chrono::month{static_cast<unsigned>(month)},
                                    std::chrono::day{static_cast<unsigned>(day)}};

        // Create a CalendarEvent and insert it into the map
        view::CalendarEvent event{name, date};
        calendarEvents[category].insert(event);
    }

    return calendarEvents;
}

std::filesystem::path removeTrailingSlash(const std::filesystem::path &path) {
    std::string pathStr = path.string();
    if (!pathStr.empty() && pathStr.back() == '/') {
        pathStr.pop_back();
    }
    return pathStr;
}

template <class T, class U>
void setIfValue(const boost::property_tree::ptree &ptree, const std::string &key, U &destination) {
    if (const auto value = ptree.get_optional<T>(key)) {
        destination = value.get();
    }
}

bool isParentOf(const std::filesystem::path &parent, const std::filesystem::path &child) {
    const auto nParent = removeTrailingSlash(parent.lexically_normal());
    const auto nChild = removeTrailingSlash(child.lexically_normal());
    auto parentIter = nParent.begin();
    auto childIter = nChild.begin();

    while (parentIter != nParent.end() && childIter != nChild.end()) {
        if (*parentIter != *childIter) {
            return false;
        }
        ++parentIter;
        ++childIter;
    }

    return parentIter == nParent.end() && childIter != nChild.end();
}

variables_map parseCLIOptions(const std::vector<std::string> &argv) {
    namespace po = boost::program_options;

    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()
      ("help,h", "show this message")
      ("config,c", po::value<std::string>(), ("override the default config file path (" + Configuration::kDefaultConfigLocation + ")").c_str())
      ("log-dir-path", po::value<std::string>(), "path where log files are stored")
      ("log-name-format", po::value<std::string>(), "format in which log entry markdown files are saved")
      ("sunday-start", "have the calendar display sunday as first day of the week")
      ("first-line-section", "override the default behaviour of ignoring sections (lines starting with `#`) in the first line of a log entry file")
      ("password", po::value<std::string>(), "password for encrypted log repositories or to be used with --encrypt/--decrypt")
      ("encrypt", "apply encryption to all logs in log dir path (needs --password)")
      ("decrypt", "apply decryption to all logs in log dir path (needs --password)");
    // clang-format on

    std::vector<const char *> args;
    args.reserve(argv.size());
    for (const auto &arg : argv) {
        args.push_back(arg.c_str());
    }

    po::variables_map vmap;
    po::store(po::parse_command_line(static_cast<int>(args.size()), args.data(), desc), vmap);
    po::notify(vmap);

    if (vmap.contains("help")) {
        std::cout << "Captain's Log (caps-log)! A CLI journalig tool." << '\n';
        std::cout << "Version: " << CAPS_LOG_VERSION_STRING << '\n';
        std::cout << desc;
        std::cout << std::flush;
        exit(0);
    }

    return vmap;
}
} // namespace

const std::string Configuration::kDefaultConfigLocation =
    std::getenv("HOME") + std::string{"/.caps-log/config.ini"};
const std::string Configuration::kDefaultLogDirPath =
    std::getenv("HOME") + std::string{"/.caps-log/day"};
const std::string Configuration::kDefaultScratchpadFolderName = "scratchpads";
const std::string Configuration::kDefaultLogFilenameFormat = "d%Y_%m_%d.md";
const bool Configuration::kDefaultSundayStart = false;
const bool Configuration::kDefaultAcceptSectionsOnFirstLine = false;

std::function<std::string(const std::filesystem::path &)> Configuration::makeDefaultReadFileFunc() {
    return [](const std::filesystem::path &path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            throw ConfigParsingException{"Failed to open file: " + path.string()};
        }
        return std::string((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
    };
}

Configuration::Configuration(
    const std::vector<std::string> &cliArgs,
    const std::function<std::string(const std::filesystem::path &)> &readFileFunc) {

    applyDefaults();
    boost::program_options::variables_map vmap = parseCLIOptions(cliArgs);

    m_configFilePath =
        vmap.contains("config") ? vmap["config"].as<std::string>() : kDefaultConfigLocation;

    auto content = readFileFunc(m_configFilePath);

    boost::property_tree::ptree ptree;
    try {
        std::istringstream iss(content);
        boost::property_tree::read_ini(iss, ptree);
    } catch (const boost::property_tree::ini_parser_error &e) {
        throw ConfigParsingException{"Failed to parse configuration file: " +
                                     std::string(e.what())};
    }
    overrideFromConfigFile(ptree);
    overrideFromCommandLine(vmap);
}

void Configuration::applyDefaults() {
    m_viewConfig =
        view::ViewConfig{
            .annualViewConfig =
                view::AnnualViewConfig{
                    .theme =
                        view::FtxuiTheme{
                            .emptyDateDecorator = ftxui::dim,
                            .logDateDecorator = ftxui::underlined,
                            .weekendDateDecorator = ftxui::color(ftxui::Color::Blue),
                            .eventDateDecorator = ftxui::color(ftxui::Color::Green),
                            .highlightedDateDecorator = ftxui::color(ftxui::Color::Yellow),
                            .todaysDateDecorator = ftxui::color(ftxui::Color::Red),
                            .calendarBorder = ftxui::BorderStyle::ROUNDED,
                            .calendarMonthBorder = ftxui::BorderStyle::ROUNDED,
                            .tagsMenuConfig =
                                view::MenuConfig{
                                    .border = ftxui::BorderStyle::ROUNDED,
                                },
                            .sectionsMenuConfig =
                                view::MenuConfig{
                                    .border = ftxui::BorderStyle::ROUNDED,
                                },
                            .eventsListConfig =
                                view::EventsListConfig{
                                    .border = ftxui::BorderStyle::ROUNDED,
                                },
                            .logEntryPreviewConfig =
                                view::TextPreviewConfig{
                                    .border = ftxui::BorderStyle::ROUNDED,
                                    .markdownTheme = view::getDefaultMarkdownTheme(),
                                },
                        },
                    .sundayStart = Configuration::kDefaultSundayStart,
                    .recentEventsWindow = Configuration::kDefaultRecentEventsWindow,
                },
            .scratchpadViewConfig =
                view::ScratchpadViewConfig{
                    .theme =
                        view::ScratchpadTheme{
                            .menuConfig =
                                view::MenuConfig{
                                    .border = ftxui::BorderStyle::ROUNDED,
                                },
                            .previewConfig =
                                view::TextPreviewConfig{
                                    .border = ftxui::BorderStyle::ROUNDED,
                                    .markdownTheme = view::getDefaultMarkdownTheme(),
                                },
                        },
                },
        };
    m_password = "";
    m_cryptoApplicationType = std::nullopt;
    m_gitRepoConfig = std::nullopt;
    m_acceptSectionsOnFirstLine = Configuration::kDefaultAcceptSectionsOnFirstLine;
    m_logDirPath = expandTilde(Configuration::kDefaultLogDirPath);
    m_logFilenameFormat = Configuration::kDefaultLogFilenameFormat;
    m_calendarEvents = view::CalendarEvents{};
}

void Configuration::overrideFromConfigFile(const boost::property_tree::ptree &ptree) {
    setIfValue<std::string>(ptree, "log-dir-path", m_logDirPath);
    setIfValue<std::string>(ptree, "log-filename-format", m_logFilenameFormat);
    setIfValue<bool>(ptree, "sunday-start", m_viewConfig.annualViewConfig.sundayStart);
    setIfValue<bool>(ptree, "first-line-section", m_acceptSectionsOnFirstLine);
    setIfValue<std::string>(ptree, "password", m_password);
    setIfValue<unsigned>(ptree, "calendar-events.recent-events-window",
                         m_viewConfig.annualViewConfig.recentEventsWindow);

    // Parse and update calendar events
    m_calendarEvents = parseCalendarEvents(ptree);

    // sanitize log dir path
    m_logDirPath = expandTilde(m_logDirPath);

    if (ptree.get_optional<bool>("git.enable-git-log-repo").value_or(false)) {
        utils::GitRepoConfig gitConf;
        setIfValue<std::string>(ptree, "git.ssh-pub-key-path", gitConf.sshPubKeyPath);
        setIfValue<std::string>(ptree, "git.ssh-key-path", gitConf.sshKeyPath);
        setIfValue<std::string>(ptree, "git.main-branch-name", gitConf.mainBranchName);
        setIfValue<std::string>(ptree, "git.remote-name", gitConf.remoteName);
        setIfValue<std::string>(ptree, "git.repo-root", gitConf.root);

        // sanitize paths
        gitConf.sshKeyPath = expandTilde(gitConf.sshKeyPath);
        gitConf.sshPubKeyPath = expandTilde(gitConf.sshPubKeyPath);

        m_gitRepoConfig = gitConf;
    }

    m_viewConfig.annualViewConfig.theme = parseFtxuiThemeFromPTree(
        ptree, "view.annual-view.theme.", m_viewConfig.annualViewConfig.theme);
    m_viewConfig.annualViewConfig.theme.logEntryPreviewConfig.markdownTheme =
        parseMarkdownThemeFromPTree(
            ptree, "view.annual-view.theme.log-entry-preview.markdown-theme",
            m_viewConfig.annualViewConfig.theme.logEntryPreviewConfig.markdownTheme);

    m_viewConfig.scratchpadViewConfig.theme = parseScratchpadThemeFromPTree(
        ptree, "view.scratchpad-view.theme.", m_viewConfig.scratchpadViewConfig.theme);
}

void Configuration::overrideFromCommandLine(const boost::program_options::variables_map &vmap) {
    if (vmap.contains("log-dir-path")) {
        m_logDirPath = expandTilde(vmap["log-dir-path"].as<std::string>());
    }
    if (vmap.contains("log-name-format")) {
        m_logFilenameFormat = vmap["log-name-format"].as<std::string>();
    }
    if (vmap.contains("sunday-start")) {
        m_viewConfig.annualViewConfig.sundayStart = true;
    }
    if (vmap.contains("first-line-section")) {
        m_acceptSectionsOnFirstLine = true;
    }
    if (vmap.contains("password")) {
        m_password = vmap["password"].as<std::string>();
    }

    if (vmap.contains("encrypt")) {
        if (m_password.empty()) {
            throw ConfigParsingException{"Password must be provided when encrypting logs!"};
        }
        m_cryptoApplicationType = Crypto::Encrypt;
    } else if (vmap.contains("decrypt")) {
        if (m_password.empty()) {
            throw ConfigParsingException{"Password must be provided when decrypting logs!"};
        }
        m_cryptoApplicationType = Crypto::Decrypt;
    }
}

void Configuration::verify() const {
    if (m_logDirPath.empty()) {
        throw ConfigParsingException{"Log directory path can not be empty!"};
    }
    if (m_logFilenameFormat.empty()) {
        throw ConfigParsingException{"Log filename format can not be empty!"};
    }
    if (m_cryptoApplicationType.has_value() && m_password.empty()) {
        throw ConfigParsingException{
            "Password must be provided when encryption encrypting/decrypting a repository."};
    }
    if (m_gitRepoConfig.has_value()) {
        const auto &gitConf = m_gitRepoConfig.value();
        if (gitConf.sshKeyPath.empty()) {
            throw ConfigParsingException{"SSH key path can not be empty!"};
        }
        if (gitConf.sshPubKeyPath.empty()) {
            throw ConfigParsingException{"SSH public key path can not be empty!"};
        }
        if (gitConf.mainBranchName.empty()) {
            throw ConfigParsingException{"Main branch name can not be empty!"};
        }
        if (gitConf.remoteName.empty()) {
            throw ConfigParsingException{"Remote name can not be empty!"};
        }
        if (gitConf.root.empty()) {
            throw ConfigParsingException{"Git repository root can not be empty!"};
        }

        if (not isParentOf(gitConf.root, m_logDirPath) && gitConf.root != m_logDirPath) {
            throw ConfigParsingException{"Error! Git repo root (" + gitConf.root.string() +
                                         ") is not parent of log dir (" + m_logDirPath + ")!"};
        }

        // expect that  sshKeyPath, sshPubKeyPath, root exist
        if (not std::filesystem::exists(gitConf.sshKeyPath)) {
            throw ConfigParsingException{"SSH key does not exist: " + gitConf.sshKeyPath.string()};
        }
        if (not std::filesystem::exists(gitConf.sshPubKeyPath)) {
            throw ConfigParsingException{"SSH public key does not exist: " +
                                         gitConf.sshPubKeyPath.string()};
        }
        if (not std::filesystem::exists(gitConf.root)) {
            throw ConfigParsingException{"Git repository root does not exist: " +
                                         gitConf.root.string()};
        }
    }
}

[[nodiscard]] const view::ViewConfig &Configuration::getViewConfig() const { return m_viewConfig; }

[[nodiscard]] const std::optional<utils::GitRepoConfig> &Configuration::getGitRepoConfig() const {
    return m_gitRepoConfig;
}

[[nodiscard]] bool Configuration::shouldRunApplication() const {
    return not m_cryptoApplicationType.has_value();
}

[[nodiscard]] std::string Configuration::getLogDirPath() const { return m_logDirPath; }

[[nodiscard]] std::string Configuration::getPassword() const { return m_password; }

[[nodiscard]] std::string Configuration::getLogFilenameFormat() const {
    return m_logFilenameFormat;
}

[[nodiscard]] log::LocalFSLogFilePathProvider Configuration::getLogFilePathProvider() const {
    return log::LocalFSLogFilePathProvider{m_logDirPath, m_logFilenameFormat};
}

[[nodiscard]] std::string Configuration::getScratchpadDirPath() const {
    return m_logDirPath + '/' + Configuration::kDefaultScratchpadFolderName;
}

[[nodiscard]] bool Configuration::isPasswordProvided() const { return !m_password.empty(); }

[[nodiscard]] std::optional<Crypto> Configuration::getCryptoApplicationType() const {
    return m_cryptoApplicationType;
}

[[nodiscard]] std::filesystem::path Configuration::getConfigFilePath() const {
    return m_configFilePath;
}

[[nodiscard]] AppConfig Configuration::getAppConfig() const {
    return AppConfig{
        // TODO: align
        .skipFirstLine = !m_acceptSectionsOnFirstLine,
        .events = m_calendarEvents,
    };
}
} // namespace caps_log
