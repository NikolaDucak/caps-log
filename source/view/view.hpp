
#pragma once

#include "view/annual_view_layout.hpp"
#include "view/view_base.hpp"

namespace caps_log::view {

struct ViewConfig {
    AnnualViewConfig annualViewConfig;
};

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
