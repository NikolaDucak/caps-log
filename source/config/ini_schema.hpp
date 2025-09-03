#pragma once

#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <memory>
#include <regex>
#include <string>
#include <vector>

namespace caps_log::config {

using pt = boost::property_tree;

enum class Type : std::uint8_t { String, Path, Boolean, Date, Integer, Color, Style };
enum class Required : std::uint8_t { Yes, No };

// Forward declarations
class IniSchema;
class IniSchemaSection;
class IniArraySection;
class IniAnyNameSection;

// Validation result
struct ValidationResult {
    bool isValid = true;
    std::vector<std::string> errors;
};

// Type validators
class TypeValidators {
  public:
    static bool validateString(const std::string &value) {
        return true; // Any string is valid
    }

    static bool validatePath(const std::string &value) {
        return !value.empty(); // Basic path validation
    }

    static bool validateBoolean(const std::string &value) {
        return value == "true" || value == "false";
    }

    static bool validateDate(const std::string &value) {
        // Date format: DD.MM. or DD.MM.YYYY
        std::regex datePattern(R"(^\d{2}\.\d{2}\.(\d{4})?$)");
        return std::regex_match(value, datePattern);
    }

    static bool validateInteger(const std::string &value) {
        try {
            std::stoi(value);
            return true;
        } catch (...) {
            return false;
        }
    }

    static bool validateColor(const std::string &value) {
        // Named colors
        static const std::vector<std::string> namedColors = {
            "black",          "red",         "green",        "yellow",        "blue",
            "magenta",        "cyan",        "white",        "gray",          "grey",
            "bright_black",   "bright_red",  "bright_green", "bright_yellow", "bright_blue",
            "bright_magenta", "bright_cyan", "bright_white"};

        for (const auto &namedColor : namedColors) {
            if (value == namedColor)
                return true;
        }

        // rgb(r,g,b,a) format
        std::regex rgbPattern(
            R"(^rgb\(\s*(\d{1,3})\s*,\s*(\d{1,3})\s*,\s*(\d{1,3})\s*,\s*(\d{1,3})\s*\)$)");
        std::smatch matches;

        if (std::regex_match(value, matches, rgbPattern)) {
            for (int i = 1; i <= 4; ++i) {
                int val = std::stoi(matches[i].str());
                if (val < 0 || val > 255)
                    return false;
            }
            return true;
        }

        return false;
    }

    static bool validateStyle(const std::string &value) {
        static const std::vector<std::string> validStyles = {"bold",  "regular", "italic",
                                                             "blink", "inverse", "underline"};
        return std::find(validStyles.begin(), validStyles.end(), value) != validStyles.end();
    }

    static bool validateType(Type type, const std::string &value) {
        switch (type) {
        case Type::String:
            return validateString(value);
        case Type::Path:
            return validatePath(value);
        case Type::Boolean:
            return validateBoolean(value);
        case Type::Date:
            return validateDate(value);
        case Type::Integer:
            return validateInteger(value);
        case Type::Color:
            return validateColor(value);
        case Type::Style:
            return validateStyle(value);
        default:
            return false;
        }
    }

    static std::string getTypeName(Type type) {
        switch (type) {
        case Type::String:
            return "String";
        case Type::Path:
            return "Path";
        case Type::Boolean:
            return "Boolean";
        case Type::Date:
            return "Date";
        case Type::Integer:
            return "Integer";
        case Type::Color:
            return "Color";
        case Type::Style:
            return "Style";
        default:
            return "Unknown";
        }
    }
};

// Base schema element
class IniSchemaElement {
  public:
    virtual ~IniSchemaElement() = default;
    virtual bool validate(const pt::ptree &tree, const std::string &path,
                          ValidationResult &result) const = 0;
    virtual std::ostream &print(std::ostream &os, int depth = 0) const = 0;
};

// Property definition
struct PropertyDef {
    std::string name;
    Type type;
    Required required;

    PropertyDef(const std::string &n, Type t, Required r) : name(n), type(t), required(r) {}
};

// Array section for numbered items like birthadays.0, birthadays.1, etc.
class IniArraySection : public IniSchemaElement {
  private:
    std::vector<PropertyDef> properties_;
    std::vector<std::unique_ptr<IniSchemaElement>> sections_;

  public:
    IniArraySection &addProperty(const std::string &name, Type type, Required required) {
        properties_.emplace_back(name, type, required);
        return *this;
    }

    IniArraySection &addSection(std::unique_ptr<IniSchemaElement> section) {
        sections_.push_back(std::move(section));
        return *this;
    }

    bool validate(const pt::ptree &tree, const std::string &path,
                  ValidationResult &result) const override {
        bool foundAny = false;

        // Look for numbered items (0, 1, 2, etc.)
        for (int i = 0; i < 1000; ++i) {
            std::string itemPath = path + "." + std::to_string(i);
            try {
                tree.get_child(itemPath);
                foundAny = true;

                // Validate properties of this array item
                for (const auto &prop : properties_) {
                    std::string propPath = itemPath + "." + prop.name;
                    try {
                        auto value = tree.get<std::string>(propPath);
                        if (!TypeValidators::validateType(prop.type, value)) {
                            result.errors.push_back("Property '" + propPath + "' has invalid " +
                                                    TypeValidators::getTypeName(prop.type) +
                                                    " value: " + value);
                            result.isValid = false;
                        }
                    } catch (const pt::ptree_bad_path &) {
                        if (prop.required == Required::Yes) {
                            result.errors.push_back("Required property '" + propPath +
                                                    "' is missing");
                            result.isValid = false;
                        }
                    }
                }

                // Validate child sections
                for (const auto &section : sections_) {
                    section->validate(tree, itemPath, result);
                }

            } catch (const pt::ptree_bad_path &) {
                break; // No more items
            }
        }

        return result.isValid;
    }

    std::ostream &print(std::ostream &os, int depth = 0) const override {
        std::string indent(depth * 2, ' ');
        os << indent << "ArraySection:" << std::endl;
        for (const auto &prop : properties_) {
            os << indent << "  - " << prop.name << " (" << TypeValidators::getTypeName(prop.type)
               << ", " << (prop.required == Required::Yes ? "required" : "optional") << ")"
               << std::endl;
        }
        return os;
    }
};

// Any name section for dynamic section names
class IniAnyNameSection : public IniSchemaElement {
  private:
    std::vector<PropertyDef> properties_;
    std::vector<std::unique_ptr<IniSchemaElement>> sections_;

  public:
    IniAnyNameSection &addProperty(const std::string &name, Type type, Required required) {
        properties_.emplace_back(name, type, required);
        return *this;
    }

    IniAnyNameSection &addSection(std::unique_ptr<IniSchemaElement> section) {
        sections_.push_back(std::move(section));
        return *this;
    }

    template <typename T> IniAnyNameSection &addSection(T section) {
        sections_.push_back(std::make_unique<T>(std::move(section)));
        return *this;
    }

    bool validate(const pt::ptree &tree, const std::string &path,
                  ValidationResult &result) const override {
        try {
            auto section = tree.get_child(path);

            // Iterate through all child sections (any names)
            for (const auto &child : section) {
                std::string childPath = path + "." + child.first;

                // Validate properties
                for (const auto &prop : properties_) {
                    std::string propPath = childPath + "." + prop.name;
                    try {
                        auto value = tree.get<std::string>(propPath);
                        if (!TypeValidators::validateType(prop.type, value)) {
                            result.errors.push_back("Property '" + propPath + "' has invalid " +
                                                    TypeValidators::getTypeName(prop.type) +
                                                    " value: " + value);
                            result.isValid = false;
                        }
                    } catch (const pt::ptree_bad_path &) {
                        if (prop.required == Required::Yes) {
                            result.errors.push_back("Required property '" + propPath +
                                                    "' is missing");
                            result.isValid = false;
                        }
                    }
                }

                // Validate child sections
                for (const auto &childSection : sections_) {
                    childSection->validate(tree, childPath, result);
                }
            }
        } catch (const pt::ptree_bad_path &) {
            // Section doesn't exist, which might be okay
        }

        return result.isValid;
    }

    std::ostream &print(std::ostream &os, int depth = 0) const override {
        std::string indent(depth * 2, ' ');
        os << indent << "AnyNameSection:" << std::endl;
        for (const auto &prop : properties_) {
            os << indent << "  - " << prop.name << " (" << TypeValidators::getTypeName(prop.type)
               << ", " << (prop.required == Required::Yes ? "required" : "optional") << ")"
               << std::endl;
        }
        return os;
    }
};

// Named schema section
class IniSchemaSection : public IniSchemaElement {
  private:
    std::string name_;
    Required required_;
    std::vector<PropertyDef> properties_;
    std::vector<std::unique_ptr<IniSchemaElement>> sections_;

  public:
    IniSchemaSection(const std::string &name, Required required)
        : name_(name), required_(required) {}

    IniSchemaSection &addProperty(const std::string &name, Type type, Required required) {
        properties_.emplace_back(name, type, required);
        return *this;
    }

    IniSchemaSection &addSection(std::unique_ptr<IniSchemaElement> section) {
        sections_.push_back(std::move(section));
        return *this;
    }

    IniSchemaSection &addSection(IniSchemaSection section) {
        sections_.push_back(std::make_unique<IniSchemaSection>(std::move(section)));
        return *this;
    }

    IniSchemaSection &addSection(IniAnyNameSection section) {
        sections_.push_back(std::make_unique<IniAnyNameSection>(std::move(section)));
        return *this;
    }

    bool validate(const pt::ptree &tree, const std::string &path,
                  ValidationResult &result) const override {
        std::string fullPath = path.empty() ? name_ : path + "." + name_;

        try {
            tree.get_child(fullPath);

            // Validate properties
            for (const auto &prop : properties_) {
                std::string propPath = fullPath + "." + prop.name;
                try {
                    auto value = tree.get<std::string>(propPath);
                    if (!TypeValidators::validateType(prop.type, value)) {
                        result.errors.push_back("Property '" + propPath + "' has invalid " +
                                                TypeValidators::getTypeName(prop.type) +
                                                " value: " + value);
                        result.isValid = false;
                    }
                } catch (const pt::ptree_bad_path &) {
                    if (prop.required == Required::Yes) {
                        result.errors.push_back("Required property '" + propPath + "' is missing");
                        result.isValid = false;
                    }
                }
            }

            // Validate child sections
            for (const auto &section : sections_) {
                section->validate(tree, fullPath, result);
            }

        } catch (const pt::ptree_bad_path &) {
            if (required_ == Required::Yes) {
                result.errors.push_back("Required section '" + fullPath + "' is missing");
                result.isValid = false;
            }
        }

        return result.isValid;
    }

    std::ostream &print(std::ostream &os, int depth = 0) const override {
        std::string indent(depth * 2, ' ');
        os << indent << "[" << name_ << "] ("
           << (required_ == Required::Yes ? "required" : "optional") << ")" << std::endl;
        for (const auto &prop : properties_) {
            os << indent << "  - " << prop.name << " (" << TypeValidators::getTypeName(prop.type)
               << ", " << (prop.required == Required::Yes ? "required" : "optional") << ")"
               << std::endl;
        }
        for (const auto &section : sections_) {
            section->print(os, depth + 1);
        }
        return os;
    }

    const std::string &getName() const { return name_; }
};

// Root schema section (unnamed)
class IniSchemaRoot {
  private:
    std::vector<PropertyDef> properties_;
    std::vector<std::unique_ptr<IniSchemaElement>> sections_;

  public:
    IniSchemaRoot &addProperty(const std::string &name, Type type, Required required) {
        properties_.emplace_back(name, type, required);
        return *this;
    }

    IniSchemaRoot &addSection(IniSchemaSection section) {
        sections_.push_back(std::make_unique<IniSchemaSection>(std::move(section)));
        return *this;
    }

    bool validate(const pt::ptree &tree, ValidationResult &result) const {
        // Validate root properties
        for (const auto &prop : properties_) {
            try {
                auto value = tree.get<std::string>(prop.name);
                if (!TypeValidators::validateType(prop.type, value)) {
                    result.errors.push_back("Property '" + prop.name + "' has invalid " +
                                            TypeValidators::getTypeName(prop.type) +
                                            " value: " + value);
                    result.isValid = false;
                }
            } catch (const pt::ptree_bad_path &) {
                if (prop.required == Required::Yes) {
                    result.errors.push_back("Required property '" + prop.name + "' is missing");
                    result.isValid = false;
                }
            }
        }

        // Validate sections
        for (const auto &section : sections_) {
            section->validate(tree, "", result);
        }

        return result.isValid;
    }

    std::ostream &print(std::ostream &os) const {
        os << "Root properties:" << std::endl;
        for (const auto &prop : properties_) {
            os << "  - " << prop.name << " (" << TypeValidators::getTypeName(prop.type) << ", "
               << (prop.required == Required::Yes ? "required" : "optional") << ")" << std::endl;
        }
        os << "Sections:" << std::endl;
        for (const auto &section : sections_) {
            section->print(os, 1);
        }
        return os;
    }
};

// Main schema class
class IniSchema {
  private:
    IniSchemaRoot root_;

  public:
    IniSchemaRoot &getRoot() { return root_; }

    ValidationResult validate(const pt::ptree &tree) {
        ValidationResult result;
        root_.validate(tree, result);
        return result;
    }

    void print() const { root_.print(std::cout); }

    friend std::ostream &operator<<(std::ostream &os, const IniSchema &schema) {
        return schema.root_.print(os);
    }
};

// Your specific schema function
auto getCapsLogIniSchema() {
    IniSchema schema;
    // clang-format off
    auto &root = schema.getRoot();
    root.addProperty("log-dir-path", Type::Path, Required::No)
        .addProperty("log-name-format", Type::String, Required::No)
        .addProperty("sunday-start", Type::Boolean, Required::No)
        .addProperty("first-line-section", Type::Boolean, Required::No)
        .addProperty("password", Type::String, Required::No)
        .addProperty("crypto-application-type", Type::String, Required::No)
        .addProperty("recent-events-window", Type::Integer, Required::No)
        .addProperty("config-file-path", Type::Path, Required::No);
    
    root.addSection(IniSchemaSection{"git", Required::No}
        .addProperty("enable-git-log-repo", Type::Boolean, Required::Yes)
        .addProperty("ssh-pub-key-path", Type::String, Required::Yes)
        .addProperty("ssh-key-path", Type::Path, Required::Yes)
        .addProperty("main-branch-name", Type::String, Required::Yes)
        .addProperty("remote-name", Type::String, Required::Yes)
        .addProperty("repo-root", Type::Path, Required::Yes)
    );
    
    root.addSection(IniSchemaSection{"calendar-events", Required::No}
        .addSection(IniAnyNameSection{}
            .addSection(std::make_unique<IniArraySection>(
                IniArraySection{}
                    .addProperty("name", Type::String, Required::Yes)
                    .addProperty("date", Type::Date, Required::Yes)
            ))
        )
    );
    
    root.addSection(IniSchemaSection{"ui", Required::No}
        .addSection(IniSchemaSection{"logs", Required::No}
            .addSection(IniSchemaSection{"tags-list", Required::No}
                .addProperty("border", Type::Boolean, Required::No)
                .addProperty("color", Type::Color, Required::No)
                .addProperty("style", Type::Style, Required::No)
                .addProperty("selected-color", Type::Color, Required::No)
                .addProperty("selected-style", Type::Style, Required::No)
            )
            .addSection(IniSchemaSection{"sections-list", Required::No}
                .addProperty("border", Type::Boolean, Required::No)
            )
            .addSection(IniSchemaSection{"events-list", Required::No}
                .addProperty("border", Type::Boolean, Required::No)
            )
            .addSection(IniSchemaSection{"log-entry-preview", Required::No}
                .addProperty("border", Type::Boolean, Required::No)
                .addProperty("markdown-syntax-highlighting", Type::Boolean, Required::No)
            )
            .addSection(IniSchemaSection{"annual-calendar", Required::No}
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
        .addSection(IniSchemaSection{"scratchpads", Required::No}
            .addSection(IniSchemaSection{"scratchpad-list", Required::No}
                .addProperty("border", Type::Boolean, Required::No)
                .addProperty("color", Type::Color, Required::No)
                .addProperty("style", Type::Style, Required::No)
                .addProperty("selected-color", Type::Color, Required::No)
                .addProperty("selected-style", Type::Style, Required::No)
            )
            .addSection(IniSchemaSection{"scratchpad-preview", Required::No}
                .addProperty("border", Type::Boolean, Required::No)
                .addProperty("markdown-syntax-highlighting", Type::Boolean, Required::No)
            )
        )
    );
    // clang-format on
    return schema;
}

} // namespace caps_log::config
