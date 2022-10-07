Execution order:
1. S2.T2 - FAIL
2. S2.T3 - PASS
3. S2.T1 - PASS
4. S1.T1 - N/A


```lua
Sequence("S1", function(sCtx)
    Requires(Sequence("S2"):Test("T2"):ToPass())
    Test("T1", function(tCtx) Expect(true):ToBeFalse() end)
end)

Sequence("S2", function(sCtx)
    Requires(Sequence():ToBeFirst())

    Test("T1", function(tCtx)
        Requires(Test("T2"):ToFail())

        Expect(1):ToBeNear(2, 0.5):As("Num1")
        Expect("asdf"):ToBeEqual("asdf"):As("String1")
    end)

    Test("T2", function(tCtx)
        local num1 = Requires(Test("T1"):Value("Num1"))
    end)
end)
```
