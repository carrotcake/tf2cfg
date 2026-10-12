#include <QFontDatabase>
#include <QGuiApplication>
#include <QPalette>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QScreen>

#include "ui/configcontroller.h"

namespace {

/// Dark palette, so the controls we do not customise (tool tips, menu popups, scroll
/// bars, dialog chrome) match the rest of the UI.
QPalette darkPalette() {
    const QColor window(0x14, 0x17, 0x19);
    const QColor surface(0x1d, 0x21, 0x24);
    const QColor raised(0x26, 0x2b, 0x2f);
    const QColor sunken(0x0f, 0x12, 0x14);
    const QColor text(0xe8, 0xe5, 0xdf);
    const QColor muted(0x98, 0xa1, 0xa8);
    const QColor accent(0xf0, 0xa3, 0x0a);
    const QColor accentText(0x1a, 0x12, 0x06);
    const QColor border(0x3a, 0x40, 0x45);

    QPalette palette;
    palette.setColor(QPalette::Window, window);
    palette.setColor(QPalette::WindowText, text);
    palette.setColor(QPalette::Base, sunken);
    palette.setColor(QPalette::AlternateBase, surface);
    palette.setColor(QPalette::Text, text);
    palette.setColor(QPalette::PlaceholderText, muted);
    palette.setColor(QPalette::Button, raised);
    palette.setColor(QPalette::ButtonText, text);
    palette.setColor(QPalette::BrightText, text);
    palette.setColor(QPalette::Highlight, accent);
    palette.setColor(QPalette::HighlightedText, accentText);
    palette.setColor(QPalette::ToolTipBase, raised);
    palette.setColor(QPalette::ToolTipText, text);
    palette.setColor(QPalette::Light, border);
    palette.setColor(QPalette::Midlight, raised);
    palette.setColor(QPalette::Mid, border);
    palette.setColor(QPalette::Dark, sunken);
    palette.setColor(QPalette::Shadow, Qt::black);
    return palette;
}

} // namespace

int main(int argc, char *argv[]) {
    QGuiApplication a(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("TF2 Config Manager"));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("TF2 Config Manager"));

    // Basic style: every control is themed in qml/theme + qml/components, so no platform
    // style may override them.
    QQuickStyle::setStyle(QStringLiteral("Basic"));
    QGuiApplication::setPalette(darkPalette());

    /********************************************************************/   // Font Family:
    QFontDatabase::addApplicationFont("://font/tf2build.ttf");               // "TF2 Build"
    QFontDatabase::addApplicationFont("://font/tf2professor.ttf");           // "TF2 Professor"
    QFontDatabase::addApplicationFont("://font/tf2secondary.ttf");           // "TF2"
    QFontDatabase::addApplicationFont("://font/LiberationMono-Regular.ttf"); // "Liberation Mono"
    /********************************************************************/   // from res.qrc

    ConfigController cfg;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("cfg"), &cfg);

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &a,
                     []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

    engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    // The OS decides where a new window opens, which on a multi-monitor desktop can leave
    // it straddling two screens. Pin the window frame to the top-left of the primary screen
    // so every launch starts from the same, fully visible position.
    if (auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst())) {
        if (const QScreen *screen = QGuiApplication::primaryScreen())
            window->setFramePosition(screen->geometry().topLeft());
    }

    return a.exec();
}
