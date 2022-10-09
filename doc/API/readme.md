# API Reference

## Conventions
When describing code usage, the following convention is used:
- Optional fields are indicated by being wrapped in square-brackets (`[]`)
- Placeholder fields are indicated by being wrapped in arrow-brackets (`<>`)
- Functions are described as `function <name>([<parameter>: <type>, ...]) [-> <type>]`, even if that does not follow Lua's syntax.
  - If a function does not return anything, the trailing return type is omitted.
- Parameter types follow the name of that parameter, as `<name>: <type>`
  - Templated (generic) parameters uses the placeholder syntax to identify the type. The constraints and restrictions for that template type are then described bellow the template's declaration. Example: `function Add(lhs: <T>, rhs: <T>) -> <T>` is a function that accepts any type, for as long as `lhs` and `rhs` are of the same type.
  - When the type of a parameter is a function, the function's signature is used, as `<name>: function([<type>][, ... [:<type>]]) [-> <type>]`
  - When a function takes a variable number of arguments (known as a variadic function), ellipses (`...`) are used.
    - When a variadic function takes arguments of a specific type, the form of `...: <type>` is used.
    - When a variadic function takes arguments of any type, only the ellipse is used.

## Sequences
To write test sequences at the highest level.

- [Sequence](doc/API/sequence/sequence.md) - Describes a list of test.
- [Test](doc/API/sequence/test.md) - Describes a test in a Sequence.
- [Expect](doc/API/sequence/expect.md) - Describes a condition in a Test.
- [Requires](doc/API/sequence/requirement.md) - Describes the requirements that needs to be met for something to be executed.

## Commands
To write commands unique to a specific test bench using generic functionalities.

## HAL
To write commands unique to a specific test bench using functionalities not offered by the built-in [commands](doc/API/commands/readme.md).
