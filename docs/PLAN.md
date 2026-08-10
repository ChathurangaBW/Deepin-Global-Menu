# Development Plan

## Target

Build a native macOS-style global application menu for modern Deepin/DDE without replacing the Deepin shell or dock.

The implementation is split into protocol, window tracking, menu import, and presentation layers so X11 and Treeland/Wayland can evolve independently.

## Architecture

```text
Application menu exporter
        |
        | com.canonical.AppMenu.Registrar
        v
+-----------------------+
| AppMenuRegistrar      |
+-----------------------+
        |
        v
+-----------------------+
| MenuRegistry          |
+-----------------------+
        ^
        |
Active window tracker
(X11 / Treeland)
        |
        v
+-----------------------+
| GlobalMenuController  |
+-----------------------+
        |
        v
+-----------------------+
| DBusMenu importer     |  <-- next milestone
+-----------------------+
        |
        v
DDE Shell applet / panel UI
```

## Milestone 1 — Registrar foundation

- [x] Bootstrap out-of-tree CMake project.
- [x] Link against the installed `DDEShell` package / `Dde::Shell` target.
- [x] Implement window-to-menu endpoint registry.
- [x] Implement initial `com.canonical.AppMenu.Registrar` session-bus service.
- [x] Remove stale window registrations when a DBus client disappears.
- [x] Add shell-facing controller properties.
- [x] Add a minimal DDE Shell applet and diagnostic QML.
- [x] Add registry unit tests.
- [ ] Validate build on Deepin 25 with the packaged DDE Shell development files.
- [ ] Validate registrar introspection against GTK/Qt exporters.

## Milestone 2 — DBusMenu importer

- Implement `com.canonical.dbusmenu` client support.
- Fetch root layout and recursively model menu nodes.
- Handle `LayoutUpdated`, `ItemsPropertiesUpdated`, and action events.
- Expose a `QAbstractItemModel` suitable for QML.
- Render real top-level menu labels; never synthesize fake menu entries.

## Milestone 3 — Active window tracking

### X11

- Implement EWMH/XCB `_NET_ACTIVE_WINDOW` tracker.
- Map the active X11 window ID directly to registrar registrations.

### Treeland / Wayland

- Identify the supported DDE/Treeland active-toplevel interface.
- Avoid depending on X11 window IDs in the Wayland-native path.
- Add an abstraction so the controller receives a stable active-window identity independent of compositor.

## Milestone 4 — Native top panel

- Decide between an applet embedded in an existing DDE panel and a dedicated `DPanel` plugin.
- Left side: launcher/application identity + global menu.
- Right side: preserve/compose Deepin system indicators rather than reimplementing them where possible.
- Support multi-monitor placement and configurable panel height.

## Milestone 5 — Compatibility and polish

Test separately:

- Deepin/DTK applications
- Qt 5 / Qt 6
- GTK 2 / GTK 3
- GTK 4/libadwaita
- LibreOffice
- Firefox
- Chromium
- Electron applications

Then add visual integration: theme colors, blur/transparency, spacing, overflow handling, keyboard navigation, and accessibility.

## Immediate next engineering task

Implement the DBusMenu importer and add a small command-line diagnostic harness that can print the exported menu tree for a registered window. This provides a protocol-level test before coupling menu rendering to DDE Shell UI.
