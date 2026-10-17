# TF2 Config Manager

A desktop app for reading and editing Team Fortress 2 config files: your `config.cfg`,
`autoexec.cfg` and others in `tf/cfg/`. It works on the files
directly; it does not launch the game or talk to a running server.

![The Overview tab: every parsed line with its catalog input signature, platform,
context and cheat flag](docs/screenshots/overview.png)

Features:

- **Reads and writes configs the way the game does.** Same syntax rules as TF2's own
  config reader, and anything the app doesn't understand is saved back byte-for-byte,
  untouched.
- **Explains the commands.** A built-in catalog of ~3800 game commands and console
  variables with documented argument types, allowed values, platform/game usage context, argument ranges
  and usage information.
- **Builds binds.** Click a key on the visual keyboard/mouse/controller, then pick a command and fill in
  its arguments. You can also create and edit aliases and alias chains.
- **Edits safely.** Insert, change or delete commands, comments and multi-command
  (`;`-separated) lines. Changes are anchored back into the original file, and a backup
  is written before every save.

## Download

Grab the [latest release](https://github.com/carrotcake/tf2cfg/releases/latest):

| Platform | Download | Run it |
|---|---|---|
| Windows x64 | `TF2-Config-Manager-windows-x64.zip` | unzip, then double-click `tf2cfg\tf2cfgreal.exe` |
| Linux x86_64 | `TF2-Config-Manager-x86_64.AppImage` | `chmod +x`, then run it |

No extra installs needed - the packages ship their Qt runtime. The AppImage needs a
recent Linux distribution.

## Building it yourself

Needs Qt 6.12 LTS (newer or older versions are not supported) and its qmake tool. From the
repo root:

```sh
cd build      && qmake6 -o Makefile ../tf2cfgreal.pro && make -j8   # -> tf2cfgreal
cd build-test && qmake6 -o Makefile ../tests/tests.pro && make -j8   # -> tf2cfg_test
cd build-test && ./tf2cfg_test   # run the test suite
```

On Windows use `mingw32-make` and find the binaries under `release/` - see
`AGENTS.md` for the full build, test and lint workflow for both platforms.

## Docs

- `docs/ARCHITECTURE.md` - how the pieces fit together and why.
- `docs/ROADMAP.md` - open work.
- `docs/accessibility.md` - accessibility findings and fixes.
- `docs/legacy/` - historical records.
- `AGENTS.md` - developer conventions (written for coding agents; the build commands
  for Windows and Linux live in there too).

## License

MIT for the code - see [LICENSE](LICENSE). The bundled Qt runtime is Qt's own software,
under the LGPLv3 (https://www.qt.io/licensing).
