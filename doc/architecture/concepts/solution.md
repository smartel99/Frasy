In order to run your sequences and tests, the orchestrator needs to compute in which order it will run them.
The resulting computation is called a solution.
A solution is composed by sections, sequences stages, sequences, tests stages and tests.

- A test is a function defined by the user using the SDK Call Test(name, func).
- A sequence is a named group of tests. 
  - Its tests are grouped into tests stages for ordering purposes.
  - Tests in the same stage are to be considered as running in parallel
  - Parallel run is not yet supported by Frasy, but will in the future.
- A section is an uninterruptible execution of subsequences
  - A subsequence is a subset of a sequence. It means that it can contains only a restricted set of the sequence tests stages
  - Used to synchronised UUTs that are not on the same Team or IB.

# Example
## User test script
```lua
Sequence("S1", function()
  Test("T1", function()
      Requires(Sync())
      -- Stuff
  end)

  Test("T2", function()
    -- Stuff
  end)
end)

Sequence("S2", function() 
  Test("T1", function()
    -- Stuff
  end)
  
  Test("T2", function() 
    -- Stuff
  end)  
end)

Sequence("S3", function()
  Requires(Sequence("S1"):ToBeBefore())
  Requires(Sequence("S2"):ToBeBefore())
  Requires(Sync())
  Test("T1", function()
    -- Stuff
  end)

  Test("T2", function()
    -- Stuff
  end)
end)


Sequence("S4", function()
  Requires(Sequence("S3"):ToBeBefore())
  Test("T1", function()
    -- Stuff
  end)

  Test("T2", function()
    -- Stuff
  end)
end)


Sequence("S5", function()
  Requires(Sequence("S3"):ToBeBefore())
  Test("T1", function()
    -- Stuff
  end)

  Test("T2", function()
    -- Stuff
  end)
end)
```

## Solution
```json
[
  [
    [
      {
        "name": "S2",
        "tests": [["T1", "T2"]]
      }
    ]
  ],
  [
    [
      {
        "name": "S1",
        "tests": [["T2"]]
      } 
    ]
  ],
  [
    [
      {
        "name": "S1",
        "tests": [["T1"]]
      }
    ]
  ],
  [
    [
      {
        "name": "S3",
        "tests": [["T1", "T2"]]
      }
    ]    
  ],
  [
    [
      {
        "name": "S4",
        "tests": [["T1", "T2"]]
      },
      {
        "name": "S5",
        "test": [["T1", "T2"]]
      }
    ]
  ]
]
```

