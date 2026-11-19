/**
 * @file    generation.cpp
 * @brief   End to end: Lua Orchestrator.Generate() -> SaveAsJson() -> Solution::FromGeneration(),
 *          the same path as Orchestrator::runStageGenerate().
 */
#include "orchestrator_test_fixture.h"

#include <filesystem>
#include <format>
#include <fstream>
#include <gtest/gtest.h>
#include <utils/lua/save_as_json.h>
#include <utils/models/solution.h>

using Frasy::Models::Solution;
using Names = std::vector<std::string>;

class SolutionGenerationTest : public OrchestratorTestFixture {
protected:
    std::filesystem::path m_file;

    void SetUp() override
    {
        OrchestratorTestFixture::SetUp();
        // Under the test binary directory, so runs from different build trees don't share it.
        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        m_file =
          std::filesystem::current_path() / std::format("models_{}_{}.json", info->test_suite_name(), info->name());
    }

    void TearDown() override
    {
        std::error_code ec;
        std::filesystem::remove(m_file, ec);
        OrchestratorTestFixture::TearDown();
    }

    Solution generate()
    {
        sol::table order = lua.script("return Orchestrator.Generate()");
        Frasy::Lua::SaveAsJson(order, m_file.string());
        std::ifstream is(m_file);
        return Solution::FromGeneration(is);
    }

    /// Number of sections in which `sequence` has a subsequence.
    static std::size_t sectionsWith(const Solution& solution, const std::string& sequence)
    {
        std::size_t count = 0;
        for (const auto& section : solution.sections) {
            bool found = false;
            for (const auto& stage : section) {
                for (const auto& [name, tests] : stage) {
                    found = found || name == sequence;
                }
            }
            if (found) { ++count; }
        }
        return count;
    }
};

TEST_F(SolutionGenerationTest, OrderFollowsSequenceRequirements)
{
    lua.script(R"(
        Sequence("C", function()
            Requires(Sequence("B"):ToPass())
            Test("T", function() end)
        end)
        Sequence("B", function()
            Requires(Sequence("A"):ToPass())
            Test("T", function() end)
        end)
        Sequence("A", function()
            Test("T", function() end)
        end)
    )");
    const Solution solution = generate();
    EXPECT_EQ(solution.sequenceOrder, (Names {"A", "B", "C"}));
}

TEST_F(SolutionGenerationTest, OrderFollowsTestRequirements)
{
    lua.script(R"(
        Sequence("Seq", function()
            Test("T3", function()
                Requires(Test("T2"):ToPass())
            end)
            Test("T2", function()
                Requires(Test("T1"):ToPass())
            end)
            Test("T1", function() end)
        end)
    )");
    const Solution solution = generate();
    EXPECT_EQ(solution.sequences.at("Seq").testOrder, (Names {"T1", "T2", "T3"}));
}

TEST_F(SolutionGenerationTest, FirstAndLast)
{
    lua.script(R"(
        Sequence("Middle", function() Test("T", function() end) end)
        Sequence("End", function()
            Requires(Sequence():ToBeLast())
            Test("T", function() end)
        end)
        Sequence("Start", function()
            Requires(Sequence():ToBeFirst())
            Test("T", function() end)
        end)
    )");
    const Solution solution = generate();
    ASSERT_EQ(solution.sequenceOrder.size(), 3u);
    EXPECT_EQ(solution.sequenceOrder.front(), "Start");
    EXPECT_EQ(solution.sequenceOrder.back(), "End");
}

TEST_F(SolutionGenerationTest, SyncSplitKeepsOrderAcrossSections)
{
    // Sync marks SeqA, which is sectioned after the unsynced SeqB; inside SeqA, unsynced T2 runs before T1.
    lua.script(R"(
        Sequence("SeqA", function()
            Test("T1", function() Sync() end)
            Test("T2", function() end)
        end)
        Sequence("SeqB", function()
            Test("T1", function() end)
        end)
    )");
    const Solution solution = generate();
    EXPECT_EQ(solution.sections.size(), 3u);
    EXPECT_EQ(sectionsWith(solution, "SeqA"), 2u);
    EXPECT_EQ(solution.sequenceOrder, (Names {"SeqB", "SeqA"}));
    EXPECT_EQ(solution.sequences.at("SeqA").testOrder, (Names {"T2", "T1"}));
    EXPECT_EQ(solution.sequences.at("SeqB").testOrder, (Names {"T1"}));
}
