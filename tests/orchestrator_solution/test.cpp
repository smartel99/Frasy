/**
 * @file    test.cpp
 * @brief   Integration tests of the C++ Orchestrator's solution handling: background generation, load
 *          serialization, enable flags across reloads, revisions and thread-safe copies.
 *
 * Products are written to disk under the test working directory. Marker files make generation slow or broken
 * on demand, after the product is loaded.
 *
 * Debug only: other configurations verify the hash of `lua/user`, which the test directory doesn't have.
 */
#include <Brigerad/Core/Log.h>
#include <gtest/gtest.h>
#include <utils/lua/orchestrator/generate_on_load.h>
#include <utils/lua/orchestrator/orchestrator.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

using Frasy::Lua::Orchestrator;
using Frasy::Models::Solution;
using Names = std::vector<std::string>;
using namespace std::chrono_literals;

namespace {
constexpr std::size_t s_uutCount = 2;

// Product A: two sequences, ordered by requirements.
constexpr auto s_productA = R"(
Sequence("Validation", function()
    Requires(Sequence("Basic"):ToPass())
    Test("V1", function() end)
end)
Sequence("Basic", function()
    Test("T2", function() Requires(Test("T1"):ToPass()) end)
    Test("T1", function() end)
end)
)";

// Product B: different sequences.
constexpr auto s_productB = R"(
Sequence("Other", function()
    Test("O1", function() end)
end)
)";
}    // namespace

class OrchestratorSolutionTest : public ::testing::Test {
protected:
    std::filesystem::path m_root;
    Orchestrator          m_orchestrator;

    static void SetUpTestSuite() { Brigerad::Log::Init(true, true); }

    void SetUp() override
    {
#ifndef BR_DEBUG
        GTEST_SKIP() << "Debug only: loadUserFiles() verifies the lua/user hash in other configurations.";
#endif
        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        // Relative and dot-free: products are loaded with Lua's require().
        m_root = std::filesystem::path("orchestrator_solution") / info->name();
        std::filesystem::remove_all(m_root);
        writeProduct("a", s_productA);
        writeProduct("b", s_productB);
    }

    void TearDown() override
    {
        // Let a pending generation finish before the files go away.
        waitGeneration();
        std::error_code ec;
        std::filesystem::remove_all(m_root, ec);
    }

    void writeProduct(const std::string& name, const std::string& tests) const
    {
        const auto dir = m_root / name;
        std::filesystem::create_directories(dir / "tests");
        std::ofstream(dir / "environment.lua")
          << std::format("Environment.Make(function() Environment.Uut.Count({}) end)\n", s_uutCount);
        // Markers are checked when the tests are loaded: "broken" by every Lua state, "slow" by generation only.
        std::ofstream(dir / "tests" / "tests.lua") << std::format(R"(
local function exists(name)
    local f = io.open("{0}/" .. name, "r")
    if f then f:close() return true end
    return false
end
if exists("broken") then error("Broken on purpose") end
-- Generation only: loadUserFiles() loads as UUT 0, generation as UUT 1.
if exists("slow") and Context.info.uut ~= 0 then
    io.open("{0}/started", "w"):close()
    local start = os.clock()
    while os.clock() - start < 0.5 do end
end
{1})",
                                                                  m_root.generic_string(),
                                                                  tests);
    }

    void setMarker(const std::string& name) const { std::ofstream(m_root / name) << ""; }

    [[nodiscard]] std::string env(const std::string& name) const
    { return (m_root / name / "environment").generic_string(); }
    [[nodiscard]] std::string tests(const std::string& name) const
    { return (m_root / name / "tests").generic_string(); }

    bool load(const std::string& name) { return m_orchestrator.loadUserFiles(env(name), tests(name)); }

    /// Wait for a slow Lua state to reach its busy loop, so the generation holds its lock.
    [[nodiscard]] bool waitStarted() const
    {
        const auto deadline = std::chrono::steady_clock::now() + 30s;
        while (!std::filesystem::exists(m_root / "started")) {
            if (std::chrono::steady_clock::now() > deadline) { return false; }
            std::this_thread::sleep_for(1ms);
        }
        return true;
    }

    bool waitGeneration()
    {
        const auto deadline = std::chrono::steady_clock::now() + 30s;
        while (m_orchestrator.isGenerating()) {
            if (std::chrono::steady_clock::now() > deadline) { return false; }
            std::this_thread::sleep_for(5ms);
        }
        return true;
    }

    void generateAndWait()
    {
        ASSERT_TRUE(m_orchestrator.generate());
        ASSERT_TRUE(waitGeneration());
    }
};

// =============================================================================
// Generation
// =============================================================================

TEST_F(OrchestratorSolutionTest, GenerateFillsOrderedSolution)
{
    ASSERT_TRUE(load("a"));
    EXPECT_TRUE(m_orchestrator.getSolution().sequences.empty());

    generateAndWait();
    const Solution solution = m_orchestrator.getSolution();
    EXPECT_EQ(solution.sequenceOrder, (Names {"Basic", "Validation"}));
    EXPECT_EQ(solution.sequences.at("Basic").testOrder, (Names {"T1", "T2"}));
    for (const auto& [sName, sequence] : solution.sequences) {
        EXPECT_TRUE(sequence.enabled) << sName;
        EXPECT_EQ(sequence.state.size(), s_uutCount + 1) << sName;
        for (const auto& [tName, test] : sequence.tests) {
            EXPECT_EQ(test.state.size(), s_uutCount + 1) << tName;
        }
    }
}

TEST_F(OrchestratorSolutionTest, GenerationIsNotARun)
{
    ASSERT_TRUE(load("a"));
    setMarker("slow");
    ASSERT_TRUE(m_orchestrator.generate());
    EXPECT_TRUE(m_orchestrator.isGenerating());
    EXPECT_FALSE(m_orchestrator.isRunning());
    ASSERT_TRUE(waitGeneration());
}

TEST_F(OrchestratorSolutionTest, GenerateRefusedWhileGenerating)
{
    ASSERT_TRUE(load("a"));
    setMarker("slow");
    ASSERT_TRUE(m_orchestrator.generate());
    EXPECT_FALSE(m_orchestrator.generate());
    ASSERT_TRUE(waitGeneration());
    EXPECT_TRUE(m_orchestrator.generate());
}

TEST_F(OrchestratorSolutionTest, FailedGenerationKeepsSolution)
{
    ASSERT_TRUE(load("a"));
    generateAndWait();
    m_orchestrator.setSequenceEnable("Basic", false);
    const auto revision = m_orchestrator.getSolutionRevision();

    setMarker("broken");
    generateAndWait();
    EXPECT_EQ(m_orchestrator.getSolutionRevision(), revision);
    const Solution solution = m_orchestrator.getSolution();
    EXPECT_EQ(solution.sequenceOrder, (Names {"Basic", "Validation"}));
    EXPECT_FALSE(solution.sequences.at("Basic").enabled);
}

// =============================================================================
// Load id and revision
// =============================================================================

TEST_F(OrchestratorSolutionTest, LoadIdBumpedOnSuccessfulLoadOnly)
{
    EXPECT_EQ(m_orchestrator.getLoadId(), 0u);
    ASSERT_TRUE(load("a"));
    EXPECT_EQ(m_orchestrator.getLoadId(), 1u);
    ASSERT_TRUE(load("a"));
    EXPECT_EQ(m_orchestrator.getLoadId(), 2u);

    setMarker("broken");
    EXPECT_FALSE(load("a"));
    EXPECT_EQ(m_orchestrator.getLoadId(), 2u);
}

TEST_F(OrchestratorSolutionTest, RevisionBumpedOnEveryChange)
{
    ASSERT_TRUE(load("a"));
    auto revision = m_orchestrator.getSolutionRevision();

    generateAndWait();
    EXPECT_GT(m_orchestrator.getSolutionRevision(), revision);
    revision = m_orchestrator.getSolutionRevision();

    m_orchestrator.setSequenceEnable("Basic", false);
    EXPECT_GT(m_orchestrator.getSolutionRevision(), revision);
    revision = m_orchestrator.getSolutionRevision();

    m_orchestrator.setTestEnable("Basic", "T1", false);
    EXPECT_GT(m_orchestrator.getSolutionRevision(), revision);
    revision = m_orchestrator.getSolutionRevision();

    ASSERT_TRUE(load("b"));    // Product switch clears the solution.
    EXPECT_GT(m_orchestrator.getSolutionRevision(), revision);
}

TEST_F(OrchestratorSolutionTest, UnknownEnableTargetIgnored)
{
    ASSERT_TRUE(load("a"));
    // Nothing generated yet: everything is unknown.
    EXPECT_FALSE(m_orchestrator.setSequenceEnable("Basic", false));
    generateAndWait();

    const auto revision = m_orchestrator.getSolutionRevision();
    EXPECT_FALSE(m_orchestrator.setSequenceEnable("Nope", false));
    EXPECT_FALSE(m_orchestrator.setTestEnable("Basic", "Nope", false));
    EXPECT_FALSE(m_orchestrator.setTestEnable("Nope", "T1", false));
    EXPECT_EQ(m_orchestrator.getSolutionRevision(), revision);
    EXPECT_TRUE(m_orchestrator.getSolution().sequences.at("Basic").enabled);

    EXPECT_TRUE(m_orchestrator.setTestEnable("Basic", "T1", false));
    EXPECT_FALSE(m_orchestrator.getSolution().sequences.at("Basic").tests.at("T1").enabled);
}

// =============================================================================
// Enable flags across loads
// =============================================================================

TEST_F(OrchestratorSolutionTest, SameProductReloadKeepsFlags)
{
    ASSERT_TRUE(load("a"));
    generateAndWait();
    m_orchestrator.setSequenceEnable("Validation", false);
    m_orchestrator.setTestEnable("Basic", "T2", false);

    ASSERT_TRUE(load("a"));
    // Kept even before regenerating.
    EXPECT_FALSE(m_orchestrator.getSolution().sequences.at("Validation").enabled);
    generateAndWait();

    const Solution solution = m_orchestrator.getSolution();
    EXPECT_FALSE(solution.sequences.at("Validation").enabled);
    EXPECT_FALSE(solution.sequences.at("Basic").tests.at("T2").enabled);
    EXPECT_TRUE(solution.sequences.at("Basic").tests.at("T1").enabled);
}

TEST_F(OrchestratorSolutionTest, ProductSwitchClearsFlags)
{
    ASSERT_TRUE(load("a"));
    generateAndWait();
    m_orchestrator.setSequenceEnable("Basic", false);

    ASSERT_TRUE(load("b"));
    EXPECT_TRUE(m_orchestrator.getSolution().sequences.empty());
    generateAndWait();
    EXPECT_EQ(m_orchestrator.getSolution().sequenceOrder, (Names {"Other"}));

    // Back to A: nothing carried over.
    ASSERT_TRUE(load("a"));
    generateAndWait();
    EXPECT_TRUE(m_orchestrator.getSolution().sequences.at("Basic").enabled);
}

// =============================================================================
// Concurrency
// =============================================================================

TEST_F(OrchestratorSolutionTest, CallbackSetterWaitsForGeneration)
{
    ASSERT_TRUE(load("a"));
    const auto revision = m_orchestrator.getSolutionRevision();
    setMarker("slow");
    ASSERT_TRUE(m_orchestrator.generate());
    ASSERT_TRUE(waitStarted());

    // The generation may be calling the callback being replaced: the setter must wait for it.
    m_orchestrator.setLoadUserValues([](sol::state_view lua) { return lua.create_table(); });
    EXPECT_EQ(m_orchestrator.getSolutionRevision(), revision + 1);
}

TEST_F(OrchestratorSolutionTest, LoadWaitsForGeneration)
{
    ASSERT_TRUE(load("a"));
    const auto revision = m_orchestrator.getSolutionRevision();
    setMarker("slow");
    ASSERT_TRUE(m_orchestrator.generate());
    ASSERT_TRUE(waitStarted());

    // Same product: no clear, so the only bump comes from the generation, which must be done by now.
    ASSERT_TRUE(load("a"));
    EXPECT_EQ(m_orchestrator.getSolutionRevision(), revision + 1);
    EXPECT_EQ(m_orchestrator.getSolution().sequenceOrder, (Names {"Basic", "Validation"}));
}

TEST_F(OrchestratorSolutionTest, CopiesAreNeverHalfBuilt)
{
    std::atomic<bool> stop       = false;
    std::atomic<int>  copies     = 0;
    std::atomic<int>  violations = 0;
    std::thread       reader([&] {
        while (!stop) {
            const Solution solution = m_orchestrator.getSolution();
            ++copies;
            bool ok = solution.sequenceOrder.size() == solution.sequences.size();
            for (const auto& name : solution.sequenceOrder) {
                const auto it = solution.sequences.find(name);
                if (it == solution.sequences.end()) {
                    ok = false;
                    continue;
                }
                ok = ok && it->second.state.size() == s_uutCount + 1;
                ok = ok && it->second.testOrder.size() == it->second.tests.size();
                for (const auto& [tName, test] : it->second.tests) {
                    ok = ok && test.state.size() == s_uutCount + 1;
                }
            }
            if (!ok) { ++violations; }
        }
    });

    // No ASSERT in the test body here: the reader must always be stopped and joined.
    for (const auto* product : {"a", "b", "a", "a", "b"}) {
        EXPECT_TRUE(load(product));
        generateAndWait();
    }
    stop = true;
    reader.join();

    EXPECT_GT(copies.load(), 0);
    EXPECT_EQ(violations.load(), 0);
}

// =============================================================================
// GenerateOnLoad
// =============================================================================

TEST_F(OrchestratorSolutionTest, GenerateOnLoadOncePerLoad)
{
    Frasy::Lua::GenerateOnLoad generateOnLoad;
    EXPECT_FALSE(generateOnLoad.update(m_orchestrator));    // Nothing loaded.

    ASSERT_TRUE(load("a"));
    EXPECT_TRUE(generateOnLoad.update(m_orchestrator));
    ASSERT_TRUE(waitGeneration());
    EXPECT_EQ(m_orchestrator.getSolution().sequenceOrder, (Names {"Basic", "Validation"}));
    EXPECT_FALSE(generateOnLoad.update(m_orchestrator));    // Already handled.

    ASSERT_TRUE(load("b"));
    EXPECT_TRUE(generateOnLoad.update(m_orchestrator));
    ASSERT_TRUE(waitGeneration());
    EXPECT_EQ(m_orchestrator.getSolution().sequenceOrder, (Names {"Other"}));
}

TEST_F(OrchestratorSolutionTest, GenerateOnLoadRetriesWhenBusy)
{
    Frasy::Lua::GenerateOnLoad generateOnLoad;
    ASSERT_TRUE(load("a"));
    setMarker("slow");
    ASSERT_TRUE(m_orchestrator.generate());    // Busy with another generation.

    EXPECT_FALSE(generateOnLoad.update(m_orchestrator));
    ASSERT_TRUE(waitGeneration());
    // The load wasn't consumed while busy.
    EXPECT_TRUE(generateOnLoad.update(m_orchestrator));
    ASSERT_TRUE(waitGeneration());
    EXPECT_FALSE(generateOnLoad.update(m_orchestrator));
}
