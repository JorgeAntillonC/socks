# SOCKS

### Stack Oriented Computational Kernel Syntax

SOCKS is a stack oriented programming language written in C.

The project is currently being developed for Windows only.

## Status

Work in progress.

The long term goal is for SOCKS to become self hosted, meaning that the compiler will eventually be written in SOCKS itself.

Another major goal is to compile SOCKS programs directly into x64 machine code.

For now development is focused exclusively on Windows and x64.

## Features

* Stack oriented programming model
* Virtual stack machine
* Static type checking
* Procedures and control flow
* Memory layouts
* Named fields
* Scopes
* Strings and character literals
* Windows system call support
* Compiler test suite

## Example

```socks
main:(){

    IO_Init#

    "hello world!!!!\n" IO_Print#

}

include("stdsk\\io.sk")
```

## Project Structure

```text
code/
    legacy/              Previous versions of the compiler
    tests/               Test programs and expected results
    main.c               Compiler entry point
    ...

socks_highlighting/      VS Code syntax highlighting
```

## Building

The compiler is currently developed for Windows and uses a C toolchain.

Build instructions will be documented here as the compiler build system develops.

## Development

SOCKS is currently under active development.

The language syntax, compiler architecture, and standard library may change as the project develops toward self hosting and direct x64 machine code generation.

## License

No open source license has been granted at this time.

All rights reserved.
