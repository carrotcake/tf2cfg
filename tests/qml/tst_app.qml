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

            // Opening the dialog filters the catalog to its Category/Platform defaults
            // (client controls + universal), and "callvote" - "[str] <str>", a mandatory
            // argument then an optional one - is in that view and carries no hand-written
            // panel, so it renders the generic argument editors. The dropdown indexes the
            // model it shows, so the row comes from that model.
            const model = Cfg.consoleModel
            tryVerify(function () { return combo.count > 0 }, 5000, "the dropdown shows the catalog")
            compare(combo.count, model.rowCount(), "the dropdown shows the filtered catalog")

            let row = -1
            tryVerify(function () {
                for (let r = 0; r < model.rowCount(); ++r) {
                    if (model.commandAt(r).name === "callvote") {
                        row = r
                        return true
                    }
                }
                return false
            }, 5000, "callvote is offered")
            combo.currentIndex = row

            // One editor per argument, each labelled with the parsed kind. Both the labels
            // and the checkboxes are matched by their text: Controls' Label and CheckBox
            // are QML types here, not the QQuick* classes a type check would look for.
            let labels = []
            tryVerify(function () {
                labels = collectItems(contentRoot, function (it) {
                    return it.text !== undefined && String(it.text).indexOf("param ") === 0
                }, []).map(function (label) { return label.text })
                return labels.length === 2
            }, 5000, "one editor per argument")
            compare(labels.join(" | "), "param 1 (str) | param 2 (str)",
                    "each editor is labelled with the parsed kind")

            // The include checkbox belongs to the optional argument and starts unticked:
            // a mandatory argument is always part of the bind.
            const includes = collectItems(contentRoot, function (it) {
                return it.text === "include" && it.checked !== undefined
            }, [])
            compare(includes.length, 2, "every argument carries an include checkbox")
            compare(includes[0].visible, false, "a mandatory argument cannot be dropped")
            compare(includes[0].checked, true)
            compare(includes[1].visible, true, "an optional one can")
            compare(includes[1].checked, false)

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
