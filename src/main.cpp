#include "player.h" // Your main window class

#include <QApplication>
#include <QSettings>      // For reading/writing application settings
#include <QStandardPaths> // To find standard system locations
#include <QDir>           // For directory operations (checking existence, creating)
#include <QFile>          // For file operations (copying, checking existence)
#include <QTranslator>
#include <QDebug>
#include <QSplashScreen>
#include <QStyleFactory>
#include <QPalette>
#include <QThread>        // For QThread::msleep (used sparingly)
#include <QMessageBox>    // For showing critical errors

#include <QApplication>
int main(int argc, char *argv[])
{
    // Set up multimedia environment before QApplication
    qputenv("QT_MULTIMEDIA_PREFERRED_PLUGINS", "gstreamer");
    qputenv("QT_ACCESSIBILITY", "1");
    
    // Basic application setup - Do this EARLY
    QApplication::setDesktopSettingsAware(false); // Keep this if needed for specific behavior
    QApplication a(argc, argv);

#if defined(Q_OS_MAC)
    // On macOS, explicitly set the application icon using the .icns file in the app bundle Resources folder
    a.setWindowIcon(QIcon("../Resources/XFB.icns"));
#elif defined(Q_OS_LINUX)
    // On Linux, set the window icon using the PNG icon
    a.setWindowIcon(QIcon(":/Resources/48x48.png"));
#elif defined(Q_OS_WIN)
    // On Windows, set the window icon using an ICO file if available, else PNG
    a.setWindowIcon(QIcon(":/Resources/48x48.png"));
#else
    // Default fallback
    a.setWindowIcon(QIcon(":/Resources/48x48.png"));
#endif



    // --- Set Application Info (Crucial for QSettings default paths) ---
    QCoreApplication::setApplicationName("XFB");
    QCoreApplication::setOrganizationName("Netpack - Online Solutions");

    QApplication::setStyle(QStyleFactory::create("Fusion"));

    // --- Splash Screen Initialization ---
    QSplashScreen splash(QPixmap("://images/splash.png"));
    Qt::Alignment alignit = Qt::AlignBottom | Qt::AlignHCenter;
    Qt::GlobalColor msgColor = Qt::darkBlue; // Or choose a color visible on your splash
    splash.show();
    splash.showMessage(QObject::tr("Initializing..."), alignit, msgColor);
    a.processEvents(); // Ensure splash is shown

    // --- Configuration File Handling ---
    QString configFileName = "xfb.conf";
    // Prefer AppConfigLocation over AppDataLocation for config files
    QString writableConfigPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);

    if (writableConfigPath.isEmpty()) {
        qCritical() << "Could not determine writable config location!";
        // Show error and potentially exit if config is essential
        splash.hide(); // Hide splash before showing message box
        QMessageBox::critical(nullptr, QObject::tr("Configuration Error"), QObject::tr("Cannot find writable location for configuration."));
        return 1;
    }

    // Ensure the directory exists (AppConfigLocation might be just a path, not a guaranteed directory)
    QDir configDir(writableConfigPath);
    if (!configDir.exists()) {
        qInfo() << "Creating configuration directory:" << writableConfigPath;
        if (!configDir.mkpath(".")) { // mkpath creates parent dirs if needed
            qCritical() << "Failed to create configuration directory:" << writableConfigPath;
            splash.hide();
            QMessageBox::critical(nullptr, QObject::tr("Configuration Error"), QObject::tr("Could not create configuration directory:\n%1").arg(writableConfigPath));
            return 1;
        }
    }

    QString configFilePath = writableConfigPath + "/" + configFileName;
    QString resourceConfigPath = ":/" + configFileName; // Path in resources

    // Copy config from resources to writable location IF it doesn't exist there yet
    if (!QFile::exists(configFilePath)) {
        splash.showMessage(QObject::tr("Setting up default configuration..."), alignit, msgColor);
        a.processEvents();
        if (!QFile::copy(resourceConfigPath, configFilePath)) {
            qCritical() << "Failed to copy default configuration from" << resourceConfigPath << "to" << configFilePath;
            qCritical() << "Resource exists?" << QFile::exists(resourceConfigPath);
            qCritical() << "Check resource path and write permissions for" << writableConfigPath;
            splash.hide();
            QMessageBox::critical(nullptr, QObject::tr("Configuration Error"), QObject::tr("Could not copy default configuration file."));
            return 1; // Exit if default config is essential
        } else {
            qInfo() << "Copied default configuration to:" << configFilePath;
            // Ensure the copied file is writable by the user
            QFile::setPermissions(configFilePath, QFile::ReadOwner | QFile::WriteOwner | QFile::ReadGroup | QFile::ReadOther);
        }
    } else {
        qDebug() << "Using existing configuration file:" << configFilePath;
    }

    // --- Load Settings using QSettings ---
    splash.showMessage(QObject::tr("Loading settings..."), alignit, msgColor);
    a.processEvents();

    QSettings settings(configFilePath, QSettings::IniFormat); // Specify INI format explicitly

    // Read settings with defaults
    QString idioma = settings.value("Language", "en").toString(); // Default to English
    bool fullScreen = settings.value("FullScreen", false).toBool(); // Default to windowed
    bool darkMode = settings.value("DarkMode", false).toBool();   // Default to light mode

    qDebug() << "Settings Loaded - Language:" << idioma << "FullScreen:" << fullScreen << "DarkMode:" << darkMode;

    // --- Translator Setup ---
    QTranslator tr;
    bool translatorLoaded = false;
    if (idioma == "pt") {
        splash.showMessage(QObject::tr("Loading Portuguese GUI..."), alignit, msgColor);
        a.processEvents();
        translatorLoaded = tr.load(":/portugues.qm");
    } else if (idioma == "fr") {
        splash.showMessage(QObject::tr("Loading French GUI..."), alignit, msgColor);
        a.processEvents();
        translatorLoaded = tr.load(":/frances.qm");
    } else {
        // English is default, no translator needed unless you have specific en strings
        splash.showMessage(QObject::tr("Loading English GUI..."), alignit, msgColor);
        a.processEvents();
    }

    if (idioma != "en" && translatorLoaded) {
        a.installTranslator(&tr);
        qDebug() << "Installed translator for language:" << idioma;
    } else if (idioma != "en" && !translatorLoaded) {
        qWarning() << "Failed to load translator file for language:" << idioma;
        splash.showMessage(QObject::tr("Failed to load translation!"), alignit, Qt::red);
        QThread::msleep(1500); // Give time to see error message
    } else {
        qDebug() << "Using default English GUI.";
    }


    // --- Theme Application (Palette & Stylesheet) ---
    splash.showMessage(QObject::tr("Applying theme..."), alignit, msgColor);
    a.processEvents();

    QString qssFilePath;
    if (darkMode) {
        QPalette darkPalette;
        // Modern Dark Palette (Catppuccin Mocha inspired)
        darkPalette.setColor(QPalette::Window, QColor(30, 30, 46));        // #1e1e2e
        darkPalette.setColor(QPalette::WindowText, QColor(205, 214, 244)); // #cdd6f4
        darkPalette.setColor(QPalette::Base, QColor(24, 24, 37));          // #181825
        darkPalette.setColor(QPalette::AlternateBase, QColor(30, 30, 46)); // #1e1e2e
        darkPalette.setColor(QPalette::ToolTipBase, QColor(205, 214, 244));
        darkPalette.setColor(QPalette::ToolTipText, QColor(30, 30, 46));
        darkPalette.setColor(QPalette::Text, QColor(205, 214, 244));
        darkPalette.setColor(QPalette::Button, QColor(49, 50, 68));        // #313244
        darkPalette.setColor(QPalette::ButtonText, QColor(205, 214, 244));
        darkPalette.setColor(QPalette::BrightText, Qt::red);
        darkPalette.setColor(QPalette::Highlight, QColor(137, 180, 250));  // #89b4fa
        darkPalette.setColor(QPalette::HighlightedText, QColor(30, 30, 46));
        a.setPalette(darkPalette);
        qssFilePath = ":/resources/darkstylesheet.qss";
        qDebug() << "Set modern dark palette.";
    } else {
        QPalette lightPalette;
        // Modern Light Palette (Catppuccin Latte inspired)
        lightPalette.setColor(QPalette::Window, QColor(239, 241, 245));    // #eff1f5
        lightPalette.setColor(QPalette::WindowText, QColor(76, 79, 105));  // #4c4f69
        lightPalette.setColor(QPalette::Base, QColor(255, 255, 255));      // #ffffff
        lightPalette.setColor(QPalette::AlternateBase, QColor(239, 241, 245));
        lightPalette.setColor(QPalette::ToolTipBase, QColor(76, 79, 105));
        lightPalette.setColor(QPalette::ToolTipText, QColor(255, 255, 255));
        lightPalette.setColor(QPalette::Text, QColor(76, 79, 105));
        lightPalette.setColor(QPalette::Button, QColor(230, 233, 239));    // #e6e9ef
        lightPalette.setColor(QPalette::ButtonText, QColor(76, 79, 105));
        lightPalette.setColor(QPalette::BrightText, Qt::red);
        lightPalette.setColor(QPalette::Highlight, QColor(30, 102, 245));  // #1e66f5
        lightPalette.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
        a.setPalette(lightPalette);
        qssFilePath = ":/resources/stylesheet.qss";
        qDebug() << "Set modern light palette.";
    }

    // Load and apply the main stylesheet
    QFile styleFile(qssFilePath);
    if (styleFile.open(QFile::ReadOnly | QFile::Text)) { // Open as text explicitly
        QString styleSheet = QLatin1String(styleFile.readAll()); // Read as Latin1 for QSS
        a.setStyleSheet(styleSheet);
        styleFile.close();
        qDebug() << "Applied main global stylesheet:" << qssFilePath;
    } else {
        qWarning() << "Could not open main stylesheet file:" << qssFilePath;
    }

    // --- Create Main Window (AFTER theme/translator is set) ---
    splash.showMessage(QObject::tr("Loading main window..."), alignit, msgColor);
    a.processEvents();

    player w; // Database initialization should happen inside player's constructor or an init method called by it

    // --- Show Main Window & Finish Splash ---
    splash.showMessage(QObject::tr("XFB is Ready!"), alignit, msgColor);
    QThread::msleep(300); // Optional short delay to ensure final message is seen

    if (fullScreen) {
        w.showFullScreen();
    } else {
        w.show();
    }

    splash.finish(&w); // Hides splash when main window 'w' is ready/active

    // --- Start Event Loop ---
    return a.exec();
}
