
#pragma once

#include "view/annual_view_layout_base.hpp"
#include "view/input_handler.hpp"
#include "view/scratchpad_view_layout_base.hpp"
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/component/task.hpp>

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
    struct Help {
        std::string message;
    };
    struct None {};

    using PopUpType = std::variant<Ok, YesNo, Loading, TextBox, Help, None>;

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
    bool sundayStart;
    unsigned recentEventsWindow;
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
