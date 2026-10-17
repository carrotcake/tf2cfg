import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtTest
import tf2cfg

// Root is an Item that hosts the TestCase, the Qt Quick Test pattern: everything a test
// creates is parented into `stage`, so it lives in the test's own window. That matters
// because keyClick() (Qt 6 takes no item argument) delivers to the focused item of the
// test window - an item in a window of its own would never receive the keys.
Rectangle {
    id: host
    width: 1000
    height: 500
    color: "transparent"

    /// The backend singleton, pulled in where the tests need it (the app's QML does the
    /// same with `import tf2cfg`; the tests used to read a context property named `cfg`).
    readonly property var cfgRef: Cfg

    Item {
        id: stage
        anchors.fill: parent
    }

    TestCase {
        id: testCase
        name: "AppUI"
        when: windowShown

        /// The app's QML carries no objectNames, so tests walk the tree by type name.
        function findItem(root, pred) {
            if (!root)
                return null
            if (pred(root))
                return root
            const kids = root.children || []
            for (let i = 0; i < kids.length; ++i) {
                const hit = findItem(kids[i], pred)
                if (hit)
                    return hit
            }
            return null
        }

        function isType(item, name) {
            return item.toString().indexOf(name) === 0
        }

        /// Every item in the tree that matches, in tree order.
        function collectItems(root, pred, out) {
            if (!root)
                return out
            if (pred(root))
                out.push(root)
            const kids = root.children || []
            for (let i = 0; i < kids.length; ++i)
                collectItems(kids[i], pred, out)
            return out
        }

        /// Instantiate a component from the app's QML into the test window.
        function createInStage(url, props) {
            const component = Qt.createComponent(url)
            if (component.status !== Component.Ready)
                return { item: null, error: component.errorString() }
            return { item: component.createObject(stage, props), error: "" }
        }

        /// Focus lands a frame after forceActiveFocus(); keys sent before it reach nobody.
        function focusItem(item) {
            item.forceActiveFocus()
            tryVerify(function () { return item.activeFocus === true }, 1000, "item takes focus")
        }

        function spyOn(target, signalName) {
            const spy = Qt.createQmlObject("import QtTest; SignalSpy { }", testCase, "spy")
            spy.target = target
            spy.signalName = signalName
            return spy
        }

        /// Every text a row renders, so a test can assert on the visible columns.
        function rowTexts(row) {
            const texts = []
            const kids = row.children || []
            for (let i = 0; i < kids.length; ++i) {
                if (isType(kids[i], "QQuickText"))
                    texts.push(kids[i].text)
            }
            return texts
        }

        // -- the shell --------------------------------------------------------

        function test_shellLoadsThenOpensTheEditor() {
            const component = Qt.createComponent("qrc:/qml/Main.qml")
            compare(component.status, Component.Ready, component.errorString())
            const win = component.createObject(null)
            verify(win !== null, "Main.qml instantiated")
            verify(win.visible, "the window became visible")
            verify(findItem(win.contentItem, function (it) { return isType(it, "StartPage") }) !== null,
                   "the shell starts on the start page")

            // Loading a config moves the shell to the editor page, which owns the tabs.
            host.cfgRef.loadCfg(host.cfgRef.presetAt(0))
            tryCompare(host.cfgRef, "loaded", true, 10000)
            tryVerify(function () {
                return findItem(win.contentItem, function (it) { return isType(it, "Tf2TabBar") }) !== null
            }, 5000, "the editor page and its tab strip appear after a load")

            win.destroy()
            // The second window took the focus; the key tests need it back.
            host.Window.window.requestActivate()
        }

        // -- the Overview tab's catalog columns -------------------------------

        function test_overviewRowShowsCatalogColumns() {
            verify(host.cfgRef.presets.length > 0, "the bundled presets are in the resource bundle")
            host.cfgRef.loadCfg(host.cfgRef.presetAt(0))
            tryCompare(host.cfgRef, "loaded", true, 10000)

            const made = createInStage("qrc:/qml/pages/tabs/CommandOverviewTab.qml",
                                       { width: 900, height: 300 })
            compare(made.error, "")
            const tab = made.item
            verify(tab !== null, "CommandOverviewTab instantiated")
            wait(300)

            const list = findItem(tab, function (it) { return isType(it, "QQuickListView") })
            verify(list !== null, "the command ListView exists")
            tryVerify(function () { return list.count > 0 }, 5000)

            const row = list.itemAtIndex(0)
            verify(row !== null, "the first row delegate instantiated")
            // Row 0 of the bundled preset is "unbindall": input "none", context
            // "engine", platform blank (universal), no cheat marker (not a cheat command).
            const texts = rowTexts(row)
            verify(texts.indexOf("unbindall") >= 0, "line column: " + texts.join(" | "))
            verify(texts.indexOf("none") >= 0, "input column: " + texts.join(" | "))
            verify(texts.indexOf("engine") >= 0, "context column: " + texts.join(" | "))
            verify(texts.indexOf("") >= 0, "platform blank for universal: " + texts.join(" | "))
            verify(texts.indexOf("cheat") < 0, "no cheat marker: " + texts.join(" | "))
            // The marker binds on the catalog flag; noclip carries it.
            verify(host.cfgRef.consoleModel.findCommand("noclip").cheat === true,
                   "catalog flags feed the marker")

            // The platform column names the platform the way the Platform filter does: insert a
            // linux-only cvar (gl_clear, one of the gl_* rows the current Linux client still
            // registers) and read the cell off the delegate that renders it. The row this pin
            // first used, gl_blitmode, left with the 2026-10-17 Linux reconciliation - the
            // client can no longer name it (tst_engine.cpp, catalogRowsAnsweredByTheLinuxClient).
            const inserted = host.cfgRef.insertRowAfter(0, "gl_clear 1")
            verify(inserted > 0, "the linux-only line went in at row " + inserted)
            list.positionViewAtIndex(inserted, ListView.Beginning)
            let cells = []
            tryVerify(function () {
                const delegate = list.itemAtIndex(inserted)
                if (!delegate)
                    return false
                cells = rowTexts(delegate)
                return cells.indexOf("Linux-only") >= 0
            }, 5000, "the platform cell reads Linux-only")
            verify(cells.indexOf("linux") < 0,
                   "the catalog's token is not shown: " + cells.join(" | "))

            tab.destroy()
        }

        // -- the catalog signature parser (C++, shared with the argbounds reader) -----

        function test_parseSignatureReachesQmlAsAnArgumentList() {
            // Cfg.parseSignature is the one parse of allcmds field 8 (cmd::parseSignature).
            // The Build-A-Bind dialog and the guided pane walk its result with forEach and
            // read kind/optional per argument, so that shape is the contract.
            const specs = Cfg.parseSignature("[str] <flt>")
            compare(specs.length, 2, "two arguments")
            compare(specs[0].kind, "str", "first argument kind")
            compare(specs[0].optional, false, "[] is mandatory")
            compare(specs[1].kind, "flt", "second argument kind")
            compare(specs[1].optional, true, "<> is optional")

            const seen = []
            specs.forEach(function (spec, i) { seen.push(i + ":" + spec.kind) })
            compare(seen.join(","), "0:str,1:flt", "forEach walks it the way the editors do")

            compare(Cfg.parseSignature("none").length, 0, "no arguments")
            compare(Cfg.parseSignature("bool")[0].kind, "bool", "a cvar's value type is its argument")
        }

        // -- the Build-A-Bind argument editors --------------------------------

        function test_bindDialogBuildsOneEditorPerSignatureArgument() {
            const made = createInStage("qrc:/qml/dialogs/BindEditorDialog.qml", { keyToken: "a" })
            compare(made.error, "")
            const dialog = made.item
            verify(dialog !== null, "BindEditorDialog instantiated")
            dialog.openForKey("a")
            tryCompare(dialog, "opened", true, 5000)

            // A Popup's content lives under its contentItem, not under the Popup object.
            const contentRoot = dialog.contentItem
            verify(contentRoot !== null, "the dialog content is instantiated")

            const combo = findItem(contentRoot, function (it) { return it.objectName === "commandCombo" })
            verify(combo !== null, "the command dropdown exists")

            // Opening the dialog shows the whole catalog: Category defaults to "all", so no
            // category filter reaches the model, while Platform keeps "universal" (a
            // platform-specific row is not usable here). The dropdown indexes the model it
            // shows, so a row comes from that model.
            const categoryCombo = findItem(contentRoot, function (it) { return it.objectName === "categoryCombo" })
            verify(categoryCombo !== null, "the category dropdown exists")
            compare(categoryCombo.currentText, "all", "the dialog opens on every category")
            compare(Cfg.consoleModel.contextFilter, "", "no category filter is applied on open")

            const model = Cfg.consoleModel
            tryVerify(function () { return combo.count > 0 }, 5000, "the dropdown shows the catalog")
            compare(combo.count, model.rowCount(), "the dropdown shows the filtered catalog")

            // One editor per argument, each labelled with the parsed kind, and an include
            // checkbox on the optional argument alone. "bot_hurt" is a `server commands` row -
            // out of the list while Category defaulted to "client controls" - and its two
            // arguments differ in kind, so a swapped or defaulted kind shows up. Neither
            // command has a hand-written panel, so both render the generic editors.
            // Labels and checkboxes are matched by their text: Controls' Label and CheckBox
            // are QML types here, not the QQuick* classes a type check would look for.
            const cases = [
                { name: "callvote", labels: "param 1 (str) | param 2 (str)" },
                { name: "bot_hurt", labels: "param 1 (str) | param 2 (flt)" }
            ]
            for (let c = 0; c < cases.length; ++c) {
                const expected = cases[c]

                let row = -1
                tryVerify(function () {
                    for (let r = 0; r < model.rowCount(); ++r) {
                        if (model.commandAt(r).name === expected.name) {
                            row = r
                            return true
                        }
                    }
                    return false
                }, 5000, expected.name + " is offered")
                combo.currentIndex = row

                let labels = []
                tryVerify(function () {
                    labels = collectItems(contentRoot, function (it) {
                        return it.text !== undefined && String(it.text).indexOf("param ") === 0
                    }, []).map(function (label) { return label.text })
                    return labels.join(" | ") === expected.labels
                }, 5000, expected.name + ": one editor per argument")

                const includes = collectItems(contentRoot, function (it) {
                    return it.text === "include" && it.checked !== undefined
                }, [])
                compare(includes.length, 2, expected.name + ": every argument carries an include checkbox")
                compare(includes[0].visible, false, expected.name + ": a mandatory argument cannot be dropped")
                compare(includes[0].checked, true, expected.name + ": a mandatory argument is always in the bind")
                compare(includes[1].visible, true, expected.name + ": an optional one can be left out")
                compare(includes[1].checked, false, expected.name + ": and starts out of the bind")
            }

            // The platform dropdown reads as words while the filter keeps the catalog token,
            // so both halves of that mapping are pinned here. Labels first, read off the
            // field the way the user sees it.
            const platformCombo = findItem(contentRoot, function (it) { return it.objectName === "platformCombo" })
            verify(platformCombo !== null, "the platform dropdown exists")
            const platformLabels = ["Universal", "Linux-only", "Xbox-only", "All"]
            compare(platformCombo.count, platformLabels.length,
                    "the menu offers every platform the catalog has, and no more")
            for (let o = 0; o < platformLabels.length; ++o) {
                platformCombo.currentIndex = o
                compare(platformCombo.currentText, platformLabels[o],
                        "option " + o + " reads as " + platformLabels[o])
            }

            // "All" carries the whole catalog, which is where the xbox row count comes from.
            platformCombo.activated(3)
            compare(Cfg.consoleModel.platformFilter, "", "'all' clears the platform filter")
            let xboxRows = 0
            for (let r = 0; r < model.rowCount(); ++r) {
                if (model.commandAt(r).platform === "xbox")
                    xboxRows++
            }
            verify(xboxRows > 0, "the catalog lists xbox rows to narrow to")

            // Selecting an option filters by the catalog's token, never by the label the user
            // reads: "Xbox-only" as a filter value would match no row at all.
            platformCombo.currentIndex = 2
            platformCombo.activated(2)
            compare(Cfg.consoleModel.platformFilter, "xbox", "Xbox-only filters by the catalog token")
            compare(platformCombo.currentText, "Xbox-only", "and the field still reads as words")
            tryVerify(function () { return model.rowCount() === xboxRows }, 5000,
                      "the list narrows to the " + xboxRows + " xbox rows")
            let onlyXbox = true
            for (let r = 0; r < model.rowCount(); ++r) {
                if (model.commandAt(r).platform !== "xbox") {
                    onlyXbox = false
                    break
                }
            }
            verify(onlyXbox, "every remaining row is xbox")

            // Back to the default: the filter lives on the shared model, not in this dialog.
            platformCombo.currentIndex = 0
            platformCombo.activated(0)
            compare(Cfg.consoleModel.platformFilter, "universal", "the field is back on universal")

            dialog.close()
            dialog.destroy()
        }

        // -- W2: key caps take focus and activate on Return/Space -------------

        function test_keyCapActivatesOnReturnAndSpace() {
            const made = createInStage("qrc:/qml/keyboard/KeyCap.qml", { keyName: "a" })
            compare(made.error, "")
            const cap = made.item
            verify(cap.bindable, "cap 'a' is bindable")
            verify(cap.activeFocusOnTab, "a bindable cap joins the tab chain")

            const spy = spyOn(cap, "activated")
            focusItem(cap)
            keyClick(Qt.Key_Return)
            compare(spy.count, 1, "Return activates the cap")
            compare(spy.signalArguments[0][0], cap.keyId, "the signal carries the engine key id")

            keyClick(Qt.Key_Space)
            compare(spy.count, 2, "Space activates the cap")

            cap.destroy()
        }

        function test_keyCapStaysOutWhenNotBindable() {
            // "prtsc" carries no key token (D16), so it must not take focus or activate.
            const made = createInStage("qrc:/qml/keyboard/KeyCap.qml", { keyName: "prtsc" })
            compare(made.error, "")
            const cap = made.item
            verify(!cap.bindable, "cap 'prtsc' is not bindable")
            verify(!cap.activeFocusOnTab, "a non-bindable cap stays out of the tab chain")

            const spy = spyOn(cap, "activated")
            focusItem(cap)
            keyClick(Qt.Key_Return)
            compare(spy.count, 0, "a non-bindable cap never activates")

            cap.destroy()
        }

        function test_keyCapHighlightsOnFocus() {
            // W2: focus shows the same highlight as hover, which is the visible focus cue.
            const made = createInStage("qrc:/qml/keyboard/KeyCap.qml", { keyName: "a" })
            compare(made.error, "")
            const cap = made.item
            verify(!cap.highlighted, "no highlight before focus")
            cap.forceActiveFocus()
            tryVerify(function () { return cap.highlighted === true }, 1000,
                      "focus highlights the cap")
            cap.destroy()
        }

        // -- W3: the tab strip switches with Left/Right ----------------------

        function test_tabBarArrowKeysWrap() {
            const made = createInStage("qrc:/qml/components/Tf2TabBar.qml",
                                       { model: ["one", "two", "three"], currentIndex: 0 })
            compare(made.error, "")
            const bar = made.item
            compare(bar.tabCount, 3)

            const spy = spyOn(bar, "activated")
            focusItem(bar)
            keyClick(Qt.Key_Right)
            compare(spy.count, 1, "Right moves to the next tab")
            compare(spy.signalArguments[0][0], 1)

            // The strip only reports the wanted index; the owner sets currentIndex.
            bar.currentIndex = 2
            keyClick(Qt.Key_Right)
            compare(spy.signalArguments[1][0], 0, "Right wraps to the first tab")

            bar.currentIndex = 0
            keyClick(Qt.Key_Left)
            compare(spy.signalArguments[2][0], 2, "Left wraps to the last tab")

            bar.destroy()
        }

        // -- W4: the strip is one tab list, its tabs announce themselves ------

        function test_tabBarAccessibleRoles() {
            const made = createInStage("qrc:/qml/components/Tf2TabBar.qml",
                                       { model: ["one", "two", "three"], currentIndex: 1 })
            compare(made.error, "")
            const bar = made.item

            // Fixed after the Narrator pass: a page tab is only announced inside a tab
            // list, so the strip carries PageTabList - the same pair Qt's own TabBar
            // builds in C++ (QQuickTabBar/QQuickTabButton).
            compare(bar.Accessible.role, Accessible.PageTabList,
                    "the strip announces as a tab list")

            const tabs = []
            collectItems(bar, function (it) {
                return it.Accessible && it.Accessible.role === Accessible.PageTab
            }, tabs)
            compare(tabs.length, 3, "every tab announces as a page tab")
            compare(tabs[0].Accessible.name, "one", "the tab carries its title")
            compare(tabs[1].Accessible.name, "two")
            compare(tabs[2].Accessible.name, "three")
            verify(!tabs[0].Accessible.selected, "a non-current tab is not selected")
            verify(tabs[1].Accessible.selected, "the current tab is selected")

            bar.currentIndex = 2
            tryVerify(function () { return tabs[2].Accessible.selected === true }, 1000,
                      "selection follows currentIndex")
            verify(!tabs[1].Accessible.selected)

            bar.destroy()
        }

        function test_tabBarFocusFollowsTheCurrentTab() {
            // A screen reader announces whatever holds focus. While the strip itself held
            // it, Narrator reported "Tab contains 3 items" and never a tab name, so focus
            // now lands on the current tab and follows it.
            const made = createInStage("qrc:/qml/components/Tf2TabBar.qml",
                                       { model: ["one", "two", "three"], currentIndex: 0 })
            compare(made.error, "")
            const bar = made.item
            const tabs = []
            collectItems(bar, function (it) {
                return it.Accessible && it.Accessible.role === Accessible.PageTab
            }, tabs)
            compare(tabs.length, 3)

            focusItem(bar)
            verify(bar.activeFocus, "the strip reports focus within itself")
            verify(tabs[0].activeFocus, "the current tab holds the focus")
            verify(!tabs[1].activeFocus, "the other tabs do not")

            bar.currentIndex = 2
            tryVerify(function () { return tabs[2].activeFocus === true }, 1000,
                      "focus follows currentIndex")
            verify(!tabs[0].activeFocus, "the tab that lost currentIndex gave focus up")

            // The keys still work while a tab, not the strip, holds the focus.
            const spy = spyOn(bar, "activated")
            keyClick(Qt.Key_Left)
            compare(spy.count, 1, "Left still reaches the strip")
            compare(spy.signalArguments[0][0], 1, "Left reports the previous tab")

            bar.destroy()
        }
    }
}
