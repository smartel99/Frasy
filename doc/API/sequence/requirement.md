# class `RequirementSpecifier`

# `function RequirementSpecifier.Requires(self, clause: any [, ...]) -> `[`RequirementSpecifier`](#class-requirementspecifier)
```lua
{
    ToPass,         -- Requires(Test("OtherTest"):ToPass()), this test can only be run if OtherTest succeeds.
    ToFail,         -- Requires(Test("OtherTest"):ToFail()), this test can only be run if OtherTest fails.
    ToBeCompleted,  -- Requires(Test("OtherTest"):ToBeCompleted()), this test can only be run if OtherTest is completed
    ToPrecede,      -- Requires(Test():ToPrecede(Test("OtherTest"))), this test needs to run right before OtherTest
    ToFollow,       -- Requires(Test():ToFollow(Test("OtherTest"))), this test needs to run right after OtherTest
    ToBeFirst,      -- Requires(Test():ToBeFirst()), this test needs to be first. Can only be on the current test, and can only be one (per sequence).
    ToBeLast,       -- Requires(Test():ToBeLast()), this test needs to be last. Can only be on the current test, and can only be one (per sequence).
}
```