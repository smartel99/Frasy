# Sequence

The `Sequence` function allows the packaging of [Test](doc/API/sequence/test.md) into an element that can be toggled (
executed or not).
It is, put simply, nothing more than a suite of tests.

Multiple `Sequence`s can be active at the same time. In those cases, Frasy will order them based on the `Requires`
clauses of those sequences.

In its simplest form, a `Sequence` has a name and a function to execute. The sequence's function is where the `Test`s
are located,
and can define as many tests as desired.

Frasy loads the enabled sequences when the sequencing is initiated by the user, and an instance of every `Sequence` is
created for each UUT that needs to be tested.

This process, under the hood, possesses the following logic:

```py
uuts = LoadMappingOfEnabledUuts()
sequences = LoadAllEnabledSequences()
context = GetCurrentContext()

for uuts in uuts:
    for sequence in sequence:
        DoSequence(sequence, uuts, context)
```

## Note

It is important to note that the order in which sequences are declared is not representative of the order in which they
will be executed!

Frasy will order the sequences based on the rules and requirements given to each sequence, as well as the available
resources, finding the most optimized execution order.

# Functions

## `function Sequence(name: string, sequence: function(`[`SequenceContext`](#class-sequencecontext)`))`

Creates a sequence of test called `name`, described by the function `sequence`.

**Parameters:**

- `name: string`: Name assigned to the sequence. **Must be unique!**
- `sequence: function(`[`Map`](mapping.md#class-map)`, `[`SequenceContext`](#class-sequencecontext)`)`: Function that
  describes the tests that are done by the sequence.

**Returns:** None

**Example:**

```lua
Sequence("MySequence", function(sequenceContext)
    Requires(Sequence():ToBeFirst)
    Test("MyTest", function(testContext)
        local resistance = TestBench.GetResistance(testContext.Map.TP1)
        Expect(resistance).ToBeNear(1000, 0.10)
    end)
end)
```

**Exceptions:**

- Defining a `Sequence` nested into another sequence will result in
  a [`NestedSequence`](validation_error.md#nestedsequence-exception) exception.
- Multiple sequences under the same name will result in
  a [`SequenceAlreadyDefined`](validation_error.md#sequencealreadydefined-exception) exception.

---

## class `SequenceContext`

```lua
{
    name = "SequenceName",            -- Name of the sequence
    sequenceFunc = function(ctx) end, -- Function used to populate the sequence's tests
    tests = {},                       -- List of all the status of all tests contained in the sequence.
    map = {},                         -- Pin mapping for the UUT's signals.
    uuts = "",                         -- Name of the UUT on which the sequence is being run.
    startTime = 0,                    -- Time at which the sequence started executing.
    endTime = 0,                      -- Time at which the sequence ended.
    hasPassed = true,                 -- True if all tests passed.
    hasBeenSkipped = false,           -- True if sequence has been skipped.
    requirements = {},                -- List of all the requirements.
}
```