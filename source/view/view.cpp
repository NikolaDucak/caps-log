#include "view/view.hpp"
#include "view/annual_view_layout.hpp"
#include "view/scratchpad_view_layout.hpp"
#include <ftxui/component/component.hpp>
namespace caps_log::view {
using namespace ftxui;

constexpr std::size_t kIndexAnnualViewLayout = 0;
constexpr std::size_t kIndexScratchpadViewLayout = 1;
constexpr std::size_t kIndexYesNot = 2;
constexpr std::size_t kIndexOk = 3;
constexpr std::size_t kIndexLoading = 4;
constexpr std::size_t kIndexTextBox = 5;

class PopUpViewLayoutWrapper : public PopUpViewBase, public ComponentBase {
    int m_currentScreen = kIndexAnnualViewLayout;
    int m_previousScreen = kIndexAnnualViewLayout;
    PopUpCallback m_callback = nullptr;
    std::string m_message;
    View *m_view = nullptr;
    Component m_prompt = nullptr;

    // todo: move to separate class
    std::string m_inputText;

  public:
    PopUpViewLayoutWrapper(const PopUpViewLayoutWrapper &) = delete;
    PopUpViewLayoutWrapper(PopUpViewLayoutWrapper &&) = delete;
    PopUpViewLayoutWrapper &operator=(const PopUpViewLayoutWrapper &) = delete;
    PopUpViewLayoutWrapper &operator=(PopUpViewLayoutWrapper &&) = delete;
    explicit PopUpViewLayoutWrapper(View *view) : m_view(view) {
        auto buttons = Container::Horizontal({
            Button("Yes",
                   [this] {
                       m_callback(PopUpViewBase::Result::Yes{});
                       resetToPrevious();
                   }),
            Button("No",
                   [this] {
                       m_callback(PopUpViewBase::Result::No{});
                       resetToPrevious();
                   }),
        });

        auto okButton = Container::Horizontal({
            Button("Ok",
                   [this] {
                       m_callback(PopUpViewBase::Result::Ok{});
                       resetToPrevious();
                   }),
        });
        auto txtBoxHandler = [this] {
            if (m_callback) {
                auto inputText = m_inputText;
                m_inputText.clear(); // Clear input text after submission
                m_callback(PopUpViewBase::Result::Input{inputText});
            }
            resetToPrevious();
        };
        auto textBox = Container::Vertical({Input(&m_inputText, "", {.on_enter = txtBoxHandler}),
                                            Button("Submit", txtBoxHandler)});

        auto yesNoRenderer = Renderer(buttons, [this, buttons]() {
            return vbox(text(m_message), separator(), buttons->Render() | center) | center | border;
        });

        auto okRenderer = Renderer(okButton, [this, okButton]() {
            return vbox(text(m_message), separator(), okButton->Render() | center) | center |
                   border;
        });

        auto textBoxRenderer = Renderer(textBox, [this, textBox]() {
            return vbox(text(m_message), separator(), textBox->Render() | center) | center | border;
        });

        auto loadingRenderer =
            Renderer([this] { return window(text("Loading..."), text(m_message)); });

        Components comps{m_view->getAnnualViewLayout()->getComponent(),
                         m_view->getScratchpadViewLayout()->getComponent(),
                         yesNoRenderer,
                         okRenderer,
                         loadingRenderer,
                         textBoxRenderer};

        assert(
            kIndexAnnualViewLayout ==
            std::distance(comps.begin(), std::find(comps.begin(), comps.end(),
                                                   m_view->getAnnualViewLayout()->getComponent())));
        assert(kIndexScratchpadViewLayout ==
               std::distance(comps.begin(),
                             std::find(comps.begin(), comps.end(),
                                       m_view->getScratchpadViewLayout()->getComponent())));
        assert(kIndexYesNot ==
               std::distance(comps.begin(), std::find(comps.begin(), comps.end(), yesNoRenderer)));
        assert(kIndexOk ==
               std::distance(comps.begin(), std::find(comps.begin(), comps.end(), okRenderer)));
        assert(kIndexLoading == std::distance(comps.begin(), std::find(comps.begin(), comps.end(),
                                                                       loadingRenderer)));

        assert(kIndexTextBox == std::distance(comps.begin(), std::find(comps.begin(), comps.end(),
                                                                       textBoxRenderer)));

        auto tab = Container::Tab(comps, &m_currentScreen);
        this->m_prompt = tab;
        Add(tab);
    }

    ~PopUpViewLayoutWrapper() override {};

    void show(const PopUpType &popUp) override {
        std::visit(
            [this](const auto &popUpData) {
                using T = std::decay_t<decltype(popUpData)>;
                if constexpr (std::is_same_v<T, PopUpViewBase::Ok>) {
                    promptOk(popUpData.message, popUpData.callback);
                } else if constexpr (std::is_same_v<T, PopUpViewBase::YesNo>) {
                    prompt(popUpData.message, popUpData.callback);
                } else if constexpr (std::is_same_v<T, PopUpViewBase::TextBox>) {
                    promptTxt(popUpData.message, popUpData.callback);
                } else if constexpr (std::is_same_v<T, PopUpViewBase::Loading>) {
                    loadingScreen(popUpData.message);
                } else if constexpr (std::is_same_v<T, PopUpViewBase::None>) {
                    resetToPrevious();
                }
            },
            popUp);
    }

    void switchLayout() {
        assert(m_currentScreen == kIndexAnnualViewLayout ||
               m_currentScreen == kIndexScratchpadViewLayout);
        if (m_currentScreen == kIndexAnnualViewLayout) {
            m_currentScreen = kIndexScratchpadViewLayout;
        } else if (m_currentScreen == kIndexScratchpadViewLayout) {
            m_currentScreen = kIndexAnnualViewLayout;
        }
        m_previousScreen = m_currentScreen;
    }

  private:
    void prompt(std::string message, PopUpCallback callback) {
        m_message = std::move(message);
        m_callback = std::move(callback);
        m_previousScreen = m_currentScreen;
        m_currentScreen = kIndexYesNot;
    }

    void promptOk(std::string message, PopUpCallback callback) {
        m_message = std::move(message);
        m_callback = std::move(callback);
        m_previousScreen = m_currentScreen;
        m_currentScreen = kIndexOk;
    }

    void loadingScreen(std::string message) {
        m_message = std::move(message);
        m_callback = nullptr;
        m_previousScreen = m_currentScreen;
        m_currentScreen = kIndexLoading;
    }

    void promptTxt(std::string message, PopUpCallback callback) {
        m_message = std::move(message);
        m_callback = std::move(callback);
        m_previousScreen = m_currentScreen;
        m_currentScreen = kIndexTextBox;
    }

    void resetToPrevious() {
        m_currentScreen = m_previousScreen;
        m_previousScreen = 0;
    }

    Element OnRender() override {
        auto currentView = m_prompt->ChildAt(m_currentScreen)->Render();
        if (m_currentScreen == kIndexAnnualViewLayout ||
            m_currentScreen == kIndexScratchpadViewLayout) {
            return currentView;
        }
        auto previousView = m_prompt->ChildAt(m_previousScreen)->Render();
        return dbox(previousView | dim | color(Color::Grey37), currentView | clear_under | center);
    }
};

View::View(const ViewConfig &conf, std::chrono::year_month_day today,
           std::function<ftxui::Dimensions()> terminalSizeProvider)
    : m_annualViewLayout{std::make_shared<AnnualViewLayout>(
          this, terminalSizeProvider, today, conf.sundayStart, conf.recentEventsWindow)},
      m_scratchpadViewLayout{std::make_shared<ScratchpadViewLayout>(this, terminalSizeProvider)},
      m_rootWithPopUpSupport{std::make_shared<PopUpViewLayoutWrapper>(this)},
      m_terminalSizeProvider{std::move(terminalSizeProvider)} {}

PopUpViewBase &View::getPopUpView() { return *m_rootWithPopUpSupport; }

void View::run() {
    // Caps-log expects the terminal to be at least 80x24
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
    ftxui::Terminal::SetFallbackSize(ftxui::Dimensions{/*dimx=*/80, /*dimy=*/24});
    m_running = true;
    m_screen.Loop(m_rootWithPopUpSupport);
}

void View::stop() { m_screen.Exit(); }

void View::post(const ftxui::Task &task) {
    m_screen.Post(task);
    m_screen.PostEvent(Event::Custom);
}

bool View::handleInputEvent(const UIEvent &event) {
    if (m_inputHandler == nullptr) {
        return false;
    }
    if (std::holds_alternative<UiStarted>(event)) {
        m_screen.Post([this]() { m_inputHandler->handleInputEvent(UIEvent{UiStarted{}}); });
        return true;
    }
    return m_inputHandler->handleInputEvent(event);
}

std::shared_ptr<AnnualViewLayoutBase> View::getAnnualViewLayout() { return m_annualViewLayout; }

std::shared_ptr<ScratchpadViewLayoutBase> View::getScratchpadViewLayout() {
    return m_scratchpadViewLayout;
}

void View::withRestoredIO(std::function<void()> func) {
    // We shouldn't ever trigger the function if we are not running, but for the purposes of
    // testing, we allow it.
    if (m_running) {
        m_screen.WithRestoredIO(std::move(func))();
    } else {
        func();
    }
}

void View::setInputHandler(InputHandlerBase *handler) { m_inputHandler = handler; }

void View::switchLayout() { m_rootWithPopUpSupport->switchLayout(); }

bool View::onEvent(ftxui::Event event) { return m_rootWithPopUpSupport->OnEvent(std::move(event)); }

std::string View::render() const {
    auto dimensitons = m_terminalSizeProvider();
    ftxui::Screen screen{dimensitons.dimx, dimensitons.dimy};

    auto root = m_rootWithPopUpSupport->Render();
    ftxui::Render(screen, root);

    return screen.ToString();
}

} // namespace caps_log::view
