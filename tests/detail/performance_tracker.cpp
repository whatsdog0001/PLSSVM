/**
 * @author Alexander Van Craen
 * @author Marcel Breyer
 * @copyright 2018-today The PLSSVM project - All Rights Reserved
 * @license This file is part of the PLSSVM project which is released under the MIT license.
 *          See the LICENSE.md file in the project root for full license information.
 *
 * @brief Tests for the performance tracker class.
 */

#include "plssvm/detail/performance_tracker.hpp"

#include "../naming.hpp"         // naming::label_type_to_name
#include "../types_to_test.hpp"  // util::label_type_gtest
#include "../utility.hpp"        // util::redirect_output

#include "fmt/core.h"            // fmt::format
#include "gtest/gtest.h"         // TEST, TYPED_TEST_SUITE, TYPED_TEST, EXPECT_EQ, EXPECT_TRUE, EXPECT_FALSE, ::testing::Test

#include <iostream>              // std::cout
#include <string>                // std::string

template <typename T>
class TrackingEntry : public ::testing::Test, public util::redirect_output<> {};
TYPED_TEST_SUITE(TrackingEntry, util::label_type_gtest, naming::label_type_to_name);

TYPED_TEST(TrackingEntry, construct) {
    using type = TypeParam;

    // construct a tracking entry
    const plssvm::detail::tracking_entry e{ "category", "name", type{} };

    // check the values
    EXPECT_EQ(e.entry_category, "category");
    EXPECT_EQ(e.entry_name, "name");
    EXPECT_EQ(e.entry_value, type{});
}

TYPED_TEST(TrackingEntry, output_operator) {
    using type = TypeParam;

    // construct a tracking entry
    const plssvm::detail::tracking_entry e{ "category", "name", type{} };

    // output the value
    std::cout << e;

    // check the output value
    EXPECT_EQ(this->get_capture(), fmt::format("{}", type{}));
}

TEST(TrackingEntry, is_tracking_entry) {
    // check whether the provided type is a tracking entry or not, ignoring any top-level const, reference, and volatile qualifiers
    EXPECT_TRUE(plssvm::detail::is_tracking_entry<plssvm::detail::tracking_entry<int>>::value);
    EXPECT_TRUE(plssvm::detail::is_tracking_entry_v<plssvm::detail::tracking_entry<int>>);
    EXPECT_TRUE(plssvm::detail::is_tracking_entry<plssvm::detail::tracking_entry<const int &>>::value);
    EXPECT_TRUE(plssvm::detail::is_tracking_entry_v<plssvm::detail::tracking_entry<const int &>>);
    EXPECT_TRUE(plssvm::detail::is_tracking_entry<plssvm::detail::tracking_entry<std::string>>::value);
    EXPECT_TRUE(plssvm::detail::is_tracking_entry_v<plssvm::detail::tracking_entry<std::string>>);
}
TEST(TrackingEntry, is_no_tracking_entry) {
    // the following types are NOT tracking entries
    EXPECT_FALSE(plssvm::detail::is_tracking_entry<int>::value);
    EXPECT_FALSE(plssvm::detail::is_tracking_entry_v<int>);
    EXPECT_FALSE(plssvm::detail::is_tracking_entry<const int &>::value);
    EXPECT_FALSE(plssvm::detail::is_tracking_entry_v<const int &>);
    EXPECT_FALSE(plssvm::detail::is_tracking_entry<std::string>::value);
    EXPECT_FALSE(plssvm::detail::is_tracking_entry_v<std::string>);
}

// the macros are only available if PLSSVM_PERFORMANCE_TRACKER_ENABLED is defined!
#if defined(PLSSVM_PERFORMANCE_TRACKER_ENABLED)

TEST(PerformanceTracker, pause_and_resume_macros) {
    // tracking is enabled per default
    EXPECT_TRUE(plssvm::detail::performance_tracker::instance().is_tracking());
    // disable performance tracking
    PLSSVM_DETAIL_PERFORMANCE_TRACKER_PAUSE();
    // tracking should now be disabled
    EXPECT_FALSE(plssvm::detail::performance_tracker::instance().is_tracking());
    // re-enable performance tracking
    PLSSVM_DETAIL_PERFORMANCE_TRACKER_RESUME();
    // tracking should now be enabled again
    EXPECT_TRUE(plssvm::detail::performance_tracker::instance().is_tracking());
}
TEST(PerformanceTracker, add_entry_macro) {
    // add different tracking entries
    PLSSVM_DETAIL_PERFORMANCE_TRACKER_ADD_TRACKING_ENTRY((plssvm::detail::tracking_entry{ "foo", "bar", 42 }));
    PLSSVM_DETAIL_PERFORMANCE_TRACKER_ADD_TRACKING_ENTRY((plssvm::detail::tracking_entry{ "foo", "baz", 3.1415 }));
    PLSSVM_DETAIL_PERFORMANCE_TRACKER_ADD_TRACKING_ENTRY((plssvm::detail::tracking_entry{ "", "foobar", 'a' }));

    // get the tracking entries
    const std::unordered_multimap<std::string, std::string> entries = plssvm::detail::performance_tracker::instance().get_tracking_entries();

    // check entries for correctness
    EXPECT_EQ(entries.size(), 3);

    auto r1 = entries.equal_range("foo");
    ASSERT_EQ(std::distance(r1.first, r1.second), 2);
    for (auto it = r1.first; it != r1.second; ++it) {
        EXPECT_EQ(it->first, "foo");
        EXPECT_TRUE(it->second == "  bar: 42\n" || it->second == "  baz: 3.1415\n");
    }

    auto r2 = entries.equal_range("");
    ASSERT_EQ(std::distance(r2.first, r2.second), 1);
    EXPECT_EQ(r2.first->first, "");
    EXPECT_EQ(r2.first->second, "foobar: a\n");
}

#endif

TEST(PerformanceTracker, pause_and_resume) {
    // tracking is enabled per default
    EXPECT_TRUE(plssvm::detail::performance_tracker::instance().is_tracking());
    // disable performance tracking
    plssvm::detail::performance_tracker::instance().pause_tracking();
    // tracking should now be disabled
    EXPECT_FALSE(plssvm::detail::performance_tracker::instance().is_tracking());
    // re-enable performance tracking
    plssvm::detail::performance_tracker::instance().resume_tracking();
    // tracking should now be enabled again
    EXPECT_TRUE(plssvm::detail::performance_tracker::instance().is_tracking());
}
TEST(PerformanceTrackker, add_generic_tracking_entry) {
    // add different tracking entries
    plssvm::detail::performance_tracker::instance().add_tracking_entry(plssvm::detail::tracking_entry{ "foo", "bar", 42 });
    plssvm::detail::performance_tracker::instance().add_tracking_entry(plssvm::detail::tracking_entry{ "foo", "baz", 3.1415 });
    plssvm::detail::performance_tracker::instance().add_tracking_entry(plssvm::detail::tracking_entry{ "", "foobar", 'a' });

    // get the tracking entries
    const std::unordered_multimap<std::string, std::string> entries = plssvm::detail::performance_tracker::instance().get_tracking_entries();

    // check entries for correctness
    EXPECT_EQ(entries.size(), 3);

    auto r1 = entries.equal_range("foo");
    ASSERT_EQ(std::distance(r1.first, r1.second), 2);
    for (auto it = r1.first; it != r1.second; ++it) {
        EXPECT_EQ(it->first, "foo");
        EXPECT_TRUE(it->second == "  bar: 42\n" || it->second == "  baz: 3.1415\n");
    }

    auto r2 = entries.equal_range("");
    ASSERT_EQ(std::distance(r2.first, r2.second), 1);
    EXPECT_EQ(r2.first->first, "");
    EXPECT_EQ(r2.first->second, "foobar: a\n");
}
TEST(PerformanceTracker, add_string_tracking_entry) {
    // add a tracking entry
    plssvm::detail::performance_tracker::instance().add_tracking_entry(plssvm::detail::tracking_entry{ "foo", "bar", std::string{ "baz" } });

    // get the tracking entries
    const std::unordered_multimap<std::string, std::string> entries = plssvm::detail::performance_tracker::instance().get_tracking_entries();

    // check entries for correctness
    EXPECT_EQ(entries.size(), 1);

    auto r = entries.equal_range("foo");
    ASSERT_EQ(std::distance(r.first, r.second), 1);
    EXPECT_EQ(r.first->first, "foo");
    EXPECT_EQ(r.first->second, "  bar: \"baz\"\n");
}
TEST(PerformanceTracker, add_parser_train_tracking_entry) {
    // create a parameter train object
    constexpr int argc = 3;
    char argv_arr[argc][20] = { "./plssvm-train", "/path/to/train", "/path/to/model" };
    char* argv[] { argv_arr[0], argv_arr[1], argv_arr[3] };
    const plssvm::detail::cmd::parser_train parser{ argc, argv };

    // save cmd::parser_train entry
    plssvm::detail::performance_tracker::instance().add_tracking_entry(plssvm::detail::tracking_entry{ "parameter", "", parser });

    // get the tracking entries
    const std::unordered_multimap<std::string, std::string> entry = plssvm::detail::performance_tracker::instance().get_tracking_entries();

    // check entries for correctness
    EXPECT_EQ(entry.size(), 1);

    auto r = entry.equal_range("parameter");
    ASSERT_EQ(std::distance(r.first, r.second), 1);
    EXPECT_EQ(r.first->first, "parameter");
    EXPECT_FALSE(r.first->second.empty());
}
TEST(PerformanceTracker, add_parser_predict_tracking_entry) {
    // create a parameter train object
    constexpr int argc = 4;
    char argv_arr[argc][20] = { "./plssvm-predict", "/path/to/train", "/path/to/model", "/path/to/predict" };
    char* argv[] { argv_arr[0], argv_arr[1], argv_arr[3], argv_arr[4] };
    const plssvm::detail::cmd::parser_predict parser{ argc, argv };

    // save cmd::parser_train entry
    plssvm::detail::performance_tracker::instance().add_tracking_entry(plssvm::detail::tracking_entry{ "parameter", "", parser });

    // get the tracking entries
    const std::unordered_multimap<std::string, std::string> entry = plssvm::detail::performance_tracker::instance().get_tracking_entries();

    // check entries for correctness
    EXPECT_EQ(entry.size(), 1);

    auto r = entry.equal_range("parameter");
    ASSERT_EQ(std::distance(r.first, r.second), 1);
    EXPECT_EQ(r.first->first, "parameter");
    EXPECT_FALSE(r.first->second.empty());
}
TEST(PerformanceTracker, add_parser_scale_tracking_entry) {
    // create a parameter train object
    constexpr int argc = 3;
    char argv_arr[argc][20] = { "./plssvm-train", "/path/to/train", "/path/to/scaled" };
    char* argv[] { argv_arr[0], argv_arr[1], argv_arr[3] };
    const plssvm::detail::cmd::parser_scale parser{ argc, argv };

    // save cmd::parser_train entry
    plssvm::detail::performance_tracker::instance().add_tracking_entry(plssvm::detail::tracking_entry{ "parameter", "", parser });

    // get the tracking entries
    const std::unordered_multimap<std::string, std::string> entry = plssvm::detail::performance_tracker::instance().get_tracking_entries();

    // check entries for correctness
    EXPECT_EQ(entry.size(), 1);

    auto r = entry.equal_range("parameter");
    ASSERT_EQ(std::distance(r.first, r.second), 1);
    EXPECT_EQ(r.first->first, "parameter");
    EXPECT_FALSE(r.first->second.empty());
}

TEST(PerformanceTracker, save_no_additional_entries) {
    // create temporary file
    const util::temporary_file tmp_file{};  // automatically removes the created file at the end of its scope
    // save entries to file
    plssvm::detail::performance_tracker::instance().save(tmp_file.filename);

    // the file must not be empty
    EXPECT_FALSE(std::filesystem::is_empty(tmp_file.filename));

    // the tracking entries must be empty now
    EXPECT_TRUE(plssvm::detail::performance_tracker::instance().get_tracking_entries().empty());
}
TEST(PerformanceTracker, save_entries) {
    // create temporary file
    const util::temporary_file tmp_file{};  // automatically removes the created file at the end of its scope
    // save entries to file
    plssvm::detail::performance_tracker::instance().add_tracking_entry(plssvm::detail::tracking_entry{ "foo", "bar", 42 });
    plssvm::detail::performance_tracker::instance().add_tracking_entry(plssvm::detail::tracking_entry{ "foo", "baz", 3.1415 });
    plssvm::detail::performance_tracker::instance().add_tracking_entry(plssvm::detail::tracking_entry{ "", "foobar", 'a' });
    plssvm::detail::performance_tracker::instance().save(tmp_file.filename);

    // the file must not be empty
    EXPECT_FALSE(std::filesystem::is_empty(tmp_file.filename));

    // the tracking entries must be empty now
    EXPECT_TRUE(plssvm::detail::performance_tracker::instance().get_tracking_entries().empty());
}