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
        ftxui::Screen screen{data.screen_width, 41};                                  // NOLINT
        caps_log::view::Calendar calendar{screen, {2024y, std::chrono::January, 1d}}; // NOLINT

        ftxui::Render(screen, calendar.Render());
        const auto screenRender = screen.ToString();
        std::string expectedOutput = readFile(filePath(data));
        EXPECT_EQ(screenRender, expectedOutput) << "Got: " << screenRender;
    }
}

TEST(CalnedarComponentTest, EventHandling) {
    std::chrono::year_month_day start{2024y, std::chrono::January, 1d};       // NOLINT
    std::chrono::year_month_day next{2024y, std::chrono::January, 2d};        // NOLINT
    std::chrono::year_month_day next_month{2024y, std::chrono::February, 1d}; // NOLINT
    ftxui::Screen screen{184, 41};                                            // NOLINT
    caps_log::view::Calendar calendar{screen, start};                         // NOLINT
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
