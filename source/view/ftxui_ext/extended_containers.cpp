#include "extended_containers.hpp"

#include "ftxui/component/component.hpp"
#include "ftxui/component/event.hpp"
#include "ftxui/component/mouse.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/screen/box.hpp"

#include <memory>

namespace caps_log::view::ftxui_ext {
using namespace ftxui;

class ContainerBase : public ComponentBase {
  public:
    ContainerBase(Components children, int *selector)
        : selector_(selector != 0 ? selector : &selected_) {
        for (auto &child : children) {
            Add(std::move(child));
        }
    }

    bool OnEvent(Event event) override {
        if (event.is_mouse()) {
            return onMouseEvent(event);
        }

        if (!Focused()) {
            return false;
        }

        if (ActiveChild() && ActiveChild()->OnEvent(event)) {
            return true;
        }

        return eventHandler(event);
    }

    Component ActiveChild() override {
        if (children_.empty()) {
            return nullptr;
        }

        return children_[*selector_ % children_.size()];
    }

    void SetActiveChild(ComponentBase *child) override {
        for (std::size_t i = 0; i < children_.size(); ++i) {
            if (children_[i].get() == child) {
                *selector_ = (int)i;
                return;
            }
        }
    }

  protected:
    // NOLINTNEXTLINE(performance-unnecessary-value-param)
    virtual bool eventHandler(Event /*unused*/) { return false; }

    virtual bool onMouseEvent(Event event) { return ComponentBase::OnEvent(std::move(event)); }

    void moveSelector(int dir) {
        for (int i = *selector_ + dir; i >= 0 && i < (int)children_.size(); i += dir) {
            if (children_[i]->Focusable()) {
                *selector_ = i;
                return;
            }
        }
    }

    void moveSelectorWrap(int dir) {
        if (children_.empty()) {
            return;
        }
        for (size_t offset = 1; offset < children_.size(); ++offset) {
            size_t index =
                ((size_t(*selector_ + offset * dir + children_.size())) % children_.size());
            if (children_[index]->Focusable()) {
                *selector_ = (int)index;
                return;
            }
        }
    }
    int selected_ = 0;        // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    int *selector_ = nullptr; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
};

class GridContainer : public ContainerBase {
  public:
    using ContainerBase::ContainerBase;
    GridContainer(int width, Components components, int *selector)
        : ContainerBase(std::move(components), selector), m_width(width) {}

    Element Render() override {
        Elements elements;
        for (auto &child : children_) {
            elements.push_back(child->Render());
        }
        if (elements.empty()) {
            return text("Empty container") | reflect(m_box);
        }
        return vbox(std::move(elements)) | reflect(m_box);
    }

    bool eventHandler(Event event) override {
        int oldSelected = *selector_;

        if (event == Event::ArrowLeft || event == Event::Character('h')) {
            moveSelector(-1);
        } else if (event == Event::ArrowRight || event == Event::Character('l')) {
            moveSelector(+1);
        } else if (event == Event::ArrowUp || event == Event::Character('k')) {
            moveSelector(-m_width);
        } else if (event == Event::ArrowDown || event == Event::Character('j')) {
            moveSelector(+m_width);
        } else if (event == Event::Home) {
            for (size_t i = 0; i < children_.size(); ++i) {
                moveSelector(-1);
            }
        } else if (event == Event::End) {
            for (size_t i = 0; i < children_.size(); ++i) {
                moveSelector(1);
            }
        }

        *selector_ = std::max(0, std::min(int(children_.size()) - 1, *selector_));
        return oldSelected != *selector_;
    }

    bool onMouseEvent(Event event) override {
        if (ContainerBase::onMouseEvent(event)) {
            return true;
        }

        if (event.mouse().button != Mouse::WheelUp && event.mouse().button != Mouse::WheelDown) {
            return false;
        }

        if (!m_box.Contain(event.mouse().x, event.mouse().y)) {
            return false;
        }

        if (event.mouse().button == Mouse::WheelUp) {
            moveSelector(-1);
        }
        if (event.mouse().button == Mouse::WheelDown) {
            moveSelector(+1);
        }
        *selector_ = std::max(0, std::min(int(children_.size()) - 1, *selector_));

        return true;
    }

  private:
    int m_width, m_height{};
    Box m_box;
};

class AnyDirContainer : public ContainerBase {
  public:
    using ContainerBase::ContainerBase;

    Element Render() override {
        Elements elements;
        for (auto &child : children_) {
            elements.push_back(child->Render());
        }
        if (elements.empty()) {
            return text("Empty container") | reflect(m_box);
        }
        return vbox(std::move(elements)) | reflect(m_box);
    }

    bool eventHandler(Event event) override {
        int oldSelected = *selector_;

        if (event == Event::ArrowLeft || event == Event::Character('h') ||
            (event == Event::ArrowUp || event == Event::Character('k'))) {
            moveSelector(-1);
        }
        if (event == Event::ArrowRight || event == Event::Character('l') ||
            (event == Event::ArrowDown || event == Event::Character('j'))) {
            moveSelector(+1);
        }

        if (event == Event::Home) {
            for (size_t i = 0; i < children_.size(); ++i) {
                moveSelector(-1);
            }
        }
        if (event == Event::End) {
            for (size_t i = 0; i < children_.size(); ++i) {
                moveSelector(1);
            }
        }

        *selector_ = std::max(0, std::min(int(children_.size()) - 1, *selector_));
        return oldSelected != *selector_;
    }

    bool onMouseEvent(Event event) override {
        if (ContainerBase::onMouseEvent(event)) {
            return true;
        }

        if (event.mouse().button != Mouse::WheelUp && event.mouse().button != Mouse::WheelDown) {
            return false;
        }

        if (!m_box.Contain(event.mouse().x, event.mouse().y)) {
            return false;
        }

        if (event.mouse().button == Mouse::WheelUp) {
            moveSelector(-1);
        }
        if (event.mouse().button == Mouse::WheelDown) {
            moveSelector(+1);
        }
        *selector_ = std::max(0, std::min(int(children_.size()) - 1, *selector_));

        return true;
    }

  private:
    int m_width, m_height;
    Box m_box;
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
    CustomInputContainer(Components children, int *selector, Event sswitch, Event switchBack)
        : ContainerBase(std::move(children), selector), m_switch(std::move(sswitch)),
          m_switchBack(std::move(switchBack)) {}

    Element Render() override {
        Elements elements;
        for (auto &child : children_) {
            elements.push_back(child->Render());
        }
        if (elements.empty()) {
            return text("Empty container") | reflect(m_box);
        }
        return hbox(std::move(elements)) | reflect(m_box);
    }

    bool eventHandler(Event event) override {
        int oldSelected = *selector_;
        if (event == m_switch) {
            moveSelectorWrap(+1);
        }
        if (event == m_switchBack) {
            moveSelectorWrap(-1);
        }
        *selector_ = std::max(0, std::min(int(children_.size()) - 1, *selector_));
        return oldSelected != *selector_;
    }

    bool onMouseEvent(Event event) override {
        if (ContainerBase::onMouseEvent(event)) {
            return true;
        }

        if (event.mouse().button != Mouse::WheelUp && event.mouse().button != Mouse::WheelDown) {
            return false;
        }

        if (!m_box.Contain(event.mouse().x, event.mouse().y)) {
            return false;
        }

        if (event.mouse().button == Mouse::WheelUp) {
            moveSelector(-1);
        }
        if (event.mouse().button == Mouse::WheelDown) {
            moveSelector(+1);
        }
        *selector_ = std::max(0, std::min(int(children_.size()) - 1, *selector_));

        return true;
    }

    // Component override.
    bool OnEvent(Event event) override {
        if (!Focused()) {
            return false;
        }

        if (event == m_switch || event == m_switchBack) {
            return eventHandler(event);
        }

        if (ActiveChild() && ActiveChild()->OnEvent(event)) {
            return true;
        }

        return false;
    }

    bool Focusable() const override { return true; }

  private:
    int m_width{}, m_height{};
    Box m_box;
    Event m_switch, m_switchBack;
};

// clang-tidy keeps complaining about value arguments even tho they are std::moved.
// NOLINTNEXTLINE (performance-unnecessary-value-param)
Component CustomContainer(Components children, Event next, Event prev) {
    return std::make_shared<CustomInputContainer>(std::move(children), nullptr, std::move(next),
                                                  std::move(prev));
}

} // namespace caps_log::view::ftxui_ext
