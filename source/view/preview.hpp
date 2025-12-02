#pragma once

#include <ftxui/component/component.hpp>
#include <string>

namespace caps_log::view {

struct PreviewOptions {
    bool border = true;
    bool markdownSyntaxHighlighting = true;
};

class Preview : public ftxui::ComponentBase {
    int m_topLineIndex = 0;
    ftxui::Elements m_lines;
    ftxui::Element m_title = ftxui::text("log preview");
    PreviewOptions m_options;

  public:
    explicit Preview(const PreviewOptions &options = {});

    ftxui::Element OnRender() override;
    [[nodiscard]] bool Focusable() const override;
    bool OnEvent(ftxui::Event event) override;

    void resetScroll();
    void setContent(const std::string &title, const std::string &str);
};

} // namespace caps_log::view
