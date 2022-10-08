# API Reference

## Conventions
When describing code usage, the following convention is used:
- Optional fields are indicated by being wrapped in square-brackets (`[]`)
- Placeholder fields are indicated by being wrapped in arrow-brackets (`<>`)
- Functions are described as `function <name>([<parameter>: <type>, ...]) [-> <type>]`, even if that does not follow Lua's syntax.
  - If a function does not return anything, the trailing return type is omitted.
- Parameter types follow the name of that parameter, as `<name>: <type>`
  - When the type of a parameter is a function, the function's signature is used, as `<name>: function([<type>, ...]) [-> <type>]`

## Sequences
To write test sequences at the highest level.

- [Sequence](sequence.md) - Describes a list of test.
- [Test](test.md) - Describes a test in a Sequence.
- [Expect](expect.md) - Describes a condition in a Test.
- [Requires](requirement.md) - Describes the requirements that needs to be met for something to be executed.

## Commands
To write commands unique to a specific test bench using generic functionalities.

## HAL
To write commands unique to a specific test bench using functionalities not offered by [commands](#commands).
