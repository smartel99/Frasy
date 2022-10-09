## SequenceAlreadyDefined Exception
Triggered when a new [`Sequence`](sequence.md#function-sequencename-string-sequence-functionsequencecontextclass-sequencecontext---requirementspecifierrequirementmdclass-requirementspecifier) uses the name of an already defined sequence.

### Example
```lua
Sequence("MySequence", function(context) end)
Sequence("MySequence", function(context) end) -- Error! "MySequence" already exists.
```

## NestedSequence Exception
Triggered when a [`Sequence`](sequence.md#function-sequencename-string-sequence-functionsequencecontextclass-sequencecontext---requirementspecifierrequirementmdclass-requirementspecifier) is defined inside another sequence.

### Example 1
```lua
Sequence("MySequence", function(context)
    Sequence("MyOtherSequence", function(context) end) -- Error! Cannot nest sequences into other sequences!
end)
```

### Example 2
```lua
Sequence("MySequence", function(context)
    Test("MyTest", function(c)
        Sequence("MyOtherSequence", function(context) end) -- Error! Cannot nest sequences into other sequences!
    end)
end)
```

## SequenceNotFound Exception
Triggered when a [`Sequence`](requirement.md#function-sequencename-string---requirementspecifier) that does not exist gets queried in a [`Requires`](requirement.md#function-requiresrequirement-functionargs---returntype--args---returntype) expression.

### Example
```lua
Sequence("MySequence", function(context) 
    Requires(Sequence("Foo"):ToPass) -- Error! No sequence called Foo!
end)
```

## TestNotFound Exception
Triggered when a [`Test`](requirement.md#function-testname-string---requirementspecifier) that does not exist gets queried in a [`Requires`](requirement.md#function-requiresrequirement-functionargs---returntype--args---returntype) expression.

### Example
```lua
Sequence("MySequence", function(context) 
    Test("MyTest", function(c)
        Requires(Test("OtherTest"):ToPass) -- Error! No test called OtherTest in MySequence!
    end)
end)
```

## BadRequiresScope Exception
Triggered when a call to [`Requires`](requirement.md#function-requiresrequirement-functionargs---returntype--args---returntype) occurs outside of scope of a `Test` or `Sequence`.

### Example
```lua
Sequence("MySequence", function(context)
    -- The tests...
end)

Requires(Sequence("MySequence"):ToBeFirst) -- Error! Not in a valid scope!
```

## ValueNotFound Exception
Triggered when [`RequirementSpecifier.Value`](doc/API/sequence/requirement.md#function-requirementspecifiervalueself-requirementspecifier-name-string---t) is asked to find a value that does not exist in the owner's scope.

### Example
```lua
Sequence("MySequence", function(sequenceContext)
    Test("Test1", function(testContext)
        local value = GetAValue()
        Expect(value):ToBeNear(1.0, 0.1)
    end)

    Test("Test2", function(testContext)
        local value = Requires(Test("Test1"):Value, "value") -- Error! Test1.value does not exist!
    end)
end)
```

### Solution
Export the desired value:
```lua
Sequence("MySequence", function(sequenceContext)
    Test("Test1", function(testContext)
        local value = GetAValue()
        Expect(value):ToBeNear(1.0, 0.1):ExportAs("value")
    end)

    Test("Test2", function(testContext)
        local value = Requires(Test("Test1"):Value, "value") -- All good! value is exported.
    end)
end)
```

## InvalidRequirement Exception
Triggered when the scope given to an ordering requirement is not valid.
This can be due to:
- The scope given to an ordering requirement is the owner's scope
- The requirements of scope given to an ordering requirement conflicts with the one being attempted to be defined (example: both trying to be first)
- The `Sequence` not existing
- The `Test` not existing
- A user-defined `RequirementSpecifier` was provided

### Example
```lua
Sequence("MySequence", function(sequenceContext)
    Test("Foo", function(testContext)
        Requires(Test():ToBeFirst) -- Error, two tests are trying to be first!
    end)

    Test("Bar", function(testContext)
        Requires(Test():ToBeBefore, Test("MyTest")) -- Error, MyTest needs to be before Bar!
    end)

    Test("MyTest", function(testContext)
        Requires(Test():ToBeFirst) -- Error, two tests are trying to be first!
        Requires(Test():ToBeBefore, Test("Bar")) -- Error, Bar needs to be before MyTest!
        Requires(Test():ToBeBefore, Test()) -- Error, recursive requirement!
        Requires(Test():ToBeBefore, Test("OtherTest")) -- Error, OtherTest does not exist!
    end)
end)
```
