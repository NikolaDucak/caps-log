#include "../source/model/filesystem_accessor_base.hpp"
#include "../source/model/log_file.hpp"
#include "../utils/mock_filesystem.hpp"

#include <filesystem>
#include <gtest/gtest.h>
#include <memory>

namespace fs = std::filesystem;

using clog::model::Date;

TEST(log_file_test, exists) {
    auto mock_fs = std::make_shared<mock_filesystem_accessor>(
        std::map<Date, std::string, comparator> {
            { { 1, 1, 1 }, "" },
        });
    clog::model::log_file file1 { mock_fs, Date { 1, 1, 1 } };
    clog::model::log_file file2 { mock_fs, Date { 2, 1, 1 } };

    ASSERT_TRUE(file1.exists());
    ASSERT_FALSE(file2.exists());
}

TEST(log_file_test, has_data) {
    auto mock_fs = std::make_shared<mock_filesystem_accessor>(
        std::map<Date, std::string, comparator> {
            // no content
            { { 1, 1, 1 }, "" },
            { { 1, 2, 1 }, clog::model::log_file::LOG_FILE_BASE_TEMPLATE },
            { { 1, 3, 1 },
              clog::model::log_file::format_base_template({ 1, 3, 1 }) },
            { { 1, 4, 1 }, "\n\n" },

            // content
            { { 1, 5, 1 }, "one line content" },
            { { 1, 6, 1 }, "# one line content in section form" },
            { { 1, 7, 1 }, "one line content \nno! two line content" },
        });
    EXPECT_FALSE(
        (clog::model::log_file { mock_fs, Date { 1, 1, 1 } }.has_data()));
    EXPECT_FALSE(
        (clog::model::log_file { mock_fs, Date { 1, 2, 1 } }.has_data()));
    EXPECT_FALSE(
        (clog::model::log_file { mock_fs, Date { 1, 3, 1 } }.has_data()));
    EXPECT_FALSE(
        (clog::model::log_file { mock_fs, Date { 1, 4, 1 } }.has_data()));

    EXPECT_TRUE(
        (clog::model::log_file { mock_fs, Date { 1, 5, 1 } }.has_data()));
    EXPECT_TRUE(
        (clog::model::log_file { mock_fs, Date { 1, 6, 1 } }.has_data()));
    EXPECT_TRUE(
        (clog::model::log_file { mock_fs, Date { 1, 7, 1 } }.has_data()));
}

TEST(log_file_test, create) {
    auto mock_fs = std::make_shared<mock_filesystem_accessor>(
        std::map<Date, std::string, comparator> {});

    // succesfull creation
    clog::model::log_file { mock_fs, Date { 1, 1, 1 } }.create("dummy data");
    EXPECT_EQ(mock_fs->logs.size(), 1);
    EXPECT_EQ((mock_fs->logs.begin()->first), (Date { 1, 1, 1 }));
    EXPECT_EQ(mock_fs->logs.begin()->second, "dummy data");

    // creating an already egxistion log
    clog::model::log_file { mock_fs, Date { 1, 1, 1 } }.create(
        "double dummy data");
    EXPECT_EQ(mock_fs->logs.size(), 1);
    EXPECT_EQ((mock_fs->logs.begin()->first), (Date { 1, 1, 1 }));
    EXPECT_EQ(mock_fs->logs.begin()->second, "dummy data");
}

TEST(log_file_test, get_tasks) {
    struct task_parsing_test_data {
        std::string task_string;
        std::optional<::clog::model::task> parsed_task;
    };

    using clog::model::task;
    std::vector<task_parsing_test_data> test_data {
        { "", {} },
        // valid tasks
        { "[ ] random-title", task { "random-title", "", ' ' } },
        { "[ ] random title", task { "random title", "", ' ' } },
        { "[ ]     random title   ", task { "random title", "", ' ' } },
        { " - [ ] random-title", task { "random-title", "", ' ' } },
        { "- [ ] random title", task { "random title", "", ' ' } },
        { " - [ ]     random-title", task { "random-title", "", ' ' } },
        { "- [ ]     random title", task { "random title", "", ' ' } },

        // completion char
        { "[x] random-title", task { "random-title", "", 'x' } },
        { "[-] random title", task { "random title", "", '-' } },
        { "[.]     random title   ", task { "random title", "", '.' } },

        // time info
        { "- [f] (20min) random title", task { "random title", "20min", 'f' } },
        { " - [ ] (1h) random-title", task { "random-title", "1h", ' ' } },
        { "- [ ]  (random words wtf) random title",
          task { "random title", "random words wtf", ' ' } },

        // aditional info

        // multiline info
    };

    for (const auto& data : test_data) {
        auto fs = std::make_shared<mock_single_file_fiflesystem>(
            Date { 1, 1, 1 }, data.task_string);
        auto result =
            clog::model::log_file { fs, Date { 1, 1, 1 } }.get_tasks();
        if (data.parsed_task) {
            EXPECT_EQ(result.size(), 1);
            EXPECT_EQ(result[0].title, data.parsed_task->title);
            EXPECT_EQ(result[0].time_info, data.parsed_task->time_info);
            EXPECT_EQ(result[0].completion_value,
                      data.parsed_task->completion_value);
        } else {
            EXPECT_EQ(result.size(), 0);
        }
    }
}

TEST(log_file_test, get_non_empty_section_titles) {
    auto mock_fs = std::make_shared<mock_filesystem_accessor>(
        std::map<Date, std::string, comparator> {
            { { 1, 1, 1 }, { "# empty \n" } },
            { { 1, 2, 1 }, { "# empty \n# empty2" } },
            { { 1, 3, 1 },
              { "# empty \n# non-empty\nSome content.\n# empty" } },
        });

    auto sections = clog::model::log_file {
        mock_fs, Date { 1, 1, 1 }
    }.get_non_empty_section_titles();
    EXPECT_EQ(sections.size(), 0);

    sections = clog::model::log_file {
        mock_fs, Date { 1, 2, 1 }
    }.get_non_empty_section_titles();
    EXPECT_EQ(sections.size(), 0);

    sections = clog::model::log_file {
        mock_fs, Date { 1, 3, 1 }
    }.get_non_empty_section_titles();
    EXPECT_EQ(sections, std::vector<std::string> { "non-empty" });
}
