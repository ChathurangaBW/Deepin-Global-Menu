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
(X11 / Treeland)        <-- next major milestone
        |
        v
+-----------------------+
| GlobalMenuController  |
+-----------------------+
        |
        v
+-----------------------+
| DBusMenu importer     |
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

- [x] Implement `com.canonical.dbusmenu` client support.
- [x] Fetch the root layout with `GetLayout` and recursively model menu nodes.
- [x] Handle `LayoutUpdated` by refreshing the exported tree.
- [x] Handle `ItemsPropertiesUpdated` in-place, with a full refresh fallback for unknown nodes.
- [x] Send `Event(clicked)` actions.
- [x] Send `AboutToShow` and refresh when the exporter reports a change.
- [x] Expose a recursive QVariant tree suitable for QML prototyping.
- [x] Render real top-level labels in the diagnostic DDE applet; never synthesize fake menu entries.
- [x] Add `dgm-inspect` for direct service/path or registrar-window inspection.
- [x] Add DBusMenu data-model tests.
- [ ] Validate the wire decoder against real GTK/Qt exporters on Deepin.
- [ ] Replace/augment the QVariant tree with a dedicated QAbstractItemModel if required by final submenu rendering.
- [ ] Implement interactive recursive submenus in QML.

## Milestone 3 — Active window tracking

### X11

- [ ] Add an `ActiveWindowTracker` abstraction.
- [ ] Implement EWMH/XCB `_NET_ACTIVE_WINDOW` tracking.
- [ ] Map the active X11 window ID directly to registrar registrations.
- [ ] Add tests around tracker/controller handoff where protocol-independent.

### Treeland / Wayland

- [ ] Identify the supported DDE/Treeland active-toplevel interface.
- [ ] Avoid depending on X11 window IDs in the Wayland-native path.
- [ ] Implement a Treeland tracker behind the same abstraction.

## Milestone 4 — Native top panel

- [ ] Decide between an applet embedded in an existing DDE panel and a dedicated `DPanel` plugin after the active-window prototype is running.
- [ ] Left side: launcher/application identity + global menu.
- [ ] Right side: preserve/compose Deepin system indicators rather than reimplementing them where possible.
- [ ] Support multi-monitor placement and configurable panel height.

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

Validate `dgm-inspect` against a real exported menu on Deepin, then implement the X11 active-window tracker behind a small abstraction. Once the controller can automatically select the active window's registered endpoint, build the first interactive top-level menu/submenu UI. Treeland support follows behind the same tracker interface.
