# DualProcRpc

Two-process RPC framework — Program A calls, Program B executes, communicating over HTTP + WebSocket.

## Quick Start

```bash
# 1. Start Program B (the executor)
./ProgramB.exe
# Click "Start Services" → HTTP on :8080, WebSocket on :8081

# 2. Start Program A (the caller)
./ProgramA.exe
# Select a function, edit params, click "Call Function"
```

## Build from Source

Requires Qt 6.5+, CMake 3.16+, MinGW 8.1+.

```bash
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=<path-to-qt6>
cmake --build .
```

## Architecture

```
ProgramA (Caller) ──HTTP POST──▶ ProgramB (Executor)
                  ◀──HTTP Resp──
                  ◀══WebSocket══  (async push)
```

- **Sync mode**: HTTP request → HTTP response
- **Async mode**: HTTP request → WebSocket push

## Docs

[ARCHITECTURE.md](ARCHITECTURE.md) — full architecture, API reference, protocol spec, extension guide.
