#include "lua_test_fixture.h"

#include <gtest/gtest.h>

class SortUtilsTest : public LuaTestFixture
{
protected:
    void SetUp() override
    {
        LuaTestFixture::SetUp();
        lua.script("Sort = require('lua/core/framework/sort_utils')");
    }

    /// Helper: get the number of layers in a sorted result
    int getLayerCount(const std::string& varName)
    {
        return lua.script("return #" + varName).get<int>();
    }

    /// Helper: check if a scope name is in a specific layer
    bool scopeInLayer(const std::string& varName, int layerIndex, const std::string& scopeName)
    {
        return lua.script(
            "for _, name in ipairs(" + varName + "[" + std::to_string(layerIndex) + "]) do "
            "if name == '" + scopeName + "' then return true end "
            "end; return false"
        ).get<bool>();
    }
};

// =============================================================================
// Sort.HasMetDependencies
// =============================================================================

TEST_F(SortUtilsTest, HasMetDependencies_AllMet)
{
    auto result = lua.script(R"(
        -- requirement has deps on "A" and "B", scopes remaining is {"C", "D"}
        -- A and B are not in remaining scopes, so deps are met
        return Sort.HasMetDependencies({A = true, B = true}, {"C", "D"})
    )").get<bool>();
    EXPECT_TRUE(result);
}

TEST_F(SortUtilsTest, HasMetDependencies_UnmetDependency)
{
    auto result = lua.script(R"(
        -- requirement has dep on "A", scopes remaining is {"A", "B"}
        -- A is still in remaining scopes, so dep is NOT met
        return Sort.HasMetDependencies({A = true}, {"A", "B"})
    )").get<bool>();
    EXPECT_FALSE(result);
}

TEST_F(SortUtilsTest, HasMetDependencies_NoDependencies)
{
    auto result = lua.script(R"(
        return Sort.HasMetDependencies({}, {"A", "B", "C"})
    )").get<bool>();
    EXPECT_TRUE(result);
}

// =============================================================================
// Sort.SortScopes — basic cases
// =============================================================================

TEST_F(SortUtilsTest, SortScopes_NoDependencies)
{
    lua.script(R"(
        local scopes = {"A", "B", "C"}
        local requirements = { A = {}, B = {}, C = {} }
        result = Sort.SortScopes(scopes, nil, nil, requirements)
    )");
    // All in one layer since no dependencies
    EXPECT_EQ(getLayerCount("result"), 1);
    EXPECT_TRUE(scopeInLayer("result", 1, "A"));
    EXPECT_TRUE(scopeInLayer("result", 1, "B"));
    EXPECT_TRUE(scopeInLayer("result", 1, "C"));
}

TEST_F(SortUtilsTest, SortScopes_LinearChain)
{
    lua.script(R"(
        -- C depends on B, B depends on A
        local scopes = {"A", "B", "C"}
        local requirements = {
            A = {},
            B = { A = true },
            C = { B = true },
        }
        result = Sort.SortScopes(scopes, nil, nil, requirements)
    )");
    // Should produce 3 layers: {A}, {B}, {C}
    EXPECT_EQ(getLayerCount("result"), 3);
    EXPECT_TRUE(scopeInLayer("result", 1, "A"));
    EXPECT_TRUE(scopeInLayer("result", 2, "B"));
    EXPECT_TRUE(scopeInLayer("result", 3, "C"));
}

TEST_F(SortUtilsTest, SortScopes_DiamondDependency)
{
    lua.script(R"(
        -- B and C depend on A, D depends on B and C
        local scopes = {"A", "B", "C", "D"}
        local requirements = {
            A = {},
            B = { A = true },
            C = { A = true },
            D = { B = true, C = true },
        }
        result = Sort.SortScopes(scopes, nil, nil, requirements)
    )");
    // Layer 1: A, Layer 2: B and C, Layer 3: D
    EXPECT_EQ(getLayerCount("result"), 3);
    EXPECT_TRUE(scopeInLayer("result", 1, "A"));
    EXPECT_TRUE(scopeInLayer("result", 2, "B"));
    EXPECT_TRUE(scopeInLayer("result", 2, "C"));
    EXPECT_TRUE(scopeInLayer("result", 3, "D"));
}

// =============================================================================
// Sort.SortScopes — first/last edges
// =============================================================================

TEST_F(SortUtilsTest, SortScopes_FirstEdge)
{
    lua.script(R"(
        local scopes = {"A", "B", "C"}
        local requirements = { A = {}, B = {}, C = {} }
        result = Sort.SortScopes(scopes, "A", nil, requirements)
    )");
    // "A" should be in a layer by itself at the front
    EXPECT_EQ(getLayerCount("result"), 2);
    EXPECT_TRUE(scopeInLayer("result", 1, "A"));
    EXPECT_TRUE(scopeInLayer("result", 2, "B"));
    EXPECT_TRUE(scopeInLayer("result", 2, "C"));
}

TEST_F(SortUtilsTest, SortScopes_LastEdge)
{
    lua.script(R"(
        local scopes = {"A", "B", "C"}
        local requirements = { A = {}, B = {}, C = {} }
        result = Sort.SortScopes(scopes, nil, "C", requirements)
    )");
    // "C" should be in a layer by itself at the end
    EXPECT_EQ(getLayerCount("result"), 2);
    EXPECT_TRUE(scopeInLayer("result", 1, "A"));
    EXPECT_TRUE(scopeInLayer("result", 1, "B"));
    EXPECT_TRUE(scopeInLayer("result", 2, "C"));
}

TEST_F(SortUtilsTest, SortScopes_FirstAndLast)
{
    lua.script(R"(
        local scopes = {"A", "B", "C"}
        local requirements = { A = {}, B = {}, C = {} }
        result = Sort.SortScopes(scopes, "A", "C", requirements)
    )");
    // A first, C last, B in middle
    EXPECT_EQ(getLayerCount("result"), 3);
    EXPECT_TRUE(scopeInLayer("result", 1, "A"));
    EXPECT_TRUE(scopeInLayer("result", 2, "B"));
    EXPECT_TRUE(scopeInLayer("result", 3, "C"));
}

// =============================================================================
// Sort.SortScopes — error cases
// =============================================================================

TEST_F(SortUtilsTest, SortScopes_CircularDependencyThrows)
{
    sol::protected_function_result result = lua.safe_script(R"(
        local scopes = {"A", "B"}
        local requirements = {
            A = { B = true },
            B = { A = true },
        }
        Sort.SortScopes(scopes, nil, nil, requirements)
    )", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}

TEST_F(SortUtilsTest, SortScopes_NilScopes)
{
    lua.script("result = Sort.SortScopes(nil, nil, nil, {})");
    auto result = lua.script("return type(result)").get<std::string>();
    EXPECT_EQ(result, "table");
    EXPECT_EQ(getLayerCount("result"), 0);
}

// =============================================================================
// Sort.Sectionize
// =============================================================================

TEST_F(SortUtilsTest, Sectionize_NoSyncRequirements)
{
    lua.script(R"(
        local stages = { {"A", "B"}, {"C"} }
        local requirements = {}
        result = Sort.Sectionize(stages, requirements)
    )");
    // No sync requirements, everything in one section
    auto sectionCount = lua.script("return #result").get<int>();
    EXPECT_EQ(sectionCount, 1);
}

TEST_F(SortUtilsTest, Sectionize_WithSyncRequirement)
{
    lua.script(R"(
        local stages = { {"A", "B"}, {"C"} }
        local requirements = { B = true }
        result = Sort.Sectionize(stages, requirements)
    )");
    // B has sync requirement, splits into sections
    auto sectionCount = lua.script("return #result").get<int>();
    EXPECT_GT(sectionCount, 1);
}

TEST_F(SortUtilsTest, Sectionize_MultipleSyncs)
{
    lua.script(R"(
        local stages = { {"A", "B", "C"} }
        local requirements = { A = true, C = true }
        result = Sort.Sectionize(stages, requirements)
    )");
    // A and C both have sync requirements
    auto sectionCount = lua.script("return #result").get<int>();
    EXPECT_GE(sectionCount, 2);
}

TEST_F(SortUtilsTest, Sectionize_NilStages)
{
    lua.script("result = Sort.Sectionize(nil, {})");
    auto result = lua.script("return type(result)").get<std::string>();
    EXPECT_EQ(result, "table");
    auto count = lua.script("return #result").get<int>();
    EXPECT_EQ(count, 0);
}

// =============================================================================
// Sort.CombineSectionized
// =============================================================================

TEST_F(SortUtilsTest, CombineSectionized_BasicCase)
{
    lua.script(R"(
        -- Simple case: one sequence with one section of tests
        local sectionizedSequences = { { {"S1"} } }
        local sectionizedTests = { S1 = { { {"T1", "T2"} } } }
        result = Sort.CombineSectionized(sectionizedSequences, sectionizedTests)
    )");
    // Should produce a non-empty output
    auto count = lua.script("return #result").get<int>();
    EXPECT_GT(count, 0);
}

TEST_F(SortUtilsTest, CombineSectionized_MultipleSequences)
{
    lua.script(R"(
        local sectionizedSequences = { { {"S1", "S2"} } }
        local sectionizedTests = {
            S1 = { { {"T1"} } },
            S2 = { { {"T1"} } },
        }
        result = Sort.CombineSectionized(sectionizedSequences, sectionizedTests)
    )");
    auto count = lua.script("return #result").get<int>();
    EXPECT_GT(count, 0);
}
