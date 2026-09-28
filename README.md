# Deepin Global Menu

A native global application menu for modern Deepin/DDE, targeting the current `dde-shell` plugin architecture.

> **Status:** X11 prototype milestone. The AppMenu registrar, DBusMenu importer, automatic X11 active-window tracking, and interactive recursive menu UI are implemented. Deepin 25 validation and a native Treeland/Wayland active-window backend are still required before calling this production-ready.

## Goals

- Native DDE Shell integration instead of embedding a foreign desktop panel.
- Qt 6 / modern Deepin support.
- Global menu model driven by the active application's exported DBusMenu.
- Clean separation between active-window tracking, AppMenu registration, DBusMenu importing, and shell UI.
- X11 support where needed, with Treeland/Wayland as a first-class target.

## Current architecture

- `dgm-core`: AppMenu registrar, menu endpoint registry, DBusMenu importer, active-window tracker abstraction, and UI-facing controller.
- X11 backend: watches EWMH `_NET_ACTIVE_WINDOW` through XCB and selects the matching registered DBusMenu endpoint automatically.
- `ds-global-menu`: DDE Shell applet wrapper with clickable top-level menus and recursive submenus.
- `dgm-inspect`: command-line protocol diagnostic tool.
- `tests`: registry, importer, data-model, and tracker/controller handoff tests.
- `docs/PLAN.md`: milestone plan and compatibility strategy.

## Build prerequisites

Core protocol development needs:

- CMake 3.16+
- Qt 6 Core and DBus development packages

Automatic X11 active-window tracking additionally uses:

- `pkg-config`
- XCB development files (`libxcb1-dev` on Debian/Ubuntu/Deepin)

If XCB is unavailable, the core still builds, but automatic active-window selection is disabled.

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

To explicitly disable the X11 tracker:

```bash
cmake -S . -B build-core -DDGM_BUILD_PLUGIN=OFF -DDGM_ENABLE_X11_TRACKER=OFF
```

## Runtime behavior

On X11, the applet watches the root window's `_NET_ACTIVE_WINDOW` property. When focus changes, the controller looks up that window ID in `com.canonical.AppMenu.Registrar`, imports the matching `com.canonical.dbusmenu` tree, and exposes it to the QML applet.

The QML layer renders top-level menu buttons, recursive submenus, separators, enabled/disabled state, and check/radio toggle state. Actions are sent back through DBusMenu `Event(clicked)`, while submenu opening triggers `AboutToShow`.

On a Wayland/Treeland session, XWayland applications may still be discoverable when an X11 display is available, but a native Treeland active-toplevel backend is not implemented yet.

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

## Validation still required

Before treating the project as release-ready, validate it on a clean Deepin 25 installation against real exporters from GTK, Qt, LibreOffice, Firefox/Chromium, and Electron applications. The Treeland/Wayland native active-window path also remains outstanding.

## Roadmap

See [`docs/PLAN.md`](docs/PLAN.md).

## License

Project code is GPL-3.0-or-later. Third-party/reference code must retain its original licensing and attribution.
