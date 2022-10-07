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
Triggered when a [`Sequence`](sequence.md#function-sequencename-string-sequence-functionsequencecontextclass-sequencecontext---requirementspecifierrequirementmdclass-requirementspecifier) that does not exist gets queried in a [`Requires`](requirement.md#class-requirementspecifier) expression.

### Example
```lua
Sequence("MySequence", function(context) 

end):Requires(Sequence("Foo"):ToPass()) -- Error! No sequence called Foo!
```
