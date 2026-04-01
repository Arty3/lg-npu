# Agent Instructions

## Context

Treat /README.md as authoritative for overall repo context.

When answering, consult /README.md first. If more detail is needed, consult the relevant file(s) under docs/.

In your response, explicitly mention which file(s) you used (e.g. /README.md, docs/arch/overview.md).

Inform the user if you are unable to access these files and can therefore not use them.

Use WSL for shell commands, the project is made on WSL.

## Persona & Tone

- Concise, neutral, code-focused. Prioritize correctness and readability.

## Forbidden Actions

- Managing dependencies unless explicitly told so.
- Creating new documentation files unless explicitly told to (e.g. `CHANGES.md`).
- Editing agent-related files such as this one.

## Scratch Space

Use agent_space/ at the repo root for any scripts, tests, throw-away files, etc. That might prove useful to you but may pollute the source tree. This directory is git ignored.

## Code Style

- Follow `.editorconfig` exactly.
- For *.md documents, you are to use mermaid graphs when appropriate.
- Always use `++i` instead of `i++` where possible, therefore `i++` should only be used if increment should only happen *after*, otherwise always use `++i`
- Place curly brackets on the next line, e.g.:
```verilog
// Bad
typedef enum logic [1:0] {
	S_IDLE,
	S_CHECK,
	S_VALID
} state_e;

// Good
typedef enum logic [1:0]
{
	S_IDLE,
	S_CHECK,
	S_VALID
}   state_e;
```
- Avoid extra symbols in comments for separation, e.g.:
```verilog
// -----------
// Section     (bad)
// -----------

// Section     (good)
```
- Avoid overusing UTF-8 characters in comments, prefer ASCII.
- File banners must have a new line in between them and the file contents.
- When working in sw/ always use the sw/shared/annotations.h header file, use its features as much as possible.
- In C/C++ write pointers as `void* ptr` rather than `void *ptr`
- In C, do not use `stdbool.h`, instead use `int`.
- Apply `const`, `static`, etc. as much as possible, C code should be as explicit as can be.
- When writing comments in C, prefer `/**/` syntax over `//`.
- Prefer `!var` over `var == 0` on simple checks, complex checks like `strcmp` should stay as `strcmp(a, b) == 0`.
- In C, when a function takes no arguments you should specify this with void: `void f(void);`
- For C/C++, all source files must include the necessary headers even if their respective header already includes them. (e.g. `#include <string.h>` when their respective header already includes it.)
- For one line bodies in C/C++, avoid curly braces, e.g.:
```c
// Bad
if (!buf)
{
    return NULL;
}

while (1)
{
	func();
}

// Good
if (!buf)
    return NULL;

while (1)
	func();
```
- When using annotations.h, you should define functions with this style (annotations on top, definition below):
```c
NO_DISCARD ALWAYS_INLINE
static size_t my_func(void);
```
- In C/C++ headers are to be protected with the convention:
```c
#ifndef _HEADER_NAME_H
#define _HEADER_NAME_H

#endif /* _HEADER_NAME_H */
```
- Prefer for loops over while loops:
```c
// Bad
while (cond)
	...

// Good
for (; cond ;)
	...
```

## Verifications

- Code matches documentation in docs/ (do this check every time source code or logic is updated)
