# Requirements
Requirements allows the specifications of conditions and ordering constraints for the owner (either a [`Sequence`](sequence.md) or a [`Test`](test.md)) to be executed.

Requirements can be defined only inside `Test`s and `Sequence`s. Defining a requirement outside these scopes will trigger a [`BadRequiresScope`](validation_error.md#badrequiresscope-exception) exception.

There are two types of requirements:
- Ordering
- Conditional

## Ordering Requirements
These requirements allows for the specification of the execution order, either through hard or soft constraints.
They should be read as "This must be done at this point in time".

Frasy is able to apply these constraints before execution, and uses them to build the list of tasks that needs to be executed.


## Conditional Requirements
These requirements allows for the specification of execution under prerequisite conditions for a `Sequence` or a `Test`.
They should be read as "This must be done only when this condition is met".

Frasy will try to arrange the execution order to minimize the time spent waiting for the condition to be met, but it does not guarantee order.
If a condition is evaluated to be false, the [`Sequence`](sequence.md) or a [`Test`](test.md) gets dropped with a result of "Skipped".


# Functions

## `function Requires(requirement: function([<Args...>]) -> <ReturnType> [, <Args...>]) -> <ReturnType>`
Applies a requirement to the current scope.
If the requirement is not constant in nature, i.e. depends on run-time conditions to be evaluated, the execution of the scope will be blocked until the condition can be evaluated.

This function allows for the definition of limits and rules that Frasy will then use when optimizing the ordering of execution.

**Ordering Requirement:**
```lua
Requires(Test():ToBeFirst) -- The test will always be first.
```

**Conditional Requirement:**
```lua
Requires(Test("OtherTest"):ToFail) -- The test will only run if OtherTest fails.
```

Values can also be shared across tests and sequences exported through [`ExpectResult:ExportAs`](expect.md#function-exportasself-expectresult-name-string) using `Value`.
This behaves just like a normal [conditional requirement](#conditional-requirements). Read the documentation for [`ExpectResult:ExportAs`](expect.md#function-exportasself-expectresult-name-string) for an example on how to share values across sequences.

**Parameters:**
- `requirement: function([<Args>]) -> <ReturnType>`: A function to be evaluated by the clause. This function can take any number of arguments and return any type except `nil`.
- `args: Args...` (optional): The arguments to call `requirement` with.

**Returns:** The value resulting from the invocation of `requirement`.

---

## `function Sequence([name: string]) -> `[`RequirementSpecifier`](#class-requirementspecifier)
Creates a requirement specifier for the requested sequence. This requirement specifier can then be used in a `Requires` expression.

When used without a name, creates a requirement specifier for the currently active sequence.

**Parameters:**
- `name: string` (optional): The name of the `Sequence` to get.

**Returns:** A [`RequirementSpecifier`](#class-requirementspecifier) for the requested sequence, if it is found.

**Exceptions:**
- Requesting a `Sequence` that does not exist will result in a [`SequenceNotFound`](validation_error.md#sequencenotfound-exception) exception.

**Example:**
In this example, `MySequence` is required to be executed first.
```lua
Sequence("MySequence", function(context)
    Requires(Sequence():ToBeFirst)
end)
```

---

## `function Test([name: string]) ->` [`RequirementSpecifier`](#class-requirementspecifier)
Creates a requirement specifier for the requested test. This requirement specifier can then be used in a `Requires` expression.

When used without a name, creates a requirement specifier for the currently active sequence.

It is possible to create a requirement specifier based of a test defined in a different `Sequence` by going through that `Sequence`'s requirement specifier:
```lua
Sequence("MySequence", function(context)
    Requires(Sequence("OtherSequence"):Test("TestInOtherSequence"):ToPass)
end)
```
**Parameters:**
- `name: string` (optional): The name of the `Test` to get.

**Returns:** A [`RequirementSpecifier`](#class-requirementspecifier) for the requested test, if it is found.

**Exceptions:**
- Requesting a `Test` that does not exist within the currently active sequence will result in a [`TestNotFound`](validation_error.md#testnotfound-exception) exception.

**Example:**
In this example, the current test is required to be executed after `OtherTest`, and requires `SomeTest` to have passed.
```lua
Sequence("MySequence", function(context)
    Test("MyTest", function(testContext)
        Requires(Test():ToFollow, Test("OtherTest"))
        Requires(Test("SomeTest"):ToPass)
    end)

    Test("OtherTest", function(testContext) end)
    Test("SomeTest", function(testContext) end)
end)
```

---

# Types
## class `RequirementSpecifier`


```lua
{
    -- Conditional
    ToPass,         -- Requires(Test("OtherTest"):ToPass), this test can only be run if OtherTest succeeds.
    ToFail,         -- Requires(Test("OtherTest"):ToFail), this test can only be run if OtherTest fails.
    ToBeCompleted,  -- Requires(Test("OtherTest"):ToBeCompleted), this test can only be run if OtherTest is completed, whether it has passed or not.
    Value,          -- local value = Requires(Test("OtherTest"):Value, "ValueName"), Fetches the value exported under the name "ValueName" of "OtherTest".

    -- Ordering
    ToBeBefore,     -- Requires(Test():ToBeBefore, Test("OtherTest")), this test needs to run at any point before OtherTest.
    ToBeAfter,      -- Requires(Test():ToBeAfter, Test("OtherTest")), this test needs to run at any point after OtherTest.
    ToPrecede,      -- Requires(Test():ToPrecede, Test("OtherTest")), this test needs to run right before OtherTest
    ToFollow,       -- Requires(Test():ToFollow, Test("OtherTest")), this test needs to run right after OtherTest
    ToBeFirst,      -- Requires(Test():ToBeFirst), this test needs to be first. Can only be on the current test, and can only be one (per sequence).
    ToBeLast,       -- Requires(Test():ToBeLast), this test needs to be last. Can only be on the current test, and can only be one (per sequence).
}
```