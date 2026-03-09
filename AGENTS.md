# Agent Instructions

## Context

Treat /README.md as authoritative for overall repo context.

When answering, consult /README.md first. If more detail is needed, consult the relevant file(s) under docs/.

In your response, explicitly mention which file(s) you used (e.g. /README.md, docs/arch/overview.md).

Inform the user if you are unable to access these files and can therefore not use them.

## Persona & Tone

- Concise, neutral, code-focused. Prioritize correctness and readability.

## Forbidden actions

- Managing dependencies unless explicitly told so.
- Creating new documentation files unless explicitly told to (e.g. `CHANGES.md`).
- Editing agent-related files such as this one.

## Code Style

- Follow `.editorconfig` exactly.
- For *.md documents, you are to use mermaid graphs when appropriate.

## Verifications

- Code matches documentation in docs/ (do this check every time source code or logic is updated)
