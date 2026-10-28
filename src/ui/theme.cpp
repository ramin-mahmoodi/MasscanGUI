#include "theme.hpp"

namespace masscan::theme {

QString getDarkStyle() {
    return R"(
        /* Global & Reset */
        QWidget {
            font-family: "Segoe UI";
            font-size: 13px;
            font-weight: 400;
            color: #e6ebf2;
        }

        QLabel#appTitle {
            font-family: "Segoe UI";
            font-size: 16px;
            font-weight: 700;
            color: #e6ebf2;
        }

        QLabel#sectionTitle {
            font-family: "Segoe UI";
            font-size: 14px;
            font-weight: 700;
            color: #e6ebf2;
            background: transparent;
            border: none;
        }

        QLabel#fieldLabel, QLabel[class="fieldLabel"] {
            font-family: "Segoe UI";
            font-size: 12px;
            font-weight: 400;
            color: #8b97a8;
        }

        QLabel#cmdTitleLabel {
            font-family: "Segoe UI";
            font-size: 13px;
            font-weight: 400;
            color: #8b97a8;
        }

        QLabel#dimLabel, QLabel[class="dimLabel"] {
            font-family: "Segoe UI";
            font-size: 12px;
            font-weight: 400;
            color: #8b97a8;
        }

        QLabel#summaryLabel, QLabel[class="summaryLabel"] {
            font-family: "Segoe UI";
            font-size: 12px;
            font-weight: 400;
            color: #3b9eff;
        }

        QMainWindow, QDialog {
            background-color: #0f1318;
        }

        QWidget#centralWidget, QWidget#bodyContainer, QWidget#rightColumn {
            background-color: #0f1318;
        }

        QScrollArea, QScrollArea > QWidget, QScrollArea > QWidget > QWidget {
            border: none;
            background: transparent;
            background-color: transparent;
        }

        /* Header (44px) */
        QWidget#headerWidget {
            background-color: #171c24;
            border: none;
            border-bottom: 1px solid #2a3340;
        }

        /* Footer (34px) */
        QWidget#footerWidget {
            background-color: #171c24;
            border: none;
            border-top: 1px solid #2a3340;
        }

        QLabel#footerStateLabel, QLabel#footerFoundLabel {
            font-family: "Segoe UI";
            font-size: 13px;
            font-weight: 700;
            color: #e6ebf2;
        }

        QLabel#footerPctLabel, QLabel#footerEtaLabel, QLabel#footerKppsLabel {
            font-family: "Segoe UI";
            font-size: 13px;
            font-weight: 400;
            color: #8b97a8;
        }

        /* Action Bar */
        QWidget#actionBar {
            background-color: transparent;
            border: none;
            border-top: 1px solid #2a3340;
        }

        QWidget#topActionBar {
            background-color: #171c24;
            border: 1px solid #2a3340;
            border-radius: 8px;
        }

        /* Input Controls */
        QLineEdit, QComboBox {
            background-color: #252931;
            border: 1px solid #363a41;
            border-radius: 6px;
            padding: 0px 8px;
            color: #e6ebf2;
            min-height: 30px;
            max-height: 30px;
            selection-background-color: #3b9eff;
        }

        QLineEdit:hover, QComboBox:hover {
            background-color: #2a2f38;
            border-color: #454b56;
        }

        QLineEdit:focus {
            background-color: #1e1e1e;
            border: 1px solid #3b9eff;
            color: #ffffff;
        }

        QLineEdit QToolButton {
            border: none;
            background: transparent;
            padding: 0px;
            margin: 0px;
        }

        QLineEdit QToolButton:hover, QLineEdit QToolButton:pressed {
            border: none;
            background: transparent;
        }

        QComboBox:focus, QComboBox:on {
            background-color: #1e1e1e;
            border: 1px solid #3b9eff;
            color: #ffffff;
        }

        QComboBox::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: top right;
            width: 22px;
            border-left: none;
        }

        QComboBox::down-arrow {
            image: url(:/chev_down.svg);
            width: 10px;
            height: 6px;
        }

        QComboBox QFrame {
            background: transparent;
            background-color: transparent;
            border: none;
        }

        QComboBox QAbstractItemView {
            background-color: #1e2530;
            border: 1px solid #2a3340;
            border-radius: 6px;
            padding: 4px;
            outline: none;
            selection-background-color: #3b9eff;
            selection-color: #ffffff;
            color: #e6ebf2;
        }

        QComboBox QAbstractItemView::item {
            min-height: 26px;
            padding: 4px 8px;
            border-radius: 4px;
            color: #e6ebf2;
        }

        QComboBox QAbstractItemView::item:hover {
            background-color: #2a3340;
            color: #ffffff;
        }

        QComboBox QAbstractItemView::item:selected {
            background-color: #3b9eff;
            color: #ffffff;
        }

        /* Buttons */
        QPushButton {
            background-color: #1e2530;
            border: 1px solid #2a3340;
            border-radius: 6px;
            padding: 0px 10px;
            min-height: 30px;
            max-height: 30px;
            font-family: "Segoe UI";
            font-size: 13px;
            font-weight: 400;
            color: #e6ebf2;
        }

        QPushButton:hover {
            background-color: #252d3a;
            border-color: #2a3340;
        }

        QPushButton:pressed {
            background-color: #2a3340;
        }

        QPushButton:disabled {
            background-color: #1e2530;
            border-color: #2a3340;
            color: #8b97a8;
            opacity: 0.55;
        }

        QPushButton[compact="true"] {
            padding: 0px;
            min-width: 32px;
            max-width: 34px;
        }

        QPushButton[class="primary"], QPushButton#btnStart {
            background-color: #3b9eff;
            border: 1px solid #3b9eff;
            color: #ffffff;
            font-family: "Segoe UI";
            font-size: 13px;
            font-weight: 400;
        }

        QPushButton[class="primary"]:hover, QPushButton#btnStart:hover {
            background-color: #52abff;
            border-color: #52abff;
        }

        QPushButton[class="primary"]:pressed, QPushButton#btnStart:pressed {
            background-color: #268be6;
        }

        QPushButton[class="danger"], QPushButton#btnStop {
            background-color: #ef5350;
            border: 1px solid #ef5350;
            color: #ffffff;
            font-family: "Segoe UI";
            font-size: 13px;
            font-weight: 400;
        }

        QPushButton[class="danger"]:hover, QPushButton#btnStop:hover {
            background-color: #f46a67;
        }

        /* CheckBoxes */
        QCheckBox {
            spacing: 8px;
            font-family: "Segoe UI";
            font-size: 13px;
            font-weight: 400;
            color: #e6ebf2;
        }

        QCheckBox::indicator {
            width: 15px;
            height: 15px;
            border: 1px solid #8e96a2;
            border-radius: 4px;
            background-color: #252931;
        }

        QCheckBox::indicator:hover {
            border-color: #3b9eff;
            background-color: #2c3340;
        }

        QCheckBox::indicator:checked {
            background-color: #3b9eff;
            border-color: #3b9eff;
            image: url(:/check.svg);
        }

        /* Lists & Tables */
        QListWidget#countryList, QListWidget#targetsList {
            background-color: #1e2530;
            border: 1px solid #2a3340;
            border-radius: 6px;
            padding: 0px;
            color: #e6ebf2;
            outline: none;
        }

        QListWidget#countryList::item, QListWidget#targetsList::item {
            color: #e6ebf2;
            outline: none;
            border: none;
            background: transparent;
        }

        QListWidget#countryList:focus, QListWidget#targetsList:focus {
            outline: none;
        }

        QTableView, QTableWidget {
            background-color: #171c24;
            border: none;
            color: #e6ebf2;
            font-family: "Segoe UI";
            font-size: 13px;
            font-weight: 400;
            gridline-color: transparent;
            selection-background-color: #1e2e45;
            selection-color: #e6ebf2;
        }

        QHeaderView::section {
            background-color: #171c24;
            border: none;
            border-bottom: 1px solid #2a3340;
            padding: 6px 10px;
            color: #8b97a8;
            font-family: "Segoe UI";
            font-size: 13px;
            font-weight: 400;
            text-align: left;
        }

        /* TextEdit / Log */
        QPlainTextEdit {
            background-color: #171c24;
            border: none;
            color: #e6ebf2;
            font-family: "Segoe UI";
            font-size: 13px;
            font-weight: 400;
            padding: 8px;
        }

        /* Progress Bar */
        QProgressBar#footerProgressBar {
            background-color: #2a3340;
            border: none;
            border-radius: 4px;
        }

        QProgressBar#footerProgressBar::chunk {
            background-color: #3b9eff;
            border-radius: 4px;
        }

        /* Scrollbars (slim 6px, smooth rounded pill, transparent track) */
        QScrollBar:vertical {
            background: transparent;
            width: 6px;
            margin: 0px;
            border: none;
        }

        QScrollBar::handle:vertical {
            background: #2a3340;
            min-height: 24px;
            border-radius: 3px;
        }

        QScrollBar::handle:vertical:hover {
            background: #3b9eff;
        }

        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
            background: transparent;
            border: none;
        }

        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
            background: transparent;
            border: none;
        }

        QScrollBar:horizontal {
            background: transparent;
            height: 6px;
            margin: 0px;
            border: none;
        }

        QScrollBar::handle:horizontal {
            background: #2a3340;
            min-width: 24px;
            border-radius: 3px;
        }

        QScrollBar::handle:horizontal:hover {
            background: #3b9eff;
        }

        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0px;
            background: transparent;
            border: none;
        }

        QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {
            background: transparent;
            border: none;
        }

        QAbstractScrollArea::corner {
            background: transparent;
            border: none;
        }

        /* Header buttons */
        QPushButton#themeToggleBtn {
            background: transparent;
            border: 1px solid transparent;
            border-radius: 6px;
            padding: 0px;
            min-height: 26px;
            max-height: 26px;
        }

        QPushButton#menuBtn {
            background: transparent;
            border: 1px solid transparent;
            border-radius: 6px;
            padding: 0px;
            min-height: 30px;
            max-height: 30px;
        }

        QPushButton#themeToggleBtn:hover, QPushButton#menuBtn:hover {
            background-color: #1e2530;
            border: 1px solid #2a3340;
        }

        /* Tab Pills */
        QPushButton[class="pillActive"], QPushButton#pillActive {
            background: transparent;
            border: none;
            border-bottom: 2px solid #3b9eff;
            font-family: "Segoe UI";
            font-size: 13px;
            font-weight: 700;
            color: #e6ebf2;
            padding: 0px;
            min-height: 30px;
            max-height: 30px;
            border-radius: 0px;
        }

        QPushButton[class="pillInactive"], QPushButton#pillInactive {
            background: transparent;
            border: none;
            border-bottom: 2px solid transparent;
            font-family: "Segoe UI";
            font-size: 13px;
            font-weight: 400;
            color: #8b97a8;
            padding: 0px;
            min-height: 30px;
            max-height: 30px;
            border-radius: 0px;
        }

        QPushButton[class="pillInactive"]:hover, QPushButton#pillInactive:hover {
            color: #e6ebf2;
        }

        /* Badges */
        QWidget[class="badgePill"] {
            background-color: #1e2530;
            border-radius: 12px;
        }

        /* Tooltip */
        QToolTip {
            background-color: transparent;
            border: none;
            color: transparent;
        }
    )";
}

QString getLightStyle() {
    return R"(
        /* Global & Reset */
        QWidget {
            font-family: "Segoe UI";
            font-size: 13px;
            font-weight: 400;
            color: #1a2230;
        }

        QLabel#appTitle {
            font-family: "Segoe UI";
            font-size: 16px;
            font-weight: 700;
            color: #1a2230;
        }

        QLabel#sectionTitle {
            font-family: "Segoe UI";
            font-size: 14px;
            font-weight: 700;
            color: #1a2230;
            background: transparent;
            border: none;
        }

        QLabel#fieldLabel, QLabel[class="fieldLabel"] {
            font-family: "Segoe UI";
            font-size: 12px;
            font-weight: 400;
            color: #5b6a7e;
        }

        QLabel#cmdTitleLabel {
            font-family: "Segoe UI";
            font-size: 13px;
            font-weight: 400;
            color: #5b6a7e;
        }

        QLabel#dimLabel, QLabel[class="dimLabel"] {
            font-family: "Segoe UI";
            font-size: 12px;
            font-weight: 400;
            color: #5b6a7e;
        }

        QLabel#summaryLabel, QLabel[class="summaryLabel"] {
            font-family: "Segoe UI";
            font-size: 12px;
            font-weight: 400;
            color: #1f73d6;
        }

        QMainWindow, QDialog {
            background-color: #f2f4f8;
        }

        QWidget#centralWidget, QWidget#bodyContainer, QWidget#rightColumn {
            background-color: #f2f4f8;
        }

        QScrollArea, QScrollArea > QWidget, QScrollArea > QWidget > QWidget {
            border: none;
            background: transparent;
            background-color: transparent;
        }

        /* Header (44px) */
        QWidget#headerWidget {
            background-color: #ffffff;
            border: none;
            border-bottom: 1px solid #d3dae4;
        }

        /* Footer (34px) */
        QWidget#footerWidget {
            background-color: #ffffff;
            border: none;
            border-top: 1px solid #d3dae4;
        }

        QLabel#footerStateLabel, QLabel#footerFoundLabel {
            font-family: "Segoe UI";
            font-size: 13px;
            font-weight: 700;
            color: #1a2230;
        }

        QLabel#footerPctLabel, QLabel#footerEtaLabel, QLabel#footerKppsLabel {
            font-family: "Segoe UI";
            font-size: 13px;
            font-weight: 400;
            color: #5b6a7e;
        }

        /* Action Bar */
        QWidget#actionBar {
            background-color: transparent;
            border: none;
            border-top: 1px solid #d3dae4;
        }

        QWidget#topActionBar {
            background-color: #ffffff;
            border: 1px solid #d3dae4;
            border-radius: 8px;
        }

        /* Input Controls */
        QLineEdit, QComboBox {
            background-color: #f6f8fb;
            border: 1px solid #d3dae4;
            border-radius: 6px;
            padding: 0px 8px;
            color: #1a2230;
            min-height: 30px;
            max-height: 30px;
            selection-background-color: #1f73d6;
        }

        QLineEdit:hover, QComboBox:hover {
            background-color: #ffffff;
            border-color: #b5c0d0;
        }

        QLineEdit:focus {
            background-color: #ffffff;
            border: 1px solid #1f73d6;
            color: #1a2230;
        }

        QLineEdit QToolButton {
            border: none;
            background: transparent;
            padding: 0px;
            margin: 0px;
        }

        QLineEdit QToolButton:hover, QLineEdit QToolButton:pressed {
            border: none;
            background: transparent;
        }

        QComboBox:focus, QComboBox:on {
            background-color: #ffffff;
            border: 1px solid #1f73d6;
            color: #1a2230;
        }

        QComboBox::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: top right;
            width: 22px;
            border-left: none;
        }

        QComboBox::down-arrow {
            image: url(:/chev_down_light.svg);
            width: 10px;
            height: 6px;
        }

        QComboBox QFrame {
            background: transparent;
            background-color: transparent;
            border: none;
        }

        QComboBox QAbstractItemView {
            background-color: #ffffff;
            border: 1px solid #d3dae4;
            border-radius: 6px;
            padding: 4px;
            outline: none;
            selection-background-color: #1f73d6;
            selection-color: #ffffff;
            color: #1a2230;
        }

        QComboBox QAbstractItemView::item {
            min-height: 26px;
            padding: 4px 8px;
            border-radius: 4px;
            color: #1a2230;
        }

        QComboBox QAbstractItemView::item:hover {
            background-color: #eef1f6;
            color: #1a2230;
        }

        QComboBox QAbstractItemView::item:selected {
            background-color: #1f73d6;
            color: #ffffff;
        }

        /* Buttons */
        QPushButton {
            background-color: #eef1f6;
            border: 1px solid #d3dae4;
            border-radius: 6px;
            padding: 0px 10px;
            min-height: 30px;
            max-height: 30px;
            font-family: "Segoe UI";
            font-size: 13px;
            font-weight: 400;
            color: #1a2230;
        }

        QPushButton:hover {
            background-color: #e2e7ef;
            border-color: #d3dae4;
        }

        QPushButton:pressed {
            background-color: #d3dae4;
        }

        QPushButton:disabled {
            background-color: #eef1f6;
            border-color: #d3dae4;
            color: #5b6a7e;
            opacity: 0.55;
        }

        QPushButton[compact="true"] {
            padding: 0px;
            min-width: 32px;
            max-width: 34px;
        }

        QPushButton[compact="true"] {
            padding: 0px;
            min-width: 32px;
            max-width: 34px;
        }

        QPushButton[class="primary"], QPushButton#btnStart {
            background-color: #1f73d6;
            border: 1px solid #1f73d6;
            color: #ffffff;
            font-family: "Segoe UI";
            font-size: 13px;
            font-weight: 400;
        }

        QPushButton[class="primary"]:hover, QPushButton#btnStart:hover {
            background-color: #3182e6;
            border-color: #3182e6;
        }

        QPushButton[class="primary"]:pressed, QPushButton#btnStart:pressed {
            background-color: #175bb0;
        }

        QPushButton[class="danger"], QPushButton#btnStop {
            background-color: #d32f2f;
            border: 1px solid #d32f2f;
            color: #ffffff;
            font-family: "Segoe UI";
            font-size: 13px;
            font-weight: 400;
        }

        QPushButton[class="danger"]:hover, QPushButton#btnStop:hover {
            background-color: #e53935;
        }

        /* CheckBoxes */
        QCheckBox {
            spacing: 8px;
            font-family: "Segoe UI";
            font-size: 13px;
            font-weight: 400;
            color: #1a2230;
        }

        QCheckBox::indicator {
            width: 15px;
            height: 15px;
            border: 1px solid #6c7889;
            border-radius: 4px;
            background-color: #ffffff;
        }

        QCheckBox::indicator:hover {
            border-color: #1f73d6;
            background-color: #f2f5f9;
        }

        QCheckBox::indicator:checked {
            background-color: #1f73d6;
            border-color: #1f73d6;
            image: url(:/check.svg);
        }

        /* Lists & Tables */
        QListWidget#countryList, QListWidget#targetsList {
            background-color: #eef1f6;
            border: 1px solid #d3dae4;
            border-radius: 6px;
            padding: 0px;
            color: #1a2230;
            outline: none;
        }

        QListWidget#countryList::item, QListWidget#targetsList::item {
            color: #1a2230;
            outline: none;
            border: none;
            background: transparent;
        }

        QListWidget#countryList:focus, QListWidget#targetsList:focus {
            outline: none;
        }

        QTableView, QTableWidget {
            background-color: #ffffff;
            border: none;
            color: #1a2230;
            font-family: "Segoe UI";
            font-size: 13px;
            font-weight: 400;
            gridline-color: transparent;
            selection-background-color: #e6f0fa;
            selection-color: #1f73d6;
        }

        QHeaderView::section {
            background-color: #ffffff;
            border: none;
            border-bottom: 1px solid #d3dae4;
            padding: 6px 10px;
            color: #5b6a7e;
            font-family: "Segoe UI";
            font-size: 13px;
            font-weight: 400;
            text-align: left;
        }

        /* TextEdit / Log */
        QPlainTextEdit {
            background-color: #ffffff;
            border: none;
            color: #1a2230;
            font-family: "Segoe UI";
            font-size: 13px;
            font-weight: 400;
            padding: 8px;
        }

        /* Progress Bar */
        QProgressBar#footerProgressBar {
            background-color: #d3dae4;
            border: none;
            border-radius: 4px;
        }

        QProgressBar#footerProgressBar::chunk {
            background-color: #1f73d6;
            border-radius: 4px;
        }

        /* Scrollbars (slim 6px, smooth rounded pill, transparent track) */
        QScrollBar:vertical {
            background: transparent;
            width: 6px;
            margin: 0px;
            border: none;
        }

        QScrollBar::handle:vertical {
            background: #d3dae4;
            min-height: 24px;
            border-radius: 3px;
        }

        QScrollBar::handle:vertical:hover {
            background: #1f73d6;
        }

        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
            background: transparent;
            border: none;
        }

        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
            background: transparent;
            border: none;
        }

        QScrollBar:horizontal {
            background: transparent;
            height: 6px;
            margin: 0px;
            border: none;
        }

        QScrollBar::handle:horizontal {
            background: #d3dae4;
            min-width: 24px;
            border-radius: 3px;
        }

        QScrollBar::handle:horizontal:hover {
            background: #1f73d6;
        }

        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0px;
            background: transparent;
            border: none;
        }

        QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {
            background: transparent;
            border: none;
        }

        QAbstractScrollArea::corner {
            background: transparent;
            border: none;
        }

        /* Header buttons */
        QPushButton#themeToggleBtn {
            background: transparent;
            border: 1px solid transparent;
            border-radius: 6px;
            padding: 0px;
            min-height: 26px;
            max-height: 26px;
        }

        QPushButton#menuBtn {
            background: transparent;
            border: 1px solid transparent;
            border-radius: 6px;
            padding: 0px;
            min-height: 30px;
            max-height: 30px;
        }

        QPushButton#themeToggleBtn:hover, QPushButton#menuBtn:hover {
            background-color: #eef1f6;
            border: 1px solid #d3dae4;
        }

        /* Tab Pills */
        QPushButton[class="pillActive"], QPushButton#pillActive {
            background: transparent;
            border: none;
            border-bottom: 2px solid #1f73d6;
            font-family: "Segoe UI";
            font-size: 13px;
            font-weight: 700;
            color: #1a2230;
            padding: 0px;
            min-height: 30px;
            max-height: 30px;
            border-radius: 0px;
        }

        QPushButton[class="pillInactive"], QPushButton#pillInactive {
            background: transparent;
            border: none;
            border-bottom: 2px solid transparent;
            font-family: "Segoe UI";
            font-size: 13px;
            font-weight: 400;
            color: #5b6a7e;
            padding: 0px;
            min-height: 30px;
            max-height: 30px;
            border-radius: 0px;
        }

        QPushButton[class="pillInactive"]:hover, QPushButton#pillInactive:hover {
            color: #1a2230;
        }

        /* Badges */
        QWidget[class="badgePill"] {
            background-color: #eef1f6;
            border-radius: 12px;
        }

        /* Tooltip */
        QToolTip {
            background-color: transparent;
            border: none;
            color: transparent;
        }
    )";
}

} // namespace masscan::theme
