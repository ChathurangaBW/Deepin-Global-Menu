# Deepin Global Menu

A native global application menu for modern Deepin/DDE, targeting the current `dde-shell` plugin architecture.

> Status: early development. The repository is being bootstrapped around Qt 6, DDE Shell, and the `com.canonical.AppMenu.Registrar` / DBusMenu protocol.

## Goals

- Native DDE Shell integration instead of embedding a foreign desktop panel.
- Qt 6 / modern Deepin support.
- Global menu model driven by the active application's exported DBusMenu.
- Clean separation between window tracking, AppMenu registration, DBusMenu importing, and shell UI.
- X11 support first where necessary, with Treeland/Wayland treated as a first-class target rather than an afterthought.

## Development

Development work is currently happening on feature branches until the first runnable prototype is ready.

## License

GPL-3.0-or-later is planned for project code. Third-party/reference code must retain its original licensing and attribution.
