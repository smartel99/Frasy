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

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using Frasy::Lua::Orchestrator;
using Frasy::Models::ExecutionState;
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

// Runs: one passing sequence, one with a failing test.
constexpr auto s_productRun = R"(
Sequence("Good", function()
    Test("G1", function() Expect(1, "One"):ToBeEqual(1) end)
    Test("G2", function()
        Requires(Test("G1"):ToPass())
        Expect(true, "Truth"):ToBeTrue()
    end)
end)
Sequence("Bad", function()
    Requires(Sequence("Good"):ToPass())
    Test("B1", function() Expect(false, "Falsehood"):ToBeTrue() end)
    Test("B2", function() Expect(2, "Two"):ToBeEqual(2) end)
end)
)";

// Runs: X2 aborts the UUT (stopAll) when the "abort_now" marker exists, so no end event is reported.
constexpr auto s_productAbort = R"(
Sequence("Abort", function()
    Test("X1", function() end)
    Test("X2", function()
        Requires(Test("X1"):ToPass())
        if exists("abort_now") then Expect(false, "Abort", { policy = ErrorPolicy.stopAll }):ToBeTrue() end
    end)
    Test("X3", function() Requires(Test("X2"):ToPass()) end)
end)
)";

// Runs: during execution, L2 waits for the "release" marker, after writing "started_<uut>".
constexpr auto s_productLive = R"(
Sequence("Live", function()
    Test("L1", function() end)
    Test("L2", function()
        Requires(Test("L1"):ToPass())
        -- Test bodies also run during generation (to collect requirements): wait only when executing.
        if Context.info.stage ~= Stage.execution then return end
        mark("started_" .. Context.info.uut)
        while not exists("release") do SleepFor(5) end
    end)
end)
)";

// Runs: Sync splits "Split" across sections (S2 first, then S1). During execution, S1 waits for "release".
constexpr auto s_productSplit = R"(
Sequence("Split", function()
    Test("S1", function()
        Sync()
        if Context.info.stage ~= Stage.execution then return end
        mark("started_" .. Context.info.uut)
        while not exists("release") do SleepFor(5) end
    end)
    Test("S2", function() end)
end)
Sequence("Other", function()
    Test("O1", function() end)
end)
)";

// Runs: skipped and disabled scopes. "Setup" runs first and disables in Lua with enable_list directly
// (Orchestrator.Disable(seq, test) writes a shape IsScopeEnabled can't read until Phase 6a).
constexpr auto s_productSkips = R"(
Sequence("Setup", function()
    Requires(Sequence():ToBeFirst())
    if Context.info.stage == Stage.execution then
        Context.orchestrator.enable_list.Off = { enabled = false }
        Context.orchestrator.enable_list.Partial = { enabled = true, P1 = { enabled = true }, P2 = { enabled = false } }
    end
    Test("S1", function() end)
end)
Sequence("Gate", function()
    Test("G1", function() Expect(false, "Falsehood"):ToBeTrue() end)
end)
Sequence("Gated", function()
    Requires(Sequence("Gate"):ToPass())
    Test("D1", function() end)
end)
Sequence("Inner", function()
    Test("T1", function() Expect(false, "Falsehood"):ToBeTrue() end)
    Test("T2", function() Requires(Test("T1"):ToPass()) end)
end)
Sequence("Off", function()
    Test("O1", function() end)
end)
Sequence("Partial", function()
    Test("P1", function() end)
    Test("P2", function() end)
end)
)";

// Runs: the sequence body raises a string error when executing, so its tests never run.
constexpr auto s_productCrash = R"(
Sequence("Crash", function()
    if Context.info.stage == Stage.execution then error("Crash on purpose") end
    Test("K1", function() end)
end)
)";

// Folder names: also the run titles, so they must not match a marker name.
const std::vector<std::pair<std::string, const char*>> s_products = {
  {"a", s_productA},
  {"b", s_productB},
  {"run", s_productRun},
  {"abort", s_productAbort},
  {"live", s_productLive},
  {"split", s_productSplit},
  {"crash", s_productCrash},
  {"skips", s_productSkips},
};
}    // namespace

class OrchestratorSolutionTest : public ::testing::Test {
protected:
    struct Event {
        std::string type;
        std::size_t uut;
        std::string name;
        std::string parent;
        bool        pass;
    };

    std::filesystem::path    m_root;
    std::vector<std::string> m_serials = {"S1", "S1", "S2"};    // Index 0 is a copy of UUT 1, as in the apps.
    // Progress callback storage, declared before m_orchestrator so it outlives a run (its destructor waits).
    std::mutex         m_eventsMutex;
    std::vector<Event> m_events;
    Orchestrator       m_orchestrator;

    static void SetUpTestSuite()
    {
        // Once per process: a second Init() makes the rotating file sink rename its own open file.
        static std::once_flag once;
        std::call_once(once, [] { Brigerad::Log::Init(true, true); });
    }

    void SetUp() override
    {
#ifndef BR_DEBUG
        GTEST_SKIP() << "Debug only: loadUserFiles() verifies the lua/user hash in other configurations.";
#endif
        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        // Relative and dot-free: products are loaded with Lua's require().
        m_root = std::filesystem::path("orchestrator_solution_products") / info->name();
        std::filesystem::remove_all(m_root);
        for (const auto& [name, tests] : s_products) {
            writeProduct(name, tests);
        }
    }

    void TearDown() override
    {
        if (m_root.empty()) { return; }    // Skipped.
        // Let a pending run or generation finish before the files go away.
        setMarker("release");
        waitRun();
        waitGeneration();
        std::error_code ec;
        std::filesystem::remove_all(m_root, ec);
        // Run outputs: logs/last and logs/<title>, the title being the product's folder name.
        // Not logs/ itself, which holds the logger's open file.
        std::filesystem::remove_all("logs/last", ec);
        for (const auto& [name, tests] : s_products) {
            std::filesystem::remove_all("logs/" + name, ec);
        }
    }

    void writeProduct(const std::string& name, const std::string& tests) const
    {
        const auto dir = m_root / name;
        std::filesystem::create_directories(dir / "tests");
        std::ofstream(dir / "environment.lua")
          << std::format("Environment.Make(function() Environment.Uut.Count({}) end)\n", s_uutCount);
        // Markers are checked when the tests are loaded: "broken" by every Lua state, "slow" by generation only.
        // `root`, `exists` and `mark` are visible to the product's tests (same chunk).
        std::ofstream(dir / "tests" / "tests.lua") << std::format(R"(
local root = "{0}"
local function exists(name)
    local f = io.open(root .. "/" .. name, "r")
    if f then f:close() return true end
    return false
end
local function mark(name) io.open(root .. "/" .. name, "w"):close() end
-- Generation only: loadUserFiles() loads as UUT 0, generation as UUT 1.
if exists("slow") and Context.info.uut ~= 0 and Context.info.stage == Stage.generation then
    mark("started")
    local start = os.clock()
    while os.clock() - start < 0.5 do end
end
if exists("broken") then error("Broken on purpose") end
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

    /// Wait for a marker written by Lua. "started": a slow generation reached its busy loop, so it holds its lock.
    [[nodiscard]] bool waitStarted(const std::string& marker = "started") const
    {
        const auto deadline = std::chrono::steady_clock::now() + 30s;
        while (!std::filesystem::exists(m_root / marker)) {
            if (std::chrono::steady_clock::now() > deadline) { return false; }
            std::this_thread::sleep_for(1ms);
        }
        return true;
    }

    bool waitRun()
    {
        const auto deadline = std::chrono::steady_clock::now() + 60s;
        while (m_orchestrator.isRunning()) {
            if (std::chrono::steady_clock::now() > deadline) { return false; }
            std::this_thread::sleep_for(5ms);
        }
        return true;
    }

    /// Run the loaded product on every UUT, without verification. Doesn't wait.
    void run() { m_orchestrator.runSolution("operator", m_serials, false, true); }

    [[nodiscard]] bool runAndWait()
    {
        run();
        return waitRun();
    }

    /// State of a sequence (`test` empty) or test for a UUT.
    [[nodiscard]] static ExecutionState stateOf(const Solution&    solution,
                                                std::size_t        uut,
                                                const std::string& sequence,
                                                const std::string& test = "")
    {
        const auto& sq = solution.sequences.at(sequence);
        return test.empty() ? sq.state.at(uut) : sq.tests.at(test).state.at(uut);
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
    // Busy with another generation, which fails: the load is still not generated afterwards.
    setMarker("slow");
    setMarker("broken");
    ASSERT_TRUE(m_orchestrator.generate());

    EXPECT_FALSE(generateOnLoad.update(m_orchestrator));
    ASSERT_TRUE(waitGeneration());
    std::filesystem::remove(m_root / "slow");
    std::filesystem::remove(m_root / "broken");

    // The load wasn't consumed while busy.
    EXPECT_TRUE(generateOnLoad.update(m_orchestrator));
    ASSERT_TRUE(waitGeneration());
    EXPECT_EQ(m_orchestrator.getSolution().sequenceOrder, (Names {"Basic", "Validation"}));
    EXPECT_FALSE(generateOnLoad.update(m_orchestrator));
}

TEST_F(OrchestratorSolutionTest, GenerateOnLoadSkipsGeneratedLoad)
{
    Frasy::Lua::GenerateOnLoad generateOnLoad;
    ASSERT_TRUE(load("a"));
    generateAndWait();    // Generated by someone else (viewer button, run).
    const auto revision = m_orchestrator.getSolutionRevision();

    EXPECT_FALSE(generateOnLoad.update(m_orchestrator));
    EXPECT_FALSE(m_orchestrator.isGenerating());
    EXPECT_EQ(m_orchestrator.getSolutionRevision(), revision);
}

// =============================================================================
// Runs: live states
// =============================================================================

TEST_F(OrchestratorSolutionTest, RunTracksPassAndFail)
{
    ASSERT_TRUE(load("run"));
    ASSERT_TRUE(runAndWait());

    const Solution solution = m_orchestrator.getSolution();
    for (std::size_t uut = 1; uut <= s_uutCount; ++uut) {
        SCOPED_TRACE(std::format("UUT {}", uut));
        EXPECT_EQ(stateOf(solution, uut, "Good"), ExecutionState::passed);
        EXPECT_EQ(stateOf(solution, uut, "Good", "G1"), ExecutionState::passed);
        EXPECT_EQ(stateOf(solution, uut, "Good", "G2"), ExecutionState::passed);
        EXPECT_EQ(stateOf(solution, uut, "Bad"), ExecutionState::failed);
        EXPECT_EQ(stateOf(solution, uut, "Bad", "B1"), ExecutionState::failed);
        EXPECT_EQ(stateOf(solution, uut, "Bad", "B2"), ExecutionState::passed);
    }
}

TEST_F(OrchestratorSolutionTest, RunShowsLiveStates)
{
    ASSERT_TRUE(load("live"));
    run();
    for (std::size_t uut = 1; uut <= s_uutCount; ++uut) {
        ASSERT_TRUE(waitStarted(std::format("started_{}", uut)));
    }

    // Both UUTs are inside L2. The report precedes the marker, so the state is already there.
    const Solution live = m_orchestrator.getSolution();
    for (std::size_t uut = 1; uut <= s_uutCount; ++uut) {
        SCOPED_TRACE(std::format("UUT {}", uut));
        EXPECT_EQ(stateOf(live, uut, "Live"), ExecutionState::running);
        EXPECT_EQ(stateOf(live, uut, "Live", "L1"), ExecutionState::passed);
        EXPECT_EQ(stateOf(live, uut, "Live", "L2"), ExecutionState::running);
    }
    EXPECT_TRUE(m_orchestrator.isRunning());

    const auto revision = m_orchestrator.getSolutionRevision();
    setMarker("release");
    ASSERT_TRUE(waitRun());
    EXPECT_GT(m_orchestrator.getSolutionRevision(), revision);
    const Solution done = m_orchestrator.getSolution();
    for (std::size_t uut = 1; uut <= s_uutCount; ++uut) {
        SCOPED_TRACE(std::format("UUT {}", uut));
        EXPECT_EQ(stateOf(done, uut, "Live"), ExecutionState::passed);
        EXPECT_EQ(stateOf(done, uut, "Live", "L2"), ExecutionState::passed);
    }
}

TEST_F(OrchestratorSolutionTest, RunAbortEndsAsError)
{
    ASSERT_TRUE(load("abort"));
    setMarker("abort_now");
    ASSERT_TRUE(runAndWait());

    const Solution solution = m_orchestrator.getSolution();
    for (std::size_t uut = 1; uut <= s_uutCount; ++uut) {
        SCOPED_TRACE(std::format("UUT {}", uut));
        EXPECT_EQ(stateOf(solution, uut, "Abort"), ExecutionState::error);
        EXPECT_EQ(stateOf(solution, uut, "Abort", "X1"), ExecutionState::passed);
        EXPECT_EQ(stateOf(solution, uut, "Abort", "X2"), ExecutionState::error);
        EXPECT_EQ(stateOf(solution, uut, "Abort", "X3"), ExecutionState::idle);    // Never reached.
    }
}

TEST_F(OrchestratorSolutionTest, RunResetsPreviousStates)
{
    ASSERT_TRUE(load("abort"));
    ASSERT_TRUE(runAndWait());
    ASSERT_EQ(stateOf(m_orchestrator.getSolution(), 1, "Abort", "X3"), ExecutionState::passed);

    // Same product, no reload: X3 isn't reached this time and must not keep the previous result.
    setMarker("abort_now");
    ASSERT_TRUE(runAndWait());
    EXPECT_EQ(stateOf(m_orchestrator.getSolution(), 1, "Abort", "X3"), ExecutionState::idle);
}

TEST_F(OrchestratorSolutionTest, RunDisabledUut)
{
    ASSERT_TRUE(load("run"));
    m_orchestrator.toggleUut(2);
    ASSERT_TRUE(runAndWait());

    const Solution solution = m_orchestrator.getSolution();
    EXPECT_EQ(stateOf(solution, 1, "Good"), ExecutionState::passed);
    for (const auto& [sName, sequence] : solution.sequences) {
        EXPECT_EQ(sequence.state.at(2), ExecutionState::disabled) << sName;
        for (const auto& [tName, test] : sequence.tests) {
            EXPECT_EQ(test.state.at(2), ExecutionState::disabled) << tName;
        }
    }
}

TEST_F(OrchestratorSolutionTest, RunStillForwardsProgressCallback)
{
    // Captures fixture members only: they outlive a run still going after a failed ASSERT.
    m_orchestrator.setProgressCallback([this](const std::string& type,
                                              std::size_t        uut,
                                              const std::string& name,
                                              const std::string& parentA,
                                              const std::string&,
                                              bool pass) {
        std::lock_guard lock(m_eventsMutex);
        m_events.push_back({type, uut, name, parentA, pass});
    });

    ASSERT_TRUE(load("run"));
    ASSERT_TRUE(runAndWait());
    m_orchestrator.setProgressCallback({});

    std::lock_guard lock(m_eventsMutex);
    const auto      has = [&](const std::string& type, std::size_t uut, const std::string& name, bool pass) {
        return std::ranges::any_of(
          m_events, [&](const Event& e) { return e.type == type && e.uut == uut && e.name == name && e.pass == pass; });
    };
    for (std::size_t uut = 1; uut <= s_uutCount; ++uut) {
        EXPECT_TRUE(has("sequence_start", uut, "Good", true)) << uut;
        EXPECT_TRUE(has("test_end", uut, "B1", false)) << uut;
        EXPECT_TRUE(has("sequence_end", uut, "Bad", false)) << uut;
        EXPECT_TRUE(has("expectation", uut, "Falsehood", false)) << uut;
    }
    // States are tracked as well.
    EXPECT_EQ(stateOf(m_orchestrator.getSolution(), 1, "Bad"), ExecutionState::failed);
}

TEST_F(OrchestratorSolutionTest, RunSplitSequenceStaysRunningBetweenSections)
{
    ASSERT_TRUE(load("split"));
    run();
    for (std::size_t uut = 1; uut <= s_uutCount; ++uut) {
        ASSERT_TRUE(waitStarted(std::format("started_{}", uut)));
    }

    // The scenario must really split "Split" across sections.
    const Solution live     = m_orchestrator.getSolution();
    std::size_t    sections = 0;
    for (const auto& section : live.sections) {
        bool found = false;
        for (const auto& stage : section) {
            for (const auto& [name, tests] : stage) {
                found = found || name == "Split";
            }
        }
        if (found) { ++sections; }
    }
    ASSERT_EQ(sections, 2u);

    // S2 ran in an earlier section, S1 holds in the last one: no end reported in between.
    for (std::size_t uut = 1; uut <= s_uutCount; ++uut) {
        SCOPED_TRACE(std::format("UUT {}", uut));
        EXPECT_EQ(stateOf(live, uut, "Split", "S2"), ExecutionState::passed);
        EXPECT_EQ(stateOf(live, uut, "Split", "S1"), ExecutionState::running);
        EXPECT_EQ(stateOf(live, uut, "Split"), ExecutionState::running);
    }

    setMarker("release");
    ASSERT_TRUE(waitRun());
    const Solution done = m_orchestrator.getSolution();
    for (std::size_t uut = 1; uut <= s_uutCount; ++uut) {
        SCOPED_TRACE(std::format("UUT {}", uut));
        EXPECT_EQ(stateOf(done, uut, "Split"), ExecutionState::passed);
        EXPECT_EQ(stateOf(done, uut, "Split", "S1"), ExecutionState::passed);
        EXPECT_EQ(stateOf(done, uut, "Other"), ExecutionState::passed);
    }
}

TEST_F(OrchestratorSolutionTest, RunSequenceStringErrorIsError)
{
    ASSERT_TRUE(load("crash"));
    ASSERT_TRUE(runAndWait());

    const Solution solution = m_orchestrator.getSolution();
    for (std::size_t uut = 1; uut <= s_uutCount; ++uut) {
        SCOPED_TRACE(std::format("UUT {}", uut));
        EXPECT_EQ(stateOf(solution, uut, "Crash"), ExecutionState::error);
        EXPECT_EQ(stateOf(solution, uut, "Crash", "K1"), ExecutionState::idle);    // Never reached.
    }
}

TEST_F(OrchestratorSolutionTest, GenerateOnLoadKeepsRunResults)
{
    // MCP run_tests order: load, run in the same frame, GenerateOnLoad only sees the load afterwards.
    Frasy::Lua::GenerateOnLoad generateOnLoad;
    ASSERT_TRUE(load("live"));
    run();
    ASSERT_TRUE(waitStarted("started_1"));
    EXPECT_FALSE(generateOnLoad.update(m_orchestrator));    // Running.

    setMarker("release");
    ASSERT_TRUE(waitRun());
    const auto revision = m_orchestrator.getSolutionRevision();
    EXPECT_FALSE(generateOnLoad.update(m_orchestrator));    // The run already generated this load.
    EXPECT_FALSE(m_orchestrator.isGenerating());
    EXPECT_EQ(m_orchestrator.getSolutionRevision(), revision);
    EXPECT_EQ(stateOf(m_orchestrator.getSolution(), 1, "Live"), ExecutionState::passed);

    // A new load is still generated.
    ASSERT_TRUE(load("live"));
    EXPECT_TRUE(generateOnLoad.update(m_orchestrator));
    ASSERT_TRUE(waitGeneration());
}

TEST_F(OrchestratorSolutionTest, RunReportsSkipsAndDisables)
{
    m_orchestrator.setProgressCallback([this](const std::string& type,
                                              std::size_t        uut,
                                              const std::string& name,
                                              const std::string& parentA,
                                              const std::string&,
                                              bool pass) {
        std::lock_guard lock(m_eventsMutex);
        m_events.push_back({type, uut, name, parentA, pass});
    });
    ASSERT_TRUE(load("skips"));
    ASSERT_TRUE(runAndWait());
    m_orchestrator.setProgressCallback({});

    using enum ExecutionState;
    const Solution solution = m_orchestrator.getSolution();
    for (std::size_t uut = 1; uut <= s_uutCount; ++uut) {
        SCOPED_TRACE(std::format("UUT {}", uut));
        EXPECT_EQ(stateOf(solution, uut, "Setup"), passed);
        EXPECT_EQ(stateOf(solution, uut, "Gate"), failed);
        // Skipped by an unmet sequence requirement: no end reported, and not an error.
        EXPECT_EQ(stateOf(solution, uut, "Gated"), skipped);
        EXPECT_EQ(stateOf(solution, uut, "Gated", "D1"), skipped);
        // Skipped by an unmet test requirement: not shown as passed.
        EXPECT_EQ(stateOf(solution, uut, "Inner", "T1"), failed);
        EXPECT_EQ(stateOf(solution, uut, "Inner", "T2"), skipped);
        EXPECT_EQ(stateOf(solution, uut, "Inner"), failed);
        // Disabled in Lua.
        EXPECT_EQ(stateOf(solution, uut, "Off"), disabled);
        EXPECT_EQ(stateOf(solution, uut, "Off", "O1"), disabled);
        EXPECT_EQ(stateOf(solution, uut, "Partial", "P1"), passed);
        EXPECT_EQ(stateOf(solution, uut, "Partial", "P2"), disabled);
        EXPECT_EQ(stateOf(solution, uut, "Partial"), failed);    // A disabled test never passes.
    }

    // The new types reach the user callback too.
    std::lock_guard lock(m_eventsMutex);
    const auto      has = [&](const std::string& type, const std::string& name) {
        return std::ranges::any_of(m_events, [&](const Event& e) { return e.type == type && e.name == name; });
    };
    EXPECT_TRUE(has("sequence_skipped", "Gated"));
    EXPECT_TRUE(has("sequence_disabled", "Off"));
    EXPECT_TRUE(has("test_skipped", "T2"));
    EXPECT_TRUE(has("test_disabled", "P2"));
}
