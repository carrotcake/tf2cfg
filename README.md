# tf2cfg

A Qt 6 / QML desktop app for reading and editing Team Fortress 2 `.cfg` files - the
class configs and `autoexec.cfg` under `tf/cfg/` in the game folder. It works on the
files: it does not launch the game and does not talk to a running server.

![The Overview tab: every parsed line with its catalog input signature, platform,
context and cheat flag](docs/screenshots/overview.png)

What it does:

- **Parses like the game.** The lexer reproduces TF2's tokenizer, and the file on disk
  stays the source of truth - lines the app does not own are written back byte-for-byte.
- **Browse and search.** The parsed commands as a table, plus the ~3800-row command
  catalog with argument types, enums and bounds.
- **Build binds.** Click a key on the visual keyboard and fill the dialog: catalog
  command picker, per-command argument panels, alias support.
- **Edit in place.** Insert/edit/delete commands, comment rows and compound
  (`;`-separated) lines, with save anchors back into the original file and a backup
  written before every save.
- Keyboard reachable throughout, with a visible focus ring (see
  `docs/accessibility.md`).

## Download

Both artifacts are attached to the
[latest release](https://github.com/carrotcake/tf2cfg/releases/latest):

| Platform | Asset | How to run |
|---|---|---|
| Windows x64 | `TF2-Config-Manager-windows-x64.zip` | unzip, then run `tf2cfg\tf2cfgreal.exe` |
| Linux x86_64 | `TF2-Config-Manager-x86_64.AppImage` | `chmod +x` it, then run it |

The zip carries its own Qt. The AppImage targets current distributions (glibc 2.39 /
Qt 6.4.2 floor).

## Build

Windows, the toolchain the releases use - Qt 6.12 at `C:\Qt\6.12.0\mingw_64` and MinGW at
`C:\Qt\Tools\mingw1310_64\bin`, both prepended to `PATH`:

```pwsh
cd build      && qmake6 -o Makefile ..\tf2cfgreal.pro && mingw32-make -j8   # -> release/tf2cfgreal.exe
cd build-test && qmake6 -o Makefile ..\tests\tests.pro && mingw32-make -j8  # -> release/tf2cfg_test.exe
cd build-test && .\release\tf2cfg_test.exe -o testlog.txt,txt
```

Linux (system Qt; the repo is developed on two boxes, whichever Qt is installed must be
6.12 - see AGENTS.md "Build, test, lint"):

```sh
cd build       && qmake6 -o Makefile ../tf2cfgreal.pro && make -j8   # -> build/tf2cfgreal
cd build-test  && qmake6 -o Makefile ../tests/tests.pro && make -j8   # -> build-test/tf2cfg_test
cd build-test  && QT_QPA_PLATFORM=offscreen ./tf2cfg_test -o testlog.txt,txt
cd build-qmltest && qmake6 -o Makefile ../tests/qml/qmltests.pro && make -j8
cd build-qmltest && QT_QPA_PLATFORM=offscreen ./tst_app -o testlog.txt,txt
```

Notes: no `release/` subdir on Linux - qmake puts the binary in the build dir root
(Windows/MinGW differs); local lint via `qmllint -I qmlmodules` over
`qml tests/qml`.

The bars, checked before every commit: the engine suite green, `qmllint` at 0 errors, and
`python3 tools/contrast_check.py` after any `Theme.qml` change.

Notes: the QML is compiled into the binary through `qml.qrc`, so QML edits need a rebuild;
`pwsh -File deploy.ps1` packages a self-contained `dist/` on Windows.

## Docs

- `AGENTS.md` - layout, build/test/lint workflow, the project's bars.
- `docs/ARCHITECTURE.md` - the live contracts (parser, surgical save, models, QML rules)
  and the numbered deviations D1-D20.
- `docs/ROADMAP.md` - the single list of open work.
- `docs/accessibility.md` - the A1-A8 findings and the W1-W8 work items.
- `docs/legacy/` - dated records (audits, reviews, the original QML-rewrite design).

## License

MIT - see [LICENSE](LICENSE).

The Qt 6 runtime libraries bundled with the Windows zip and the AppImage are Qt's own,
under the LGPLv3 (`https://www.qt.io/licensing`); the MIT license above covers this
project's code.
