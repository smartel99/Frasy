# On this Page <!-- omit in toc -->
- [Description](#description)
  - [Ordering Requirements](#ordering-requirements)
  - [Conditional Requirements](#conditional-requirements)
- [Functions](#functions)
  - [`function Requires(requirement: function([<Args...>]) -> <ReturnType> [, <Args...>]) -> <ReturnType>`](#function-requiresrequirement-functionargs---returntype--args---returntype)
  - [`function Sequence([name: string]) -> ``RequirementSpecifier`](#function-sequencename-string---requirementspecifier)
  - [`function Test([name: string]) ->` `RequirementSpecifier`](#function-testname-string---requirementspecifier)
- [Types](#types)
  - [class `RequirementSpecifier`](#class-requirementspecifier)
    - [Methods](#methods)
      - [`function RequirementSpecifier.ToPass(self: RequirementSpecifier) -> bool`](#function-requirementspecifiertopassself-requirementspecifier---bool)
      - [`function RequirementSpecifier.ToFail(self: RequirementSpecifier) -> bool`](#function-requirementspecifiertofailself-requirementspecifier---bool)
      - [`function RequirementSpecifier.ToBeCompleted(self: RequirementSpecifier) -> bool`](#function-requirementspecifiertobecompletedself-requirementspecifier---bool)
      - [`function RequirementSpecifier.Value(self: RequirementSpecifier, name: string) -> <T>`](#function-requirementspecifiervalueself-requirementspecifier-name-string---t)
      - [`function RequirementSpecifier.ToBeBefore(self: RequirementSpecifier, other: RequirementSpecifier)`](#function-requirementspecifiertobebeforeself-requirementspecifier-other-requirementspecifier)
      - [`function RequirementSpecifier.ToBeAfter(self: RequirementSpecifier, other: RequirementSpecifier)`](#function-requirementspecifiertobeafterself-requirementspecifier-other-requirementspecifier)
      - [`function RequirementSpecifier.ToPrecede(self: RequirementSpecifier, other: RequirementSpecifier)`](#function-requirementspecifiertoprecedeself-requirementspecifier-other-requirementspecifier)
      - [`function RequirementSpecifier.ToFollow(self: RequirementSpecifier, other: RequirementSpecifier)`](#function-requirementspecifiertofollowself-requirementspecifier-other-requirementspecifier)
      - [`function RequirementSpecifier.ToBeFirst(self: RequirementSpecifier)`](#function-requirementspecifiertobefirstself-requirementspecifier)
      - [`function RequirementSpecifier.ToBeLast(self: RequirementSpecifier)`](#function-requirementspecifiertobelastself-requirementspecifier)
  - [`function RequirementSpecifier(predicator: function([...]) -> <ReturnType>) -> RequirementSpecifier`](#function-requirementspecifierpredicator-function---returntype---requirementspecifier)

# Description
Requirements allows the specifications of conditional and ordering constraints for the owner (either a [`Sequence`](sequence.md) or a [`Test`](test.md)) to be executed.

Requirements can be defined only inside the scope of a `Test` or `Sequence`. Defining a requirement outside these scopes will trigger a [`BadRequiresScope`](doc/API/errors/validation_error.md#badrequiresscope-exception) exception.

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

Values can also be shared across tests and sequences exported through [`ExpectResult:ExportAs`](expect.md#function-exportasself-expectresult-name-string) using `Value`.
This behaves just like a normal [conditional requirement](#conditional-requirements). Read the documentation for [`ExpectResult:ExportAs`](expect.md#function-exportasself-expectresult-name-string) for an example on how to share values across sequences.

**Parameters:**
- `requirement: function([<Args>]) -> <ReturnType>`: A function to be evaluated by the clause. This function can take any number of arguments and return any type except `nil`.
- `args: Args...` (optional): The arguments to call `requirement` with.

**Returns:** The value resulting from the invocation of `requirement`.

**Exceptions:**
- Defining a requirement outside these scopes will trigger a [`BadRequiresScope`](doc/API/errors/validation_error.md#badrequiresscope-exception) exception.

**Examples:**
In this example, an ordering requirement is applied to `MySequence`, specifying that it must be executed first.
```lua
Sequence("MySequence", function(context)
    Requires(Sequence():ToBeFirst)
end)
```

Here, a conditional requirement is applied to `MySequence`, specifying that it should only run if `OtherSequence` has passed all of its tests.
```lua
Sequence("MySequence", function(context)
    Requires(Sequence("OtherSequence"):ToPass)
end)
```

In this other scenario, a value is transferred from `Test1` to `Test2`, behaving like a conditional requirement where the value must exist before `Test2` can run.
```lua
Sequence("MySequence", function(sequenceContext)
    Test("Test1", function(testContext)
        local value = GetAValue()
        Expect(value):ToBeEqual(5):ExportAs("MyValue")
    end)

    Test("Test2", function(testContext)
        local valueFromOtherTest = Requires(Test("Test1"):Value, "MyValue")

        SetAValue(valueFromOtherTest)
        local value = GetAValue()
        Expect(value):Not:ToBeEqual(4)
    end)
end)
```

Lastly, this example demonstrates how an arbitrary function can be used to determine the conditional execution of a scope.
Here, `Requires` is used to execute `Test2` only if `value` from `Test1` is equal to 5.
```lua
Sequence("MySequence", function(sequenceContext)
    Test("Test1", function(testContext)
        local value = GetAValue()
        Expect(value):ToBeInRange(1, 10):ExportAs("value")
    end)

    Test("Test2", function(testContext)
        Requires(RequirementSpecifier(function() return Test("Test1"):Value("value") == 5 end):ToPass)
    end)
end)
```

---

## `function Sequence([name: string]) -> `[`RequirementSpecifier`](#class-requirementspecifier)
Creates a requirement specifier for the requested sequence. This requirement specifier can then be used in a `Requires` expression.

When used without a name, creates a requirement specifier for the currently active sequence.

**Parameters:**
- `name: string` (optional): The name of the `Sequence` to get.

**Returns:** A [`RequirementSpecifier`](#class-requirementspecifier) for the requested sequence, if it is found.

**Exceptions:**
- Requesting a `Sequence` that does not exist will result in a [`SequenceNotFound`](doc/API/errors/validation_error.md#sequencenotfound-exception) exception.

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
- Requesting a `Test` that does not exist within the currently active sequence will result in a [`TestNotFound`](doc/API/errors/validation_error.md#testnotfound-exception) exception.

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
Class used to define requirements for a `Test` or a `Sequence`. It can be instantiated through the [`Test(name: string)`](#function-testname-string---requirementspecifier) function or the [`Sequence(name: string)`](#function-sequencename-string---requirementspecifier) function. Alternatively, 

### Methods

#### `function RequirementSpecifier.ToPass(self: RequirementSpecifier) -> bool`
When used as an argument to `Requires`, waits for the specified scope (either a `Test` or a`Sequence`) to be completed successfully.
If the scope does not complete successfully, or can't be completed at all, the scope in which the requirement is defined will not be executed.

**Parameters:**
- `self: RequirementSpecifier`: Instance of the requirement specifier, passed implicitly.

**Returns:**
- True if the specified scope has successfully completed.
- False otherwise.

**Exceptions:** None

**Example:**
```lua
Sequence("MySequence", function(sequenceContext) 
    Test("Fails", function(testContext)
        Expect(true):ToBeFalse()
    end)

    Test("WillNotRun", function(testContext)
        Requires(Test("Fails"):ToPass)
    end)

    Test("Passes", function(testContext)
        Expect(true):ToBeTrue()
    end)

    Test("WillRun", function(testContext)
        Requires(Test("Passes"):ToPass)
    end)
end)
```

--- 

#### `function RequirementSpecifier.ToFail(self: RequirementSpecifier) -> bool`
When used as an argument to `Requires`, waits for the specified scope (either a `Test` or a`Sequence`) to be completed unsuccessfully.
If the scope does not complete unsuccessfully, or can't be completed at all, the scope in which the requirement is defined will not be executed.

**Parameters:**
- `self: RequirementSpecifier`: Instance of the requirement specifier, passed implicitly.

**Returns:**
- True if the specified scope has unsuccessfully completed.
- False otherwise.

**Exceptions:** None

**Example:**
```lua
Sequence("MySequence", function(sequenceContext) 
    Test("Passes", function(testContext)
        Expect(true):ToBeTrue()
    end)

    Test("WillNotRun", function(testContext)
        Requires(Test("Passes"):ToFail)
    end)

    Test("Fails", function(testContext)
        Expect(false):ToBeTrue()
    end)

    Test("WillRun", function(testContext)
        Requires(Test("Fails"):ToFail)
    end)
end)
```

---

#### `function RequirementSpecifier.ToBeCompleted(self: RequirementSpecifier) -> bool`
When used as an argument to `Requires`, waits for the specified scope (either a `Test` or a`Sequence`) to be completed.
The result of the specified scope does not matter, but if the scope does not complete for any reason, 
the scope in which the requirement is defined will not be executed.

**Parameters:**
- `self: RequirementSpecifier`: Instance of the requirement specifier, passed implicitly.

**Returns:**
- True if the specified scope has completed.
- False otherwise.

**Exceptions:** None

**Example:**
```lua
Sequence("MySequence", function(sequenceContext) 
    Test("Completes", function(testContext)
        Expect(true):ToBeTrue()
    end)

    Test("WillRun", function(testContext)
        Requires(Test("Completes"):ToBeCompleted)
    end)

    Test("DoesntComplete", function(testContext)
        Requires(RequirementSpecifier(function() return false end):ToPass) -- will never run.
    end)

    Test("WillNotRun", function(testContext)
        Requires(Test("DoesntComplete"):ToBeCompleted)
    end)
end)
```

---

#### `function RequirementSpecifier.Value(self: RequirementSpecifier, name: string) -> <T>`
Fetches a named value exported by the owning scope.

**Parameters:**
- `self: RequirementSpecifier`: Instance of the requirement specifier, passed implicitly.
- `name: string`: Name of the exported value.

**Returns:** The value, if found.

**Exceptions:**
- If the requested value can't be found in the owning scope, a [`ValueNotFound`](doc/API/errors/validation_error.md#valuenotfound-exception) will be thrown.

**Example:**
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

---

#### `function RequirementSpecifier.ToBeBefore(self: RequirementSpecifier, other: RequirementSpecifier)`
Specifies that the owning scope must be executed before the `other` scope.
This is an ordering constraint, applied by Frasy when optimizing the order in which things should happen.

**Parameters:**
- `self: RequirementSpecifier`: Instance of the requirement specifier, passed implicitly.
- `other: RequirementSpecifier`: The scope that should occur before the owner's.

**Returns:** None

**Exceptions:**
- [`InvalidRequirement`](doc/API/errors/validation_error.md#invalidrequirement-exception)

**Example:**
```lua
Sequence("MySequence", function(sequenceContext)
    Test("Foo", function(testContext)
        print("Foo")
    end)

    Test("Bar", function(testContext)
        Requires(Test():ToBeBefore, Test("Foo"))
        print("Bar")
    end)
end)
```

Outputs:
```
Bar
Foo
```

---

#### `function RequirementSpecifier.ToBeAfter(self: RequirementSpecifier, other: RequirementSpecifier)`
Specifies that the owning scope must be executed after the `other` scope.
This is an ordering constraint, applied by Frasy when optimizing the order in which things should happen.

**Parameters:**
- `self: RequirementSpecifier`: Instance of the requirement specifier, passed implicitly.
- `other: RequirementSpecifier`: The scope that should occur after the owner's.

**Returns:** None

**Exceptions:**
- [`InvalidRequirement`](doc/API/errors/validation_error.md#invalidrequirement-exception)

**Example:**
```lua
Sequence("MySequence", function(sequenceContext)
    Test("Foo", function(testContext)
        print("Foo")
    end)

    Test("Bar", function(testContext)
        Requires(Test():ToBeAfter, Test("Foo"))
        print("Bar")
    end)
end)
```

Outputs:
```
Foo
Bar
```

---

#### `function RequirementSpecifier.ToPrecede(self: RequirementSpecifier, other: RequirementSpecifier)`
Specifies that the owning scope must be executed right before the `other` scope.
This is an ordering constraint, applied by Frasy when optimizing the order in which things should happen.

**Parameters:**
- `self: RequirementSpecifier`: Instance of the requirement specifier, passed implicitly.
- `other: RequirementSpecifier`: The scope that should occur right after the owner's.

**Returns:** None

**Exceptions:**
- [`InvalidRequirement`](doc/API/errors/validation_error.md#invalidrequirement-exception)

**Example:**
```lua
Sequence("MySequence", function(sequenceContext)
    Test("Foo", function(testContext)
        print("Foo")
    end)

    Test("Bar", function(testContext)
        Requires(Test():ToPrecede, Test("Foo"))
        print("Bar")
    end)

    Test("Far", function(testContext)
        Requires(Test():ToBeBefore, Test("Foo"))
        print("Far")
    end)
end)
```

Outputs:
```
Far
Bar
Foo
```

---

#### `function RequirementSpecifier.ToFollow(self: RequirementSpecifier, other: RequirementSpecifier)`
Specifies that the owning scope must be executed right after the `other` scope.
This is an ordering constraint, applied by Frasy when optimizing the order in which things should happen.

**Parameters:**
- `self: RequirementSpecifier`: Instance of the requirement specifier, passed implicitly.
- `other: RequirementSpecifier`: The scope that should occur right before the owner's.

**Returns:** None

**Exceptions:**
- [`InvalidRequirement`](doc/API/errors/validation_error.md#invalidrequirement-exception)

**Example:**
```lua
Sequence("MySequence", function(sequenceContext)
    Test("Foo", function(testContext)
        print("Foo")
    end)

    Test("Bar", function(testContext)
        Requires(Test():ToFollow, Test("Foo"))
        print("Bar")
    end)

    Test("Far", function(testContext)
        Requires(Test():ToBeAfter, Test("Foo"))
        print("Far")
    end)
end)
```

Outputs:
```
Foo
Bar
Far
```

---

#### `function RequirementSpecifier.ToBeFirst(self: RequirementSpecifier)`
Specifies that the owning scope must be executed first. Only a single scope (1 `Test` per `Sequence`,1 unique `Sequence`) is allowed to bear this requirement.
This is an ordering constraint, applied by Frasy when optimizing the order in which things should happen.

**Parameters:**
- `self: RequirementSpecifier`: Instance of the requirement specifier, passed implicitly.

**Returns:** None

**Exceptions:**
- [`InvalidRequirement`](doc/API/errors/validation_error.md#invalidrequirement-exception)

**Example:**
```lua
Sequence("Foo", function(sequenceContext)
    Test("Foo", function(testContext)
        print("Foo.Foo")
    end)

    Test("Bar", function(testContext)
        Requires(Test():ToBeFirst)
        print("Foo.Bar")
    end)
end)

Sequence("Bar", function(sequenceContext)
    Requires(Sequence():ToBeFirst)

    Test("Foo", function(testContext)
        print("Bar.Foo")
    end)
end)
```

Outputs:
```
Bar.Foo
Foo.Bar
Foo.Foo
```

---

#### `function RequirementSpecifier.ToBeLast(self: RequirementSpecifier)`
Specifies that the owning scope must be executed last. Only a single scope (1 `Test` per `Sequence`,1 unique `Sequence`) is allowed to bear this requirement.
This is an ordering constraint, applied by Frasy when optimizing the order in which things should happen.

**Parameters:**
- `self: RequirementSpecifier`: Instance of the requirement specifier, passed implicitly.

**Returns:** None

**Exceptions:**
- [`InvalidRequirement`](doc/API/errors/validation_error.md#invalidrequirement-exception)

**Example:**
```lua
Sequence("Foo", function(sequenceContext)
    Test("Foo", function(testContext)
        print("Foo.Foo")
    end)

    Test("Bar", function(testContext)
        Requires(Test():ToBeLast)
        print("Foo.Bar")
    end)
end)

Sequence("Bar", function(sequenceContext)
    Requires(Sequence():ToBeLast)

    Test("Foo", function(testContext)
        print("Bar.Foo")
    end)
end)
```

Outputs:
```
Foo.Foo
Foo.Bar
Bar.Foo
```

---

## `function RequirementSpecifier(predicator: function([...]) -> <ReturnType>) -> RequirementSpecifier`
Creates a requirement specifier from a user defined function.

**Parameters:**
- `predicator: function([...]) -> <ReturnType>`: A function that generates the value on which the requirement specifier acts.
The constraints for what that function does are extremely loose. 
It can take any parameters, return anything and do any operation, for as long as it returns something that is usable as a `RequirementSpecifier`.

**Returns:** A `RequirementSpecifier`

**Exceptions:** None

**Example:**
Here, `Requires` is used to execute `Test2` only if `value` from `Test1` is equal to 5.
```lua
Sequence("MySequence", function(sequenceContext)
    Test("Test1", function(testContext)
        local value = GetAValue()
        Expect(value):ToBeInRange(1, 10):ExportAs("value")
    end)

    Test("Test2", function(testContext)
        Requires(RequirementSpecifier(function() return Test("Test1"):Value("value") == 5 end):ToPass)
    end)
end)
```