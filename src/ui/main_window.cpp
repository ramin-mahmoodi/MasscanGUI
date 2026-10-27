#include "main_window.hpp"
#include "collapsible_section.hpp"
#include "fluent_scroll_bar.hpp"
#include "fluent_combo_box.hpp"
#include "jelly_checkbox.hpp"
#include "glitch_button.hpp"
#include "icons.hpp"
#include "theme.hpp"
#include "terminal_box.hpp"
#include "github_button.hpp"
#include "fluent_tooltip.hpp"
#include "../core/geo.hpp"

#include <iostream>
#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QResizeEvent>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QTextStream>
#include <QMessageBox>
#include <QInputDialog>
#include <QProgressDialog>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QScrollBar>
#include <QTimer>
#include <QPropertyAnimation>
#include <QWheelEvent>
#include <QEasingCurve>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTime>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace masscan::ui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      m_scanner(new ScannerProcess(this)),
      m_drawerAnim(this) {

    setWindowTitle("MasscanGUI");
    setWindowIcon(QIcon(":/Icon.ico"));
    resize(1100, 650);
    setMinimumSize(360, 440);

    m_drawerAnim.setDuration(300);
    m_drawerAnim.onProgress = [this](double p) {
        QEasingCurve curve(QEasingCurve::BezierSpline);
        curve.addCubicBezierSegment(QPointF(0.65, 0.0), QPointF(0.35, 1.0), QPointF(1.0, 1.0));
        const qreal eased = curve.valueForProgress(p);
        m_drawerProgress = m_drawerStart + (m_drawerTarget - m_drawerStart) * eased;
        if (m_menuBtn) {
            m_menuBtn->setProgress(m_drawerProgress);
        }
        if (m_drawerOverlay && m_drawerOverlay->isVisible()) {
            m_drawerOverlay->setProgress(m_drawerProgress);
        }
    };
    m_drawerAnim.onFinished = [this]() {
        m_drawerProgress = m_drawerTarget;
        if (m_menuBtn) {
            m_menuBtn->setProgress(m_drawerProgress);
        }
        if (m_drawerOverlay) {
            m_drawerOverlay->hide();
        }
        layoutBodyWidgets();
    };

    // Default target: 10.0.0.0/24
    TargetItem defaultTarget;
    defaultTarget.text = "10.0.0.0/24";
    defaultTarget.code = "";
    defaultTarget.info = "256 IPs";
    defaultTarget.isCountry = false;
    m_targets.push_back(defaultTarget);

    setupUi();
    for (auto* btn : findChildren<QPushButton*>()) {
        btn->setCursor(Qt::PointingHandCursor);
    }
    setStyleSheet(theme::getDarkStyle());

    connect(m_scanner, &ScannerProcess::statusChanged, this, &MainWindow::handleScannerStatus);
    connect(m_scanner, &ScannerProcess::progressUpdated, this, &MainWindow::handleScannerProgress);
    connect(m_scanner, &ScannerProcess::portDiscovered, this, &MainWindow::handlePortDiscovered);
    connect(m_scanner, &ScannerProcess::logLineReceived, this, &MainWindow::handleLogLine);
    connect(m_scanner, &ScannerProcess::scanFinished, this, &MainWindow::handleScanFinished);
    connect(m_scanner, &ScannerProcess::scanError, this, [this](const QString& err) {
        handleLogLine("[ERROR] " + err);
        m_footerStateLabel->setText("Error: " + err);
    });

    checkDependencies();
    populateCountryList();
    updateCommandAndValidation();
    updateThemeDependentAssets();
}

void MainWindow::setupUi() {
    auto* central = new QWidget(this);
    central->setObjectName("centralWidget");
    setCentralWidget(central);

    auto* rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // 1. Header (44px)
    createHeader();
    rootLayout->addWidget(m_headerWidget);

    // 2. Body Area (manual positioning like Slint so drawer overlays rightColumn smoothly)
    m_bodyContainer = new QWidget(central);
    m_bodyContainer->setObjectName("bodyContainer");

    m_rightColumn = createRightColumn();
    m_rightColumn->setParent(m_bodyContainer);

    m_drawerBackdrop = new DrawerBackdrop(m_bodyContainer);
    connect(m_drawerBackdrop, &DrawerBackdrop::clicked, this, [this]() {
        setDrawerOpen(false, true);
    });

    m_sidebarCard = createSidebar();
    m_sidebarCard->setParent(m_bodyContainer);

    m_drawerOverlay = new DrawerAnimOverlay(m_bodyContainer);

    rootLayout->addWidget(m_bodyContainer, 1);

    // 3. Footer (34px)
    createFooter();
    rootLayout->addWidget(m_footerWidget);
}

void MainWindow::createHeader() {
    m_headerWidget = new QWidget(this);
    m_headerWidget->setObjectName("headerWidget");
    m_headerWidget->setFixedHeight(44);

    auto* layout = new QHBoxLayout(m_headerWidget);
    layout->setContentsMargins(16, 0, 12, 0);
    layout->setSpacing(8);

    // Menu button (narrow mode)
    m_menuBtn = new HamburgerButton(m_headerWidget);
    m_menuBtn->setObjectName("menuBtn");
    m_menuBtn->setDark(m_isDarkTheme);
    m_menuBtn->setVisible(false);
    connect(m_menuBtn, &QAbstractButton::clicked, this, &MainWindow::onToggleSidebarMenu);
    layout->addWidget(m_menuBtn);

    // Logo & Title
    m_logoLabel = new QLabel(m_headerWidget);
    qreal dpr = devicePixelRatioF();
    m_logoLabel->setPixmap(Icons::renderSvgFile(":/Icon.svg", 22, dpr));
    layout->addWidget(m_logoLabel);

    m_titleLabel = new QLabel("MasscanGUI", m_headerWidget);
    m_titleLabel->setObjectName("appTitle");
    layout->addWidget(m_titleLabel);

    layout->addStretch(1);

    // Badges
    m_masscanBadge = new BadgeWidget("masscan", m_headerWidget);
    m_npcapBadge = new BadgeWidget("Npcap", m_headerWidget);
    connect(m_masscanBadge, &BadgeWidget::clicked, this, &MainWindow::onRecheckDependencies);
    connect(m_npcapBadge, &BadgeWidget::clicked, this, &MainWindow::onRecheckDependencies);

    layout->addWidget(m_masscanBadge);
    layout->addWidget(m_npcapBadge);

    // GitHub Star Button (next to the top badges)
    m_githubBtn = new GitHubButton(m_headerWidget);
    m_githubBtn->setDark(m_isDarkTheme);
    connect(m_githubBtn, &QAbstractButton::clicked, this, [this]() {
        m_githubBtn->toggleStarred();
        QDesktopServices::openUrl(QUrl("https://github.com/robertdavidgraham/masscan"));
    });
    layout->addWidget(m_githubBtn);

    // Theme toggle
    m_themeToggleBtn = new ThemeToggleBtn(m_headerWidget);
    m_themeToggleBtn->setDark(m_isDarkTheme);
    connect(m_themeToggleBtn, &QAbstractButton::clicked, this, &MainWindow::onToggleTheme);
    layout->addWidget(m_themeToggleBtn);
}

void MainWindow::createFooter() {
    m_footerWidget = new QWidget(this);
    m_footerWidget->setObjectName("footerWidget");
    m_footerWidget->setFixedHeight(34);

    auto* layout = new QHBoxLayout(m_footerWidget);
    layout->setContentsMargins(16, 0, 16, 0);
    layout->setSpacing(14);

    m_footerDot = new QLabel(m_footerWidget);
    m_footerDot->setFixedSize(8, 8);
    m_footerDot->setStyleSheet("background-color: #8b97a8; border-radius: 4px;");
    layout->addWidget(m_footerDot);

    m_footerStateLabel = new QLabel("Idle", m_footerWidget);
    m_footerStateLabel->setObjectName("footerStateLabel");
    layout->addWidget(m_footerStateLabel);

    // Slim progress bar (8px height)
    m_footerProgressBar = new QProgressBar(m_footerWidget);
    m_footerProgressBar->setObjectName("footerProgressBar");
    m_footerProgressBar->setFixedHeight(8);
    m_footerProgressBar->setRange(0, 100);
    m_footerProgressBar->setValue(0);
    m_footerProgressBar->setTextVisible(false);
    layout->addWidget(m_footerProgressBar, 1);

    m_footerPctLabel = new QLabel("0.0% done", m_footerWidget);
    m_footerPctLabel->setObjectName("footerPctLabel");
    layout->addWidget(m_footerPctLabel);

    m_footerEtaLabel = new QLabel("ETA –", m_footerWidget);
    m_footerEtaLabel->setObjectName("footerEtaLabel");
    layout->addWidget(m_footerEtaLabel);

    m_footerKppsLabel = new QLabel("0.00 kpps", m_footerWidget);
    m_footerKppsLabel->setObjectName("footerKppsLabel");
    layout->addWidget(m_footerKppsLabel);

    m_footerFoundLabel = new QLabel("Found 0", m_footerWidget);
    m_footerFoundLabel->setObjectName("footerFoundLabel");
    layout->addWidget(m_footerFoundLabel);
}

CardWidget* MainWindow::createSidebar() {
    m_sidebarCard = new CardWidget(this, 8);
    m_sidebarCard->setObjectName("sidebarCard");
    m_sidebarCard->setFixedWidth(380);
    m_sidebarCard->setDark(m_isDarkTheme);

    auto* sidebarLayout = new QVBoxLayout(m_sidebarCard);
    sidebarLayout->setContentsMargins(1, 1, 1, 1);
    sidebarLayout->setSpacing(0);

    auto* scroll = new QScrollArea(m_sidebarCard);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    if (scroll->viewport()) {
        scroll->viewport()->setAttribute(Qt::WA_StyledBackground, false);
    }

    m_sidebarScrollOverlay = new FluentScrollBarOverlay(scroll);
    m_sidebarScrollOverlay->setDark(m_isDarkTheme);

    auto* scrollContent = new QWidget(scroll);
    scrollContent->setObjectName("sidebarScrollContent");
    scrollContent->setAttribute(Qt::WA_StyledBackground, false);
    auto* contentLayout = new QVBoxLayout(scrollContent);
    // Perfectly uniform 10px margins in all 4 directions (top, bottom, left, right)
    contentLayout->setContentsMargins(10, 10, 10, 10);
    contentLayout->setSpacing(10);

    auto makeField = [](const QString& label, QLineEdit*& outEdit, const QString& placeholder = "", const QString& defaultVal = "") {
        auto* box = new QWidget();
        auto* l = new QVBoxLayout(box);
        l->setContentsMargins(0, 0, 0, 0);
        l->setSpacing(3);
        auto* lbl = new QLabel(label, box);
        lbl->setObjectName("fieldLabel");
        lbl->setProperty("class", "fieldLabel");
        outEdit = new QLineEdit(defaultVal, box);
        outEdit->setPlaceholderText(placeholder);
        l->addWidget(lbl);
        l->addWidget(outEdit);
        return box;
    };

    // ----------------- Section 1: Targets -----------------
    auto* secTargets = new CollapsibleSection("Targets", Icons::Target, scrollContent, true);
    auto* tLayout = new QVBoxLayout();

    auto* lblGeo = new QLabel("Countries — Built-in database: 238 countries, IPv4, snapshot 2026-04");
    lblGeo->setWordWrap(true);
    lblGeo->setObjectName("fieldLabel");
    lblGeo->setProperty("class", "fieldLabel");
    tLayout->addWidget(lblGeo);

    // Search bar
    m_countrySearchEdit = new QLineEdit();
    m_countrySearchEdit->setPlaceholderText("Search country or code… (e.g. iran, de)");
    m_countrySearchAction = m_countrySearchEdit->addAction(
        Icons::get(Icons::Search, m_isDarkTheme ? QColor("#8b97a8") : QColor("#5b6a7e"), 16),
        QLineEdit::LeadingPosition);
    connect(m_countrySearchAction, &QAction::triggered, m_countrySearchEdit, qOverload<>(&QLineEdit::setFocus));
    tLayout->addWidget(m_countrySearchEdit);

    // Country List (144px height)
    m_countryListWidget = new QListWidget();
    m_countryListWidget->setObjectName("countryList");
    m_countryListWidget->setFixedHeight(144);
    m_countryListWidget->setUniformItemSizes(true);
    m_countryListWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_countryListWidget->setSelectionMode(QAbstractItemView::NoSelection);
    m_countryListWidget->setFocusPolicy(Qt::NoFocus);
    m_countryDelegate = new CountryItemDelegate(&m_pickedCountryCodes, m_countryListWidget);
    m_countryDelegate->setDark(m_isDarkTheme);
    m_countryListWidget->setItemDelegate(m_countryDelegate);
    m_countryScrollOverlay = new FluentScrollBarOverlay(m_countryListWidget);
    m_countryScrollOverlay->setDark(m_isDarkTheme);
    tLayout->addWidget(m_countryListWidget);

    m_btnAddCountries = new QPushButton("Add selected to Targets");
    m_btnAddCountries->setEnabled(false);
    tLayout->addWidget(m_btnAddCountries);

    // Manual targets
    auto* lblManual = new QLabel("Manual IP / CIDR / range (IPv4 & IPv6)");
    lblManual->setObjectName("fieldLabel");
    lblManual->setProperty("class", "fieldLabel");
    tLayout->addWidget(lblManual);

    auto* manualRow = new QHBoxLayout();
    manualRow->setSpacing(6);
    m_manualTargetEdit = new QLineEdit();
    m_manualTargetEdit->setPlaceholderText("1.2.3.0/24 or 10.0.0.1-10.0.0.9");
    m_btnAddManual = new QPushButton("Add");
    manualRow->addWidget(m_manualTargetEdit);
    manualRow->addWidget(m_btnAddManual);
    tLayout->addLayout(manualRow);

    auto* lblTargetsList = new QLabel("Targets");
    lblTargetsList->setObjectName("fieldLabel");
    lblTargetsList->setProperty("class", "fieldLabel");
    tLayout->addWidget(lblTargetsList);

    // Targets list (88px height)
    m_targetsListWidget = new QListWidget();
    m_targetsListWidget->setObjectName("targetsList");
    m_targetsListWidget->setFixedHeight(88);
    m_targetsListWidget->setUniformItemSizes(true);
    m_targetsListWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_targetsListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_targetsListWidget->setFocusPolicy(Qt::NoFocus);
    m_targetDelegate = new TargetItemDelegate(m_targetsListWidget);
    m_targetDelegate->setDark(m_isDarkTheme);
    m_targetsListWidget->setItemDelegate(m_targetDelegate);
    m_targetsScrollOverlay = new FluentScrollBarOverlay(m_targetsListWidget);
    m_targetsScrollOverlay->setDark(m_isDarkTheme);
    tLayout->addWidget(m_targetsListWidget);

    // Target buttons row: Edit, Remove, Clear All (equal 1:1:1 width)
    auto* tBtnRow = new QHBoxLayout();
    tBtnRow->setSpacing(6);
    m_btnEditTarget = new QPushButton("Edit");
    m_btnRemoveTarget = new QPushButton("Remove");
    m_btnClearTargets = new QPushButton("Clear All");

    tBtnRow->addWidget(m_btnEditTarget, 1);
    tBtnRow->addWidget(m_btnRemoveTarget, 1);
    tBtnRow->addWidget(m_btnClearTargets, 1);
    tLayout->addLayout(tBtnRow);

    m_targetsSummaryLabel = new QLabel("1 target (256 IPs)");
    m_targetsSummaryLabel->setObjectName("summaryLabel");
    tLayout->addWidget(m_targetsSummaryLabel);

    m_allIpv4Check = new JellyCheckBox("Entire IPv4 internet (0.0.0.0/0)");
    tLayout->addWidget(m_allIpv4Check);

    m_btnImportTargets = new QPushButton("Import list from file…");
    tLayout->addWidget(m_btnImportTargets);

    auto* excRow = new QHBoxLayout();
    excRow->setSpacing(8);
    excRow->addWidget(makeField("Exclude (--exclude)", m_excludeEdit, "10.0.0.5,10.0.0.9"));
    excRow->addWidget(makeField("Exclude file", m_excludeFileEdit, "exclude.txt"));
    tLayout->addLayout(excRow);

    secTargets->setContentLayout(tLayout);
    m_sections.push_back(secTargets);
    contentLayout->addWidget(secTargets);

    // ----------------- Section 2: Ports -----------------
    auto* secPorts = new CollapsibleSection("Ports & scan types", Icons::Ports, scrollContent, true);
    auto* pLayout = new QVBoxLayout();

    auto* pRow1 = new QHBoxLayout();
    pRow1->setSpacing(8);

    auto* presetBox = new QWidget();
    auto* pl = new QVBoxLayout(presetBox);
    pl->setContentsMargins(0, 0, 0, 0);
    pl->setSpacing(3);
    auto* plbl = new QLabel("Port preset", presetBox);
    plbl->setObjectName("fieldLabel");
    plbl->setProperty("class", "fieldLabel");
    m_portPresetCombo = new FluentComboBox(presetBox);
    m_portPresetCombo->addItems({"Custom", "SSH (22)", "HTTP / HTTPS", "Web + alt", "FTP (21)", "Telnet (23)", "SMTP", "RDP (3389)", "MySQL (3306)", "DNS (UDP 53)"});
    pl->addWidget(plbl);
    pl->addWidget(m_portPresetCombo);

    pRow1->addWidget(presetBox, 1);
    pRow1->addWidget(makeField("Ports (-p)", m_portsEdit, "80,443,U:53,1-1024", "80,443,8000-8100"), 1);
    pLayout->addLayout(pRow1);

    auto* pRow2 = new QHBoxLayout();
    pRow2->setSpacing(8);
    pRow2->addWidget(makeField("Top ports (--top-ports)", m_topPortsEdit, "e.g. 100"));
    pRow2->addWidget(makeField("Exclude ports", m_excludePortsEdit, "25,135-139"));
    pLayout->addLayout(pRow2);

    auto* lblScanTypes = new QLabel("Scan types (none selected → TCP)");
    lblScanTypes->setObjectName("fieldLabel");
    lblScanTypes->setProperty("class", "fieldLabel");
    pLayout->addWidget(lblScanTypes);

    auto* protoGrid = new QGridLayout();
    protoGrid->setSpacing(4);
    m_tcpCheck = new JellyCheckBox("TCP SYN (-sS)");
    m_tcpCheck->setChecked(true);
    m_udpCheck = new JellyCheckBox("UDP (-sU)");
    m_sctpCheck = new JellyCheckBox("SCTP (-sZ)");
    m_oprotoCheck = new JellyCheckBox("Other IP (-sO)");
    m_pingCheck = new JellyCheckBox("ICMP ping");
    m_arpCheck = new JellyCheckBox("ARP");

    protoGrid->addWidget(m_tcpCheck, 0, 0);
    protoGrid->addWidget(m_udpCheck, 0, 1);
    protoGrid->addWidget(m_sctpCheck, 1, 0);
    protoGrid->addWidget(m_oprotoCheck, 1, 1);
    protoGrid->addWidget(m_pingCheck, 2, 0);
    protoGrid->addWidget(m_arpCheck, 2, 1);
    pLayout->addLayout(protoGrid);

    secPorts->setContentLayout(pLayout);
    m_sections.push_back(secPorts);
    contentLayout->addWidget(secPorts);

    // ----------------- Section 3: Rate & Timing -----------------
    auto* secRate = new CollapsibleSection("Rate & timing", Icons::Gauge, scrollContent, true);
    auto* rLayout = new QVBoxLayout();

    auto* rRow1 = new QHBoxLayout();
    rRow1->setSpacing(8);
    rRow1->addWidget(makeField("Rate (pps)", m_rateEdit, "", "2500"));
    rRow1->addWidget(makeField("Retries", m_retriesEdit, "default"));
    rLayout->addLayout(rRow1);

    auto* rRow2 = new QHBoxLayout();
    rRow2->setSpacing(8);
    rRow2->addWidget(makeField("Wait after send (s)", m_waitEdit, "10 (or forever)"));
    rRow2->addWidget(makeField("Seed", m_seedEdit, "number or time"));
    rLayout->addLayout(rRow2);

    auto* rRow3 = new QHBoxLayout();
    rRow3->setSpacing(8);
    rRow3->addWidget(makeField("Shard (n/total)", m_shardEdit, "1/3"));
    rRow3->addWidget(makeField("TTL", m_ttlEdit, "255"));
    rLayout->addLayout(rRow3);

    auto* rRow4 = new QHBoxLayout();
    rRow4->setSpacing(8);
    rRow4->addWidget(makeField("Blackrock rounds", m_blackrockEdit, "default"));
    rRow4->addWidget(makeField("Router IP (--router-ip)", m_rtipEdit, "auto"));
    rLayout->addLayout(rRow4);

    m_offlineCheck = new JellyCheckBox("Offline (don't transmit)");
    m_infiniteCheck = new JellyCheckBox("Loop forever (--infinite)");
    m_sendqCheck = new JellyCheckBox("Fast send queue (--sendq)");
    m_noresetCheck = new JellyCheckBox("No RST (--noreset)");

    rLayout->addWidget(m_offlineCheck);
    rLayout->addWidget(m_infiniteCheck);
    rLayout->addWidget(m_sendqCheck);
    rLayout->addWidget(m_noresetCheck);

    rLayout->addWidget(makeField("Extra args (anything else, typed as-is)", m_extraArgsEdit, "--some-flag value"));

    secRate->setContentLayout(rLayout);
    m_sections.push_back(secRate);
    contentLayout->addWidget(secRate);

    // ----------------- Section 4: Network (collapsed) -----------------
    auto* secNet = new CollapsibleSection("Network", Icons::Network, scrollContent, false);
    auto* nLayout = new QVBoxLayout();

    auto* adaptBox = new QWidget();
    auto* al = new QVBoxLayout(adaptBox);
    al->setContentsMargins(0, 0, 0, 0);
    al->setSpacing(3);
    auto* albl = new QLabel("Adapter (-e)", adaptBox);
    albl->setObjectName("fieldLabel");
    albl->setProperty("class", "fieldLabel");
    m_adapterCombo = new FluentComboBox(adaptBox);
    m_adapterCombo->addItems({"auto", "eth0", "wlan0", "lo"});
    al->addWidget(albl);
    al->addWidget(m_adapterCombo);
    nLayout->addWidget(adaptBox);

    auto* nRow1 = new QHBoxLayout();
    nRow1->setSpacing(8);
    nRow1->addWidget(makeField("Source IP (--adapter-ip)", m_aipEdit, "192.168.1.200"));
    nRow1->addWidget(makeField("Source port", m_aportEdit, "61000"));
    nLayout->addLayout(nRow1);

    auto* nRow2 = new QHBoxLayout();
    nRow2->setSpacing(8);
    nRow2->addWidget(makeField("Source MAC", m_amacEdit, "aa-bb-cc-dd-ee-ff"));
    nRow2->addWidget(makeField("Router MAC", m_rmacEdit, "auto (ARP)"));
    nLayout->addLayout(nRow2);

    auto* nRow3 = new QHBoxLayout();
    nRow3->setSpacing(8);
    nRow3->addWidget(makeField("VLAN", m_vlanEdit, "none"));
    nRow3->addWidget(makeField("Router MAC (IPv6)", m_rmac6Edit, "auto"));
    nLayout->addLayout(nRow3);

    secNet->setContentLayout(nLayout);
    m_sections.push_back(secNet);
    contentLayout->addWidget(secNet);

    // ----------------- Section 5: Banners (collapsed) -----------------
    auto* secBanners = new CollapsibleSection("Banners & content", Icons::Tag, scrollContent, false);
    auto* bLayout = new QVBoxLayout();

    m_bannersCheck = new JellyCheckBox("Grab banners (--banners)");
    bLayout->addWidget(m_bannersCheck);

    m_bannerNoteWidget = new QWidget();
    m_bannerNoteWidget->setObjectName("bannerNoteWidget");
    auto* bnl = new QVBoxLayout(m_bannerNoteWidget);
    bnl->setContentsMargins(8, 8, 8, 8);
    m_bannerNoteLabel = new QLabel(
        "Banner grabbing needs its own source IP, or a firewall rule for the source port. "
        "Otherwise the OS sends RST and kills the connection. Not needed on Windows.",
        m_bannerNoteWidget);
    m_bannerNoteLabel->setWordWrap(true);
    bnl->addWidget(m_bannerNoteLabel);
    m_bannerNoteWidget->setVisible(false);
    bLayout->addWidget(m_bannerNoteWidget);

    auto* lblCapture = new QLabel("Capture (--capture)");
    lblCapture->setObjectName("fieldLabel");
    lblCapture->setProperty("class", "fieldLabel");
    bLayout->addWidget(lblCapture);

    auto* bGrid = new QGridLayout();
    bGrid->setSpacing(4);
    m_capCertCheck = new JellyCheckBox("cert");
    m_capServernameCheck = new JellyCheckBox("servername");
    m_capHtmlCheck = new JellyCheckBox("html");
    m_capHeartbleedCheck = new JellyCheckBox("heartbleed");
    m_capTicketbleedCheck = new JellyCheckBox("ticketbleed");
    bGrid->addWidget(m_capCertCheck, 0, 0);
    bGrid->addWidget(m_capServernameCheck, 0, 1);
    bGrid->addWidget(m_capHtmlCheck, 1, 0);
    bGrid->addWidget(m_capHeartbleedCheck, 1, 1);
    bGrid->addWidget(m_capTicketbleedCheck, 2, 0);
    bLayout->addLayout(bGrid);

    m_rawudpCheck = new JellyCheckBox("Raw UDP banners (--rawudp)");
    bLayout->addWidget(m_rawudpCheck);

    auto makeComboField = [](const QString& label, QComboBox*& combo, const QStringList& items) {
        auto* box = new QWidget();
        auto* l = new QVBoxLayout(box);
        l->setContentsMargins(0, 0, 0, 0);
        l->setSpacing(3);
        auto* lbl = new QLabel(label, box);
        lbl->setProperty("class", "fieldLabel");
        combo = new FluentComboBox(box);
        combo->addItems(items);
        l->addWidget(lbl);
        l->addWidget(combo);
        return box;
    };

    bLayout->addWidget(makeComboField("Hello type (--hello)", m_helloKindCombo, {"none", "http", "ssl", "smbv1"}));
    bLayout->addWidget(makeComboField("Vulnerability check (--vuln)", m_vulnCombo, {"none", "heartbleed", "ticketbleed", "poodle"}));
    bLayout->addWidget(makeField("Don't capture (--nocapture)", m_nocapEdit, "cert"));

    auto* bRow1 = new QHBoxLayout();
    bRow1->setSpacing(8);
    bRow1->addWidget(makeField("Hello string", m_helloEdit, "optional"));
    bRow1->addWidget(makeField("Hello file", m_helloFileEdit, "hello.bin"));
    bLayout->addLayout(bRow1);

    auto* bRow2 = new QHBoxLayout();
    bRow2->setSpacing(8);
    bRow2->addWidget(makeField("Hello timeout (s)", m_helloTimeoutEdit, "default"));
    bRow2->addWidget(makeField("Connection timeout (s)", m_ctimeoutEdit, "30"));
    bLayout->addLayout(bRow2);

    auto* lblHttpReq = new QLabel("HTTP request (used with --banners)");
    lblHttpReq->setObjectName("fieldLabel");
    lblHttpReq->setProperty("class", "fieldLabel");
    bLayout->addWidget(lblHttpReq);

    auto* bRow3 = new QHBoxLayout();
    bRow3->setSpacing(8);
    bRow3->addWidget(makeField("Method", m_httpMethodEdit, "GET"));
    bRow3->addWidget(makeField("Version", m_httpVersionEdit, "HTTP/1.0"));
    bLayout->addLayout(bRow3);

    auto* bRow4 = new QHBoxLayout();
    bRow4->setSpacing(8);
    bRow4->addWidget(makeField("URL", m_httpUrlEdit, "/"));
    bRow4->addWidget(makeField("Host", m_httpHostEdit, "example.com"));
    bLayout->addLayout(bRow4);

    auto* bRow5 = new QHBoxLayout();
    bRow5->setSpacing(8);
    bRow5->addWidget(makeField("User-agent", m_httpUaEdit, "default"));
    bRow5->addWidget(makeField("Cookie", m_httpCookieEdit, "a=b"));
    bLayout->addLayout(bRow5);

    auto* bRow6 = new QHBoxLayout();
    bRow6->setSpacing(8);
    bRow6->addWidget(makeField("Header (Name:value)", m_httpHeaderEdit, "X-Test:1"));
    bRow6->addWidget(makeField("Payload", m_httpPayloadEdit, "body"));
    bLayout->addLayout(bRow6);

    auto* bRow7 = new QHBoxLayout();
    bRow7->setSpacing(8);
    bRow7->addWidget(makeField("nmap payloads file", m_nmapplEdit, "nmap-payloads"));
    bRow7->addWidget(makeField("pcap payloads file", m_pcapplEdit, "payloads.pcap"));
    bLayout->addLayout(bRow7);

    secBanners->setContentLayout(bLayout);
    m_sections.push_back(secBanners);
    contentLayout->addWidget(secBanners);

    // ----------------- Section 6: Output -----------------
    auto* secOut = new CollapsibleSection("Output", Icons::File, scrollContent, true);
    auto* oLayout = new QVBoxLayout();

    auto* fmtBox = new QWidget();
    auto* fl = new QVBoxLayout(fmtBox);
    fl->setContentsMargins(0, 0, 0, 0);
    fl->setSpacing(3);
    auto* flbl = new QLabel("Format", fmtBox);
    flbl->setObjectName("fieldLabel");
    flbl->setProperty("class", "fieldLabel");
    m_fmtCombo = new FluentComboBox(fmtBox);
    m_fmtCombo->addItems({
        "NDJSON (-oD) — best for live", "JSON (-oJ)", "List (-oL)",
        "XML (-oX)", "Grepable (-oG)", "Binary (-oB) — smallest",
        "Unicornscan (-oU)", "Host only (-oH)", "Redis (-oR host:port)"
    });
    fl->addWidget(flbl);
    fl->addWidget(m_fmtCombo);
    oLayout->addWidget(fmtBox);

    oLayout->addWidget(makeField("File name", m_outfileEdit, "", "scan.ndjson"));

    auto* oGrid = new QGridLayout();
    oGrid->setSpacing(4);
    m_openOnlyCheck = new JellyCheckBox("Open only");
    m_openOnlyCheck->setChecked(true);
    m_flushCheck = new JellyCheckBox("Live (--output-flush)");
    m_flushCheck->setChecked(true);
    m_appendCheck = new JellyCheckBox("Append to file");
    m_pktraceCheck = new JellyCheckBox("Packet trace");
    oGrid->addWidget(m_openOnlyCheck, 0, 0);
    oGrid->addWidget(m_flushCheck, 0, 1);
    oGrid->addWidget(m_appendCheck, 1, 0);
    oGrid->addWidget(m_pktraceCheck, 1, 1);
    oLayout->addLayout(oGrid);

    auto* oRow1 = new QHBoxLayout();
    oRow1->setSpacing(8);
    oRow1->addWidget(makeField("Rotate every", m_rotateEdit, "hourly, 10min"));
    oRow1->addWidget(makeField("Rotate dir", m_rotdirEdit, "/var/log/masscan"));
    oLayout->addLayout(oRow1);

    oLayout->addWidget(makeField("Save received packets (--pcap)", m_pcapEdit, "capture.pcap"));

    auto* oRow2 = new QHBoxLayout();
    oRow2->setSpacing(8);
    oRow2->addWidget(makeField("Rotate offset", m_rotOffsetEdit, "e.g. 1h"));
    oRow2->addWidget(makeField("Rotate size", m_rotSizeEdit, "e.g. 100mb"));
    oLayout->addLayout(oRow2);

    auto* oRow3 = new QHBoxLayout();
    oRow3->setSpacing(8);
    oRow3->addWidget(makeField("Show (--show)", m_showEdit, "open,closed,host,all"));
    oRow3->addWidget(makeField("Hide (--noshow)", m_noshowEdit, "closed"));
    oLayout->addLayout(oRow3);

    oLayout->addWidget(makeField("XML stylesheet", m_stylesheetEdit, "nmap.xsl"));

    auto* oGrid2 = new QGridLayout();
    oGrid2->setSpacing(4);
    m_noStylesheetCheck = new JellyCheckBox("No stylesheet");
    m_reasonCheck = new JellyCheckBox("Reason (--reason)");
    m_interactiveCheck = new JellyCheckBox("Interactive");
    m_nostatusCheck = new JellyCheckBox("Hide status line");
    m_ndjsonStatusCheck = new JellyCheckBox("NDJSON status");
    m_jsonStatusCheck = new JellyCheckBox("JSON status");
    oGrid2->addWidget(m_noStylesheetCheck, 0, 0);
    oGrid2->addWidget(m_reasonCheck, 0, 1);
    oGrid2->addWidget(m_interactiveCheck, 1, 0);
    oGrid2->addWidget(m_nostatusCheck, 1, 1);
    oGrid2->addWidget(m_ndjsonStatusCheck, 2, 0);
    oGrid2->addWidget(m_jsonStatusCheck, 2, 1);
    oLayout->addLayout(oGrid2);

    oLayout->addWidget(makeComboField("Verbosity", m_verboseCombo, {"normal", "-v", "-vv"}));

    secOut->setContentLayout(oLayout);
    m_sections.push_back(secOut);
    contentLayout->addWidget(secOut);

    // ----------------- Section 7: Mode (collapsed) -----------------
    auto* secMode = new CollapsibleSection("Mode", Icons::Mode, scrollContent, false);
    auto* mLayout = new QVBoxLayout();
    mLayout->addWidget(makeComboField("What to run", m_modeCombo, {
        "Scan", "List targets only (-sL)", "Show config (--echo)",
        "List adapters (--iflist)", "Self-test (--selftest)"
    }));
    secMode->setContentLayout(mLayout);
    m_sections.push_back(secMode);
    contentLayout->addWidget(secMode);

    // ----------------- Section 8: Paths (collapsed) -----------------
    auto* secPaths = new CollapsibleSection("Paths", Icons::Paths, scrollContent, false);
    auto* pathsLayout = new QVBoxLayout();
    pathsLayout->addWidget(makeField("Output folder", m_outdirEdit, "", "MassGUI"));
    pathsLayout->addWidget(makeField("masscan binary", m_binEdit, "", "masscan"));
    pathsLayout->addWidget(makeField("Config file (-c)", m_conffileEdit, "masscan.conf"));
    pathsLayout->addWidget(makeField("Data dir (--datadir)", m_datadirEdit, "optional"));
    pathsLayout->addWidget(makeField("Service probes (--nmap-service-probes)", m_probesEdit, "nmap-service-probes"));
    pathsLayout->addWidget(makeField("Script (--script)", m_scriptEdit, "optional"));
    secPaths->setContentLayout(pathsLayout);
    m_sections.push_back(secPaths);
    contentLayout->addWidget(secPaths);

    // ----------------- Section 9: Advanced (collapsed) -----------------
    auto* secAdv = new CollapsibleSection("Advanced packet options", Icons::Sliders, scrollContent, false);
    auto* advLayout = new QVBoxLayout();

    auto* advRow1 = new QHBoxLayout();
    advRow1->setSpacing(8);
    advRow1->addWidget(makeField("TCP MSS", m_mssEdit, "default"));
    advRow1->addWidget(makeField("Min packet size", m_minpktEdit, "60"));
    advLayout->addLayout(advRow1);

    advLayout->addWidget(makeField("BPF filter (--bpf)", m_bpfEdit, "optional"));

    auto* advRow2 = new QHBoxLayout();
    advRow2->setSpacing(8);
    advRow2->addWidget(makeField("Data length (<1500)", m_datalenEdit, "0"));
    advRow2->addWidget(makeField("TCP window scale", m_wscaleEdit, "default"));
    advLayout->addLayout(advRow2);

    auto* advRow3 = new QHBoxLayout();
    advRow3->setSpacing(8);
    advRow3->addWidget(makeField("TCP timestamp echo", m_tsechoEdit, "default"));
    m_sackokCheck = new JellyCheckBox("TCP SACK OK");
    advRow3->addWidget(m_sackokCheck);
    advLayout->addLayout(advRow3);

    m_badsumCheck = new JellyCheckBox("Bad checksum (--badsum)");
    advLayout->addWidget(m_badsumCheck);

    secAdv->setContentLayout(advLayout);
    m_sections.push_back(secAdv);
    contentLayout->addWidget(secAdv);

    contentLayout->addStretch(1);
    scroll->setWidget(scrollContent);
    sidebarLayout->addWidget(scroll, 1);

    // ----------------- Fixed Bottom ActionBar (52px) -----------------
    auto* actionBar = new QWidget(m_sidebarCard);
    actionBar->setObjectName("actionBar");
    actionBar->setFixedHeight(52);

    auto* actLayout = new QHBoxLayout(actionBar);
    // Align side margins (10px) with scrollContent sections above
    actLayout->setContentsMargins(10, 10, 10, 10);
    actLayout->setSpacing(8);

    m_btnStart = new GlitchButton("Start Scan", actionBar);
    m_btnStart->setProperty("class", "primary");
    m_btnStart->setFixedHeight(32);

    m_btnStop = new QPushButton(actionBar);
    m_btnStop->setProperty("compact", true);
    m_btnStop->setToolTip("Stop");
    m_btnStop->setFixedSize(32, 32);
    m_btnStop->setEnabled(false);

    m_btnResume = new QPushButton(actionBar);
    m_btnResume->setProperty("compact", true);
    m_btnResume->setToolTip("Resume");
    m_btnResume->setFixedSize(32, 32);
    m_btnResume->setEnabled(false);

    m_btnOpenFolder = new QPushButton(actionBar);
    m_btnOpenFolder->setProperty("compact", true);
    m_btnOpenFolder->setToolTip("Open Folder");
    m_btnOpenFolder->setFixedSize(32, 32);

    actLayout->addWidget(m_btnStart, 1);
    actLayout->addWidget(m_btnStop);
    actLayout->addWidget(m_btnResume);
    actLayout->addWidget(m_btnOpenFolder);

    sidebarLayout->addWidget(actionBar);

    // Events
    connect(m_countrySearchEdit, &QLineEdit::textChanged, this, &MainWindow::onSearchCountries);
    connect(m_countryListWidget, &QListWidget::itemClicked, this, &MainWindow::onToggleCountrySelection);
    connect(m_btnAddCountries, &QPushButton::clicked, this, &MainWindow::onAddSelectedCountries);
    connect(m_btnAddManual, &QPushButton::clicked, this, &MainWindow::onAddManualTarget);
    connect(m_manualTargetEdit, &QLineEdit::returnPressed, this, &MainWindow::onAddManualTarget);
    connect(m_btnEditTarget, &QPushButton::clicked, this, &MainWindow::onEditSelectedTarget);
    connect(m_btnRemoveTarget, &QPushButton::clicked, this, &MainWindow::onRemoveSelectedTarget);
    connect(m_btnClearTargets, &QPushButton::clicked, this, &MainWindow::onClearAllTargets);
    connect(m_btnImportTargets, &QPushButton::clicked, this, &MainWindow::onImportTargets);
    connect(m_portPresetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onPortPresetSelected);

    connect(m_btnStart, &QPushButton::clicked, this, &MainWindow::onStartScan);
    connect(m_btnStop, &QPushButton::clicked, this, &MainWindow::onStopScan);
    connect(m_btnResume, &QPushButton::clicked, this, &MainWindow::onResumeScan);
    connect(m_btnOpenFolder, &QPushButton::clicked, this, &MainWindow::onOpenOutputFolder);

    // Output format selection updates default file name (matching Slint OutputSection)
    connect(m_fmtCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
        static const char* defaultFiles[] = {
            "scan.ndjson", "scan.json", "scan.txt", "scan.xml",
            "scan.gnmap", "scan.scan", "scan.txt", "scan.txt", "127.0.0.1:6379"
        };
        if (idx >= 0 && idx < 9 && m_outfileEdit) {
            m_outfileEdit->setText(defaultFiles[idx]);
        }
    });

    // Auto-update command & validation on any config input change in the sidebar
    for (auto* edit : scrollContent->findChildren<QLineEdit*>()) {
        if (edit != m_countrySearchEdit && edit != m_manualTargetEdit) {
            connect(edit, &QLineEdit::textChanged, this, [this, edit]() {
                m_resumeMode = false;
                if (edit == m_binEdit) checkDependencies();
                updateCommandAndValidation();
            });
        }
    }
    for (auto* chk : scrollContent->findChildren<QCheckBox*>()) {
        connect(chk, &QCheckBox::toggled, this, [this]() {
            m_resumeMode = false;
            updateCommandAndValidation();
        });
    }
    for (auto* cmb : scrollContent->findChildren<QComboBox*>()) {
        if (cmb != m_portPresetCombo) {
            connect(cmb, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
                m_resumeMode = false;
                updateCommandAndValidation();
            });
        }
    }

    return m_sidebarCard;
}

QWidget* MainWindow::createRightColumn() {
    auto* right = new QWidget(this);
    right->setObjectName("rightColumn");
    auto* layout = new QVBoxLayout(right);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    // 0. Narrow-mode Action Bar Card (stays reachable while drawer is closed, matching Slint)
    m_narrowActionCard = new CardWidget(right, 8);
    m_narrowActionCard->setObjectName("narrowActionCard");
    m_narrowActionCard->setFixedHeight(52);
    m_narrowActionCard->setDark(m_isDarkTheme);
    m_narrowActionCard->setVisible(false);

    auto* nActLayout = new QHBoxLayout(m_narrowActionCard);
    nActLayout->setContentsMargins(10, 10, 10, 10);
    nActLayout->setSpacing(8);

    m_btnStartNarrow = new GlitchButton("Start Scan", m_narrowActionCard);
    m_btnStartNarrow->setProperty("class", "primary");
    m_btnStartNarrow->setFixedHeight(32);

    m_btnStopNarrow = new QPushButton(m_narrowActionCard);
    m_btnStopNarrow->setProperty("compact", true);
    m_btnStopNarrow->setToolTip("Stop");
    m_btnStopNarrow->setFixedSize(32, 32);
    m_btnStopNarrow->setEnabled(false);

    m_btnResumeNarrow = new QPushButton(m_narrowActionCard);
    m_btnResumeNarrow->setProperty("compact", true);
    m_btnResumeNarrow->setToolTip("Resume");
    m_btnResumeNarrow->setFixedSize(32, 32);
    m_btnResumeNarrow->setEnabled(false);

    m_btnOpenFolderNarrow = new QPushButton(m_narrowActionCard);
    m_btnOpenFolderNarrow->setProperty("compact", true);
    m_btnOpenFolderNarrow->setToolTip("Open Folder");
    m_btnOpenFolderNarrow->setFixedSize(32, 32);

    nActLayout->addWidget(m_btnStartNarrow, 1);
    nActLayout->addWidget(m_btnStopNarrow);
    nActLayout->addWidget(m_btnResumeNarrow);
    nActLayout->addWidget(m_btnOpenFolderNarrow);

    connect(m_btnStartNarrow, &QPushButton::clicked, this, &MainWindow::onStartScan);
    connect(m_btnStopNarrow, &QPushButton::clicked, this, &MainWindow::onStopScan);
    connect(m_btnResumeNarrow, &QPushButton::clicked, this, &MainWindow::onResumeScan);
    connect(m_btnOpenFolderNarrow, &QPushButton::clicked, this, &MainWindow::onOpenOutputFolder);

    layout->addWidget(m_narrowActionCard);

    // 1. Command Preview Card
    m_cmdCard = new CardWidget(right, 8);
    m_cmdCard->setObjectName("cmdCard");
    m_cmdCard->setDark(m_isDarkTheme);

    auto* cl = new QVBoxLayout(m_cmdCard);
    cl->setContentsMargins(10, 10, 10, 10);
    cl->setSpacing(6);

    auto* cmdScroll = new QScrollArea(m_cmdCard);
    cmdScroll->setFrameShape(QFrame::NoFrame);
    cmdScroll->setWidgetResizable(true);
    cmdScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    cmdScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_cmdScrollOverlay = new FluentScrollBarOverlay(cmdScroll);
    m_cmdScrollOverlay->setDark(m_isDarkTheme);

    auto* cmdScrollContent = new QWidget(cmdScroll);
    cmdScrollContent->setObjectName("cmdScrollContent");
    cmdScrollContent->setAttribute(Qt::WA_StyledBackground, false);
    auto* csl = new QVBoxLayout(cmdScrollContent);
    csl->setContentsMargins(0, 0, 0, 0);
    csl->setSpacing(6);

    m_terminalBox = new TerminalCodeBox(cmdScrollContent);
    m_terminalBox->setDark(m_isDarkTheme);
    m_commandTextLabel = m_terminalBox->commandLabel();
    m_btnCopy = m_terminalBox->copyButton();
    connect(m_btnCopy, &QAbstractButton::clicked, this, &MainWindow::onCopyCommand);
    csl->addWidget(m_terminalBox);

    m_warningsContainer = new QWidget(cmdScrollContent);
    m_warningsLayout = new QVBoxLayout(m_warningsContainer);
    m_warningsLayout->setContentsMargins(0, 0, 0, 0);
    m_warningsLayout->setSpacing(6);
    csl->addWidget(m_warningsContainer);

    cmdScroll->setWidget(cmdScrollContent);
    cl->addWidget(cmdScroll, 1);

    layout->addWidget(m_cmdCard);

    // 2. Results & Log Card (Stretch 1)
    m_resultsCard = new CardWidget(right, 8);
    m_resultsCard->setObjectName("resultsCard");
    m_resultsCard->setDark(m_isDarkTheme);
    auto* rl = new QVBoxLayout(m_resultsCard);
    rl->setContentsMargins(0, 0, 0, 0);
    rl->setSpacing(0);

    // Tab Header (Pills)
    auto* tabHeader = new QWidget(m_resultsCard);
    auto* thLayout = new QHBoxLayout(tabHeader);
    thLayout->setContentsMargins(8, 6, 8, 0);
    thLayout->setSpacing(4);

    m_pillResults = new QPushButton("Results", tabHeader);
    m_pillResults->setProperty("class", "pillActive");
    m_pillResults->setFixedWidth(100);

    m_pillLog = new QPushButton("Log", tabHeader);
    m_pillLog->setProperty("class", "pillInactive");
    m_pillLog->setFixedWidth(80);

    thLayout->addWidget(m_pillResults);
    thLayout->addWidget(m_pillLog);
    thLayout->addStretch(1);
    rl->addWidget(tabHeader);

    auto* divLine = new QFrame(m_resultsCard);
    divLine->setFixedHeight(1);
    divLine->setStyleSheet(m_isDarkTheme ? "background-color: #2a3340;" : "background-color: #d3dae4;");
    rl->addWidget(divLine);

    // Tab 0: Results
    m_tabResultsWidget = new QWidget(m_resultsCard);
    auto* rwl = new QVBoxLayout(m_tabResultsWidget);
    rwl->setContentsMargins(10, 10, 10, 10);
    rwl->setSpacing(8);

    auto* filterRow = new QHBoxLayout();
    filterRow->setSpacing(8);
    m_filterResultsEdit = new QLineEdit(m_tabResultsWidget);
    m_filterResultsEdit->setPlaceholderText("Filter by IP, port, service…");
    m_filterResultsAction = m_filterResultsEdit->addAction(
        Icons::get(Icons::Search, m_isDarkTheme ? QColor("#8b97a8") : QColor("#5b6a7e"), 16),
        QLineEdit::LeadingPosition);
    connect(m_filterResultsAction, &QAction::triggered, m_filterResultsEdit, qOverload<>(&QLineEdit::setFocus));

    m_btnExport = new QPushButton("Export", m_tabResultsWidget);
    m_btnExport->setFixedHeight(32);
    m_btnClear = new QPushButton("Clear", m_tabResultsWidget);
    m_btnClear->setFixedHeight(32);

    connect(m_btnExport, &QPushButton::clicked, this, &MainWindow::onExportResults);
    connect(m_btnClear, &QPushButton::clicked, this, &MainWindow::onClearResults);
    connect(m_filterResultsEdit, &QLineEdit::textChanged, this, [this](const QString& q) {
        QString lowerQ = q.trimmed().toLower();
        for (int r = 0; r < m_resultsTable->rowCount(); ++r) {
            bool match = false;
            for (int c = 0; c < m_resultsTable->columnCount(); ++c) {
                auto* it = m_resultsTable->item(r, c);
                if (it && it->text().toLower().contains(lowerQ)) {
                    match = true;
                    break;
                }
            }
            m_resultsTable->setRowHidden(r, !match && !lowerQ.isEmpty());
        }
    });

    filterRow->addWidget(m_filterResultsEdit, 1);
    filterRow->addWidget(m_btnExport);
    filterRow->addWidget(m_btnClear);
    rwl->addLayout(filterRow);

    m_resultsTable = new QTableWidget(m_tabResultsWidget);
    m_resultsTable->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_resultsTable->setColumnCount(6);
    m_resultsTable->setHorizontalHeaderLabels({"IP", "Port", "Proto", "Service", "Status", "Time"});
    m_resultsTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_resultsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_resultsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_resultsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_resultsTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_resultsTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_resultsTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_resultsTable->verticalHeader()->setVisible(false);
    m_resultsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableScrollOverlay = new FluentScrollBarOverlay(m_resultsTable);
    m_tableScrollOverlay->setDark(m_isDarkTheme);
    rwl->addWidget(m_resultsTable);

    rl->addWidget(m_tabResultsWidget, 1);

    // Tab 1: Log
    m_tabLogWidget = new QWidget(m_resultsCard);
    auto* lwl = new QVBoxLayout(m_tabLogWidget);
    lwl->setContentsMargins(8, 8, 8, 8);
    lwl->setSpacing(6);

    m_logEdit = new QPlainTextEdit(m_tabLogWidget);
    m_logEdit->setReadOnly(true);
    m_logEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_logScrollOverlay = new FluentScrollBarOverlay(m_logEdit);
    m_logScrollOverlay->setDark(m_isDarkTheme);
    lwl->addWidget(m_logEdit);

    m_tabLogWidget->setVisible(false);
    rl->addWidget(m_tabLogWidget, 1);

    connect(m_pillResults, &QPushButton::clicked, this, [this]() { onTabSwitched(0); });
    connect(m_pillLog, &QPushButton::clicked, this, [this]() { onTabSwitched(1); });

    layout->addWidget(m_resultsCard, 1);

    return right;
}

void MainWindow::onTabSwitched(int tabIndex) {
    m_currentTab = tabIndex;
    QColor accentCol = m_isDarkTheme ? QColor("#3b9eff") : QColor("#1f73d6");
    QColor mutedCol  = m_isDarkTheme ? QColor("#8b97a8") : QColor("#5b6a7e");
    if (tabIndex == 0) {
        m_tabResultsWidget->setVisible(true);
        m_tabLogWidget->setVisible(false);
        m_pillResults->setProperty("class", "pillActive");
        m_pillLog->setProperty("class", "pillInactive");
        m_pillResults->setIcon(Icons::get(Icons::List, accentCol, 14));
        m_pillLog->setIcon(Icons::get(Icons::Terminal, mutedCol, 14));
    } else {
        m_tabResultsWidget->setVisible(false);
        m_tabLogWidget->setVisible(true);
        m_pillResults->setProperty("class", "pillInactive");
        m_pillLog->setProperty("class", "pillActive");
        m_pillResults->setIcon(Icons::get(Icons::List, mutedCol, 14));
        m_pillLog->setIcon(Icons::get(Icons::Terminal, accentCol, 14));
    }

    // Refresh styling on tabs
    m_pillResults->style()->unpolish(m_pillResults);
    m_pillResults->style()->polish(m_pillResults);
    m_pillLog->style()->unpolish(m_pillLog);
    m_pillLog->style()->polish(m_pillLog);
}

void MainWindow::populateCountryList(const QString& query) {
    m_countryListWidget->clear();
    QString q = query.trimmed().toLower();

    for (const auto& c : geo::countries()) {
        QString name = QString::fromStdString(c.name);
        QString code = QString::fromStdString(c.code);
        QString codeLower = code.toLower();

        if (!q.isEmpty()) {
            if (!name.toLower().contains(q) && !codeLower.contains(q)) {
                continue;
            }
        }

        auto* item = new QListWidgetItem(m_countryListWidget);
        item->setData(RoleCode, code);
        item->setData(RoleLabel, QString("%1 (%2)").arg(name, code.toUpper()));
        item->setData(RoleInfo, QString::fromUtf8("%1 ranges · %2 IPs")
            .arg(QString::fromStdString(thousands(c.ranges)))
            .arg(QString::fromStdString(geo::human_number(c.addresses))));
        item->setSizeHint(QSize(0, 40));
    }
}

void MainWindow::onSearchCountries(const QString& q) {
    populateCountryList(q);
}

void MainWindow::onToggleCountrySelection(QListWidgetItem* item) {
    if (!item) return;
    std::string code = item->data(RoleCode).toString().toStdString();
    if (m_pickedCountryCodes.find(code) != m_pickedCountryCodes.end()) {
        m_pickedCountryCodes.erase(code);
    } else {
        m_pickedCountryCodes.insert(code);
    }

    size_t count = m_pickedCountryCodes.size();
    if (count > 0) {
        m_btnAddCountries->setEnabled(true);
        m_btnAddCountries->setText(QString("Add %1 selected to Targets").arg(count));
    } else {
        m_btnAddCountries->setEnabled(false);
        m_btnAddCountries->setText("Add selected to Targets");
    }

    m_countryListWidget->viewport()->update();
}

void MainWindow::onAddSelectedCountries() {
    for (const auto& code : m_pickedCountryCodes) {
        if (const auto* c = geo::find(code)) {
            bool exists = false;
            for (const auto& t : m_targets) {
                if (t.isCountry && t.code.toStdString() == code) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                TargetItem t;
                t.text = QString("%1 (%2)").arg(QString::fromStdString(c->name), QString::fromStdString(c->code).toUpper());
                t.code = QString::fromStdString(c->code);
                t.info = QString::fromUtf8("%1 ranges · %2 IPs")
                    .arg(QString::fromStdString(thousands(c->ranges)))
                    .arg(QString::fromStdString(geo::human_number(c->addresses)));
                t.isCountry = true;
                m_targets.push_back(t);
            }
        }
    }
    m_pickedCountryCodes.clear();
    m_btnAddCountries->setEnabled(false);
    m_btnAddCountries->setText("Add selected to Targets");
    m_countryListWidget->viewport()->update();
    m_resumeMode = false;
    updateCommandAndValidation();
}

void MainWindow::onAddManualTarget() {
    QString manual = m_manualTargetEdit->text().trimmed();
    if (manual.isEmpty()) return;

    const QStringList parts = manual.split(QRegularExpression("[\\s,]+"), Qt::SkipEmptyParts);
    for (const QString& part : parts) {
        TargetItem t;
        t.text = part;
        t.code = "";
        t.isCountry = false;

        if (auto sz = target_size(part.toStdString())) {
            t.info = QString::fromStdString(thousands(*sz)) + " IP" + (*sz == 1 ? "" : "s");
        } else {
            t.info = "custom";
        }
        m_targets.push_back(t);
    }

    m_manualTargetEdit->clear();
    m_resumeMode = false;
    updateCommandAndValidation();
}

void MainWindow::onEditSelectedTarget() {
    int row = m_targetsListWidget->currentRow();
    if (row >= 0 && static_cast<size_t>(row) < m_targets.size()) {
        auto& t = m_targets[row];
        if (t.isCountry) return;
        m_manualTargetEdit->setText(t.text);
        m_manualTargetEdit->setFocus();
        m_targets.erase(m_targets.begin() + row);
        m_resumeMode = false;
        updateCommandAndValidation();
    }
}

void MainWindow::onRemoveSelectedTarget() {
    int row = m_targetsListWidget->currentRow();
    if (row >= 0 && static_cast<size_t>(row) < m_targets.size()) {
        const auto& t = m_targets[row];
        if (t.isCountry) {
            m_pickedCountryCodes.erase(t.code.toStdString());
            populateCountryList();
        }
        m_targets.erase(m_targets.begin() + row);
        m_resumeMode = false;
        updateCommandAndValidation();
    }
}

void MainWindow::onClearAllTargets() {
    m_targets.clear();
    m_pickedCountryCodes.clear();
    populateCountryList();
    m_resumeMode = false;
    updateCommandAndValidation();
}

void MainWindow::onImportTargets() {
    QString path = QFileDialog::getOpenFileName(this, "Import Target List", "", "Text Files (*.txt);;All Files (*.*)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (!line.isEmpty() && !line.startsWith("#")) {
                TargetItem t;
                t.text = line;
                t.isCountry = false;
                if (auto sz = target_size(line.toStdString())) {
                    t.info = QString::fromStdString(thousands(*sz)) + " IP" + (*sz == 1 ? "" : "s");
                } else {
                    t.info = "custom";
                }
                m_targets.push_back(t);
            }
        }
        m_resumeMode = false;
        updateCommandAndValidation();
    }
}

void MainWindow::onPortPresetSelected(int index) {
    QString text = m_portPresetCombo->itemText(index);
    QString ports;
    if (text == "SSH (22)") {
        ports = "22";
    } else if (text == "HTTP / HTTPS") {
        ports = "80,443";
    } else if (text == "Web + alt") {
        ports = "80,443,8080,8443";
    } else if (text == "FTP (21)") {
        ports = "21";
    } else if (text == "Telnet (23)") {
        ports = "23";
    } else if (text == "SMTP") {
        ports = "25,465,587";
    } else if (text == "RDP (3389)") {
        ports = "3389";
    } else if (text == "MySQL (3306)") {
        ports = "3306";
    } else if (text == "DNS (UDP 53)") {
        ports = "U:53";
    } else {
        return;
    }
    m_portsEdit->setText(ports);
    if (m_topPortsEdit) m_topPortsEdit->clear();
    if (m_udpCheck) m_udpCheck->setChecked(ports.startsWith("U:"));
    m_resumeMode = false;
    updateCommandAndValidation();
}

Snapshot MainWindow::getSnapshot() const {
    Snapshot s;
    s.ports = m_portsEdit ? m_portsEdit->text().trimmed().toStdString() : "80,443";
    s.top = m_topPortsEdit ? m_topPortsEdit->text().trimmed().toStdString() : "";
    s.exports = m_excludePortsEdit ? m_excludePortsEdit->text().trimmed().toStdString() : "";
    s.rate = m_rateEdit ? m_rateEdit->text().trimmed().toStdString() : "2500";
    s.retries = m_retriesEdit ? m_retriesEdit->text().trimmed().toStdString() : "";
    s.wait = m_waitEdit ? m_waitEdit->text().trimmed().toStdString() : "";
    s.seed = m_seedEdit ? m_seedEdit->text().trimmed().toStdString() : "";
    s.shard = m_shardEdit ? m_shardEdit->text().trimmed().toStdString() : "";
    s.ttl = m_ttlEdit ? m_ttlEdit->text().trimmed().toStdString() : "";
    s.blackrock = m_blackrockEdit ? m_blackrockEdit->text().trimmed().toStdString() : "";
    s.rtip = m_rtipEdit ? m_rtipEdit->text().trimmed().toStdString() : "";

    s.offline = m_offlineCheck && m_offlineCheck->isChecked();
    s.infinite = m_infiniteCheck && m_infiniteCheck->isChecked();
    s.sendq = m_sendqCheck && m_sendqCheck->isChecked();
    s.noreset = m_noresetCheck && m_noresetCheck->isChecked();
    s.extra = m_extraArgsEdit ? m_extraArgsEdit->text().trimmed().toStdString() : "";

    s.tcp = m_tcpCheck && m_tcpCheck->isChecked();
    s.udp = m_udpCheck && m_udpCheck->isChecked();
    s.sctp = m_sctpCheck && m_sctpCheck->isChecked();
    s.oproto = m_oprotoCheck && m_oprotoCheck->isChecked();
    s.ping = m_pingCheck && m_pingCheck->isChecked();
    s.arp = m_arpCheck && m_arpCheck->isChecked();

    s.adapter = m_adapterCombo && m_adapterCombo->currentText() != "auto" ? m_adapterCombo->currentText().toStdString() : "";
    s.aip = m_aipEdit ? m_aipEdit->text().trimmed().toStdString() : "";
    s.aport = m_aportEdit ? m_aportEdit->text().trimmed().toStdString() : "";
    s.amac = m_amacEdit ? m_amacEdit->text().trimmed().toStdString() : "";
    s.rmac = m_rmacEdit ? m_rmacEdit->text().trimmed().toStdString() : "";
    s.vlan = m_vlanEdit ? m_vlanEdit->text().trimmed().toStdString() : "";
    s.rmac6 = m_rmac6Edit ? m_rmac6Edit->text().trimmed().toStdString() : "";

    s.banners = m_bannersCheck && m_bannersCheck->isChecked();
    s.cap_cert = m_capCertCheck && m_capCertCheck->isChecked();
    s.cap_servername = m_capServernameCheck && m_capServernameCheck->isChecked();
    s.cap_html = m_capHtmlCheck && m_capHtmlCheck->isChecked();
    s.cap_heartbleed = m_capHeartbleedCheck && m_capHeartbleedCheck->isChecked();
    s.cap_ticketbleed = m_capTicketbleedCheck && m_capTicketbleedCheck->isChecked();
    s.rawudp = m_rawudpCheck && m_rawudpCheck->isChecked();

    s.hello_kind = m_helloKindCombo && m_helloKindCombo->currentText() != "none" ? m_helloKindCombo->currentText().toStdString() : "";
    s.vuln = m_vulnCombo && m_vulnCombo->currentText() != "none" ? m_vulnCombo->currentText().toStdString() : "";
    s.nocap = m_nocapEdit ? m_nocapEdit->text().trimmed().toStdString() : "";
    s.hello = m_helloEdit ? m_helloEdit->text().trimmed().toStdString() : "";
    s.hello_file = m_helloFileEdit ? m_helloFileEdit->text().trimmed().toStdString() : "";
    s.hello_timeout = m_helloTimeoutEdit ? m_helloTimeoutEdit->text().trimmed().toStdString() : "";
    s.ctimeout = m_ctimeoutEdit ? m_ctimeoutEdit->text().trimmed().toStdString() : "";

    s.http_method = m_httpMethodEdit ? m_httpMethodEdit->text().trimmed().toStdString() : "";
    s.http_version = m_httpVersionEdit ? m_httpVersionEdit->text().trimmed().toStdString() : "";
    s.http_url = m_httpUrlEdit ? m_httpUrlEdit->text().trimmed().toStdString() : "";
    s.http_host = m_httpHostEdit ? m_httpHostEdit->text().trimmed().toStdString() : "";
    s.ua = m_httpUaEdit ? m_httpUaEdit->text().trimmed().toStdString() : "";
    s.http_cookie = m_httpCookieEdit ? m_httpCookieEdit->text().trimmed().toStdString() : "";
    s.http_header = m_httpHeaderEdit ? m_httpHeaderEdit->text().trimmed().toStdString() : "";
    s.http_payload = m_httpPayloadEdit ? m_httpPayloadEdit->text().trimmed().toStdString() : "";
    s.nmappl = m_nmapplEdit ? m_nmapplEdit->text().trimmed().toStdString() : "";
    s.pcappl = m_pcapplEdit ? m_pcapplEdit->text().trimmed().toStdString() : "";

    int fmtIdx = m_fmtCombo ? m_fmtCombo->currentIndex() : 0;
    static const char* fmts[] = {"D", "J", "L", "X", "G", "B", "U", "H", "R"};
    s.fmt = fmts[fmtIdx < 9 ? fmtIdx : 0];
    s.outfile = m_outfileEdit ? m_outfileEdit->text().trimmed().toStdString() : "scan.ndjson";

    s.open_only = m_openOnlyCheck && m_openOnlyCheck->isChecked();
    s.flush = m_flushCheck && m_flushCheck->isChecked();
    s.append = m_appendCheck && m_appendCheck->isChecked();
    s.pktrace = m_pktraceCheck && m_pktraceCheck->isChecked();

    s.rotate = m_rotateEdit ? m_rotateEdit->text().trimmed().toStdString() : "";
    s.rotdir = m_rotdirEdit ? m_rotdirEdit->text().trimmed().toStdString() : "";
    s.pcap = m_pcapEdit ? m_pcapEdit->text().trimmed().toStdString() : "";
    s.rot_offset = m_rotOffsetEdit ? m_rotOffsetEdit->text().trimmed().toStdString() : "";
    s.rot_size = m_rotSizeEdit ? m_rotSizeEdit->text().trimmed().toStdString() : "";
    s.show = m_showEdit ? m_showEdit->text().trimmed().toStdString() : "";
    s.noshow = m_noshowEdit ? m_noshowEdit->text().trimmed().toStdString() : "";
    s.stylesheet = m_stylesheetEdit ? m_stylesheetEdit->text().trimmed().toStdString() : "";
    s.no_stylesheet = m_noStylesheetCheck && m_noStylesheetCheck->isChecked();
    s.reason = m_reasonCheck && m_reasonCheck->isChecked();
    s.interactive = m_interactiveCheck && m_interactiveCheck->isChecked();
    s.nostatus = m_nostatusCheck && m_nostatusCheck->isChecked();
    s.ndjson_status = m_ndjsonStatusCheck && m_ndjsonStatusCheck->isChecked();
    s.json_status = m_jsonStatusCheck && m_jsonStatusCheck->isChecked();
    s.verbose = m_verboseCombo ? m_verboseCombo->currentIndex() : 0;

    s.mode = m_modeCombo ? m_modeCombo->currentIndex() : 0;
    s.outdir = m_outdirEdit ? m_outdirEdit->text().trimmed().toStdString() : "MassGUI";
    s.bin = m_binEdit ? m_binEdit->text().trimmed().toStdString() : "masscan";
    s.conffile = m_conffileEdit ? m_conffileEdit->text().trimmed().toStdString() : "";
    s.datadir = m_datadirEdit ? m_datadirEdit->text().trimmed().toStdString() : "";
    s.probes = m_probesEdit ? m_probesEdit->text().trimmed().toStdString() : "";
    s.script = m_scriptEdit ? m_scriptEdit->text().trimmed().toStdString() : "";

    s.mss = m_mssEdit ? m_mssEdit->text().trimmed().toStdString() : "";
    s.minpkt = m_minpktEdit ? m_minpktEdit->text().trimmed().toStdString() : "";
    s.bpf = m_bpfEdit ? m_bpfEdit->text().trimmed().toStdString() : "";
    s.datalen = m_datalenEdit ? m_datalenEdit->text().trimmed().toStdString() : "";
    s.wscale = m_wscaleEdit ? m_wscaleEdit->text().trimmed().toStdString() : "";
    s.tsecho = m_tsechoEdit ? m_tsechoEdit->text().trimmed().toStdString() : "";
    s.sackok = m_sackokCheck && m_sackokCheck->isChecked();
    s.badsum = m_badsumCheck && m_badsumCheck->isChecked();

    s.exclude = m_excludeEdit ? m_excludeEdit->text().trimmed().toStdString() : "";
    s.excludefile = m_excludeFileEdit ? m_excludeFileEdit->text().trimmed().toStdString() : "";
    s.all_ipv4 = m_allIpv4Check && m_allIpv4Check->isChecked();

    return s;
}

void MainWindow::updateCommandAndValidation() {
    Snapshot s = getSnapshot();

    std::vector<std::string> manualTargets;
    std::vector<std::string> countryCodes;
    uint64_t totalIps = 0;
    bool exactIps = true;

    m_targetsListWidget->clear();

    for (const auto& t : m_targets) {
        auto* item = new QListWidgetItem(m_targetsListWidget);
        item->setSizeHint(QSize(0, 26));
        item->setData(RoleLabel, t.text);
        item->setData(RoleInfo, t.info);

        if (t.isCountry) {
            std::string code = t.code.toStdString();
            countryCodes.push_back(code);
            if (const auto* c = geo::find(code)) {
                totalIps += c->addresses;
            }
        } else {
            manualTargets.push_back(t.text.toStdString());
            if (auto sz = target_size(t.text.toStdString())) {
                totalIps += *sz;
            } else {
                exactIps = false;
            }
        }
    }

    size_t count = m_targets.size();
    if (count == 0) {
        m_targetsSummaryLabel->setText("No targets");
    } else {
        m_targetsSummaryLabel->setText(
            QString::fromUtf8("%1 target%2 · %3%4 IPs")
                .arg(count)
                .arg(count == 1 ? "" : "s")
                .arg(exactIps ? QString::fromUtf8("≈ ") : QString::fromUtf8("≥ "))
                .arg(QString::fromStdString(geo::human_number(totalIps)))
        );
    }

    if (m_bannerNoteWidget) {
        bool showBannerNote = s.banners && s.aip.empty() && s.aport.empty();
        m_bannerNoteWidget->setVisible(showBannerNote);
    }

    m_currentBuilt = build_command(s, manualTargets, countryCodes, m_resumeMode);

    QString fullCmd;
    for (size_t i = 0; i < m_currentBuilt.args.size(); ++i) {
        if (i > 0) fullCmd += " ";
        fullCmd += QString::fromStdString(m_currentBuilt.args[i]);
    }
    if (m_terminalBox) {
        m_terminalBox->setCommand(fullCmd);
    } else if (m_commandTextLabel) {
        m_commandTextLabel->setText(fullCmd);
    }

    // Clear old warning rows
    QLayoutItem* child;
    while ((child = m_warningsLayout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    bool hasErr = false;
    qreal dpr = devicePixelRatioF();
    QString errCol   = m_isDarkTheme ? "#ef5350" : "#d32f2f";
    QString warnCol  = m_isDarkTheme ? "#f5a524" : "#b9740a";
    QString mutedCol = m_isDarkTheme ? "#8b97a8" : "#5b6a7e";

    for (const auto& w : m_currentBuilt.warnings) {
        auto* row = new QWidget(m_warningsContainer);
        auto* rl = new QHBoxLayout(row);
        rl->setContentsMargins(0, 0, 0, 0);
        rl->setSpacing(6);

        auto* ico = new QLabel(row);
        ico->setFixedSize(14, 17);
        ico->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        auto* txt = new QLabel(QString::fromStdString(w.text), row);
        txt->setWordWrap(true);

        if (w.kind == WarnKind::Err) {
            hasErr = true;
            ico->setPixmap(Icons::getPixmap(Icons::Error, QColor(errCol), 14, dpr));
            txt->setStyleSheet(QString("font-family: \"Segoe UI\"; font-size: 12px; font-weight: 400; color: %1;").arg(errCol));
        } else if (w.kind == WarnKind::Warn) {
            ico->setPixmap(Icons::getPixmap(Icons::Alert, QColor(warnCol), 14, dpr));
            txt->setStyleSheet(QString("font-family: \"Segoe UI\"; font-size: 12px; font-weight: 400; color: %1;").arg(warnCol));
        } else {
            ico->setPixmap(Icons::getPixmap(Icons::Info, QColor(mutedCol), 14, dpr));
            txt->setStyleSheet(QString("font-family: \"Segoe UI\"; font-size: 12px; font-weight: 400; color: %1;").arg(mutedCol));
        }

        rl->addWidget(ico, 0, Qt::AlignTop);
        rl->addWidget(txt, 1);
        m_warningsLayout->addWidget(row);
        row->show();
    }
    m_warningsContainer->setVisible(!m_currentBuilt.warnings.empty());
    m_warningsLayout->activate();
    if (m_warningsContainer->parentWidget() && m_warningsContainer->parentWidget()->layout()) {
        m_warningsContainer->parentWidget()->layout()->activate();
    }

    updateCommandCardHeight();

    bool canStart = !hasErr && (!manualTargets.empty() || !countryCodes.empty() || s.all_ipv4 || !s.conffile.empty() || s.mode >= 3);
    bool startAllowed = canStart && !m_scanner->isRunning();
    m_btnStart->setEnabled(startAllowed);
    if (m_btnStartNarrow) m_btnStartNarrow->setEnabled(startAllowed);
}

void MainWindow::updateCommandCardHeight() {
    if (!m_cmdCard || !m_warningsLayout) return;

    int cardW = m_cmdCard->width();
    if (cardW <= 100) {
        bool isNarrow = width() < 880;
        cardW = std::max(240, width() - (isNarrow ? 24 : (380 + 36)));
    }
    int innerW = std::max(160, cardW - 20);

    int cmdH = 75;
    if (m_terminalBox) {
        cmdH = m_terminalBox->heightForWidth(innerW);
    } else if (m_commandTextLabel) {
        cmdH = m_commandTextLabel->heightForWidth(innerW);
        if (cmdH <= 0) {
            cmdH = m_commandTextLabel->sizeHint().height();
        }
        cmdH = std::max(18, cmdH);
    }

    int warnsH = 0;
    int warnCount = m_warningsLayout->count();
    int txtW = std::max(120, innerW - 20);
    for (int i = 0; i < warnCount; ++i) {
        if (auto* item = m_warningsLayout->itemAt(i)) {
            if (auto* row = item->widget()) {
                int rowH = 17;
                const auto labels = row->findChildren<QLabel*>();
                if (labels.size() >= 2) {
                    int h = labels[1]->heightForWidth(txtW);
                    if (h > 0) rowH = std::max(17, h);
                }
                warnsH += rowH;
                if (i > 0) warnsH += m_warningsLayout->spacing();
            }
        }
    }

    // padding-top(10) + cmdH + (warnsH > 0 ? 6 + warnsH : 0) + padding-bottom(10) + border(2)
    int preferredH = 10 + cmdH + (warnsH > 0 ? (6 + warnsH) : 0) + 10 + 2;
    m_cmdCard->setFixedHeight(std::clamp(preferredH, 98, 230));
}

void MainWindow::onStartScan() {
    if (m_currentBuilt.args.empty()) return;

    QString program = QString::fromStdString(m_currentBuilt.args[0]);
    QStringList args;
    for (size_t i = 1; i < m_currentBuilt.args.size(); ++i) {
        args.push_back(QString::fromStdString(m_currentBuilt.args[i]));
    }

    std::vector<std::string> manualTargets;
    std::vector<std::string> countryCodes;
    for (const auto& t : m_targets) {
        if (t.isCountry) countryCodes.push_back(t.code.toStdString());
        else manualTargets.push_back(t.text.toStdString());
    }

    if (m_allIpv4Check && m_allIpv4Check->isChecked()) {
        manualTargets.insert(manualTargets.begin(), "0.0.0.0/0");
    }

    if (!countryCodes.empty()) {
        QString dir = m_outdirEdit->text().trimmed();
        if (dir.isEmpty()) dir = "MassGUI";
        size_t lines = 0;
        uint64_t addrs = 0;
        write_targets_file((dir + "/targets.txt").toStdString(), manualTargets, countryCodes, lines, addrs);
    }

    QString actualProgram = program;
    if (QFileInfo::exists(program) && QFileInfo(program).isFile()) {
        actualProgram = program;
    } else if (QFileInfo::exists(QCoreApplication::applicationDirPath() + "/masscan.exe")) {
        actualProgram = QCoreApplication::applicationDirPath() + "/masscan.exe";
    } else if (QFileInfo::exists("masscan.exe")) {
        actualProgram = QDir::current().absoluteFilePath("masscan.exe");
    } else {
        QString inPath = QStandardPaths::findExecutable(program);
        if (!inPath.isEmpty()) {
            actualProgram = inPath;
        }
    }

    bool realBinaryExists = QFileInfo::exists(actualProgram) && QFileInfo(actualProgram).isFile();
    if (!realBinaryExists && promptAndDownloadMasscan(false)) {
        actualProgram = QCoreApplication::applicationDirPath() + "/masscan.exe";
        realBinaryExists = QFileInfo::exists(actualProgram) && QFileInfo(actualProgram).isFile();
    }
    bool simulate = !realBinaryExists;

    if (m_scanner->startScan(actualProgram, args, simulate)) {
        m_btnStart->setEnabled(false);
        m_btnStop->setEnabled(true);
        m_btnResume->setEnabled(false);
        if (m_btnStartNarrow) m_btnStartNarrow->setEnabled(false);
        if (m_btnStopNarrow) m_btnStopNarrow->setEnabled(true);
        if (m_btnResumeNarrow) m_btnResumeNarrow->setEnabled(false);
        if (m_menuOpen) setDrawerOpen(false, true);
    }
}

void MainWindow::onStopScan() {
    m_scanner->stopScan();
    m_btnStart->setEnabled(true);
    m_btnStop->setEnabled(false);
    m_btnResume->setEnabled(m_scanner->canResume());
    if (m_btnStartNarrow) m_btnStartNarrow->setEnabled(true);
    if (m_btnStopNarrow) m_btnStopNarrow->setEnabled(false);
    if (m_btnResumeNarrow) m_btnResumeNarrow->setEnabled(m_scanner->canResume());
}

void MainWindow::onResumeScan() {
    m_resumeMode = true;
    updateCommandAndValidation();
    QString bin = QString::fromStdString(m_currentBuilt.args.empty() ? "masscan" : m_currentBuilt.args[0]);
    m_scanner->resumeScan(bin);
    m_btnStart->setEnabled(false);
    m_btnStop->setEnabled(true);
    m_btnResume->setEnabled(false);
    if (m_btnStartNarrow) m_btnStartNarrow->setEnabled(false);
    if (m_btnStopNarrow) m_btnStopNarrow->setEnabled(true);
    if (m_btnResumeNarrow) m_btnResumeNarrow->setEnabled(false);
    if (m_menuOpen) setDrawerOpen(false, true);
}

void MainWindow::onOpenOutputFolder() {
    QString dir = m_outdirEdit->text().trimmed();
    if (dir.isEmpty()) dir = "MassGUI";
    QDir().mkpath(dir);
    QDesktopServices::openUrl(QUrl::fromLocalFile(QDir::current().absoluteFilePath(dir)));
}

void MainWindow::onCopyCommand() {
    QString cmdToCopy = m_terminalBox ? m_terminalBox->command() : (m_commandTextLabel ? m_commandTextLabel->text() : "");
    QApplication::clipboard()->setText(cmdToCopy);
    handleLogLine(QString("[%1] Command copied to clipboard.").arg(QTime::currentTime().toString("HH:mm:ss")));
}

void MainWindow::onClearResults() {
    m_resultsTable->setRowCount(0);
    m_discoveredKeys.clear();
    if (m_scanner) {
        m_scanner->resetFoundCount();
    }
    m_footerFoundLabel->setText("Found 0");
    m_pillResults->setText("Results");
}

void MainWindow::onExportResults() {
    QString filename = QFileDialog::getSaveFileName(this, "Export Results", "results.csv", "CSV Files (*.csv)");
    if (filename.isEmpty()) return;

    QFile file(filename);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "IP,Port,Proto,Service,Status,Time\n";
        for (int r = 0; r < m_resultsTable->rowCount(); ++r) {
            QStringList row;
            for (int c = 0; c < m_resultsTable->columnCount(); ++c) {
                auto* it = m_resultsTable->item(r, c);
                QString v = it ? it->text() : "";
                v.replace("\"", "\"\"");
                row << "\"" + v + "\"";
            }
            out << row.join(",") << "\n";
        }
        file.close();
    }
}

void MainWindow::onToggleTheme() {
    if (m_themeToggleBtn && isVisible()) {
        m_themeToggleBtn->captureBeforeTheme(this);
    }
    m_isDarkTheme = !m_isDarkTheme;
    if (m_isDarkTheme) {
        setStyleSheet(theme::getDarkStyle());
    } else {
        setStyleSheet(theme::getLightStyle());
    }
    updateThemeDependentAssets();
    if (m_themeToggleBtn && isVisible()) {
        m_themeToggleBtn->startThemeTransition(this);
    }
}

void MainWindow::updateThemeDependentAssets() {
    QColor icoCol    = m_isDarkTheme ? QColor("#e6ebf2") : QColor("#1a2230");
    QColor mutedCol  = m_isDarkTheme ? QColor("#8b97a8") : QColor("#5b6a7e");
    QColor accentCol = m_isDarkTheme ? QColor("#3b9eff") : QColor("#1f73d6");

    m_themeToggleBtn->setDark(m_isDarkTheme);
    FluentToolTip::instance()->setDark(m_isDarkTheme);
    if (m_menuBtn) m_menuBtn->setDark(m_isDarkTheme);

    m_btnAddCountries->setIcon(Icons::get(Icons::Plus, icoCol, 16));
    m_btnAddManual->setIcon(Icons::get(Icons::Plus, icoCol, 16));
    m_btnEditTarget->setIcon(Icons::get(Icons::Edit, icoCol, 16));
    m_btnRemoveTarget->setIcon(Icons::get(Icons::Close, icoCol, 16));
    m_btnClearTargets->setIcon(Icons::get(Icons::Trash, icoCol, 16));
    m_btnImportTargets->setIcon(Icons::get(Icons::Import, icoCol, 16));

    m_btnStart->setIcon(Icons::get(Icons::Play, Qt::white, 16));
    m_btnStop->setIcon(Icons::get(Icons::Stop, icoCol, 16));
    m_btnResume->setIcon(Icons::get(Icons::Resume, icoCol, 16));
    m_btnOpenFolder->setIcon(Icons::get(Icons::Folder, icoCol, 16));
    if (m_btnStartNarrow)      m_btnStartNarrow->setIcon(Icons::get(Icons::Play, Qt::white, 16));
    if (m_btnStopNarrow)       m_btnStopNarrow->setIcon(Icons::get(Icons::Stop, icoCol, 16));
    if (m_btnResumeNarrow)     m_btnResumeNarrow->setIcon(Icons::get(Icons::Resume, icoCol, 16));
    if (m_btnOpenFolderNarrow) m_btnOpenFolderNarrow->setIcon(Icons::get(Icons::Folder, icoCol, 16));

    if (m_btnCopy && !m_terminalBox) m_btnCopy->setIcon(Icons::get(Icons::Copy, icoCol, 16));
    if (m_btnExport) m_btnExport->setIcon(Icons::get(Icons::Export, icoCol, 16));
    if (m_btnClear)  m_btnClear->setIcon(Icons::get(Icons::Trash, icoCol, 16));

    if (m_commandTextLabel && !m_terminalBox) {
        m_commandTextLabel->setStyleSheet(QString(
            "font-family: \"Consolas\", \"Courier New\"; font-size: 13px; font-weight: 400; color: %1;"
        ).arg(m_isDarkTheme ? "#3b9eff" : "#1f73d6"));
    }

    if (m_bannerNoteWidget && m_bannerNoteLabel) {
        QString warnBg  = m_isDarkTheme ? "#3a2c10" : "#fff1d6";
        QString warnCol = m_isDarkTheme ? "#f5a524" : "#b9740a";
        m_bannerNoteWidget->setStyleSheet(QString(
            "QWidget#bannerNoteWidget { background-color: %1; border: 1px solid %2; border-radius: 6px; }"
        ).arg(warnBg, warnCol));
        m_bannerNoteLabel->setStyleSheet(QString(
            "font-family: \"Segoe UI\"; font-size: 12px; font-weight: 400; color: %1; background: transparent; border: none;"
        ).arg(warnCol));
    }

    m_pillResults->setIconSize(QSize(14, 14));
    m_pillLog->setIconSize(QSize(14, 14));
    m_pillResults->setIcon(Icons::get(Icons::List, m_currentTab == 0 ? accentCol : mutedCol, 14));
    m_pillLog->setIcon(Icons::get(Icons::Terminal, m_currentTab == 1 ? accentCol : mutedCol, 14));

    if (m_countrySearchAction) {
        m_countrySearchAction->setIcon(Icons::get(Icons::Search, mutedCol, 16));
    }
    if (m_filterResultsAction) {
        m_filterResultsAction->setIcon(Icons::get(Icons::Search, mutedCol, 16));
    }

    if (m_terminalBox) m_terminalBox->setDark(m_isDarkTheme);

    updateCommandAndValidation();

    if (m_sidebarCard) m_sidebarCard->setDark(m_isDarkTheme);
    if (m_narrowActionCard) m_narrowActionCard->setDark(m_isDarkTheme);
    if (m_cmdCard) m_cmdCard->setDark(m_isDarkTheme);
    if (m_githubBtn) m_githubBtn->setDark(m_isDarkTheme);
    if (m_drawerOverlay) m_drawerOverlay->clearPixmaps();
    if (m_resultsCard) m_resultsCard->setDark(m_isDarkTheme);
    if (m_countryDelegate) m_countryDelegate->setDark(m_isDarkTheme);
    if (m_targetDelegate) m_targetDelegate->setDark(m_isDarkTheme);

    for (auto* sec : m_sections) {
        if (sec) sec->applyTheme(m_isDarkTheme);
    }

    if (m_sidebarScrollOverlay) {
        m_sidebarScrollOverlay->setDark(m_isDarkTheme);
    }
    if (m_countryScrollOverlay) {
        m_countryScrollOverlay->setDark(m_isDarkTheme);
    }
    if (m_targetsScrollOverlay) {
        m_targetsScrollOverlay->setDark(m_isDarkTheme);
    }
    if (m_cmdScrollOverlay) {
        m_cmdScrollOverlay->setDark(m_isDarkTheme);
    }
    if (m_tableScrollOverlay) {
        m_tableScrollOverlay->setDark(m_isDarkTheme);
    }
    if (m_logScrollOverlay) {
        m_logScrollOverlay->setDark(m_isDarkTheme);
    }

    m_masscanBadge->setStatus(m_masscanState, m_masscanInfo, m_isDarkTheme);
    m_npcapBadge->setStatus(m_npcapState, m_npcapInfo, m_isDarkTheme);

    if (m_countryListWidget) m_countryListWidget->viewport()->update();
    if (m_targetsListWidget) m_targetsListWidget->viewport()->update();
}

void MainWindow::setDrawerOpen(bool open, bool animate) {
    if (m_menuOpen == open && !m_drawerAnim.isRunning()) return;
    m_menuOpen = open;
    if (m_menuBtn) {
        m_menuBtn->setOpen(m_menuOpen, false);
        m_menuBtn->setProgress(m_drawerProgress);
    }
    qreal target = m_menuOpen ? 1.0 : 0.0;
    if (animate && width() < 880 && isVisible() && m_bodyContainer && m_sidebarCard && m_drawerOverlay) {
        const int bodyW = m_bodyContainer->width();
        const int bodyH = m_bodyContainer->height();
        const int sideW = std::min(380, std::max(240, bodyW));

        if (!m_drawerAnim.isRunning()) {
            if (open) {
                // Opening from closed state: reuse cached sidebarPix if already valid
                QPixmap bgPix = m_bodyContainer->grab();
                if (!m_drawerOverlay->hasSidebarPixmap()) {
                    m_drawerOverlay->setPixmaps(bgPix, m_sidebarCard->grab(), sideW);
                } else {
                    m_drawerOverlay->setBgPixmap(bgPix);
                }
            } else {
                // Closing from open state: grab current sidebar, reuse bgPix if available
                QPixmap sidebarPix = m_sidebarCard->grab();
                m_sidebarCard->hide();
                if (m_drawerBackdrop) m_drawerBackdrop->hide();
                if (m_drawerOverlay->hasBgPixmap()) {
                    m_drawerOverlay->setSidebarPixmap(sidebarPix, sideW);
                } else {
                    m_drawerOverlay->hide();
                    QPixmap bgPix = m_bodyContainer->grab();
                    m_drawerOverlay->setPixmaps(bgPix, sidebarPix, sideW);
                }
            }
            m_drawerOverlay->setGeometry(0, 0, bodyW, bodyH);
            m_drawerOverlay->setProgress(m_drawerProgress);
            m_drawerOverlay->show();
            m_drawerOverlay->raise();
        }

        m_drawerStart = m_drawerProgress;
        m_drawerTarget = target;
        m_drawerAnim.start();
    } else {
        m_drawerAnim.stop();
        m_drawerProgress = target;
        if (m_menuBtn) m_menuBtn->setProgress(target);
        if (m_drawerOverlay) {
            m_drawerOverlay->hide();
            m_drawerOverlay->clearPixmaps();
        }
        layoutBodyWidgets();
    }
}

void MainWindow::onToggleSidebarMenu() {
    setDrawerOpen(!m_menuOpen, true);
}

void MainWindow::onRecheckDependencies() {
    checkDependencies();
    if (m_masscanState == 0) {
        promptAndDownloadMasscan(false);
    }
}

bool MainWindow::promptAndDownloadMasscan(bool silentCheck) {
    QString appDir = QCoreApplication::applicationDirPath();
    QString targetExe = appDir + "/masscan.exe";

    if (QFileInfo::exists(targetExe) && QFileInfo(targetExe).isFile()) {
        return true;
    }
    if (QFileInfo::exists("masscan.exe") && QFileInfo("masscan.exe").isFile()) {
        return true;
    }
    if (silentCheck) {
        return false;
    }

    // 1. Ask permission
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Masscan Required");
    msgBox.setText("The Masscan executable ('masscan.exe') was not found next to MasscanGUI.");
    msgBox.setInformativeText("Would you like to automatically download the latest official release from GitHub?\n\nSource: https://github.com/ramin-mahmoodi/Masscan");
    msgBox.setIcon(QMessageBox::Question);
    auto* btnDownload = msgBox.addButton("Download Masscan", QMessageBox::AcceptRole);
    auto* btnCancel = msgBox.addButton("Cancel", QMessageBox::RejectRole);
    msgBox.setDefaultButton(btnDownload);
    msgBox.exec();

    if (msgBox.clickedButton() != btnDownload) {
        handleLogLine(QString("[%1] Masscan download declined by user.").arg(QTime::currentTime().toString("HH:mm:ss")));
        return false;
    }

    // 2. Prepare download
    QUrl downloadUrl("https://github.com/ramin-mahmoodi/Masscan/releases/latest/download/masscan-windows-x86_64.zip");
    QString tmpZipPath = QDir::tempPath() + "/masscan-windows-x86_64.zip";

    QProgressDialog progress(this);
    progress.setWindowTitle("Downloading Masscan");
    progress.setLabelText("Connecting to GitHub releases...");
    progress.setRange(0, 100);
    progress.setValue(0);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);
    progress.show();
    QApplication::processEvents();

    QNetworkAccessManager nam;
    QNetworkRequest request(downloadUrl);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setHeader(QNetworkRequest::UserAgentHeader, "MasscanGUI/1.0");

    QNetworkReply* reply = nam.get(request);

    QFile file(tmpZipPath);
    if (!file.open(QIODevice::WriteOnly)) {
        progress.close();
        QMessageBox::critical(this, "Download Error", "Could not create temporary file in:\n" + tmpZipPath);
        return false;
    }

    bool userCancelled = false;
    connect(&progress, &QProgressDialog::canceled, [&]() {
        userCancelled = true;
        reply->abort();
    });

    connect(reply, &QNetworkReply::downloadProgress, [&](qint64 bytesReceived, qint64 bytesTotal) {
        if (bytesTotal > 0) {
            int pct = static_cast<int>((bytesReceived * 100) / bytesTotal);
            progress.setValue(pct);
            progress.setLabelText(QString("Downloading masscan: %1% (%2 KB / %3 KB)")
                                      .arg(pct)
                                      .arg(bytesReceived / 1024)
                                      .arg(bytesTotal / 1024));
        } else {
            progress.setLabelText(QString("Downloading masscan: %1 KB received").arg(bytesReceived / 1024));
        }
    });

    connect(reply, &QNetworkReply::readyRead, [&]() {
        file.write(reply->readAll());
    });

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    file.flush();
    file.close();

    if (userCancelled) {
        QFile::remove(tmpZipPath);
        handleLogLine(QString("[%1] Download canceled by user.").arg(QTime::currentTime().toString("HH:mm:ss")));
        return false;
    }

    if (reply->error() != QNetworkReply::NoError) {
        QString errStr = reply->errorString();
        reply->deleteLater();
        QFile::remove(tmpZipPath);
        progress.close();
        QMessageBox::critical(this, "Download Failed", "Failed to download Masscan from GitHub:\n" + errStr);
        return false;
    }

    reply->deleteLater();
    progress.setLabelText("Extracting masscan.exe...");
    progress.setValue(100);
    QApplication::processEvents();

    // 3. Extract masscan.exe
    bool extracted = false;
    int code = QProcess::execute("tar.exe", {"-xf", tmpZipPath, "-C", appDir});
    if (code == 0 && QFileInfo::exists(targetExe)) {
        extracted = true;
    }

    if (!extracted) {
        QString psCmd = QString("Expand-Archive -Path '%1' -DestinationPath '%2' -Force").arg(tmpZipPath, appDir);
        QProcess::execute("powershell.exe", {"-NoProfile", "-Command", psCmd});
        if (QFileInfo::exists(targetExe)) {
            extracted = true;
        }
    }

    QFile::remove(tmpZipPath);
    progress.close();

    if (!extracted) {
        QMessageBox::warning(this, "Extraction Failed", 
            "Downloaded archive could not be extracted automatically.\n"
            "Please extract masscan.exe manually to:\n" + targetExe);
        return false;
    }

    // 4. Update dependencies and UI
    checkDependencies();
    updateCommandAndValidation();

    handleLogLine(QString("[%1] Masscan successfully downloaded and installed to %2").arg(QTime::currentTime().toString("HH:mm:ss"), targetExe));
    QMessageBox::information(this, "Installation Complete", 
        "Masscan has been successfully downloaded and placed next to MasscanGUI!\n\nLocation: " + targetExe);

    return true;
}

void MainWindow::checkDependencies() {
    m_masscanState = 0;
    m_masscanInfo = "not found";

    QString conf = m_binEdit ? m_binEdit->text().trimmed() : "masscan";
    QString appDir = QCoreApplication::applicationDirPath();

    if (!conf.isEmpty() && conf != "masscan") {
        if (QFileInfo::exists(conf) && QFileInfo(conf).isFile()) {
            m_masscanState = 1;
            m_masscanInfo = "custom path";
        } else if (!QStandardPaths::findExecutable(conf).isEmpty()) {
            m_masscanState = 2;
            m_masscanInfo = "installed";
        }
    } else if (QFileInfo::exists(appDir + "/masscan.exe") ||
               QFileInfo::exists(appDir + "/masscan/masscan.exe") ||
               QFileInfo::exists("masscan.exe")) {
        // Found next to application binary or in working directory: State 1 (blue)
        m_masscanState = 1;
        m_masscanInfo = "next to app";
    } else if (!QStandardPaths::findExecutable("masscan").isEmpty()) {
        m_masscanState = 2;
        m_masscanInfo = "installed";
    }

    m_npcapState = 0;
    m_npcapInfo = "not found";
#ifdef _WIN32
    char winDir[MAX_PATH];
    if (GetWindowsDirectoryA(winDir, sizeof(winDir))) {
        QString sWinDir = QString::fromLocal8Bit(winDir);
        if (QFileInfo::exists(sWinDir + "/System32/Npcap/wpcap.dll")) {
            m_npcapState = 2;
            m_npcapInfo = "installed";
        } else if (QFileInfo::exists(appDir + "/wpcap.dll") || QFileInfo::exists(appDir + "/Npcap/wpcap.dll")) {
            m_npcapState = 1;
            m_npcapInfo = "next to app";
        } else if (QFileInfo::exists(sWinDir + "/System32/wpcap.dll")) {
            m_npcapState = 2;
            m_npcapInfo = "installed (WinPcap)";
        }
    }
#endif

    m_masscanBadge->setStatus(m_masscanState, m_masscanInfo, m_isDarkTheme);
    m_npcapBadge->setStatus(m_npcapState, m_npcapInfo, m_isDarkTheme);
}

void MainWindow::showEvent(QShowEvent* event) {
    QMainWindow::showEvent(event);
    applyResponsiveLayout(width());
    updateCommandCardHeight();

    static bool initialMasscanChecked = false;
    if (!initialMasscanChecked) {
        initialMasscanChecked = true;
        QTimer::singleShot(400, this, [this]() {
            if (m_masscanState == 0) {
                promptAndDownloadMasscan(false);
            }
        });
    }
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    if (m_drawerOverlay) m_drawerOverlay->clearPixmaps();
    applyResponsiveLayout(event->size().width());
    updateCommandCardHeight();
}

void MainWindow::layoutBodyWidgets() {
    if (!m_bodyContainer || !m_sidebarCard || !m_rightColumn) return;

    const int gap = 12;
    const int bodyW = m_bodyContainer->width();
    const int bodyH = m_bodyContainer->height();
    if (bodyW <= 0 || bodyH <= 0) return;

    const bool isNarrow = width() < 880;
    const int sideH = std::max(100, bodyH - 2 * gap);

    if (!isNarrow) {
        const int sideW = 380;
        if (m_drawerOverlay) m_drawerOverlay->hide();
        if (m_drawerBackdrop) m_drawerBackdrop->hide();
        m_sidebarCard->setRadius(8);
        if (auto* l = m_sidebarCard->layout()) l->setContentsMargins(1, 1, 1, 1);
        m_sidebarCard->setFixedSize(sideW, sideH);
        m_sidebarCard->move(gap, gap);
        m_sidebarCard->show();

        const int rightX = gap * 2 + sideW;
        const int rightW = std::max(200, bodyW - sideW - 3 * gap);
        m_rightColumn->setGeometry(rightX, gap, rightW, sideH);
    } else {
        const int sideW = std::min(380, std::max(240, bodyW));
        const int rightW = std::max(200, bodyW - 2 * gap);
        m_rightColumn->setGeometry(gap, gap, rightW, sideH);

        if (m_drawerAnim.isRunning() && m_drawerOverlay && m_drawerOverlay->isVisible()) {
            m_drawerOverlay->setGeometry(0, 0, bodyW, bodyH);
            m_drawerOverlay->raise();
            return;
        }

        // Flush full-height edge-to-edge drawer with 0 corner radius and 0 outer gap
        m_sidebarCard->setRadius(0);
        if (auto* l = m_sidebarCard->layout()) l->setContentsMargins(0, 0, 1, 0);
        m_sidebarCard->setFixedSize(sideW, bodyH);

        const bool vis = m_drawerProgress > 0.5;
        if (vis) {
            m_sidebarCard->move(0, 0);
            m_sidebarCard->show();
            if (m_drawerBackdrop) {
                // Only place backdrop to the right of sidebar so sidebar scrolling has 0 backdrop overlap!
                m_drawerBackdrop->setGeometry(sideW, 0, std::max(0, bodyW - sideW), bodyH);
                m_drawerBackdrop->setOpacity(1.0);
                m_drawerBackdrop->show();
                m_drawerBackdrop->raise();
            }
            m_sidebarCard->raise();
        } else {
            m_sidebarCard->hide();
            if (m_drawerBackdrop) m_drawerBackdrop->hide();
        }
    }
}

void MainWindow::applyResponsiveLayout(int winWidth) {
    bool isNarrow = winWidth < 880;
    if (m_menuBtn) m_menuBtn->setVisible(isNarrow);
    if (m_narrowActionCard) m_narrowActionCard->setVisible(isNarrow);

    if (!isNarrow && (m_menuOpen || m_drawerProgress > 0.0 || m_drawerAnim.isRunning())) {
        m_drawerAnim.stop();
        m_menuOpen = false;
        m_drawerProgress = 0.0;
        if (m_menuBtn) m_menuBtn->setOpen(false, false);
    }

    // Pre-warm sidebar pixmap when first entering narrow mode so the very first open has 0 grab delay
    if (isNarrow && m_drawerOverlay && !m_drawerOverlay->hasSidebarPixmap() && m_sidebarCard && m_bodyContainer) {
        const int bodyW = m_bodyContainer->width();
        const int bodyH = m_bodyContainer->height();
        if (bodyW > 0 && bodyH > 0) {
            const int sideW = std::min(380, std::max(240, bodyW));
            m_sidebarCard->setRadius(0);
            if (auto* l = m_sidebarCard->layout()) l->setContentsMargins(0, 0, 1, 0);
            m_sidebarCard->setFixedSize(sideW, bodyH);
            m_drawerOverlay->setSidebarPixmap(m_sidebarCard->grab(), sideW);
        }
    }
    bool compactFilterBtns = winWidth < 560;
    if (m_btnExport) {
        m_btnExport->setText(compactFilterBtns ? "" : "Export");
        m_btnExport->setToolTip(compactFilterBtns ? "Export" : "");
    }
    if (m_btnClear) {
        m_btnClear->setText(compactFilterBtns ? "" : "Clear");
        m_btnClear->setToolTip(compactFilterBtns ? "Clear" : "");
    }

    // Responsive Footer elements
    m_footerPctLabel->setVisible(winWidth >= 440);
    m_footerEtaLabel->setVisible(winWidth >= 600);
    m_footerKppsLabel->setVisible(winWidth >= 740);

    // Responsive Badges matching Slint: root.win-w >= 820px ? 0 : root.win-w >= 480px ? 1 : 2
    int badgeMode = (winWidth >= 820) ? 0 : (winWidth >= 480 ? 1 : 2);
    m_masscanBadge->setMode(badgeMode);
    m_npcapBadge->setMode(badgeMode);
    if (m_githubBtn) {
        m_githubBtn->setMode(badgeMode);
    }

    layoutBodyWidgets();
}

void MainWindow::handleScannerStatus(const QString& stateText) {
    m_footerStateLabel->setText(stateText);
    bool isRunning = m_scanner->isRunning();
    m_footerDot->setStyleSheet(isRunning ? "background-color: #34c77b; border-radius: 4px;" : "background-color: #8b97a8; border-radius: 4px;");
}

void MainWindow::handleScannerProgress(float pct, double kpps, const QString& eta, int found) {
    m_footerProgressBar->setValue(static_cast<int>(pct));
    m_footerPctLabel->setText(QString::number(pct, 'f', 1) + "% done");
    if (!eta.isEmpty()) m_footerEtaLabel->setText("ETA " + eta);
    if (kpps > 0.0) m_footerKppsLabel->setText(QString::number(kpps, 'f', 2) + " kpps");
    m_footerFoundLabel->setText(QString("Found %1").arg(found));
    m_pillResults->setText(QString("Results (%1)").arg(found));
}

void MainWindow::handlePortDiscovered(const QString& ip, int port, const QString& proto, const QString& state, const QString& banner, const QString& timeStr) {
    // 1. De-duplicate findings (ignore repeated SYN-ACKs for the same target)
    std::string key = ip.toStdString() + ":" + std::to_string(port) + "/" + proto.toLower().toStdString();
    if (!m_discoveredKeys.insert(key).second) {
        return;
    }

    // 2. Cap table rows at 5,000 to prevent unbounded memory growth and UI lag
    if (m_resultsTable->rowCount() >= 5000) {
        m_resultsTable->removeRow(0);
    }

    int r = m_resultsTable->rowCount();
    m_resultsTable->insertRow(r);

    auto* itemIp = new QTableWidgetItem(ip);
    auto* itemPort = new QTableWidgetItem();
    itemPort->setData(Qt::DisplayRole, port); // Numeric sort
    auto* itemProto = new QTableWidgetItem(proto.toUpper());
    auto* itemService = new QTableWidgetItem(banner.isEmpty() ? "-" : banner);
    auto* itemState = new QTableWidgetItem(state);
    auto* itemTime = new QTableWidgetItem(timeStr);

    itemState->setForeground(QBrush(QColor("#34c77b")));

    m_resultsTable->setItem(r, 0, itemIp);
    m_resultsTable->setItem(r, 1, itemPort);
    m_resultsTable->setItem(r, 2, itemProto);
    m_resultsTable->setItem(r, 3, itemService);
    m_resultsTable->setItem(r, 4, itemState);
    m_resultsTable->setItem(r, 5, itemTime);

    m_resultsTable->scrollToBottom();
}

void MainWindow::handleLogLine(const QString& line) {
    m_logEdit->appendPlainText(line);
}

void MainWindow::handleScanFinished(int exitCode, bool normalExit) {
    Q_UNUSED(exitCode);
    Q_UNUSED(normalExit);
    m_btnStart->setEnabled(true);
    m_btnStop->setEnabled(false);
    m_btnResume->setEnabled(m_scanner->canResume());
    if (m_btnStartNarrow) m_btnStartNarrow->setEnabled(true);
    if (m_btnStopNarrow) m_btnStopNarrow->setEnabled(false);
    if (m_btnResumeNarrow) m_btnResumeNarrow->setEnabled(m_scanner->canResume());
    m_footerDot->setStyleSheet("background-color: #8b97a8; border-radius: 4px;");
}

} // namespace masscan::ui
