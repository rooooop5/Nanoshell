# Nanoshell

A minimal Unix-like shell written from scratch in C — with quoting-aware parsing, job control,
a crash-safe trash/undo system, and a built-in AI assistant.

---

## Overview

Nanoshell is a lightweight command-line shell that implements the fundamentals you'd expect from
a real Unix shell — process execution, signal handling, terminal control, and argument parsing
with quote support — alongside two extended builtins that go beyond a typical shell:

- **`trash`** — a soft-delete system with undo, so accidental deletions aren't permanent
- **`nix`** — an LLM-backed assistant you can query directly from the command line

---

## Features

### Command Execution

- Runs external programs via `fork()` and `execvp()`
- Standard argument passing and command syntax

### Command Parsing

- Tokenizes input into arguments, splitting on whitespace
- Supports single- and double-quoted arguments, so `echo "hello world"` is treated as one argument
- Detects and rejects unclosed quotes cleanly, rather than misparsing the rest of the input

### Signal Handling

- `Ctrl+C` (`SIGINT`) interrupts the running foreground process without killing the shell itself
- Interrupted or empty input is handled gracefully, without corrupting shell state

### Terminal & Process Control

- Foreground process management via `tcsetpgrp`
- Process groups isolated with `setpgid`, preventing shell and child output from interleaving

> **Note:** Nanoshell doesn't support pipes (`|`) or output/input redirection (`>`, `>>`, `<`) yet.
> Every command runs standalone for now.

### Built-in Commands

| Command | Description |
|---|---|
| `cd` | Change directory — supports `cd -` (previous directory) and `cd ~/path` |
| `exit` | Exit the shell |
| `help` | List available builtins |
| `pwd` | Print the current working directory |
| `trash` | Soft-delete files/directories, with undo |
| `nix` | Query an LLM assistant from the shell |

---

## `trash` — soft delete with undo

Instead of deleting files permanently, `trash` moves them into a hidden trash folder
(`~/.deleted/`) and keeps a log so they can be restored later — a safety net against accidental
deletions.

### Usage

```
trash <file>              move <file> into the trash
trash --list              show everything currently in the trash
trash --restore           restore the most recently trashed item
trash --restore <file>    restore the most trashed item matching this filename, if multiple matched, restore selected one
trash --purge             permanently delete everything in the trash
```

### How it works

- **Empty directories only.** Trashing a non-empty directory is refused — empty it first.
- **Restoring by name:** if several trashed items share a filename, `trash --restore <file>` lists
  them with an index and asks which one to restore; a single match restores immediately.
- **Restoring with no name:** `trash --restore` restores whichever active item was trashed most
  recently.
- **Persists across restarts.** Every operation is appended to `~/.deleted/.log.txt`, so the trash
  log is fully rebuilt from that file each time the shell starts.
- **Auto-created.** `~/.deleted/` is created on first use if it doesn't already exist.
- **Capacity:** up to 100 items at a time; run `trash --purge` to make room once full.

### Example

```
$ trash notes.txt
$ trash --list
 FILENAME: notes.txt | DELETED AT: 2026-07-08 10:15:02 | ORIGINAL PATH: /home/user/notes.txt
$ trash --restore
$ trash --purge
```

---

## `nix` — built-in AI assistant

`nix` sends a prompt to an LLM (via [Ollama](https://ollama.com)'s chat API) without leaving the
shell, keeping conversation history for the duration of the session so follow-up questions have
context.

### Usage

```
nix <prompt>
nix --format=json <prompt>
nix --model=<model-name> <prompt>
nix --len=<word-count> <prompt>
```

Flags can be combined:

```
nix --format=text/plain --len=50 "explain what a zombie process is"
```

### Options

| Flag | Default | Description |
|---|---|---|
| `--model` | `gpt-oss:120b` | Which model to query |
| `--format` | `markdown` | `markdown`, `text/plain`, or `json` |
| `--len` | `200` | Approximate maximum response length, in words |

Flags use `--flag=value` syntax; an unrecognized flag or a flag missing `=value` where one is
expected is rejected with an error rather than silently ignored.

### Setup

`nix` requires an API key in the environment:

```bash
export OLLAMA_API_KEY=your_key_here
```

### Notes

- Responses stream to the terminal character-by-character rather than all at once.
- Conversation history is kept in memory for the session and sent with each request, so `nix` can
  follow up on earlier questions — history doesn't persist across restarts.
- The assistant is scoped toward shell, programming, and systems topics, but will answer general
  questions too.

---

## Project Structure

```
Nanoshell/
├── src/
│   ├── core/       # parsing, execution loop, signal/terminal control
│   ├── builtins/   # cd, exit, help, pwd, trash, nix
│   └── utils/      # allocators, string helpers, HTTP client, timestamps
├── include/        # header files
├── lib/cJSON/      # vendored cJSON library (used by nix)
├── obj/            # object files (generated)
├── bin/            # compiled executable
├── Makefile
└── README.md
```

---

## Building

```bash
make
```

Produces the executable at `bin/nsh`.

A debug build with AddressSanitizer:

```bash
make debug
```

produces `bin/nsh_debug`.

### Dependencies

- `libcurl` (development headers) — required for `nix`'s HTTP requests

---

## Running

```bash
./bin/nsh
```

---

## Cleaning up build artifacts

```bash
make clean
```

---

## Roadmap

- Pipes (`|`) and output/input redirection (`>`, `>>`, `<`)
- Multi-target `trash <file1> <file2> ...`
- Environment variable expansion (`$HOME`, etc.)
- Background job control

---

## Motivation

Built to understand how shells like `bash` actually work under the hood — process management,
signal handling, terminal control, and parsing — while extending that foundation with a couple of
genuinely useful features beyond what a typical toy shell implements.

---

## Author

Saswata Mondal
GitHub — [rooooop5](https://github.com/rooooop5)