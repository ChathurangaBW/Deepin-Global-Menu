# Deepin Global Menu

A native global application menu for modern Deepin/DDE, targeting the current `dde-shell` plugin architecture.

> **Status:** early development. The first milestone implements the AppMenu registrar/registry foundation and a diagnostic DDE Shell applet. Real DBusMenu rendering and active-window tracking are the next steps.

## Goals

- Native DDE Shell integration instead of embedding a foreign desktop panel.
- Qt 6 / modern Deepin support.
- Global menu model driven by the active application's exported DBusMenu.
- Clean separation between active-window tracking, AppMenu registration, DBusMenu importing, and shell UI.
- X11 support where needed, with Treeland/Wayland as a first-class target.

## Current architecture

- `dgm-core`: AppMenu registrar, menu endpoint registry, and UI-facing controller.
- `ds-global-menu`: DDE Shell applet wrapper and diagnostic QML.
- `tests`: protocol-independent registry tests.
- `docs/PLAN.md`: milestone plan and compatibility strategy.

## Build prerequisites

The current scaffold expects a modern Deepin development environment containing:

- CMake 3.16+
- Qt 6 Core, DBus, QML and Quick development packages
- installed DDE Shell CMake package (`DDEShellConfig.cmake`)
- DDE Shell development headers/libraries exporting `Dde::Shell`

Typical development build:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The project has not yet been validated against a clean Deepin 25 SDK image; that is part of milestone 1.

## Roadmap

See [`docs/PLAN.md`](docs/PLAN.md).

## License

Project code is intended to be GPL-3.0-or-later. Third-party/reference code must retain its original licensing and attribution.
