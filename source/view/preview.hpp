#pragma once

#include <ftxui/component/component.hpp>

namespace caps_log::view {

struct PreviewOption {
    ftxui::BorderStyle border;
};

class Preview : public ftxui::ComponentBase {
    ftxui::Decorator m_borderDecorator;
    int m_topLineIndex = 0;
    ftxui::Elements m_lines;
    ftxui::Element m_title = ftxui::text("log preview");

  public:
    explicit Preview(const PreviewOption &option);

    ftxui::Element OnRender() override;
    [[nodiscard]] bool Focusable() const override;
    bool OnEvent(ftxui::Event event) override;

    void resetScroll();
    void setContent(const std::string &title, const std::string &str);
};

} // namespace caps_log::view
