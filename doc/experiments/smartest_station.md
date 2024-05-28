This experiments tries to extends frasy to support Smartest Station, while keeping support for instrumentations board.

The Smartest station is aimed

# Types

## Var

Var is a object type for this documentation only.
A Var object can be a boolean, number, string or a table. If a table, its subtypes must match the same requirements as
Var. This mean in var, there must be no nil, function or thread

## Sequence

### Sequence(name) -> Sequence

### Sequence:new(name) -> Sequence

### Sequence:AddRequirement(requirement) -> Sequence

### Sequence:AddTest(test) -> Sequence

## Test

### Test(name) -> Test

### Test:new(name) -> Test

### Test:AddRequirement(requirement) -> Test

### Test:SetFunction(func) -> Test

## Ib

Base class for communicating with CANOpen devices

It is expected to be used by board developpers only in order to develop specialized classes for their boards to be used
by End-Users.

### Ib() -> Ib

Util fonction, call Ib.new()

### Ib(ibs: Ib) -> Ib,

Util function, call Ib.new(Ib)

### Ib.new() -> Ib

Default constructor

### Ib.new(ibs: Ib) -> Ib

Copy constructor

### Ib.Type(ibs: Ib) -> Ib

Define the board type
Will update default parameters

Available only during Environment load phase

### Ib.Type() -> int

Get the Ib type

### Ib.Version(requirement: Ib) -> Ib

Define the board version requirement

Available only during Environment load phase

### Ib.Version() -> Version

Return the Ib version

### Ib.Serial() -> Serial

Return the serial number

### Ib.DownloadObject(object: Object || Table) -> Var

Request DO entry to device
Result depends on DO type

### Ib.UploadObject(object:  Object || Table, value: Var)

Request update on device DO Entry

## Object

Utility class for SDO communication

Provide quick address to objects index and subindexes

### Object.new(name: String, index: Int) -> Object

Create a new named object

### Object(name: String, index: Int) -> Object

utils function, calls Object.new(name, index)

### Object.AddSubIndex(name: String, subIndex: int?) -> Object

Create a new field to Object.
If subIndex is nil, use the last index + 1

### Object.<Field> -> Table

Return a table of the object index and field subindex.

# Examples

## Frasy provided boards

```
-- TODO find how to define Frasy boards
```

## User defined boards

```
-- TODO find how to define user boards
```

## Environment file

```
Environment(function()
    // Define uuts
    uut1 = 1
    uut2 = 2

    // Define boards used in test
    mainIo      = Ib()
                    .Type(Ibs.MainIo)     
                    .Version("^1.4.0") 
    relayBoard1 = Ib()
                    .Type(Ibs.Relay)
                    .NodeId(10) 
    relayBoard2 = Ib()
                    .Type(IBs.Relay)
                    .NodeId(11)

    // Register boards
    AddIb(mainIo)
    AddIb(relayBoard1)
    AddIb(relayBoard2)

    AddKeyValue("tpReset")
        .Link(uut1, TestPoint(mainIo, ibs.mainIo.signals.MUX7_A0))
        .Link(uut2, TestPoint(mainIo, ibs.mainIo.signals.MUX7_A1))
    AddKeyValue("tpPower")
        .Link(uut1, TestPoint(relayBoard1, ibs.relay.signals.Power))
        .Link(uut2, TestPoint(relayBoard2, ibs.relay.signals.Power))
    AddKeyValue("oVoltage")
        .Link(uut1, ibs.mainIo.objects.voltage1)
        .Link(uut2, ibs.mainIo.objects.voltage2)
end)
```

## Test file

```lua
Sequence("S1")
    .AddRequirement(Requires():ToBeFirst())
    .AddTest(Test("T1")
        .AddRequirement(Requires():ToBeFirst())
        .SetFunction(function(context)
            uuts = context.uuts
            mainIo = context.ibs.ByType(ibs.mainIo)
            oVoltage = context.KeyValues.oVoltage
            tpPower = context.keyValues.tpPower
            relay = tpPower.ibs  

            -- Set voltage OFF
            relay.SetSdo(ibs.relay.sdo.power, {relay.tp, 0})

            -- Check voltage is OFF
            voltage = mainIo.GetSdo(oVoltage)
            myCheck(voltage, 0)

            -- Set voltage ON
            relay.SetSdo(ibs.relay.sdo.power, {relay.tp, 1})

            -- Check voltage is ON
            voltage = mainIo.GetSdo(oVoltage)
            myCheck(voltage, 12)

            -- Accessing objet subindex
            subOject = mainIo.GetSdo(ibs.mainIo.objects.complexObject.subObject)
            -- My check on subObject
        end))
    .AddTest(Test("T2").AddRequirement(Requires(Test("T1")):ToPass()))
    .AddTest(Test("T3").AddRequirement(Requires(Test()):ToBeLast()))

Sequence("S2")
    .AddRequirement(Requires(Sequence("S1")):ToPass())
    .AddTest(Test("T1"))
    .AddTest(Test("T2"))
    .AddTest(Test("T3"))
```