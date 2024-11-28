#include "utils/date.hpp"
#include "view/calendar_component.hpp"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

using namespace std::chrono_literals;

namespace {
const std::filesystem::path kCalendarRenderTestData = std::filesystem::path{CAPS_LOG_TEST_DATA_DIR};

std::string readFile(const std::string &path) {
    std::ifstream ifs{path};
    if (not ifs.is_open()) {
        throw std::runtime_error{"Failed to open file: " + path};
    }
    std::stringstream buffer;
    buffer << ifs.rdbuf();
    return buffer.str();
}

class MockScreenSizeProvider : public caps_log::view::ScreenSizeProvider {
  public:
    MockScreenSizeProvider(ftxui::Dimensions size) : m_size{size} {}
    ftxui::Dimensions getScreenSize() const override { return m_size; }

  private:
    ftxui::Dimensions m_size;
};

} // namespace

TEST(CalnedarComponentTest, Render) {
    struct TestData {
        int screen_width;
        int expected_months_per_row;
    };

    const auto filePath = [](const TestData &data) {
        return kCalendarRenderTestData /
               ("screen_width_" + std::to_string(data.screen_width) + ".bin");
    };
    // NOLINTBEGIN
    std::vector<TestData> testData = {
        {34, 1},  {68, 2},  {102, 3},  {136, 4},  {238, 7},
        {272, 8}, {306, 9}, {340, 10}, {374, 11}, {408, 12},
    };
    // NOLINTEND
    for (const auto &data : testData) {
        ftxui::Dimensions dimensions{data.screen_width, 41}; // NOLINT
        ftxui::Screen screen{data.screen_width, 41};         // NOLINT
        auto sizeProvider = std::make_unique<MockScreenSizeProvider>(dimensions);
        caps_log::view::Calendar calendar{std::move(sizeProvider),
                                          {2024y, std::chrono::January, 1d}}; // NOLINT
        ftxui::Render(screen, calendar.Render());
        const auto screenRender = screen.ToString();
        std::string expectedOutput = readFile(filePath(data));
        EXPECT_EQ(screenRender, expectedOutput) << "Expected" << expectedOutput << "\n\n"
                                                << "Got: " << screenRender;
    }
}

TEST(CalnedarComponentTest, EventHandling) {
    std::chrono::year_month_day start{2024y, std::chrono::January, 1d};       // NOLINT
    std::chrono::year_month_day next{2024y, std::chrono::January, 2d};        // NOLINT
    std::chrono::year_month_day next_month{2024y, std::chrono::February, 1d}; // NOLINT
    ftxui::Screen screen{184, 41};                                            // NOLINT
    auto sizeProvider = std::make_unique<MockScreenSizeProvider>(ftxui::Dimensions{184, 41});

    caps_log::view::Calendar calendar{std::move(sizeProvider), start};

    calendar.OnEvent(ftxui::Event::ArrowRight);
    EXPECT_EQ(calendar.getFocusedDate(), next)
        << "Got date: " << caps_log::utils::date::formatToString(calendar.getFocusedDate());

    calendar.OnEvent(ftxui::Event::ArrowDown);
    calendar.OnEvent(ftxui::Event::ArrowDown);
    calendar.OnEvent(ftxui::Event::ArrowDown);
    calendar.OnEvent(ftxui::Event::ArrowDown);
    calendar.OnEvent(ftxui::Event::ArrowDown);

    EXPECT_EQ(calendar.getFocusedDate(), next_month)
        << "Got date: " << caps_log::utils::date::formatToString(calendar.getFocusedDate());
}
