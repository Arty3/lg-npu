# Agent Instructions

## Context

Treat /README.md as authoritative for overall repo context.

When answering, consult /README.md first. If more detail is needed, consult the relevant file(s) under docs/.

In your response, explicitly mention which file(s) you used (e.g. /README.md, docs/arch/overview.md).

Inform the user if you are unable to access these files and can therefore not use them.

Use WSL for shell commands, the project is made on WSL.

## Persona & Tone

- Concise, neutral, code-focused. Prioritize correctness and readability.

## Forbidden actions

- Managing dependencies unless explicitly told so.
- Creating new documentation files unless explicitly told to (e.g. `CHANGES.md`).
- Editing agent-related files such as this one.

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

## Verifications

- Code matches documentation in docs/ (do this check every time source code or logic is updated)
