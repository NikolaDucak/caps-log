#include "extended_containers.hpp"

#include "ftxui/component/component.hpp"
#include "ftxui/component/event.hpp"
#include "ftxui/component/mouse.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/screen/box.hpp"

#include <memory>

namespace clog::view::ftxui_ext {
using namespace ftxui;

class ContainerBase : public ComponentBase {
  public:
    ContainerBase(Components children, int *selector)
        : selector_(selector != 0 ? selector : &selected_) {
        for (Component &child : children) {
            Add(std::move(child));
        }
    }

    // Component override.
    bool OnEvent(Event event) override {
        if (event.is_mouse()) {
            return OnMouseEvent(event);
        }

        if (!Focused()) {
            return false;
        }

        if (ActiveChild() && ActiveChild()->OnEvent(event)) {
            return true;
        }

        return EventHandler(event);
    }

    Component ActiveChild() override {
        if (children_.empty()) {
            return nullptr;
        }

        return children_[*selector_ % children_.size()];
    }

    void SetActiveChild(ComponentBase *child) override {
        for (size_t i = 0; i < children_.size(); ++i) {
            if (children_[i].get() == child) {
                *selector_ = (int)i;
                return;
            }
        }
    }

  protected:
    // Handlers
    virtual bool EventHandler(Event /*unused*/) { return false; } // NOLINT

    virtual bool OnMouseEvent(Event event) { return ComponentBase::OnEvent(std::move(event)); }

    int selected_ = 0;
    int *selector_ = nullptr;

    void MoveSelector(int dir) {
        for (int i = *selector_ + dir; i >= 0 && i < (int)children_.size(); i += dir) {
            if (children_[i]->Focusable()) {
                *selector_ = i;
                return;
            }
        }
    }

    void MoveSelectorWrap(int dir) {
        if (children_.empty()) {
            return;
        }
        for (size_t offset = 1; offset < children_.size(); ++offset) {
            size_t i = ((size_t(*selector_ + offset * dir + children_.size())) % children_.size());
            if (children_[i]->Focusable()) {
                *selector_ = (int)i;
                return;
            }
        }
    }
};

class GridContainer : public ContainerBase {
  public:
    using ContainerBase::ContainerBase;
    GridContainer(int width, Components c, int *selector)
        : ContainerBase(std::move(c), selector), width_(width) {}

    Element Render() override {
        Elements elements;
        for (auto &it : children_) {
            elements.push_back(it->Render());
        }
        if (elements.empty()) {
            return text("Empty container") | reflect(box_);
        }
        return vbox(std::move(elements)) | reflect(box_);
    }

    bool EventHandler(Event event) override {
        int old_selected = *selector_;

        if (event == Event::ArrowLeft || event == Event::Character('h')) {
            MoveSelector(-1);
        } else if (event == Event::ArrowRight || event == Event::Character('l')) {
            MoveSelector(+1);
        } else if (event == Event::ArrowUp || event == Event::Character('k')) {
            MoveSelector(-width_);
        } else if (event == Event::ArrowDown || event == Event::Character('j')) {
            MoveSelector(+width_);
        } else if (event == Event::Home) {
            for (size_t i = 0; i < children_.size(); ++i) {
                MoveSelector(-1);
            }
        } else if (event == Event::End) {
            for (size_t i = 0; i < children_.size(); ++i) {
                MoveSelector(1);
            }
        }

        *selector_ = std::max(0, std::min(int(children_.size()) - 1, *selector_));
        return old_selected != *selector_;
    }

    bool OnMouseEvent(Event event) override {
        if (ContainerBase::OnMouseEvent(event)) {
            return true;
        }

        if (event.mouse().button != Mouse::WheelUp && event.mouse().button != Mouse::WheelDown) {
            return false;
        }

        if (!box_.Contain(event.mouse().x, event.mouse().y)) {
            return false;
        }

        if (event.mouse().button == Mouse::WheelUp)
            MoveSelector(-1);
        if (event.mouse().button == Mouse::WheelDown)
            MoveSelector(+1);
        *selector_ = std::max(0, std::min(int(children_.size()) - 1, *selector_));

        return true;
    }

    int width_, height_;
    Box box_;
};

// just a vertical container that takes all 4 directions of input
// its supposed to hold
class AnyDirContainer : public ContainerBase {
  public:
    using ContainerBase::ContainerBase;

    Element Render() override {
        Elements elements;
        for (auto &it : children_)
            elements.push_back(it->Render());
        if (elements.size() == 0)
            return text("Empty container") | reflect(box_);
        return vbox(std::move(elements)) | reflect(box_);
    }

    bool EventHandler(Event event) override {
        int old_selected = *selector_;

        if (event == Event::ArrowLeft || event == Event::Character('h') ||
            (event == Event::ArrowUp || event == Event::Character('k')))
            MoveSelector(-1);
        if (event == Event::ArrowRight || event == Event::Character('l') ||
            (event == Event::ArrowUp || event == Event::Character('j')))
            MoveSelector(+1);

        if (event == Event::Home) {
            for (size_t i = 0; i < children_.size(); ++i)
                MoveSelector(-1);
        }
        if (event == Event::End) {
            for (size_t i = 0; i < children_.size(); ++i)
                MoveSelector(1);
        }

        *selector_ = std::max(0, std::min(int(children_.size()) - 1, *selector_));
        return old_selected != *selector_;
    }

    bool OnMouseEvent(Event event) override {
        if (ContainerBase::OnMouseEvent(event))
            return true;

        if (event.mouse().button != Mouse::WheelUp && event.mouse().button != Mouse::WheelDown) {
            return false;
        }

        if (!box_.Contain(event.mouse().x, event.mouse().y))
            return false;

        if (event.mouse().button == Mouse::WheelUp)
            MoveSelector(-1);
        if (event.mouse().button == Mouse::WheelDown)
            MoveSelector(+1);
        *selector_ = std::max(0, std::min(int(children_.size()) - 1, *selector_));

        return true;
    }

    int width_, height_;
    Box box_;
};

Component Grid(int width, Components children, int *selected) {
    return std::make_shared<GridContainer>(width, std::move(children), selected);
}

Component AnyDir(Components children, int *selected) {
    return std::make_shared<AnyDirContainer>(std::move(children), selected);
}

class CustomInputContainer : public ContainerBase {
  public:
    using ContainerBase::ContainerBase;
    CustomInputContainer(Components children, int *selector, Event s, Event sb)
        : ContainerBase(std::move(children), selector), switch_(std::move(s)),
          switchBack_(std::move(sb)) {}

    Element Render() override {
        Elements elements;
        for (auto &it : children_)
            elements.push_back(it->Render());
        if (elements.size() == 0)
            return text("Empty container") | reflect(box_);
        return hbox(std::move(elements)) | reflect(box_);
    }

    bool EventHandler(Event event) override {
        int old_selected = *selector_;
        if (event == switch_)
            MoveSelectorWrap(+1);
        if (event == switchBack_)
            MoveSelectorWrap(-1);
        *selector_ = std::max(0, std::min(int(children_.size()) - 1, *selector_));
        return old_selected != *selector_;
    }

    bool OnMouseEvent(Event event) override {
        if (ContainerBase::OnMouseEvent(event))
            return true;

        if (event.mouse().button != Mouse::WheelUp && event.mouse().button != Mouse::WheelDown) {
            return false;
        }

        if (!box_.Contain(event.mouse().x, event.mouse().y))
            return false;

        if (event.mouse().button == Mouse::WheelUp)
            MoveSelector(-1);
        if (event.mouse().button == Mouse::WheelDown)
            MoveSelector(+1);
        *selector_ = std::max(0, std::min(int(children_.size()) - 1, *selector_));

        return true;
    }

    // Component override.
    bool OnEvent(Event event) override {
        if (!Focused())
            return false;

        if (event == switch_ || event == switchBack_)
            return EventHandler(event);

        if (ActiveChild() && ActiveChild()->OnEvent(event))
            return true;

        return false;
    }

    int width_, height_;
    Box box_;
    Event switch_, switchBack_;
};

Component CustomContainer(Components children, Event next, Event prev) {
    return std::make_shared<CustomInputContainer>(children, nullptr, next, prev);
}

} // namespace clog::view::ftxui_ext
