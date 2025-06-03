#include "view/scratchpad_view_layout.hpp"
#include "utils/date.hpp"

#include <ftxui/dom/elements.hpp>

namespace caps_log::view {

using namespace ftxui;

namespace {
constexpr auto kMaxMenuWidth = 40;
constexpr auto kPreviewDefaultHeight = 5; // Default height for the preview component

std::string formatTitleDate(const std::string &title, const std::string &date) {
    std::string formattedTitle = title;

    const auto maxTitleLength = (kMaxMenuWidth)-date.size() - 1; // 1 for " "

    if (formattedTitle.size() > maxTitleLength) {
        formattedTitle = formattedTitle.substr(0, maxTitleLength); // truncate
        // insert '...' as last 3 characte
        formattedTitle.replace(maxTitleLength - 3, 3, "...");
    } else {
        formattedTitle.append(maxTitleLength - formattedTitle.size(), ' '); // pad with spaces
    }

    return formattedTitle + " " + date;
}

std::size_t countLines(const std::string &str) {
    return std::count(str.begin(), str.end(), '\n') +
           1; // +1 for the last line if it doesn't end with a newline
}

} // namespace

ScratchpadViewLayout::ScratchpadViewLayout(InputHandlerBase *inputHandler)
    : m_inputHandler(inputHandler), m_height(kPreviewDefaultHeight) {
    m_windowedMenu = WindowedMenu::make(WindowedMenuOption{
        .title = "Scratchpads", .entries = &m_scratchpadTitles, .onChange = [this]() {
            if (m_windowedMenu->selected() == 0) {
                m_preview->setContent("Create a new scratchpad", "No scratchpad selected");
            } else {
                auto selectedScratchpad = m_scratchpadFileNames[m_windowedMenu->selected()];
                m_preview->setContent(selectedScratchpad,
                                      m_scratchpadContents[m_windowedMenu->selected()]);
            }
        }});
    m_preview = std::make_shared<Preview>();
    m_preview->setContent("Select a scratchpad", "No scratchpad selected");
    auto container = Container::Horizontal({
        m_windowedMenu,
        m_preview,
    });

    auto component = Renderer(container, [this]() {
        auto terminalWidth = Terminal::Size().dimx;
        const auto kFactor = 0.75; // Factor to determine the width of the preview
        const auto height = 24;

        static const auto kHelpString =
            std::string{"hjkl/arrow keys - navigation | d - delete | s - see logs | r - rename"};
        // clang-format off
        return vbox({
          text("Scratchpad Manager") | center | bold, 
          separator(),
              hbox({
                  m_windowedMenu->Render() 
                      | size(WIDTH, Constraint::GREATER_THAN, kMaxMenuWidth),
                  m_preview->Render() 
                      | size(ftxui::HEIGHT, EQUAL, static_cast<int>(height)) 
                      | size(WIDTH, Constraint::EQUAL, static_cast<int>(terminalWidth * kFactor))
              }),
              text(kHelpString) | center | dim
        }) 
        | yflex_grow | center;
        // clang-format on
    });

    m_component = CatchEvent(component, [this](const Event &event) {
        if (event == Event::Return) {
            auto scratchpadName = (m_windowedMenu->selected() == 0)
                                      ? ""
                                      : m_scratchpadFileNames[m_windowedMenu->selected()];
            return m_inputHandler->handleInputEvent(UIEvent{OpenScratchpad{scratchpadName}});
        }
        if (event == Event::Character('d')) {
            auto scratchpadName = (m_windowedMenu->selected() == 0)
                                      ? ""
                                      : m_scratchpadFileNames[m_windowedMenu->selected()];
            return m_inputHandler->handleInputEvent(UIEvent{DeleteScratchpad{scratchpadName}});
        }
        if (event == Event::Character('r')) {
            auto scratchpadName = (m_windowedMenu->selected() == 0)
                                      ? ""
                                      : m_scratchpadFileNames[m_windowedMenu->selected()];
            return m_inputHandler->handleInputEvent(UIEvent{RenameScratchpad{scratchpadName}});
        }
        if (not event.is_mouse()) {
            return m_inputHandler->handleInputEvent(UIEvent{UnhandledRootEvent{event.input()}});
        }
        return false;
    });
    setScratchpads({}); // Initialize with an empty list
}

void ScratchpadViewLayout::setScratchpads(const std::vector<ScratchpadData> &scratchpadData) {
    m_scratchpadTitles.clear();
    m_scratchpadContents.clear();
    m_scratchpadFileNames.clear();

    // push the "Make new scratchpad" entry at the top
    m_scratchpadFileNames.push_back(""); // Empty name for new scratchpad
    m_scratchpadTitles.push_back("Make new scratchpad");
    m_scratchpadContents.push_back("Create a new scratchpad");

    m_windowedMenu->selected() = 0; // Reset selection to the first item

    // sort on last modified date
    auto scratchpadsCopy = scratchpadData;
    std::sort(scratchpadsCopy.begin(), scratchpadsCopy.end(),
              [](const ScratchpadData &left, const ScratchpadData &right) {
                  return left.dateModified > right.dateModified;
              });

    const auto longestContentLength =
        std::max_element(scratchpadsCopy.begin(), scratchpadsCopy.end(),
                         [](const ScratchpadData &left, const ScratchpadData &right) {
                             return countLines(left.content) < countLines(right.content);
                         });
    // TODO: remove m_height
    m_height = (longestContentLength != scratchpadsCopy.end())
                   ? countLines(longestContentLength->content) + 2 // +2 for padding
                   : kPreviewDefaultHeight;

    for (const auto &data : scratchpadsCopy) {
        const auto title =
            formatTitleDate(data.title, utils::date::formatToString(data.dateModified, "%y-%m-%d"));
        m_scratchpadFileNames.push_back(data.title);
        m_scratchpadTitles.push_back(title);
        m_scratchpadContents.push_back(data.content);
    }
    m_preview->setContent("Select a scratchpad", "No scratchpad selected");
}

Component ScratchpadViewLayout::getComponent() { return m_component; }
} // namespace caps_log::view
