/**
 * @file    object_dictionary_registry_test.cpp
 * @author  Paul Thomas
 * @date    2026-11-19
 * @brief   Unit tests for the registry that parses each EDS and XDD file once.
 */
#include "utils/communication/can_open/object_dictionary_registry.h"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <thread>
#include <vector>

using namespace Frasy::CanOpen;

namespace {
std::string edsWithName(std::string_view name)
{
    return std::format("[1000]\nParameterName={}\nObjectType=0x7\nDataType=0x0007\nAccessType=ro\n", name);
}
}    // namespace

class ObjectDictionaryRegistryTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        ObjectDictionaryRegistry::get().clear();
        m_path = std::filesystem::temp_directory_path() / "frasy_registry_test.eds";
        write(edsWithName("First"));
    }

    void TearDown() override
    {
        std::filesystem::remove(m_path);
        ObjectDictionaryRegistry::get().clear();
    }

    void write(const std::string& content)
    {
        std::ofstream(m_path, std::ios::binary | std::ios::trunc) << content;
    }

    //! Makes the file look newer, whatever the resolution of the clock of the file system.
    void touch()
    {
        std::filesystem::last_write_time(m_path, std::filesystem::last_write_time(m_path) + std::chrono::seconds {10});
    }

    std::filesystem::path m_path;
};

TEST_F(ObjectDictionaryRegistryTest, ParsesOnce)
{
    auto& registry = ObjectDictionaryRegistry::get();
    auto  first    = registry.load(m_path);
    auto  second   = registry.load(m_path);
    ASSERT_TRUE(first.has_value()) << first.error();
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(first->get(), second->get());    // The very same object, not a copy.
    EXPECT_EQ((*first)->findObject(0x1000)->name, "First");
}

TEST_F(ObjectDictionaryRegistryTest, DifferentSpellingsOfThePathShareTheResult)
{
    auto& registry = ObjectDictionaryRegistry::get();
    auto  first    = registry.load(m_path);
    auto  second   = registry.load(m_path.parent_path() / "." / m_path.filename());
    ASSERT_TRUE(first.has_value() && second.has_value());
    EXPECT_EQ(first->get(), second->get());
}

TEST_F(ObjectDictionaryRegistryTest, ParsesAgainWhenTheFileChanged)
{
    auto& registry = ObjectDictionaryRegistry::get();
    auto  before   = registry.load(m_path);
    write(edsWithName("Second"));
    touch();
    auto after = registry.load(m_path);
    ASSERT_TRUE(before.has_value() && after.has_value());
    EXPECT_NE(before->get(), after->get());
    EXPECT_EQ((*after)->findObject(0x1000)->name, "Second");
    // The ones that still hold the old one are not affected.
    EXPECT_EQ((*before)->findObject(0x1000)->name, "First");
}

TEST_F(ObjectDictionaryRegistryTest, ForceReload)
{
    auto& registry = ObjectDictionaryRegistry::get();
    auto  before   = registry.load(m_path);
    auto  after    = registry.load(m_path, true);
    ASSERT_TRUE(before.has_value() && after.has_value());
    EXPECT_NE(before->get(), after->get());
    // The new result is the one that is shared from now on.
    EXPECT_EQ(registry.load(m_path)->get(), after->get());
}

TEST_F(ObjectDictionaryRegistryTest, MissingFile)
{
    auto result = ObjectDictionaryRegistry::get().load("this/does/not/exist.eds");
    ASSERT_FALSE(result.has_value());
    EXPECT_NE(result.error().find("exist.eds"), std::string::npos);
}

TEST_F(ObjectDictionaryRegistryTest, BrokenFileIsAnErrorUntilItChanges)
{
    auto& registry = ObjectDictionaryRegistry::get();
    write("not a dictionary");
    touch();
    EXPECT_FALSE(registry.load(m_path).has_value());
    EXPECT_FALSE(registry.load(m_path).has_value());

    write(edsWithName("Fixed"));
    touch();
    auto fixed = registry.load(m_path);
    ASSERT_TRUE(fixed.has_value()) << fixed.error();
    EXPECT_EQ((*fixed)->findObject(0x1000)->name, "Fixed");
}

TEST_F(ObjectDictionaryRegistryTest, ConcurrentLoadsShareTheResult)
{
    auto&                                                  registry = ObjectDictionaryRegistry::get();
    std::vector<ObjectDictionaryRegistry::Result>          results(8);
    std::vector<std::thread>                               threads;
    for (std::size_t i = 0; i < results.size(); ++i) {
        threads.emplace_back([&registry, &results, i, this] { results[i] = registry.load(m_path); });
    }
    for (auto& thread : threads) {
        thread.join();
    }
    for (const auto& result : results) {
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->get(), results.front()->get());
    }
}
