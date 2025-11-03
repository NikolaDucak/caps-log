#pragma once

#include "utils/string.hpp"
#include "view/annual_view_layout_base.hpp"
#include "view/input_handler.hpp"
#include "view/scratchpad_view_layout_base.hpp"
#include <boost/property_tree/ptree.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/component/task.hpp>
#include <regex>

namespace caps_log::view {

class PopUpViewBase {
  public:
    struct Result {
        struct Ok {};
        struct Yes {};
        struct No {};
        struct Cancel {};
        struct Input {
            std::string text;
        };
    };
    using PopUpResult =
        std::variant<Result::Ok, Result::Yes, Result::No, Result::Cancel, Result::Input>;

    using PopUpCallback = std::function<void(PopUpResult)>;
    struct Ok {
        std::string message;
        PopUpCallback callback = [](const auto &) { /* default no-op callback */ };
    };
    struct YesNo {
        std::string message;
        PopUpCallback callback;
    };
    struct Loading {
        std::string message;
    };
    struct TextBox {
        std::string message;
        PopUpCallback callback;
    };
    struct None {};

    using PopUpType = std::variant<Ok, YesNo, Loading, TextBox, None>;

    PopUpViewBase() = default;
    PopUpViewBase(const PopUpViewBase &) = default;
    PopUpViewBase(PopUpViewBase &&) = default;
    PopUpViewBase &operator=(const PopUpViewBase &) = default;
    PopUpViewBase &operator=(PopUpViewBase &&) = default;

    virtual ~PopUpViewBase() = default;

    virtual void show(const PopUpType &popUp) = 0;
};

class ViewBase {
  public:
    ViewBase() = default;
    ViewBase(const ViewBase &) = default;
    ViewBase(ViewBase &&) = default;
    ViewBase &operator=(const ViewBase &) = default;
    ViewBase &operator=(ViewBase &&) = default;

    virtual ~ViewBase() = default;

    virtual void run() = 0;
    virtual void stop() = 0;

    virtual void post(const ftxui::Task &task) = 0;

    virtual void withRestoredIO(std::function<void()> func) = 0;
    virtual void setInputHandler(InputHandlerBase *handler) = 0;

    virtual PopUpViewBase &getPopUpView() = 0;

    virtual std::shared_ptr<AnnualViewLayoutBase> getAnnualViewLayout() = 0;
    virtual std::shared_ptr<ScratchpadViewLayoutBase> getScratchpadViewLayout() = 0;

    virtual void switchLayout() = 0;
};

struct ViewConfig {
    // Style bitmask carried around as flags, maps nicely to FTXUI decorators later.
    struct StyleMask {
        bool bold = false;
        bool underline = false;
        bool italic = false;
        [[nodiscard]] bool empty() const { return !bold && !underline && !italic; }
    };

    // Small mixins for common UI attributes:
    struct Borderable {
        bool border = true;
    };
    struct Colorable {
        ftxui::Color color = ftxui::Color::Default;
    };
    struct Styleable {
        StyleMask style{};
    };
    struct Selectable {
        ftxui::Color selected_color = ftxui::Color::Default;
        StyleMask selected_style{};
    };

    // Annual/Logs view elements:
    struct LogView {
        struct Menu : Borderable, Colorable, Styleable, Selectable {};
        struct EventsList : Borderable {};
        struct LogEntryPreview : Borderable {
            bool markdownSyntaxHighlighting = false;
        };
        struct AnnualCalendar : Borderable {
            bool sundayStart = false;
            bool monthBorder = true;
            ftxui::Color weekdayColor = ftxui::Color::Default;
            StyleMask weekdayStyle{};
            ftxui::Color weekendColor = ftxui::Color::Default;
            StyleMask weekendStyle{};
            ftxui::Color todayColor = ftxui::Color::Default;
            StyleMask todayStyle{};
            ftxui::Color selectedDayColor = ftxui::Color::Default;
            StyleMask selectedDayStyle{};
            ftxui::Color eventDayColor = ftxui::Color::Default;
            StyleMask eventDayStyle{};
        };

        Menu tagsMenu{};
        Menu sectionsMenu{};
        EventsList eventsList{};
        LogEntryPreview logEntryPreview{};
        AnnualCalendar annualCalendar{};
        unsigned recentEventsWindow = 0;
    } logView{};

    // Scratchpad view elements:
    struct ScratchpadView {
        struct ScratchpadList : Borderable, Colorable, Styleable, Selectable {};
        struct ScratchpadPreview : Borderable {
            bool markdown_syntax_highlighting = false;
        };

        ScratchpadList scratchpadList{};
        ScratchpadPreview scratchpadPreview{};
    } scratchpadView{};
};

// ---- Color parsing (hex(#rgb|#rrggbb), rgb(), 256()/ansi256()) --------------
inline ftxui::Color parseColor(const std::string &raw) {
    const std::string val = utils::lowercase(utils::trim(raw));

    static constexpr auto kColorMax = 255;

    // hex(#abc|#aabbcc)
    {
        static const std::regex kKRegex(R"(^hex\(\s*\#([0-9a-f]{3}|[0-9a-f]{6})\s*\)$)");
        std::smatch match;
        if (std::regex_match(val, match, kKRegex)) {
            std::string hex = match[1].str();
            auto hexToU8 = [](const std::string &hex) -> uint8_t {
                static constexpr int kBase16 = 16;
                return static_cast<uint8_t>(std::strtoul(hex.c_str(), nullptr, kBase16));
            };
            uint8_t red = 0;
            uint8_t green = 0;
            uint8_t blue = 0;
            if (hex.size() == 3) {
                red = hexToU8(std::string{hex[0], hex[0]});
                green = hexToU8(std::string{hex[1], hex[1]});
                blue = hexToU8(std::string{hex[2], hex[2]});
            } else {
                red = hexToU8(hex.substr(0, 2));
                green = hexToU8(hex.substr(2, 2));
                blue = hexToU8(hex.substr(4, 2));
            }
            return ftxui::Color::RGB(red, green, blue);
        }
    }

    // rgb(r,g,b)
    {
        static const std::regex kRegex(
            R"(^rgb\(\s*(\d{1,3})\s*,\s*(\d{1,3})\s*,\s*(\d{1,3})\s*\)$)");
        std::smatch match;
        if (std::regex_match(val, match, kRegex)) {
            auto clamp = [](int val) { return std::clamp(val, 0, kColorMax); };
            int red = clamp(std::stoi(match[1].str()));
            int green = clamp(std::stoi(match[2].str()));
            int blue = clamp(std::stoi(match[3].str()));
            return ftxui::Color::RGB(static_cast<uint8_t>(red), static_cast<uint8_t>(green),
                                     static_cast<uint8_t>(blue));
        }
    }

    // 256(n) / ansi256(n)
    {
        static const std::regex kRegex(R"(^(?:256|ansi256)\(\s*(\d{1,3})\s*\)$)");
        std::smatch match;
        if (std::regex_match(val, match, kRegex)) {
            int val = 0;
            if (utils::parseInt(match[1].str(), val)) {
                val = std::max(0, std::min(kColorMax, val));
                return ftxui::Color::Palette256(val);
            }
        }
    }

    // Default if not recognized (shouldn't happen if INI already validated)
    return ftxui::Color::Default;
}

// ---- Style parsing: "bold, underline, italic" --------------------------------
inline ViewConfig::StyleMask parseStyle(const std::string &raw) {
    ViewConfig::StyleMask stype{};
    std::string val = utils::lowercase(raw);
    size_t start = 0;
    while (start <= val.size()) {
        size_t comma = val.find(',', start);
        std::string tok = utils::trim(
            val.substr(start, (comma == std::string::npos ? val.size() : comma) - start));
        if (!tok.empty()) {
            if (tok == "bold") {
                stype.bold = true;
            } else if (tok == "underline") {
                stype.underline = true;
            } else if (tok == "italic") {
                stype.italic = true;
            }
            // Unknown tokens are ignored here because we assume prior validation.
        }
        if (comma == std::string::npos) {
            break;
        }
        start = comma + 1;
    }
    return stype;
}

// ---- ptree convenience getters (safe defaults; INI is already validated) ----
namespace detail {
inline bool getBool(const boost::property_tree::ptree &cfg, const std::string &key,
                    bool def = false) {
    auto opt = cfg.get_optional<std::string>(key);
    if (!opt) {
        return def;
    }
    std::string value = utils::lowercase(utils::trim(*opt));
    return (value == "true" || value == "1" || value == "yes" || value == "on");
}
inline unsigned getUint(const boost::property_tree::ptree &cfg, const std::string &key,
                        unsigned def = 0) {
    auto opt = cfg.get_optional<std::string>(key);
    if (!opt) {
        return def;
    }
    int tmp = 0;
    if (utils::parseInt(*opt, tmp) && tmp >= 0) {
        return static_cast<unsigned>(tmp);
    }
    return def;
}
inline std::optional<std::string> getStr(const boost::property_tree::ptree &cfg,
                                         const std::string &key) {
    auto opt = cfg.get_optional<std::string>(key);
    if (!opt) {
        return std::nullopt;
    }
    return utils::trim(*opt);
}
} // namespace detail

// ---- Fill helpers for repetitive blocks -------------------------------------
inline void loadBorder(const boost::property_tree::ptree &cfg, const std::string &base,
                       ViewConfig::Borderable &out) {
    out.border = detail::getBool(cfg, base + ".border", out.border);
}

inline void loadColorStyleSelectable(const boost::property_tree::ptree &cfg,
                                     const std::string &base, ViewConfig::Colorable &col,
                                     ViewConfig::Styleable &styl,
                                     ViewConfig::Selectable *sel = nullptr) {
    if (auto val = detail::getStr(cfg, base + ".color")) {
        col.color = parseColor(*val);
    }
    if (auto val = detail::getStr(cfg, base + ".style")) {
        styl.style = parseStyle(*val);
    }
    if (sel != nullptr) {
        if (auto val = detail::getStr(cfg, base + ".selected-color")) {
            sel->selected_color = parseColor(*val);
        }
        if (auto val = detail::getStr(cfg, base + ".selected-style")) {
            sel->selected_style = parseStyle(*val);
        }
    }
}

// ---- The main extractor: build ViewConfig from a validated ptree -------------
inline ViewConfig extractViewConfig(const boost::property_tree::ptree &cfg) {
    ViewConfig viewConfig{};

    // Global:
    viewConfig.logView.annualCalendar.sundayStart =
        detail::getBool(cfg, "sunday-start", viewConfig.logView.annualCalendar.sundayStart);
    viewConfig.logView.recentEventsWindow =
        detail::getUint(cfg, "recent-events-window", viewConfig.logView.recentEventsWindow);

    // UI - Logs / Annual:
    // tags-list
    {
        auto base = std::string("ui.logs-view.tags-menu");
        loadBorder(cfg, base, viewConfig.logView.tagsMenu);
        loadColorStyleSelectable(cfg, base, viewConfig.logView.tagsMenu,
                                 viewConfig.logView.tagsMenu, &viewConfig.logView.tagsMenu);
    }
    // sections-list
    {
        auto base = std::string("ui.logs-view.sections-menu");
        loadBorder(cfg, base, viewConfig.logView.sectionsMenu);
    }
    // events-list
    {
        auto base = std::string("ui.logs-view.events-list");
        loadBorder(cfg, base, viewConfig.logView.eventsList);
    }
    // log-entry-preview
    {
        auto base = std::string("ui.logs-view.log-entry-preview");
        loadBorder(cfg, base, viewConfig.logView.logEntryPreview);
        viewConfig.logView.logEntryPreview.markdownSyntaxHighlighting =
            detail::getBool(cfg, base + ".markdown-syntax-highlighting",
                            viewConfig.logView.logEntryPreview.markdownSyntaxHighlighting);
    }
    // annual-calendar
    {
        auto base = std::string("ui.logs-view.annual-calendar");
        loadBorder(cfg, base, viewConfig.logView.annualCalendar);
        viewConfig.logView.annualCalendar.monthBorder = detail::getBool(
            cfg, base + ".month-border", viewConfig.logView.annualCalendar.monthBorder);

        if (auto val = detail::getStr(cfg, base + ".weekday-color")) {
            viewConfig.logView.annualCalendar.weekdayColor = parseColor(*val);
        }
        if (auto val = detail::getStr(cfg, base + ".weekday-style")) {
            viewConfig.logView.annualCalendar.weekdayStyle = parseStyle(*val);
        }

        if (auto val = detail::getStr(cfg, base + ".weekend-color")) {
            viewConfig.logView.annualCalendar.weekendColor = parseColor(*val);
        }
        if (auto val = detail::getStr(cfg, base + ".weekend-style")) {
            viewConfig.logView.annualCalendar.weekendStyle = parseStyle(*val);
        }

        if (auto val = detail::getStr(cfg, base + ".today-color")) {
            viewConfig.logView.annualCalendar.todayColor = parseColor(*val);
        }
        if (auto val = detail::getStr(cfg, base + ".today-style")) {
            viewConfig.logView.annualCalendar.todayStyle = parseStyle(*val);
        }

        if (auto val = detail::getStr(cfg, base + ".selected-day-color")) {
            viewConfig.logView.annualCalendar.selectedDayColor = parseColor(*val);
        }
        if (auto val = detail::getStr(cfg, base + ".selected-day-style")) {
            viewConfig.logView.annualCalendar.selectedDayStyle = parseStyle(*val);
        }

        if (auto val = detail::getStr(cfg, base + ".event-day-color")) {
            viewConfig.logView.annualCalendar.eventDayColor = parseColor(*val);
        }
    }

    // UI - Scratchpads:
    // scratchpad-list
    {
        auto base = std::string("ui.scratchpads-view.scratchpad-list");
        loadBorder(cfg, base, viewConfig.scratchpadView.scratchpadList);
        loadColorStyleSelectable(cfg, base, viewConfig.scratchpadView.scratchpadList,
                                 viewConfig.scratchpadView.scratchpadList,
                                 &viewConfig.scratchpadView.scratchpadList);
    }
    // scratchpad-preview
    {
        auto base = std::string("ui.scratchpads-view.scratchpad-preview");
        loadBorder(cfg, base, viewConfig.scratchpadView.scratchpadPreview);
        viewConfig.scratchpadView.scratchpadPreview.markdown_syntax_highlighting = detail::getBool(
            cfg, base + ".markdown-syntax-highlighting",
            viewConfig.scratchpadView.scratchpadPreview.markdown_syntax_highlighting);
    }

    // Menu (kept as default; no INI keys defined yet)

    return viewConfig;
}

class View : public ViewBase, public InputHandlerBase {
    friend class PopUpViewLayoutWrapper;
    ftxui::ScreenInteractive m_screen = ftxui::ScreenInteractive::Fullscreen();
    std::shared_ptr<AnnualViewLayoutBase> m_annualViewLayout;
    std::shared_ptr<ScratchpadViewLayoutBase> m_scratchpadViewLayout;
    std::shared_ptr<class PopUpViewLayoutWrapper> m_rootWithPopUpSupport;
    InputHandlerBase *m_inputHandler = nullptr;
    std::function<ftxui::Dimensions()> m_terminalSizeProvider;
    bool m_running = false;

  public:
    View(const ViewConfig &conf, std::chrono::year_month_day today,
         std::function<ftxui::Dimensions()> terminalSizeProvider);
    View(const View &) = delete;
    View(View &&) = delete;
    View &operator=(const View &) = delete;
    View &operator=(View &&) = delete;
    ~View() override = default;

    void run() override;
    void stop() override;

    void post(const ftxui::Task &task) override;

    void withRestoredIO(std::function<void()> func) override;
    void setInputHandler(InputHandlerBase *handler) override;

    PopUpViewBase &getPopUpView() override;

    std::shared_ptr<AnnualViewLayoutBase> getAnnualViewLayout() override;
    std::shared_ptr<ScratchpadViewLayoutBase> getScratchpadViewLayout() override;

    bool handleInputEvent(const UIEvent &event) override;

    void switchLayout() override;

    // testing tools

    bool onEvent(ftxui::Event event);

    [[nodiscard]] std::string render() const;
};

} // namespace caps_log::view
