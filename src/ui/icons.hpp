#pragma once

#include <QIcon>
#include <QPixmap>
#include <QPainter>
#include <QString>
#include <QColor>
#include <QSvgRenderer>
#include <QMap>

namespace masscan::ui {

class Icons {
public:
    static inline const char* Menu = "M 4 7 L 20 7 M 4 12 L 20 12 M 4 17 L 20 17";
    static inline const char* Close = "M 6 6 L 18 18 M 18 6 L 6 18";
    static inline const char* Play = "M 7 4 L 20 12 L 7 20 Z";
    static inline const char* Stop = "M 6 6 L 18 6 L 18 18 L 6 18 Z";
    static inline const char* Resume = "M 4 12 A 8 8 0 1 0 7 6 M 4 3 L 4 8 L 9 8";
    static inline const char* Folder = "M 3 6 L 9 6 L 11 9 L 21 9 L 21 19 L 3 19 Z";
    static inline const char* Copy = "M 10 8 H 19 A 2 2 0 0 1 21 10 V 19 A 2 2 0 0 1 19 21 H 10 A 2 2 0 0 1 8 19 V 10 A 2 2 0 0 1 10 8 Z M 4 16 A 2 2 0 0 1 2 14 V 5 A 2 2 0 0 1 4 3 H 13 A 2 2 0 0 1 15 5";
    static inline const char* Export = "M 12 4 L 12 15 M 7 10 L 12 15 L 17 10 M 4 20 L 20 20";
    static inline const char* Trash = "M 4 7 L 20 7 M 9 7 L 9 4 L 15 4 L 15 7 M 6 7 L 7 20 L 17 20 L 18 7";
    static inline const char* Search = "M 4 10 A 6 6 0 1 0 16 10 A 6 6 0 1 0 4 10 M 15 15 L 20 20";
    static inline const char* Plus = "M 12 5 L 12 19 M 5 12 L 19 12";
    static inline const char* Edit = "M 4 20 L 5 15 L 16 4 L 20 8 L 9 19 Z M 14 6 L 18 10";
    static inline const char* Import = "M 12 15 L 12 3 M 7 8 L 12 3 L 17 8 M 4 20 L 20 20";
    static inline const char* Target = "M 3 12 A 9 9 0 1 0 21 12 A 9 9 0 1 0 3 12 M 9 12 A 3 3 0 1 0 15 12 A 3 3 0 1 0 9 12";
    static inline const char* Ports = "M 9 3 L 9 8 M 15 3 L 15 8 M 6 8 L 18 8 L 18 12 A 6 6 0 0 1 6 12 Z M 12 18 L 12 22";
    static inline const char* Gauge = "M 4 18 A 9 9 0 1 1 20 18 M 12 14 L 16 8 M 12 14 L 12 14.1";
    static inline const char* Network = "M 9 3 L 15 3 L 15 8 L 9 8 Z M 3 16 L 9 16 L 9 21 L 3 21 Z M 15 16 L 21 16 L 21 21 L 15 21 Z M 12 8 L 12 12 M 6 16 L 6 12 L 18 12 L 18 16";
    static inline const char* Tag = "M 3 4 L 12 4 L 21 13 L 13 21 L 3 11 Z M 7 8 L 7.1 8";
    static inline const char* File = "M 6 3 L 14 3 L 19 8 L 19 21 L 6 21 Z M 14 3 L 14 8 L 19 8 M 9 13 L 16 13 M 9 17 L 16 17";
    static inline const char* Paths = "M 3 6 L 9 6 L 11 9 L 21 9 L 21 19 L 3 19 Z M 3 12 L 21 12";
    static inline const char* Sliders = "M 4 7 L 20 7 M 4 12 L 20 12 M 4 17 L 20 17 M 9 4 L 9 10 M 15 9 L 15 15 M 8 14 L 8 20";
    static inline const char* Mode = "M 3 5 L 21 5 L 21 19 L 3 19 Z M 7 10 L 10 12 L 7 14 M 12 15 L 17 15";
    static inline const char* List = "M 8 6 L 21 6 M 8 12 L 21 12 M 8 18 L 21 18 M 3 6 L 4 6 M 3 12 L 4 12 M 3 18 L 4 18";
    static inline const char* Terminal = "M 4 6 L 10 12 L 4 18 M 12 19 L 20 19";
    static inline const char* TerminalBox = "M 7 15 L 10 12 L 7 9 M 13 15 L 17 15 M 7.8 21 L 16.2 21 C 17.88 21 18.72 21 19.36 20.67 C 19.93 20.39 20.39 19.93 20.67 19.36 C 21 18.72 21 17.88 21 16.2 L 21 7.8 C 21 6.12 21 5.28 20.67 4.64 C 20.39 4.07 19.93 3.61 19.36 3.33 C 18.72 3 17.88 3 16.2 3 L 7.8 3 C 6.12 3 5.28 3 4.64 3.33 C 4.07 3.61 3.61 4.07 3.33 4.64 C 3 5.28 3 6.12 3 7.8 L 3 16.2 C 3 17.88 3 18.72 3.33 19.36 C 3.61 19.93 4.07 20.39 4.64 20.67 C 5.28 21 6.12 21 7.8 21 Z";
    static inline const char* Check = "M 5 13 L 9 17 L 19 7";
    static inline const char* GitHub = "M 12 2.2467 A 10.00042 10.00042 0 0 0 8.83752 21.73419 c 0.5 0.08752 0.6875 -0.21247 0.6875 -0.475 c 0 -0.23749 -0.01251 -1.025 -0.01251 -1.86249 C 7 19.85919 6.35 18.78423 6.15 18.22173 A 3.636 3.636 0 0 0 5.125 16.8092 c -0.35 -0.1875 -0.85 -0.65 -0.01251 -0.66248 A 2.00117 2.00117 0 0 1 6.65 17.17169 a 2.13742 2.13742 0 0 0 2.91248 0.825 A 2.10376 2.10376 0 0 1 10.2 16.65923 c -2.225 -0.25 -4.55 -1.11254 -4.55 -4.9375 a 3.89187 3.89187 0 0 1 1.025 -2.6875 a 3.59373 3.59373 0 0 1 0.1 -2.65 s 0.83747 -0.26251 2.75 1.025 a 9.42747 9.42747 0 0 1 5 0 c 1.91248 -1.3 2.75 -1.025 2.75 -1.025 a 3.59323 3.59323 0 0 1 0.1 2.65 a 3.869 3.869 0 0 1 1.025 2.6875 c 0 3.83747 -2.33752 4.6875 -4.5625 4.9375 a 2.36814 2.36814 0 0 1 0.675 1.85 c 0 1.33752 -0.01251 2.41248 -0.01251 2.75 c 0 0.26251 0.1875 0.575 0.6875 0.475 A 10.0053 10.0053 0 0 0 12 2.2467 Z";
    static inline const char* Star = "M 26.285 2.486 l 5.407 10.956 c 0.376 0.762 1.103 1.29 1.944 1.412 l 12.091 1.757 c 2.118 0.308 2.963 2.91 1.431 4.403 l -8.749 8.528 c -0.608 0.593 -0.886 1.448 -0.742 2.285 l 2.065 12.042 c 0.362 2.109 -1.852 3.717 -3.746 2.722 l -10.814 -5.685 c -0.752 -0.395 -1.651 -0.395 -2.403 0 l -10.814 5.685 c -1.894 0.996 -4.108 -0.613 -3.746 -2.722 l 2.065 -12.042 c 0.144 -0.837 -0.134 -1.692 -0.742 -2.285 l -8.749 -8.528 c -1.532 -1.494 -0.687 -4.096 1.431 -4.403 l 12.091 -1.757 c 0.841 -0.122 1.568 -0.65 1.944 -1.412 l 5.407 -10.956 C 22.602 0.567 25.338 0.567 26.285 2.486 Z";
    static inline const char* Clipboard = "M 9 5 H 7 A 2 2 0 0 0 5 7 V 19 A 2 2 0 0 0 7 21 H 17 A 2 2 0 0 0 19 19 V 7 A 2 2 0 0 0 17 5 H 15 M 9 5 A 2 2 0 0 1 11 3 H 13 A 2 2 0 0 1 15 5 A 2 2 0 0 1 13 7 H 11 A 2 2 0 0 1 9 5 Z";
    static inline const char* ClipboardCheck = "M 9 5 H 7 A 2 2 0 0 0 5 7 V 19 A 2 2 0 0 0 7 21 H 17 A 2 2 0 0 0 19 19 V 7 A 2 2 0 0 0 17 5 H 15 M 9 5 A 2 2 0 0 1 11 3 H 13 A 2 2 0 0 1 15 5 A 2 2 0 0 1 13 7 H 11 A 2 2 0 0 1 9 5 Z M 9 14 L 11 16 L 15 12";
    static inline const char* Error = "M 3 12 A 9 9 0 1 0 21 12 A 9 9 0 1 0 3 12 M 9 9 L 15 15 M 15 9 L 9 15";
    static inline const char* Info = "M 3 12 A 9 9 0 1 0 21 12 A 9 9 0 1 0 3 12 M 12 11 L 12 17 M 12 7.5 L 12 7.6";
    static inline const char* Alert = "M 12 4 L 22 20 L 2 20 Z M 12 10 L 12 15 M 12 17.5 L 12 17.6";
    static inline const char* ChevDown = "M 6 9 L 12 15 L 18 9";
    static inline const char* ChevRight = "M 9 6 L 15 12 L 9 18";
    static inline const char* Sun = "M 12 7 A 5 5 0 1 0 12 17 A 5 5 0 1 0 12 7 Z M 12 1 L 12 4 M 12 20 L 12 23 M 4.22 4.22 L 6.34 6.34 M 17.66 17.66 L 19.78 19.78 M 1 12 L 4 12 M 20 12 L 23 12 M 4.22 19.78 L 6.34 17.66 M 17.66 6.34 L 19.78 4.22";
    static inline const char* Moon = "M 21 12.79 A 9 9 0 1 1 11.21 3 A 7 7 0 0 0 21 12.79 Z";

    static QIcon get(const char* svgPath, const QColor& color, int size = 16) {
        static QMap<QString, QIcon> cache;
        QString key = QString::asprintf("%p_%08x_%d", (const void*)svgPath, color.rgba(), size);
        auto it = cache.find(key);
        if (it != cache.end()) {
            return it.value();
        }

        QString svgStr = QString(
            "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" width=\"24\" height=\"24\">"
            "<path d=\"%1\" fill=\"none\" stroke=\"%2\" stroke-width=\"1.6\" stroke-linecap=\"round\" stroke-linejoin=\"round\"/>"
            "</svg>"
        ).arg(QString::fromLatin1(svgPath), color.name(QColor::HexRgb));

        QSvgRenderer renderer(svgStr.toUtf8());
        QIcon icon;

        // Render at 1x, 2x, 3x for crisp High-DPI support on Windows
        for (qreal scale : {1.0, 1.5, 2.0, 3.0}) {
            int px = static_cast<int>(size * scale);
            QPixmap pix(px, px);
            pix.fill(Qt::transparent);

            QPainter p(&pix);
            p.setRenderHint(QPainter::Antialiasing, true);
            p.setRenderHint(QPainter::SmoothPixmapTransform, true);
            renderer.render(&p);
            p.end();

            pix.setDevicePixelRatio(scale);
            icon.addPixmap(pix);
        }

        cache.insert(key, icon);
        return icon;
    }

    static QPixmap getPixmap(const char* svgPath, const QColor& color, int size = 16, qreal dpr = 2.0) {
        static QMap<QString, QPixmap> cache;
        QString key = QString::asprintf("%p_%08x_%d_%.2f", (const void*)svgPath, color.rgba(), size, dpr);
        auto it = cache.find(key);
        if (it != cache.end()) {
            return it.value();
        }

        int px = static_cast<int>(size * dpr);
        QString svgStr = QString(
            "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" width=\"24\" height=\"24\">"
            "<path d=\"%1\" fill=\"none\" stroke=\"%2\" stroke-width=\"1.6\" stroke-linecap=\"round\" stroke-linejoin=\"round\"/>"
            "</svg>"
        ).arg(QString::fromLatin1(svgPath), color.name(QColor::HexRgb));

        QSvgRenderer renderer(svgStr.toUtf8());
        QPixmap pix(px, px);
        pix.fill(Qt::transparent);

        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);
        renderer.render(&p);
        p.end();

        pix.setDevicePixelRatio(dpr);
        cache.insert(key, pix);
        return pix;
    }

    static QPixmap getFilledPixmap(const char* svgPath, const QColor& color, int size = 16, qreal dpr = 2.0, double vbW = 24.0, double vbH = 24.0) {
        static QMap<QString, QPixmap> cache;
        QString key = QString::asprintf("f_%p_%08x_%d_%.2f_%.0f_%.0f", (const void*)svgPath, color.rgba(), size, dpr, vbW, vbH);
        auto it = cache.find(key);
        if (it != cache.end()) {
            return it.value();
        }

        int px = static_cast<int>(size * dpr);
        QString svgStr = QString(
            "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 %3 %4\" width=\"%3\" height=\"%4\">"
            "<path d=\"%1\" fill=\"%2\" stroke=\"none\"/>"
            "</svg>"
        ).arg(QString::fromLatin1(svgPath), color.name(QColor::HexRgb), QString::number(vbW), QString::number(vbH));

        QSvgRenderer renderer(svgStr.toUtf8());
        QPixmap pix(px, px);
        pix.fill(Qt::transparent);

        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);
        renderer.render(&p);
        p.end();

        pix.setDevicePixelRatio(dpr);
        cache.insert(key, pix);
        return pix;
    }

    static QPixmap renderSvgFile(const QString& resourcePath, int size = 22, qreal dpr = 2.0) {
        int px = static_cast<int>(size * dpr);
        QSvgRenderer renderer(resourcePath);
        QPixmap pix(px, px);
        pix.fill(Qt::transparent);

        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);
        renderer.render(&p);
        p.end();

        pix.setDevicePixelRatio(dpr);
        return pix;
    }
};

} // namespace masscan::ui
