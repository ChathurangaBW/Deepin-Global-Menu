# Deepin Global Menu

A native global application menu for modern Deepin/DDE, targeting the current `dde-shell` plugin architecture.

> **Status:** active development. The AppMenu registrar foundation and the first real `com.canonical.dbusmenu` importer are implemented on the development branch. Active-window tracking and interactive submenu rendering are next.

## Goals

- Native DDE Shell integration instead of embedding a foreign desktop panel.
- Qt 6 / modern Deepin support.
- Global menu model driven by the active application's exported DBusMenu.
- Clean separation between active-window tracking, AppMenu registration, DBusMenu importing, and shell UI.
- X11 support where needed, with Treeland/Wayland as a first-class target.

## Current architecture

- `dgm-core`: AppMenu registrar, menu endpoint registry, DBusMenu importer, and UI-facing controller.
- `ds-global-menu`: DDE Shell applet wrapper; currently renders real imported top-level labels when a menu is selected.
- `dgm-inspect`: command-line protocol diagnostic tool.
- `tests`: registry and DBusMenu data-model tests.
- `docs/PLAN.md`: milestone plan and compatibility strategy.

## Build prerequisites

Core protocol development only needs:

- CMake 3.16+
- Qt 6 Core and DBus development packages

The DDE Shell plugin additionally needs:

- Qt 6 QML and Quick development packages
- installed DDE Shell CMake package (`DDEShellConfig.cmake`)
- DDE Shell development headers/libraries exporting `Dde::Shell`

Full Deepin development build:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Core/tools-only build, useful outside a Deepin SDK image:

```bash
cmake -S . -B build-core -DDGM_BUILD_PLUGIN=OFF
cmake --build build-core
ctest --test-dir build-core --output-on-failure
```

## Inspecting a real exported menu

Resolve through a running AppMenu registrar using an X11 window ID:

```bash
dgm-inspect --window 0x04600007
```

Or inspect a known exporter directly:

```bash
dgm-inspect \
  --service :1.234 \
  --path /com/canonical/menu/123
```

Add `--watch` to print the tree again when the exporter updates it.

The project still needs validation against a clean Deepin 25 SDK/system and real GTK/Qt exporters before the current milestone is considered production-ready.

## Roadmap

See [`docs/PLAN.md`](docs/PLAN.md).

## License

Project code is GPL-3.0-or-later. Third-party/reference code must retain its original licensing and attribution.
