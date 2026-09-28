# Deepin Global Menu

A native global application menu for modern Deepin/DDE, implemented as a DDE Shell applet with X11/XWayland and Treeland-aware backends.

> **Status: v0.4.0 release-candidate implementation.** Core tests and the complete DDE Shell plugin build pass in CI, including an official Deepin 25 container. The remaining validation is graphical runtime testing inside a real Deepin 25 desktop session, because a headless CI container cannot exercise compositor focus, popup placement, or real application keyboard focus.

## What works

- Owns `com.canonical.AppMenu.Registrar` and tracks registered DBusMenu endpoints.
- Imports `com.canonical.dbusmenu` recursively, including separators, enabled/visible state, toggles, submenu preparation, live layout/property updates, and action activation.
- Imports modern GTK `org.gtk.Menus` and `org.gtk.Actions` exports, including live exporter changes and action/toggle state on Qt 6.8+.
- Tracks the active X11/XWayland window through EWMH `_NET_ACTIVE_WINDOW` and reads GTK application metadata from X properties.
- Tracks the active native Treeland application through DDE Shell's task-manager model instead of depending on X11 window IDs.
- Prefers real exported menus in this order: DBusMenu, GTK menus/actions, then a safe fallback menu.
- Provides Deepin-native fallback application/window actions through the DDE task manager.
- Provides common File/Edit/View/Help fallback actions when keyboard injection is available:
  - X11: XTest.
  - Treeland: `zwp_virtual_keyboard_manager_v1`.
- Renders clickable top-level buttons, recursive submenus, separators, disabled entries, and check/radio state in QML.
- Includes `dgm-inspect` for DBusMenu diagnostics.
- Builds and tests both the portable core and the full DDE Shell plugin in CI.

## Architecture

```text
                         +---------------------------+
                         | Active application/window |
                         +-------------+-------------+
                                       |
                    +------------------+------------------+
                    |                                     |
              X11 / XWayland                         Treeland
          _NET_ACTIVE_WINDOW               DDE task-manager dataModel
                    |                                     |
                    +------------------+------------------+
                                       |
                              GlobalMenuController
                                       |
             +-------------------------+-------------------------+
             |                         |                         |
       DBusMenu importer          GTK importer              Safe fallback
  com.canonical.dbusmenu   org.gtk.Menus/Actions      DDE actions + shortcuts
             |                         |                         |
             +-------------------------+-------------------------+
                                       |
                                 DDE Shell QML
```

The controller does not expose an unverified menu source. A fallback is available immediately; a DBusMenu or GTK source replaces it only after its exporter has been successfully imported.

## Build prerequisites

Core development needs:

- CMake 3.16+
- Qt 6 Core and DBus
- `pkg-config`
- XCB development files for the optional X11 active-window tracker

The full DDE Shell plugin additionally needs:

- Qt 6 Gui, QML, Quick
- DDE Shell development package exporting `Dde::Shell`
- DTK 6 Core/Gui development packages required by DDE Shell
- Qt Wayland + xkbcommon for native Treeland shortcut fallback
- XTest for X11 shortcut fallback

On Deepin 25, the CI dependency set in `.github/workflows/core-qa.yml` is the reference build environment.

### Full Deepin build

```bash
cmake -S . -B build -G Ninja
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

### Core/tools-only build

```bash
cmake -S . -B build-core -G Ninja \
  -DDGM_BUILD_PLUGIN=OFF \
  -DDGM_BUILD_TOOLS=ON \
  -DDGM_BUILD_TESTS=ON
cmake --build build-core --parallel
dbus-run-session -- ctest --test-dir build-core --output-on-failure
```

To explicitly disable the X11 tracker:

```bash
cmake -S . -B build-core -DDGM_BUILD_PLUGIN=OFF -DDGM_ENABLE_X11_TRACKER=OFF
```

## Runtime source selection

For each active application, the controller uses the highest-fidelity source that actually responds:

1. A registered `com.canonical.dbusmenu` endpoint.
2. A GTK `org.gtk.Menus` / `org.gtk.Actions` export discovered from active-application identity.
3. A fallback menu with DDE task-manager actions and, where supported, common keyboard shortcuts.

The fallback is intentionally conservative. It does not pretend to know application-specific commands that cannot be exported or safely inferred.

### Qt compatibility note for GTK actions

Qt versions before 6.8 mishandle the valid empty D-Bus signature used by zero-parameter GActions. On those older Qt versions the GTK menu hierarchy and activation path still work, but `DescribeAll`-based action-state/fallback grouping is disabled. Deepin 25 uses Qt 6.8+, where the full path is enabled.

## Diagnostics

Resolve a menu through the running AppMenu registrar using an X11 window ID:

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

## Validation

CI currently verifies:

- warning-enabled core compilation on Ubuntu 24.04;
- registry, DBusMenu, GTK importer, controller, fallback, and D-Bus integration tests;
- inspector CLI smoke test;
- a full plugin configure/build/test cycle inside `linuxdeepin/deepin:25`;
- generated Treeland virtual-keyboard client code and XTest linkage when available.

A real graphical Deepin 25 session is still needed to validate things a headless container cannot reproduce: shell applet loading, popup geometry, focus restoration after menu activation, real Treeland virtual-keyboard delivery, XWayland focus changes, multiple monitors, and behavior across actual third-party applications.

## Compatibility targets

The implementation has explicit paths for:

- Deepin/DTK and Qt applications exporting DBusMenu;
- GTK applications exporting `org.gtk.Menus` / `org.gtk.Actions`;
- X11/XWayland applications;
- native Treeland applications;
- applications such as Firefox, Chromium, and Electron that may require safe fallback actions when no semantic menu export is available.

## License

Project code is GPL-3.0-or-later. The vendored virtual-keyboard protocol XML retains its upstream permissive copyright/license notice.
