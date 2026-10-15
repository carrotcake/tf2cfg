// Qt Quick Test setup for the app's QML.
//
// Mirrors src/main.cpp: the Basic style, the four bundled faces, and the two context
// properties the QML tree reads (cfg, keyTable). Tests therefore drive the same
// environment the app does, and a QML file that only loads because of something main.cpp
// does (a font, a style, a context property) still fails here if that wiring breaks.
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
        QQuickStyle::setStyle(QStringLiteral("Basic"));
        loadAppFonts();
        m_cfg = new ConfigController(this);
        m_keys = new KeyTable(this);
        engine->rootContext()->setContextProperty(QStringLiteral("cfg"), m_cfg);
        engine->rootContext()->setContextProperty(QStringLiteral("keyTable"), m_keys);
    }

private:
    ConfigController *m_cfg = nullptr;
    KeyTable *m_keys = nullptr;
};

QUICK_TEST_MAIN_WITH_SETUP(appui, AppSetup)

#include "tst_app.moc"
