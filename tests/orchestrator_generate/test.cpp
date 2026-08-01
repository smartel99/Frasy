#include "orchestrator_test_fixture.h"

#include <gtest/gtest.h>

// =============================================================================
// Helper: extract sequence names from the solution in order
// The solution structure is: [ section[ stage[ {name, tests} ] ] ]
// =============================================================================

class GenerateTest : public OrchestratorTestFixture
{
protected:
    /// Define sequences/tests via Lua script, then call Generate()
    void generate()
    {
        lua.script("Orchestrator.Generate()");
    }

    /// Get the number of sections in the solution
    int sectionCount()
    {
        return lua.script("return #Context.orchestrator.solution").get<int>();
    }

    /// Get flattened sequence execution order from the solution
    /// Returns a Lua table (array) of sequence names in execution order
    void buildOrderList()
    {
        lua.script(R"(
            __order = {}
            for _, section in ipairs(Context.orchestrator.solution) do
                for _, stage in ipairs(section) do
                    for _, seq in ipairs(stage) do
                        table.insert(__order, seq.name)
                    end
                end
            end
        )");
    }

    /// Get the position (1-indexed) of a sequence in the flattened order
    int positionOf(const std::string& name)
    {
        return lua.script(
            "for i, n in ipairs(__order) do if n == '" + name + "' then return i end end; return -1"
        ).get<int>();
    }

    /// Get total number of sequences in the flattened order
    int orderSize()
    {
        return lua.script("return #__order").get<int>();
    }

    /// Get flattened test execution order for a given sequence
    void buildTestOrderList(const std::string& seqName)
    {
        lua.script(R"(
            __test_order = {}
            for _, section in ipairs(Context.orchestrator.solution) do
                for _, stage in ipairs(section) do
                    for _, seq in ipairs(stage) do
                        if seq.name == ')" + seqName + R"(' then
                            for _, tstage in ipairs(seq.tests) do
                                for _, t in ipairs(tstage) do
                                    table.insert(__test_order, t)
                                end
                            end
                        end
                    end
                end
            end
        )");
    }

    int testPositionOf(const std::string& name)
    {
        return lua.script(
            "for i, n in ipairs(__test_order) do if n == '" + name + "' then return i end end; return -1"
        ).get<int>();
    }
};

// =============================================================================
// Simple case — sequences and tests with no requirements
// =============================================================================

TEST_F(GenerateTest, SimpleSequenceWithTests)
{
    lua.script(R"(
        Sequence("Power", function()
            Test("CheckVoltage", function() end)
            Test("CheckCurrent", function() end)
        end)
    )");
    generate();
    buildOrderList();
    EXPECT_EQ(orderSize(), 1);
    EXPECT_EQ(positionOf("Power"), 1);

    // Verify tests are present in the solution
    buildTestOrderList("Power");
    EXPECT_NE(testPositionOf("CheckVoltage"), -1);
    EXPECT_NE(testPositionOf("CheckCurrent"), -1);
}

TEST_F(GenerateTest, MultipleIndependentSequences)
{
    lua.script(R"(
        Sequence("SeqA", function()
            Test("T1", function() end)
        end)
        Sequence("SeqB", function()
            Test("T1", function() end)
        end)
        Sequence("SeqC", function()
            Test("T1", function() end)
        end)
    )");
    generate();
    buildOrderList();
    EXPECT_EQ(orderSize(), 3);
    // All should be present (order may vary since no dependencies)
    EXPECT_NE(positionOf("SeqA"), -1);
    EXPECT_NE(positionOf("SeqB"), -1);
    EXPECT_NE(positionOf("SeqC"), -1);
}

// =============================================================================
// Sequence ordering via ToPass (dependency)
// =============================================================================

TEST_F(GenerateTest, SequenceOrdering_ToPass)
{
    lua.script(R"(
        Sequence("Init", function()
            Test("Setup", function() end)
        end)
        Sequence("Run", function()
            Requires(Sequence("Init"):ToPass())
            Test("Execute", function() end)
        end)
    )");
    generate();
    buildOrderList();
    EXPECT_EQ(orderSize(), 2);
    EXPECT_LT(positionOf("Init"), positionOf("Run"));
}

TEST_F(GenerateTest, ThreeSequenceChain)
{
    lua.script(R"(
        Sequence("A", function()
            Test("T", function() end)
        end)
        Sequence("B", function()
            Requires(Sequence("A"):ToPass())
            Test("T", function() end)
        end)
        Sequence("C", function()
            Requires(Sequence("B"):ToPass())
            Test("T", function() end)
        end)
    )");
    generate();
    buildOrderList();
    EXPECT_EQ(orderSize(), 3);
    EXPECT_LT(positionOf("A"), positionOf("B"));
    EXPECT_LT(positionOf("B"), positionOf("C"));
}

// =============================================================================
// Test ordering within a sequence
// =============================================================================

TEST_F(GenerateTest, TestOrdering_ToPass)
{
    lua.script(R"(
        Sequence("Seq", function()
            Test("T1", function() end)
            Test("T2", function()
                Requires(Test("T1"):ToPass())
            end)
            Test("T3", function()
                Requires(Test("T2"):ToPass())
            end)
        end)
    )");
    generate();
    buildTestOrderList("Seq");
    EXPECT_LT(testPositionOf("T1"), testPositionOf("T2"));
    EXPECT_LT(testPositionOf("T2"), testPositionOf("T3"));
}

// =============================================================================
// First / Last edge requirements
// =============================================================================

TEST_F(GenerateTest, SequenceFirst)
{
    lua.script(R"(
        Sequence("Middle", function()
            Test("T", function() end)
        end)
        Sequence("First", function()
            Requires(Sequence():ToBeFirst())
            Test("T", function() end)
        end)
        Sequence("Last", function()
            Test("T", function() end)
        end)
    )");
    generate();
    buildOrderList();
    EXPECT_EQ(positionOf("First"), 1);
}

TEST_F(GenerateTest, SequenceLast)
{
    lua.script(R"(
        Sequence("First", function()
            Test("T", function() end)
        end)
        Sequence("Last", function()
            Requires(Sequence():ToBeLast())
            Test("T", function() end)
        end)
        Sequence("Middle", function()
            Test("T", function() end)
        end)
    )");
    generate();
    buildOrderList();
    EXPECT_EQ(positionOf("Last"), orderSize());
}

TEST_F(GenerateTest, TestFirst)
{
    lua.script(R"(
        Sequence("Seq", function()
            Test("Middle", function() end)
            Test("First", function()
                Requires(Test():ToBeFirst())
            end)
            Test("Last", function() end)
        end)
    )");
    generate();
    buildTestOrderList("Seq");
    EXPECT_EQ(testPositionOf("First"), 1);
}

TEST_F(GenerateTest, TestLast)
{
    lua.script(R"(
        Sequence("Seq", function()
            Test("First", function() end)
            Test("Last", function()
                Requires(Test():ToBeLast())
            end)
            Test("Middle", function() end)
        end)
    )");
    generate();
    buildTestOrderList("Seq");
    auto size = lua.script("return #__test_order").get<int>();
    EXPECT_EQ(testPositionOf("Last"), size);
}

// =============================================================================
// Sync requirements — splits solution into sections
// =============================================================================

TEST_F(GenerateTest, SyncSplitsSections)
{
    lua.script(R"(
        Sequence("SeqA", function()
            Test("T1", function()
                Sync()
            end)
        end)
        Sequence("SeqB", function()
            Test("T1", function() end)
        end)
    )");
    generate();
    // The sync should cause the solution to have more than one section
    EXPECT_GT(sectionCount(), 1);
}

// =============================================================================
// Error cases
// =============================================================================

TEST_F(GenerateTest, NoSequencesThrows)
{
    // Don't define any sequences
    sol::protected_function_result result = lua.safe_script(
        "Orchestrator.Generate()", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}

TEST_F(GenerateTest, CircularDependencyThrows)
{
    lua.script(R"(
        Sequence("A", function()
            Requires(Sequence("B"):ToPass())
            Test("T", function() end)
        end)
        Sequence("B", function()
            Requires(Sequence("A"):ToPass())
            Test("T", function() end)
        end)
    )");
    sol::protected_function_result result = lua.safe_script(
        "Orchestrator.Generate()", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}

// =============================================================================
// Forward references — generation retries and succeeds
// =============================================================================

TEST_F(GenerateTest, ForwardReferenceSucceeds)
{
    // B references A, but B is defined first
    // Generate should retry and succeed once A is defined
    lua.script(R"(
        Sequence("B", function()
            Requires(Sequence("A"):ToPass())
            Test("T", function() end)
        end)
        Sequence("A", function()
            Test("T", function() end)
        end)
    )");
    generate(); // Should not throw
    buildOrderList();
    EXPECT_EQ(orderSize(), 2);
    EXPECT_LT(positionOf("A"), positionOf("B"));
}

TEST_F(GenerateTest, ForwardReferenceTestLevel)
{
    // T2 references T1, but T2 is defined first in the function body
    lua.script(R"(
        Sequence("Seq", function()
            Test("T2", function()
                Requires(Test("T1"):ToPass())
            end)
            Test("T1", function() end)
        end)
    )");
    generate();
    buildTestOrderList("Seq");
    EXPECT_LT(testPositionOf("T1"), testPositionOf("T2"));
}
