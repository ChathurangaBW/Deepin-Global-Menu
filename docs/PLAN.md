# Development Plan

## Target

Deliver a native macOS-style global application menu for modern Deepin/DDE without replacing the Deepin shell or dock.

## v0.4.0 implementation status

### Registrar and protocol core

- [x] Out-of-tree CMake project.
- [x] DDE Shell plugin integration through `Dde::Shell`.
- [x] `com.canonical.AppMenu.Registrar` implementation.
- [x] Window-to-menu endpoint registry with stale D-Bus client cleanup.
- [x] Recursive `com.canonical.dbusmenu` import.
- [x] `LayoutUpdated` and `ItemsPropertiesUpdated` handling.
- [x] `Event(clicked)` activation.
- [x] `AboutToShow` submenu preparation.
- [x] Recursive QML menu UI with separators, enabled/disabled entries, and toggle state.
- [x] `dgm-inspect` diagnostics.

### GTK application menus

- [x] `org.gtk.Menus` import.
- [x] `org.gtk.Actions` activation.
- [x] Action enabled/toggle state on Qt 6.8+.
- [x] Live GTK menu/action refresh handling.
- [x] Safe compatibility behavior for pre-Qt-6.8 empty-signature handling.

### X11 / XWayland

- [x] `ActiveWindowTracker` abstraction.
- [x] EWMH/XCB `_NET_ACTIVE_WINDOW` tracker.
- [x] GTK X11 metadata discovery.
- [x] `WM_CLASS` application identity fallback.
- [x] Direct active-window to AppMenu registrar mapping.
- [x] XTest shortcut fallback when available.

### Treeland / Wayland

- [x] Identify the supported DDE/Treeland active-application source.
- [x] Track active applications through `org.deepin.ds.dock.taskmanager`.
- [x] Avoid X11 IDs in the native Wayland menu-selection path.
- [x] Use application identity to probe GTK exports.
- [x] Use DDE task-manager methods for safe application/window fallback actions.
- [x] Generate a `zwp_virtual_keyboard_manager_v1` client for shortcut fallback.
- [x] Gate shortcut menus on actual backend availability.

### CI and target build

- [x] Core warning-enabled build.
- [x] Unit and D-Bus integration test suite.
- [x] GTK importer tests.
- [x] Controller/fallback tests.
- [x] Full DDE Shell plugin configure/build/test in `linuxdeepin/deepin:25`.
- [x] Qt Wayland protocol generation exercised in target CI.

## Source priority

```text
DBusMenu export
      |
      | if unavailable or broken
      v
GTK org.gtk.Menus / org.gtk.Actions
      |
      | if unavailable
      v
DDE application/window fallback
      |
      +-- optional shortcut actions when XTest or Treeland virtual keyboard exists
```

A source is not promoted merely because its address can be guessed. The controller keeps the safe fallback visible until an exporter has successfully answered.

## Remaining runtime validation

These items require a real graphical Deepin 25 session and are not reproducible in headless CI:

- [ ] Load/unload the applet in a stock DDE Shell session.
- [ ] Verify popup placement and keyboard navigation under the shipping DDE style.
- [ ] Verify focus returns to the target app before delayed shortcut injection.
- [ ] Verify Treeland virtual-keyboard delivery with the production compositor policy.
- [ ] Verify X11/XWayland focus switching, transient windows, and multi-window applications.
- [ ] Exercise real Qt/DTK, GTK, LibreOffice, Firefox, Chromium, and Electron applications.
- [ ] Exercise multi-monitor placement and very wide menu bars.

These are release-validation tasks rather than missing implementation backends.

## Future optional work

The current shell surface is an applet. A dedicated `DPanel` implementation, advanced overflow UI, localization, accessibility refinement, per-application shortcut profiles, and additional visual polish can be developed as later product work without changing the protocol/window-tracking architecture.
