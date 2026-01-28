#pragma once

#include "view/input_handler.hpp"
#include "view/preview.hpp"
#include "view/scratchpad_view_layout_base.hpp"
#include "view/theme_config.hpp"
#include "view/windowed_menu.hpp"

namespace caps_log::view {

struct ScratchpadTheme {
    MenuConfig menuConfig;
    TextPreviewConfig previewConfig;
};

struct ScratchpadViewConfig {
    ScratchpadTheme theme;
};

class ScratchpadViewLayout final : public ScratchpadViewLayoutBase {
    ftxui::Component m_component;
    std::shared_ptr<WindowedMenu> m_windowedMenu;
    std::shared_ptr<Preview> m_preview;
    std::vector<std::string> m_scratchpadTitles;
    std::vector<std::string> m_scratchpadContents;
    std::vector<std::string> m_scratchpadFileNames;

    InputHandlerBase *m_inputHandler;
    std::function<ftxui::Dimensions()> m_screenSizeProvider;
    ScratchpadViewConfig m_config;

  public:
    explicit ScratchpadViewLayout(InputHandlerBase *inputHandler,
                                  std::function<ftxui::Dimensions()> screenSizeProvider,
                                  const ScratchpadViewConfig &config);

    void setScratchpads(const std::vector<ScratchpadData> &scratchpadData) override;

    ftxui::Component getComponent() override;
};

} // namespace caps_log::view
