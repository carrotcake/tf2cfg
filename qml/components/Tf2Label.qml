import QtQuick
import QtQuick.Controls
import "../theme"
pragma ComponentBehavior: Bound

// Body label with the app's default text style.
Label {
    color: Theme.colorText
    font.family: Theme.familyUi
    font.pointSize: Theme.sizeUi
}
