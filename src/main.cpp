#include <QApplication>
#include <QFont>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QElapsedTimer>
#include <QScrollBar>
#include <iostream>

#include "core/geo.hpp"
#include "ui/main_window.hpp"
#include "ui/collapsible_section.hpp"
#include "ui/glitch_button.hpp"
#include "ui/theme.hpp"
#include "ui/fluent_tooltip.hpp"

int main(int argc, char* argv[]) {
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QApplication app(argc, argv);
    app.setApplicationName("MasscanGUI");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("MasscanCommunity");
    app.setWindowIcon(QIcon(":/Icon.ico"));

    // Load Geo IP database
    bool geoLoaded = false;
    QStringList candidates = {
        "data/geo.bin",
        QCoreApplication::applicationDirPath() + "/data/geo.bin",
        QCoreApplication::applicationDirPath() + "/geo.bin",
        ":/data/geo.bin"
    };

    for (const auto& path : candidates) {
        if (path.startsWith(":/")) {
            QFile file(path);
            if (file.open(QIODevice::ReadOnly)) {
                QByteArray data = file.readAll();
                if (masscan::geo::init(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(data.constData()), data.size()))) {
                    geoLoaded = true;
                    std::cout << "[INFO] Loaded GeoIP database from Qt resource: " << path.toStdString()
                              << " (" << masscan::geo::countries().size() << " countries)" << std::endl;
                    break;
                }
            }
        } else if (QFileInfo::exists(path)) {
            if (masscan::geo::init_from_file(path.toStdString())) {
                geoLoaded = true;
                std::cout << "[INFO] Loaded GeoIP database from disk: " << path.toStdString()
                          << " (" << masscan::geo::countries().size() << " countries)" << std::endl;
                break;
            }
        }
    }

    if (!geoLoaded) {
        std::cerr << "[WARNING] Could not load geo.bin! Country picker will be empty." << std::endl;
    }

    QFont appFont("Segoe UI");
    appFont.setPixelSize(13);
    appFont.setStyleHint(QFont::SansSerif);
    app.setFont(appFont);

    // Apply dark stylesheet
    app.setStyleSheet(masscan::theme::getDarkStyle());

    // Modern animated Fluent Tooltip filter
    app.installEventFilter(new masscan::ui::FluentToolTipFilter(&app));

    // Headless test mode for automated verification
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--render-screenshot" && i + 1 < argc) {
            masscan::ui::MainWindow win;
            win.resize(1100, 750);
            win.show();
            app.processEvents();
            QPixmap pm = win.grab();
            pm.save(argv[i + 1]);
            std::cout << "[INFO] Screenshot saved to " << argv[i + 1] << std::endl;
            return 0;
        }
        if (std::string(argv[i]) == "--render-light-screenshot" && i + 1 < argc) {
            masscan::ui::MainWindow win;
            win.onToggleTheme();
            win.resize(1100, 750);
            win.show();
            app.processEvents();
            QPixmap pm = win.grab();
            pm.save(argv[i + 1]);
            std::cout << "[INFO] Light screenshot saved to " << argv[i + 1] << std::endl;
            return 0;
        }
        if (std::string(argv[i]) == "--render-popup-screenshot" && i + 1 < argc) {
            masscan::ui::MainWindow win;
            win.resize(1100, 750);
            win.show();
            app.processEvents();

            auto* combo = win.findChild<QComboBox*>();
            if (combo) {
                combo->showPopup();
                app.processEvents();
                if (auto* v = combo->view()) {
                    if (auto* container = v->parentWidget()) {
                        QPixmap pm = container->grab();
                        pm.save(argv[i + 1]);
                        std::cout << "[INFO] Popup screenshot saved to " << argv[i + 1] << std::endl;
                    }
                }
            }
            return 0;
        }
        if (std::string(argv[i]) == "--render-scrolled-screenshot" && i + 1 < argc) {
            masscan::ui::MainWindow win;
            win.resize(1100, 750);
            win.show();
            app.processEvents();

            auto* scroll = win.findChild<QScrollArea*>();
            if (scroll) {
                scroll->verticalScrollBar()->setValue(450);
                app.processEvents();
            }

            QPixmap pm = win.grab();
            pm.save(argv[i + 1]);
            std::cout << "[INFO] Scrolled screenshot saved to " << argv[i + 1] << std::endl;
            return 0;
        }
        if (std::string(argv[i]) == "--render-focus-screenshot" && i + 1 < argc) {
            masscan::ui::MainWindow win;
            win.resize(1100, 750);
            win.show();
            app.processEvents();

            auto* edit = win.findChild<QLineEdit*>();
            if (edit) {
                edit->setFocus();
                app.processEvents();
                QPixmap pm = edit->grab();
                pm.save(argv[i + 1]);
                std::cout << "[INFO] Focus screenshot saved to " << argv[i + 1] << std::endl;
            }
            return 0;
        }
        if (std::string(argv[i]) == "--render-multiselect-screenshot" && i + 1 < argc) {
            masscan::ui::MainWindow win;
            win.resize(1100, 750);
            win.show();
            app.processEvents();

            auto* cl = win.findChild<QListWidget*>("countryList");
            if (cl && cl->count() >= 2) {
                emit cl->itemClicked(cl->item(0));
                emit cl->itemClicked(cl->item(1));
            }
            auto* tl = win.findChild<QListWidget*>("targetsList");
            if (tl && tl->count() >= 1) {
                tl->setCurrentRow(0);
            }
            app.processEvents();

            QPixmap pm = win.grab();
            pm.save(argv[i + 1]);
            std::cout << "[INFO] Multi-select screenshot saved to " << argv[i + 1] << std::endl;
            return 0;
        }
        if (std::string(argv[i]) == "--render-glitch-screenshot" && i + 1 < argc) {
            masscan::ui::MainWindow win;
            win.resize(1100, 750);
            win.show();
            app.processEvents();

            auto* gbtn = win.findChild<masscan::ui::GlitchButton*>();
            if (gbtn) {
                gbtn->triggerGlitchAt(0.0);
                app.processEvents();
            }

            QPixmap pm = win.grab();
            pm.save(argv[i + 1]);
            std::cout << "[INFO] Glitch screenshot saved to " << argv[i + 1] << std::endl;
            return 0;
        }
        if (std::string(argv[i]) == "--test") {
            std::cout << "[TEST] Running self-check..." << std::endl;
            if (!geoLoaded) {
                std::cerr << "[TEST FAIL] Geo database was not loaded!" << std::endl;
                return 1;
            }
            if (masscan::geo::countries().empty()) {
                std::cerr << "[TEST FAIL] Country list is empty!" << std::endl;
                return 2;
            }

            // Test window creation
            masscan::ui::MainWindow win;
            win.resize(1100, 750);
            win.show();
            app.processEvents();

            win.onToggleTheme();
            win.onToggleTheme();

            std::cout << "[TEST OK] MainWindow created successfully. Tests passed." << std::endl;
            return 0;
        }
    }

    masscan::ui::MainWindow window;
    window.show();

    return app.exec();
}
