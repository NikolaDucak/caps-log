#pragma once

#include "view/annual_view_layout_base.hpp"
#include "view/input_handler.hpp"
#include "view/scratchpad_view_layout_base.hpp"
#include <ftxui/component/component_base.hpp>
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
} // namespace caps_log::view
