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
- [x] Add a minimal DDE Shell applet.
- [x] Add registry unit tests.
- [ ] Validate build on Deepin 25 with the packaged DDE Shell development files.
- [ ] Validate registrar introspection against GTK/Qt exporters.

## Milestone 2 — DBusMenu importer and UI

- [x] Implement `com.canonical.dbusmenu` client support.
- [x] Fetch the root layout with `GetLayout` and recursively model menu nodes.
- [x] Handle `LayoutUpdated` by refreshing the exported tree.
- [x] Handle `ItemsPropertiesUpdated` in-place, with a full refresh fallback for unknown nodes.
- [x] Send `Event(clicked)` actions.
- [x] Send `AboutToShow` and refresh when the exporter reports a change.
- [x] Expose a recursive QVariant tree suitable for QML.
- [x] Add `dgm-inspect` for direct service/path or registrar-window inspection.
- [x] Add DBusMenu data-model tests.
- [x] Implement interactive top-level menus and recursive submenus in QML.
- [x] Render separators, disabled items, and check/radio toggle state.
- [ ] Validate the wire decoder and interactive menu behavior against real GTK/Qt exporters on Deepin.
- [ ] Replace/augment the QVariant tree with a dedicated QAbstractItemModel if profiling or final UI behavior requires it.

## Milestone 3 — Active window tracking

### X11 / XWayland

- [x] Add an `ActiveWindowTracker` abstraction.
- [x] Implement EWMH/XCB `_NET_ACTIVE_WINDOW` tracking.
- [x] Map the active X11 window ID directly to registrar registrations.
- [x] Add protocol-independent tests around tracker/controller handoff.
- [ ] Validate focus changes, transient windows, and XWayland behavior on Deepin 25.

### Treeland / Wayland

- [ ] Identify the supported DDE/Treeland active-toplevel interface.
- [ ] Avoid depending on X11 window IDs in the Wayland-native path.
- [ ] Implement a Treeland tracker behind the same abstraction.
- [ ] Map Wayland-native application/toplevel identity to exported global-menu endpoints.

Treeland protocol extensions are still experimental upstream, so this backend should be isolated behind the tracker abstraction rather than leaking compositor-specific details into the controller.

## Milestone 4 — Native top panel

- [ ] Decide between an applet embedded in an existing DDE panel and a dedicated `DPanel` plugin after real-system validation.
- [ ] Left side: launcher/application identity + global menu.
- [ ] Right side: preserve/compose Deepin system indicators rather than reimplementing them where possible.
- [ ] Support multi-monitor placement and configurable panel height.
- [ ] Add overflow behavior for applications with very wide menu bars.

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

Then add visual integration: theme colors, blur/transparency, spacing, keyboard navigation, accessibility, localization, and packaging.

## Immediate next engineering task

Run the X11 prototype on a clean Deepin 25 system and validate automatic focus-to-menu switching against real Qt and GTK exporters. Fix protocol/runtime differences found there before starting the Treeland-native tracker. After that, decide whether the final shell surface should remain an applet or move to a dedicated panel plugin.
