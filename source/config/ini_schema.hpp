#pragma once

#include "utils/string.hpp"
#include <array>
#include <boost/property_tree/ptree.hpp>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <regex>
#include <set>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace caps_log::config {
enum class Type : std::uint8_t { String, Path, Boolean, Date, Integer, Color, Style };
enum class Required : std::uint8_t { Yes, No };

struct IniProperty {
    std::string name;
    Type type;
    Required required;

    IniProperty(std::string n, Type typ, Required req)
        : name(std::move(n)), type(typ), required(req) {}
};

struct IniSection {
    std::string name;
    Required required{Required::No};

    // Children: both properties and nested sections
    std::vector<IniProperty> properties;
    std::vector<IniSection> sections;

    IniSection() = default;
    IniSection(std::string aName, Required req) : name(std::move(aName)), required(req) {}

    // Fluent adders
    IniSection &add(IniProperty property) {
        properties.push_back(std::move(property));
        return *this;
    }

    IniSection &add(IniSection section) {
        sections.push_back(std::move(section));
        return *this;
    }

    // Convenience for the DSL style used below
    IniSection &addProperty(std::string n, Type type, Required required) {
        return add(IniProperty{std::move(n), type, required});
    }

    IniSection &addSection(IniSection section) { return add(std::move(section)); }
};

using SchemaElement = std::variant<IniProperty, IniSection>;

struct IniAnyNameSection : IniSection {
    // By convention, name="*" to indicate wildcard keys beneath this node.
    IniAnyNameSection() : IniSection{"*", Required::No} {}
};

struct IniArraySection : IniSection {
    // By convention, name="[]" to indicate "array-like" entries.
    IniArraySection() : IniSection{"[]", Required::No} {}
};

class ValidationVisitor {
  public:
    explicit ValidationVisitor(const boost::property_tree::ptree &cfg) : m_ptree(cfg) {}

    // Validate an entire schema tree starting from a Section root.
    bool validate(const IniSection &root) {
        m_errors.clear();
        validateSection(root, &m_ptree, /*path=*/"");
        return m_errors.empty();
    }

    [[nodiscard]] const std::vector<std::string> &errors() const { return m_errors; }

    void operator()(const IniSection &section) { validateSection(section, &m_ptree, ""); }
    void operator()(const IniProperty &property) { validateProperty(property, &m_ptree, ""); }

  private:
    using ptree = boost::property_tree::ptree;
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
    const ptree &m_ptree;
    std::vector<std::string> m_errors;

    // ---------- Small string helpers
    static std::string joinPath(const std::string &base, const std::string &name) {
        if (base.empty()) {
            return name;
        }
        if (name.empty()) {
            return base;
        }
        return base + "." + name;
    }

    // ---------- Validators for specific types
    static bool isBool(const std::string &raw) {
        const std::string val = utils::lowercase(utils::trim(raw));
        return (val == "true" || val == "false" || val == "yes" || val == "no" || val == "on" ||
                val == "off" || val == "1" || val == "0");
    }

    static bool isInteger(const std::string &raw) {
        int tmp = 0;
        return utils::parseInt(utils::trim(raw), tmp);
    }

    static bool isDateMMDD(const std::string &raw) {
        // Accept MM-DD, months 01..12; Feb <= 29; Apr/Jun/Sep/Nov <= 30
        static const std::regex kReg(R"(^(0[1-9]|1[0-2])\-([0-2][0-9]|3[01])$)");
        std::smatch match;
        if (!std::regex_match(raw, match, kReg)) {
            return false;
        }
        int month = std::stoi(match[1].str());
        int day = std::stoi(match[2].str());
        static const std::array<int, 13> kDaysInMonth = {0,  31, 29, 31, 30, 31, 30,
                                                         31, 31, 30, 31, 30, 31}; // allow Feb 29

        static constexpr int kFebruary = 2;
        static constexpr int kMaxFebruaryDays = 29;
        return day >= 1 && day <= kDaysInMonth.at(month) &&
               (month != kFebruary || day <= kMaxFebruaryDays);
    }

    static constexpr int kMaxColor256Value = 255;

    static bool isColor(const std::string &raw) {
        const std::string val = utils::trim(utils::lowercase(raw));
        // hex(#rgb|#rrggbb)
        {
            static const std::regex kReHex(R"(^hex\(\s*\#([0-9a-f]{3}|[0-9a-f]{6})\s*\)$)");
            if (std::regex_match(val, kReHex)) {
                return true;
            }
        }
        // rgb(r,g,b) with 0..255
        {
            static const std::regex kReRgb(
                R"(^rgb\(\s*(\d{1,3})\s*,\s*(\d{1,3})\s*,\s*(\d{1,3})\s*\)$)");
            std::smatch match;
            if (std::regex_match(val, match, kReRgb)) {
                int red = std::stoi(match[1].str());
                int green = std::stoi(match[2].str());
                int blue = std::stoi(match[3].str());
                return (red | green | blue) >= 0 && red <= kMaxColor256Value &&
                       green <= kMaxColor256Value && blue <= kMaxColor256Value;
            }
        }
        // 256(n) or ansi256(n) with 0..255
        {
            static const std::regex kRe256(R"(^(?:256|ansi256)\(\s*(\d{1,3})\s*\)$)");
            std::smatch match;
            if (std::regex_match(val, match, kRe256)) {
                int val = std::stoi(match[1].str());
                return val >= 0 && val <= kMaxColor256Value;
            }
        }
        return false;
    }

    static bool isStyle(const std::string &raw) {
        static const std::set<std::string> kAllowed = {"bold", "underline", "italic"};
        std::string str = utils::lowercase(raw);
        // split by ','
        size_t start = 0;
        bool any = false;
        while (true) {
            size_t comma = str.find(',', start);
            std::string token = utils::trim(
                str.substr(start, (comma == std::string::npos ? str.size() : comma) - start));
            if (!token.empty()) {
                any = true;
                if (!kAllowed.contains(token)) {
                    return false;
                }
            }
            if (comma == std::string::npos) {
                break;
            }
            start = comma + 1;
        }
        return any; // must have at least one style
    }

    // ---------- Core validation walkers
    void validateSection(const IniSection &section, const ptree *node, const std::string &path) {
        if (node == nullptr) {
            if (section.required == Required::Yes) {
                addError(path, "required section '" + section.name + "' is missing");
            }
            return;
        }

        // Resolve which subtree(s) to validate under this Section
        if (section.name.empty()) {
            // Root pseudo-section: validate here
            validateHere(section, node, path);
            return;
        }

        if (section.name == "*") {
            // Wildcard: validate under all children
            if (node->empty()) {
                if (section.required == Required::Yes) {
                    addError(path, "required wildcard section has no children");
                }
                return;
            }
            for (const auto &keyval : *node) {
                const auto &key = keyval.first;
                const auto &child = keyval.second;
                const std::string subPath = joinPath(path, key);
                validateHere(section, &child, subPath);
            }
            return;
        }

        if (section.name == "[]") {
            // Array-like: same as wildcard, but keep the [] in the path for clarity
            if (node->empty()) {
                if (section.required == Required::Yes) {
                    addError(path, "required array section '[]' has no children");
                }
                return;
            }
            int idx = 0;
            for (const auto &keyval : *node) {
                const auto &child = keyval.second;
                const std::string subPath = joinPath(path, "[" + std::to_string(idx++) + "]");
                validateHere(section, &child, subPath);
            }
            return;
        }

        // Normal named subsection
        {
            auto opt = node->get_child_optional(section.name);
            if (!opt) {
                if (section.required == Required::Yes) {
                    addError(joinPath(path, section.name), "required section missing");
                }
                return;
            }
            const ptree &child = *opt;
            const std::string subPath = joinPath(path, section.name);
            validateHere(section, &child, subPath);
        }
    }

    void validateHere(const IniSection &sect, const ptree *node, const std::string &path) {
        // Validate properties at this node
        for (const auto &prop : sect.properties) {
            validateProperty(prop, node, path);
        }
        // Recurse into subsections
        for (const auto &sub : sect.sections) {
            validateSection(sub, node, path);
        }
    }

    void validateProperty(const IniProperty &prop, const ptree *node, const std::string &path) {
        if (node == nullptr) {
            if (prop.required == Required::Yes) {
                addError(joinPath(path, prop.name), "required property missing");
            }
            return;
        }

        auto opt = node->get_optional<std::string>(prop.name);
        if (!opt) {
            if (prop.required == Required::Yes) {
                addError(joinPath(path, prop.name), "required property missing");
            }
            return;
        }

        const std::string &value = *opt;
        const std::string full = joinPath(path, prop.name);

        switch (prop.type) {
        case Type::String:
        case Type::Path:
            if (utils::trim(value).empty() && prop.required == Required::Yes) {
                addTypedError(full, "non-empty string/path expected", value);
            }
            break;

        case Type::Boolean:
            if (!isBool(value)) {
                addTypedError(full, "boolean expected (true/false/yes/no/on/off/1/0)", value);
            }
            break;

        case Type::Integer:
            if (!isInteger(value)) {
                addTypedError(full, "integer (base-10) expected", value);
            }
            break;

        case Type::Date:
            if (!isDateMMDD(value)) {
                addTypedError(full, "date expected in MM-DD (Feb ≤ 29; Apr/Jun/Sep/Nov ≤ 30)",
                              value);
            }
            break;

        case Type::Color:
            if (!isColor(value)) {
                addTypedError(full,
                              "color expected: hex(#rgb|#rrggbb) | rgb(r,g,b) | 256(n)/ansi256(n)",
                              value);
            }
            break;

        case Type::Style:
            if (!isStyle(value)) {
                addTypedError(full, "style expected: bold, underline, italic (comma-separated)",
                              value);
            }
            break;
        }
    }

    void addError(const std::string &src, const std::string &msg) {
        if (src.empty()) {
            m_errors.push_back(msg);
        } else {
            m_errors.push_back("[" + src + "] " + msg);
        }
    }
    void addTypedError(const std::string &src, const char *expected, const std::string &got) {
        addError(src, std::string(expected) + ", got: '" + got + "'");
    }
};

class IniSchema {
  public:
    IniSchema() : m_root{"", Required::No} {}

    // Add directly to root via variant (optional convenience)
    void add(SchemaElement element) {
        std::visit(
            [this](auto &&elelm) {
                using T = std::decay_t<decltype(elelm)>;
                if constexpr (std::is_same_v<T, IniProperty>) {
                    m_root.add(std::move(elelm));
                } else if constexpr (std::is_same_v<T, IniSection>) {
                    m_root.add(std::move(elelm));
                }
            },
            std::move(element));
    }

    // Access to the root for fluent building
    IniSection &getRoot() { return m_root; }
    [[nodiscard]] const IniSection &getRoot() const { return m_root; }

    // Validate a ptree against the schema
    bool validate(const boost::property_tree::ptree &cfg, std::vector<std::string> &outErrors) {
        ValidationVisitor visitor(cfg);
        const bool isOk = visitor.validate(m_root);
        outErrors = visitor.errors();
        return isOk;
    }

  private:
    IniSection m_root;
};

// Schema creation function
inline auto getCapsLogIniSchema() {
    IniSchema schema;
    // clang-format off
    auto &root = schema.getRoot();
    root.add(IniProperty{"log-dir-path", Type::Path, Required::No})
        .add(IniProperty{"log-name-format", Type::String, Required::No})
        .add(IniProperty{"sunday-start", Type::Boolean, Required::No})
        .add(IniProperty{"first-line-section", Type::Boolean, Required::No})
        .add(IniProperty{"password", Type::String, Required::No})
        .add(IniProperty{"crypto-application-type", Type::String, Required::No})
        .add(IniProperty{"recent-events-window", Type::Integer, Required::No})
        .add(IniProperty{"config-file-path", Type::Path, Required::No});

    root.add(IniSection{"git", Required::No}
        .add(IniProperty{"enable-git-log-repo", Type::Boolean, Required::Yes})
        .add(IniProperty{"ssh-pub-key-path", Type::String, Required::Yes})
        .add(IniProperty{"ssh-key-path", Type::Path, Required::Yes})
        .add(IniProperty{"main-branch-name", Type::String, Required::Yes})
        .add(IniProperty{"remote-name", Type::String, Required::Yes})
        .add(IniProperty{"repo-root", Type::Path, Required::Yes})
    );

    // Calendar events: wildcard -> array -> object with required fields
    root.addSection(IniSection{"calendar-events", Required::No}
        .addSection(IniAnyNameSection{}
            .addSection(IniArraySection{}
                .addProperty("name", Type::String, Required::Yes)
                .addProperty("date", Type::Date, Required::Yes)
            )
        )
    );

    // UI section
    root.addSection(IniSection{"ui", Required::No}
        .addSection(IniSection{"logs-view", Required::No}
            .addSection(IniSection{"tags-menu", Required::No}
                .addProperty("border", Type::Boolean, Required::No)
                .addProperty("color", Type::Color, Required::No)
                .addProperty("style", Type::Style, Required::No)
                .addProperty("selected-color", Type::Color, Required::No)
                .addProperty("selected-style", Type::Style, Required::No)
            )
            .addSection(IniSection{"sections-menu", Required::No}
                .addProperty("border", Type::Boolean, Required::No)
                .addProperty("color", Type::Color, Required::No)
                .addProperty("style", Type::Style, Required::No)
                .addProperty("selected-color", Type::Color, Required::No)
                .addProperty("selected-style", Type::Style, Required::No)
            )
            .addSection(IniSection{"events-list", Required::No}
                .addProperty("border", Type::Boolean, Required::No)
                .addProperty("color", Type::Color, Required::No)
                .addProperty("style", Type::Style, Required::No)
                .addProperty("selected-color", Type::Color, Required::No)
                .addProperty("selected-style", Type::Style, Required::No)
            )
            .addSection(IniSection{"log-entry-preview", Required::No}
                .addProperty("border", Type::Boolean, Required::No)
                .addProperty("markdown-syntax-highlighting", Type::Boolean, Required::No)
            )
            .addSection(IniSection{"annual-calendar", Required::No}
                .add(IniProperty{"sunday-start", Type::Boolean, Required::No})
                .addProperty("recent-events-window", Type::Integer, Required::No)
                .addProperty("border", Type::Boolean, Required::No)
                .addProperty("month-border", Type::Boolean, Required::No)
                .addProperty("weekday-color", Type::Color, Required::No)
                .addProperty("weekday-style", Type::Style, Required::No)
                .addProperty("weekend-color", Type::Color, Required::No)
                .addProperty("weekend-style", Type::Style, Required::No)
                .addProperty("today-color", Type::Color, Required::No)
                .addProperty("today-style", Type::Style, Required::No)
                .addProperty("selected-day-color", Type::Color, Required::No)
                .addProperty("selected-day-style", Type::Style, Required::No)
                .addProperty("event-day-color", Type::Color, Required::No)
            )
        )
        .addSection(IniSection{"scratchpads-view", Required::No}
            .addSection(IniSection{"scratchpad-list", Required::No}
                .addProperty("border", Type::Boolean, Required::No)
                .addProperty("color", Type::Color, Required::No)
                .addProperty("style", Type::Style, Required::No)
                .addProperty("selected-color", Type::Color, Required::No)
                .addProperty("selected-style", Type::Style, Required::No)
            )
            .addSection(IniSection{"scratchpad-preview", Required::No}
                .addProperty("border", Type::Boolean, Required::No)
                .addProperty("markdown-syntax-highlighting", Type::Boolean, Required::No)
            )
        )
    );
    // clang-format on
    return schema;
}

} // namespace caps_log::config
