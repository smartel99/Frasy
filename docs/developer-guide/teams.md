# Teams

When a test fixture hosts multiple UUTs that need to **cooperate** — sharing measurement results,
coordinating power sequencing, or waiting for each other before proceeding — you group them into
a **team**. A team is a set of UUTs that synchronize at every test boundary and can exchange data
during execution.

---

## Why Teams?

Without teams, each UUT runs independently. `Sync()` barriers and `Exclusive()` regions
coordinate across *all* UUTs globally. This is fine when every UUT is identical and independent.

Teams are useful when:

- A fixture tests multiple boards that are physically **connected** to each other (e.g., a
  controller board and a sensor board that communicate over a shared bus).
- One UUT acts as a **stimulus source** while others measure responses.
- You need **partial synchronization** — two UUTs must wait for each other, but a third can
  proceed independently.
- You want to **share data** from a leader's measurement to its followers without redundant
  hardware access.

---

## Declaring Teams

Teams are declared in `environment.lua` using `Environment.Team.Add()`. The first argument is
the **leader** UUT, and subsequent arguments are the **followers**.

```lua
Environment.Make(function()
    Environment.ScriptVersion("1.0.0")
    Environment.Uut.Count(4)

    -- Team 1: UUT 1 leads UUT 2
    Environment.Team.Add(1, 2)

    -- Team 2: UUT 3 leads UUT 4
    Environment.Team.Add(3, 4)
end)
```

### Rules

- Every UUT must belong to exactly **one** team. If teams are enabled (at least one
  `Environment.Team.Add()` call exists), **all** UUTs must be assigned.
- The first argument to `Environment.Team.Add()` is the **leader**. Its position is 1.
- Subsequent arguments are followers, with positions 2, 3, etc.
- A UUT cannot be in multiple teams.

---

## Leader and Follower Roles

Every team member has a **position** — a 1-indexed number determined by its order in the
`Environment.Team.Add()` call. The first argument is position 1 (the leader), the second is
position 2, and so on.

```lua
Environment.Team.Add(3, 7, 5)
-- UUT 3 → position 1 (leader)
-- UUT 7 → position 2 (follower)
-- UUT 5 → position 3 (follower)
```

| Role | Position | Characteristics |
|---|---|---|
| Leader | 1 (first argument) | Can call `Team.Wait()`. Drives the team's coordination logic. |
| Follower | 2+ (subsequent arguments) | Calls `Team.Done()` to signal readiness. Receives data from the leader. |

The position is useful inside tests to assign different work to different members without
hard-coding UUT numbers:

```lua
Test("Multi-Channel Measurement", function()
    local channel = Team.Position()  -- 1, 2, or 3
    local v = daq:MeasureVoltage(channels[channel])
    Expect(v.average, "Channel " .. channel):ToBeInPercentage(3.3, 5.0)
end)
```

Query roles at runtime:

```lua
Team.IsLeader()   -- true if current UUT is position 1
Team.Position()   -- returns the position (1, 2, 3, ...)
Team.GetLeader()  -- returns the leader UUT number
```

---

## Automatic Synchronization

When teams are enabled, Frasy automatically synchronizes team members at the boundary of
**every test**. After each test completes, all UUTs in a team wait for each other via
`Team.Sync()`. This ensures:

- If any team member **fails**, all members are notified (the result is propagated as
  "Teammate failure").
- If any team member **crashes** (critical failure — an unrecoverable error), all members
  receive a critical failure and the sequence is aborted.

You do not call `Team.Sync()` manually — it is invoked by the orchestrator automatically after
each test body executes.

### Sync Status Propagation

| Member Status | Effect on Team |
|---|---|
| Pass | No effect on other members |
| Fail | Other members' results are marked as "Teammate failure" |
| Critical failure (crash) | All members receive an error and the sequence aborts |

---

## Data Sharing: Tell and Get

The leader can **broadcast** a value to all followers within a test using `Team.Tell()`. Followers
receive it with `Team.Get()`.

```lua
Sequence("Calibration", function()
    Test("Share Reference Voltage", function()
        if Team.IsLeader() then
            -- Leader measures the shared reference
            local ref = Context.map.ibs.daq:MeasureVoltage(Context.values.route.vref)
            Team.Tell(ref)
            Expect(ref.average, "Reference Voltage"):ToBeInPercentage(5.0, 1.0)
        else
            -- Followers receive the measurement
            local ref = Team.Get()
            Expect(ref.average, "Reference Voltage"):ToBeInPercentage(5.0, 1.0)
        end
    end)
end)
```

### Rules for Tell/Get

- `Team.Tell(value)` is a **blocking** call — all team members must participate (leader tells,
  followers get). They synchronize internally.
- `Team.Get()` blocks until the leader has called `Team.Tell()`.
- The value can be any serializable Lua type: numbers, booleans, strings, or tables containing
  these types.
- Each `Tell`/`Get` pair is matched in order — you can have multiple exchanges in a single test.

---

## Wait and Done

For scenarios where the leader needs to perform work **after** all followers have completed their
portion (but before the test ends), use `Team.Wait()` and `Team.Done()`:

```lua
Test("Power Sequencing", function()
    if Team.IsLeader() then
        -- Leader enables power, then waits for followers to confirm they see it
        enablePower()
        Team.Wait(function()
            -- This function is called repeatedly until all followers report Done
            -- Use it to keep the application responsive (e.g., pump messages)
            SleepFor(10)
        end)
        -- All followers are ready — proceed
        disablePower()
    else
        -- Follower waits until it detects power, then signals done
        waitForPowerStable()
        Team.Done()
    end
end)
```

### How Wait/Done Works

1. Each follower calls `Team.Done()` when it has completed its portion.
2. The leader calls `Team.Wait(routine)`, passing a function that is called **repeatedly** in a
   loop until all team members (followers + any errored UUTs) have reported.
3. Once all members have reported, `Team.Wait()` returns and the leader continues.

!!! warning
    Only the **leader** may call `Team.Wait()`. Only **followers** may call `Team.Done()`.
    Calling them from the wrong role will raise an error.

---

## Error Handling in Teams

When a team member encounters an unrecoverable error during execution:

1. `Team.Fail()` is called automatically by the orchestrator.
2. The failed member drops out of all synchronization barriers (it won't block the team).
3. The remaining members continue executing, but the team's barrier sizes are adjusted
   dynamically.
4. At the next `Team.Sync()` (end of test), the failure is propagated to all surviving members.

This prevents a crashed UUT from deadlocking its teammates.

---

## Complete Example

A fixture tests two boards simultaneously. Board 1 (leader) generates a signal; board 2
(follower) measures it.

**`environment.lua`**

```lua
local MyDaq = DAQ:New({ name = "daq", nodeId = 2 })

Environment.Make(function()
    Environment.ScriptVersion("1.0.0")
    Environment.Uut.Count(2)
    Environment.Ib.Add(MyDaq)
    Environment.Team.Add(1, 2)  -- UUT 1 leads, UUT 2 follows

    Environment.UutValue.Add("signal_route")
        :Link(1, DAQ.RoutingPointsEnum.MUX1_A0)  -- UUT 1's signal output
        :Link(2, DAQ.RoutingPointsEnum.MUX1_A1)  -- UUT 2's measurement input
end)
```

**`tests/signal_test.lua`**

```lua
Sequence("Signal Integrity", function()
    Test("Generate and Measure", function()
        local daq = Context.map.ibs.daq --[[@as DAQ]]

        if Team.IsLeader() then
            -- Leader: enable signal output on UUT 1
            daq:SetOutput(Context.values.signal_route, true)
            SleepFor(50) -- Allow signal to settle

            -- Tell the follower the signal is ready
            Team.Tell({ ready = true, expected_mv = 3300 })
        else
            -- Follower: wait for leader's signal
            local info = Team.Get()

            -- Measure the signal on UUT 2's input
            local v = daq:MeasureVoltage(Context.values.signal_route)
            Expect(v.average * 1000, "Signal Level (mV)")
                :ToBeInPercentage(info.expected_mv, 5.0)
        end
    end)

    Test("Cleanup", function()
        if Team.IsLeader() then
            local daq = Context.map.ibs.daq --[[@as DAQ]]
            daq:SetOutput(Context.values.signal_route, false)
        end
    end)
end)
```

---

## API Reference

| Function | Description |
|---|---|
| `Environment.Team.Add(leader, ...)` | Declare a team. First arg is leader UUT number, rest are followers. |
| `Team.IsLeader()` | Returns `true` if the current UUT is the team leader. |
| `Team.Position()` | Returns the 1-indexed position of the current UUT in its team. |
| `Team.GetLeader()` | Returns the UUT number of the current UUT's team leader. |
| `Team.HasTeam()` | Returns `true` if teams are enabled for this environment. |
| `Team.Tell(value)` | Leader broadcasts a value to all followers. Blocking. |
| `Team.Get()` | Follower receives the value broadcast by the leader. Blocking. |
| `Team.Wait(routine)` | Leader waits until all followers call `Done()`. Calls `routine` in a loop while waiting. |
| `Team.Done()` | Follower signals the leader that it has completed its portion. |

---

## Tips

- Keep `Tell`/`Get` calls **symmetric** — every `Team.Tell()` in the leader path must have a
  matching `Team.Get()` in the follower path, and vice versa. Mismatched calls will deadlock.
- Use `Team.Position()` to assign different work to different followers (e.g., each follower
  measures a different channel).
- If a test doesn't need team coordination, you can write it without any `Team.*` calls — the
  automatic sync at the test boundary still keeps everyone aligned.
- Teams work alongside `Exclusive()` and `Once()` — those primitives coordinate across *all*
  UUTs globally, regardless of team membership.
