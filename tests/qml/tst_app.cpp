// Qt Quick Test setup for the app's QML.
//
// Mirrors src/main.cpp: the Basic style, the four bundled faces, and the two singletons the
// QML tree reads (Cfg, KeyTable). Tests therefore drive the same environment the app does,
// and a QML file that only loads because of something main.cpp does (a font, a style, a
// singleton registration) still fails here if that wiring breaks.
#include <QFontDatabase>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickStyle>
#include <QtQuickTest>

#include "ui/configcontroller.h"
#include "ui/keytable.h"

namespace {

/// The same faces src/main.cpp registers: column widths and cap labels are measured,
/// and a fallback font would change those measurements.
void loadAppFonts() {
    QFontDatabase::addApplicationFont(QStringLiteral("://font/tf2build.ttf"));
    QFontDatabase::addApplicationFont(QStringLiteral("://font/tf2professor.ttf"));
    QFontDatabase::addApplicationFont(QStringLiteral("://font/tf2secondary.ttf"));
    QFontDatabase::addApplicationFont(QStringLiteral("://font/LiberationMono-Regular.ttf"));
}

} // namespace

class AppSetup : public QObject {
    Q_OBJECT
public slots:
    void qmlEngineAvailable(QQmlEngine *engine) {
        Q_UNUSED(engine)
        QQuickStyle::setStyle(QStringLiteral("Basic"));
        loadAppFonts();
        // `Cfg` and `KeyTable` need no wiring here: the qmltypes-enabled build of this
        // target registers the tf2cfg module (tst_app_qmltyperegistrations.cpp), exactly
        // as the app's build does, and the engine constructs the singletons on first use.
        // The tests reach them through `Cfg` in tst_app.qml.
    }
};

QUICK_TEST_MAIN_WITH_SETUP(appui, AppSetup)

#include "tst_app.moc"
