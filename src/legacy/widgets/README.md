# Retired widget UI

This directory holds the previous presentation layer of tf2cfg: the Qt Widgets implementation
(`StartWindow`, `EditWindow`, `KeybForm`, `BindEditDialog`) and its four Qt Designer `.ui` files.

**It is not built.** The application now uses the QML UI in `qml/`. These files are kept because
they are the source of truth for the parity contract in `docs/ui-qml-rewrite.md` — every behaviour
of the QML UI was audited against them.

## Why it was replaced

`docs/ui-qml-rewrite.md` describes the rewrite. Short version: the widget build could not express
the intended features (painting loaded binds onto the keyboard, type-aware argument widgets,
hover/anim state on 100+ key caps) without a large amount of imperative C++ and per-widget
styling. QML does it declaratively, and the key table became data instead of 104 hand-placed
buttons.

## Reviving it (not recommended)

Add to `tf2cfgreal.pro`:

```pro
QT += widgets

INCLUDEPATH += ./src/legacy/widgets

SOURCES += \
    src/legacy/widgets/bindeditdialog.cpp \
    src/legacy/widgets/editwindow.cpp \
    src/legacy/widgets/keybform.cpp \
    src/legacy/widgets/startwindow.cpp

HEADERS += \
    src/legacy/widgets/bindeditdialog.h \
    src/legacy/widgets/editwindow.h \
    src/legacy/widgets/keybform.h \
    src/legacy/widgets/startwindow.h

FORMS += \
    src/legacy/widgets/bindeditdialog.ui \
    src/legacy/widgets/editwindow.ui \
    src/legacy/widgets/keybform.ui \
    src/legacy/widgets/startwindow.ui
```

Then point `src/main.cpp` back at `ui/startwindow.h` instead of loading `qrc:/qml/Main.qml`.
`GameCfg` and `Lexers` are unchanged since the rewrite, so the old classes still compile against
them; only the include path in `editwindow.ui`'s custom-widget hint needed updating after the move.

Note that the widget build contains the defects catalogued in `docs/CODE_REVIEW.md`, including the
`Go`-button enable rule that the QML rewrite deliberately fixed (deviation D3).
