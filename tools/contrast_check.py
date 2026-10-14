"""Contrast gate for the QML theme (accessibility W8).

Parses the color tokens out of qml/theme/Theme.qml and checks every
foreground/background pair the UI actually uses against its WCAG 2.x threshold
(text 4.5:1, control boundaries 3:1). The pair list mirrors the audit method in
docs/accessibility.md section A7; re-run after any palette change:

    python tools/contrast_check.py

Accepted-as-decoration pairs (user decision 2026-10-13, see accessibility.md):
colorTextDisabled stays dim on purpose - WCAG exempts inactive controls. They are
reported informationally and never fail the run.
"""

import re
import sys
from pathlib import Path

THEME = Path(__file__).resolve().parent.parent / "qml" / "theme" / "Theme.qml"


def lum(hex_color: str) -> float:
    r, g, b = (int(hex_color[i:i + 2], 16) / 255 for i in (1, 3, 5))
    lin = [c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4 for c in (r, g, b)]
    return 0.2126 * lin[0] + 0.7152 * lin[1] + 0.0722 * lin[2]


def ratio(fg: str, bg: str) -> float:
    light, dark = sorted((lum(fg), lum(bg)), reverse=True)
    return (light + 0.05) / (dark + 0.05)


def parse_tokens(path: Path) -> dict:
    tokens = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        m = re.match(r'\s*readonly property color (\w+): "#([0-9a-fA-F]{6})"', line)
        if m:
            tokens[m.group(1)] = "#" + m.group(2).lower()
    return tokens


# (foreground, background, minimum ratio, where the pair is used)
CHECKED = [
    ("colorText", "colorBg", 4.5, "window/status text"),
    ("colorText", "colorSurface", 4.5, "panel text"),
    ("colorText", "colorSurfaceRaised", 4.5, "toast/raised-surface text"),
    ("colorText", "colorSurfaceSunken", 4.5, "field text"),
    ("colorTextMuted", "colorBg", 4.5, "muted hints on the window"),
    ("colorTextMuted", "colorSurface", 4.5, "muted hints in panels"),
    ("colorTextMuted", "colorSurfaceRaised", 4.5, "muted hints on raised surfaces"),
    ("colorRed", "colorBg", 4.5, "start-page error text"),
    ("colorRed", "colorSurface", 4.5, "error text in panels"),
    ("colorRed", "colorSurfaceRaised", 4.5, "save-failure toast text"),
    ("colorAccentText", "colorAccent", 4.5, "primary button / bind overlay label"),
    ("colorKeyCapText", "colorKeyCap", 4.5, "cap label"),
    ("colorKeyCapText", "colorKeyCapHover", 4.5, "hovered cap label"),
    ("colorKeyCapText", "colorKeyCapBound", 4.5, "bound cap label"),
    # control boundaries (WCAG 1.4.11)
    ("colorAccent", "colorSurfaceSunken", 3.0, "field focus ring"),
    ("colorAccent", "colorSurfaceRaised", 3.0, "button/tab focus ring"),
    ("colorBorderStrong", "colorSurfaceSunken", 3.0, "field border"),
    ("colorBorder", "colorSurface", 3.0, "panel border"),
    ("colorKeyCapBorder", "colorKeyCap", 3.0, "cap boundary"),
    ("colorKeyCapBoundBorder", "colorKeyCapBound", 3.0, "bound cap boundary"),
]

# Accepted as decoration (user decisions 2026-10-13): reported, never fatal.
ACCEPTED = [
    ("colorTextDisabled", "colorSurface", "disabled label in a panel"),
    ("colorTextDisabled", "colorBg", "disabled button label"),
]


def main() -> int:
    tokens = parse_tokens(THEME)
    if len(tokens) < 20:
        print(f"FAIL: only {len(tokens)} color tokens parsed from {THEME}")
        return 2

    failures = 0
    for fg, bg, minimum, where in CHECKED:
        missing = [name for name in (fg, bg) if name not in tokens]
        if missing:
            print(f"FAIL: token(s) {missing} not in Theme.qml (pair {fg}/{bg})")
            failures += 1
            continue
        r = ratio(tokens[fg], tokens[bg])
        mark = "ok " if r >= minimum else "FAIL"
        if r < minimum:
            failures += 1
        print(f"{mark} {r:5.2f} (min {minimum}) {fg} on {bg} - {where}")

    print("-- accepted as decoration (informational) --")
    for fg, bg, where in ACCEPTED:
        if fg in tokens and bg in tokens:
            print(f"     {ratio(tokens[fg], tokens[bg]):5.2f}       {fg} on {bg} - {where}")

    if failures:
        print(f"\n{failures} failing pair(s); fix Theme.qml or the pair list.")
        return 1
    print("\nall checked pairs pass")
    return 0


if __name__ == "__main__":
    sys.exit(main())
