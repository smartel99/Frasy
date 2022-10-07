# Status
```lua
{
    ToPass,
    ToFail,
    ToBeCompleted,
    ToPrecede,      -- Requires(Test():ToPrecede(Test("OtherTest"))), this test needs to run right before OtherTest
    ToFollow,       -- Requires(Test():ToFollow(Test("OtherTest"))), this test needs to run right after OtherTest

}
```