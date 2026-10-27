#pragma once

#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QAbstractButton>
#include <QLabel>
#include <QListWidget>
#include <QTableWidget>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QScrollArea>
#include <QAction>
#include <QNetworkAccessManager>
#include <set>
#include <unordered_set>
#include <vector>

#include "../core/command_builder.hpp"
#include "../core/scanner_process.hpp"
#include "badge_widget.hpp"
#include "theme_toggle.hpp"
#include "hamburger_button.hpp"
#include "card_widget.hpp"
#include "item_delegates.hpp"
#include "high_hz_animator.hpp"

namespace masscan::ui {

class CollapsibleSection;
class FluentScrollBarOverlay;
class TerminalCodeBox;
class GitHubButton;
class SlidingStackedWidget;

struct TargetItem {
    QString text;
    QString code;
    QString info;
    bool isCountry{false};
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;
    void onToggleTheme();

protected:
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

private slots:
    void updateCommandAndValidation();
    void onSearchCountries(const QString& q);
    void onToggleCountrySelection(QListWidgetItem* item);
    void onAddSelectedCountries();
    void onAddManualTarget();
    void onEditSelectedTarget();
    void onRemoveSelectedTarget();
    void onClearAllTargets();
    void onImportTargets();
    void onPortPresetSelected(int index);
    void onStartScan();
    void onStopScan();
    void onResumeScan();
    void onOpenOutputFolder();
    void onCopyCommand();
    void onClearResults();
    void onExportResults();
    void onRecheckDependencies();
    void onToggleSidebarMenu();
    void onTabSwitched(int tabIndex);

    // Scanner callbacks
    void handleScannerStatus(const QString& stateText);
    void handleScannerProgress(float pct, double kpps, const QString& eta, int found);
    void handlePortDiscovered(const QString& ip, int port, const QString& proto, const QString& state, const QString& banner, const QString& timeStr);
    void handleLogLine(const QString& line);
    void handleScanFinished(int exitCode, bool normalExit);

private:
    void setupUi();
    void createHeader();
    CardWidget* createSidebar();
    QWidget* createRightColumn();
    void createFooter();
    void checkDependencies();
    bool promptAndDownloadMasscan(bool silentCheck = false);
    void startMasscanDownload();
    void finalizeMasscanExtraction(bool success, const QString& targetExe, const QString& tmpZipPath);
    void startNpcapDownload();
    void downloadNpcapInstaller(const QString& filename);
    void applyResponsiveLayout(int winWidth);
    void layoutBodyWidgets();
    void setDrawerOpen(bool open, bool animate = true);
    void updateThemeDependentAssets();
    void updateCommandCardHeight();
    Snapshot getSnapshot() const;
    void populateCountryList(const QString& q = "");

    ScannerProcess* m_scanner{nullptr};

    // Header
    QWidget* m_headerWidget{nullptr};
    HamburgerButton* m_menuBtn{nullptr};
    QLabel* m_logoLabel{nullptr};
    QLabel* m_titleLabel{nullptr};
    BadgeWidget* m_masscanBadge{nullptr};
    BadgeWidget* m_npcapBadge{nullptr};
    GitHubButton* m_githubBtn{nullptr};
    ThemeToggleBtn* m_themeToggleBtn{nullptr};
    bool m_isDarkTheme{true};

    // Main layout components
    QWidget* m_bodyContainer{nullptr};
    DrawerBackdrop* m_drawerBackdrop{nullptr};
    DrawerAnimOverlay* m_drawerOverlay{nullptr};
    CardWidget* m_sidebarCard{nullptr};
    CardWidget* m_narrowActionCard{nullptr};
    CardWidget* m_cmdCard{nullptr};
    CardWidget* m_resultsCard{nullptr};
    QWidget* m_rightColumn{nullptr};

    // Delegates
    CountryItemDelegate* m_countryDelegate{nullptr};
    TargetItemDelegate* m_targetDelegate{nullptr};

    // Sidebar - Targets Section
    QLineEdit* m_countrySearchEdit{nullptr};
    QAction* m_countrySearchAction{nullptr};
    QListWidget* m_countryListWidget{nullptr};
    QPushButton* m_btnAddCountries{nullptr};
    QLineEdit* m_manualTargetEdit{nullptr};
    QPushButton* m_btnAddManual{nullptr};
    QListWidget* m_targetsListWidget{nullptr};
    QPushButton* m_btnEditTarget{nullptr};
    QPushButton* m_btnRemoveTarget{nullptr};
    QPushButton* m_btnClearTargets{nullptr};
    QLabel* m_targetsSummaryLabel{nullptr};
    QCheckBox* m_allIpv4Check{nullptr};
    QPushButton* m_btnImportTargets{nullptr};
    QLineEdit* m_excludeEdit{nullptr};
    QLineEdit* m_excludeFileEdit{nullptr};

    std::vector<TargetItem> m_targets;
    std::set<std::string> m_pickedCountryCodes;

    // Sidebar - Ports Section
    QComboBox* m_portPresetCombo{nullptr};
    QLineEdit* m_portsEdit{nullptr};
    QLineEdit* m_topPortsEdit{nullptr};
    QLineEdit* m_excludePortsEdit{nullptr};
    QCheckBox* m_tcpCheck{nullptr};
    QCheckBox* m_udpCheck{nullptr};
    QCheckBox* m_sctpCheck{nullptr};
    QCheckBox* m_oprotoCheck{nullptr};
    QCheckBox* m_pingCheck{nullptr};
    QCheckBox* m_arpCheck{nullptr};

    // Sidebar - Rate Section
    QLineEdit* m_rateEdit{nullptr};
    QLineEdit* m_retriesEdit{nullptr};
    QLineEdit* m_waitEdit{nullptr};
    QLineEdit* m_seedEdit{nullptr};
    QLineEdit* m_shardEdit{nullptr};
    QLineEdit* m_ttlEdit{nullptr};
    QLineEdit* m_blackrockEdit{nullptr};
    QLineEdit* m_rtipEdit{nullptr};
    QCheckBox* m_offlineCheck{nullptr};
    QCheckBox* m_infiniteCheck{nullptr};
    QCheckBox* m_sendqCheck{nullptr};
    QCheckBox* m_noresetCheck{nullptr};
    QLineEdit* m_extraArgsEdit{nullptr};

    // Sidebar - Network Section
    QComboBox* m_adapterCombo{nullptr};
    QLineEdit* m_aipEdit{nullptr};
    QLineEdit* m_aportEdit{nullptr};
    QLineEdit* m_amacEdit{nullptr};
    QLineEdit* m_rmacEdit{nullptr};
    QLineEdit* m_vlanEdit{nullptr};
    QLineEdit* m_rmac6Edit{nullptr};

    // Sidebar - Banners Section
    QCheckBox* m_bannersCheck{nullptr};
    QWidget* m_bannerNoteWidget{nullptr};
    QLabel* m_bannerNoteLabel{nullptr};
    QCheckBox* m_capCertCheck{nullptr};
    QCheckBox* m_capServernameCheck{nullptr};
    QCheckBox* m_capHtmlCheck{nullptr};
    QCheckBox* m_capHeartbleedCheck{nullptr};
    QCheckBox* m_capTicketbleedCheck{nullptr};
    QCheckBox* m_rawudpCheck{nullptr};
    QComboBox* m_helloKindCombo{nullptr};
    QComboBox* m_vulnCombo{nullptr};
    QLineEdit* m_nocapEdit{nullptr};
    QLineEdit* m_helloEdit{nullptr};
    QLineEdit* m_helloFileEdit{nullptr};
    QLineEdit* m_helloTimeoutEdit{nullptr};
    QLineEdit* m_ctimeoutEdit{nullptr};
    QLineEdit* m_httpMethodEdit{nullptr};
    QLineEdit* m_httpVersionEdit{nullptr};
    QLineEdit* m_httpUrlEdit{nullptr};
    QLineEdit* m_httpHostEdit{nullptr};
    QLineEdit* m_httpUaEdit{nullptr};
    QLineEdit* m_httpCookieEdit{nullptr};
    QLineEdit* m_httpHeaderEdit{nullptr};
    QLineEdit* m_httpPayloadEdit{nullptr};
    QLineEdit* m_nmapplEdit{nullptr};
    QLineEdit* m_pcapplEdit{nullptr};

    // Sidebar - Output Section
    QComboBox* m_fmtCombo{nullptr};
    QLineEdit* m_outfileEdit{nullptr};
    QCheckBox* m_openOnlyCheck{nullptr};
    QCheckBox* m_flushCheck{nullptr};
    QCheckBox* m_appendCheck{nullptr};
    QCheckBox* m_pktraceCheck{nullptr};
    QLineEdit* m_rotateEdit{nullptr};
    QLineEdit* m_rotdirEdit{nullptr};
    QLineEdit* m_pcapEdit{nullptr};
    QLineEdit* m_rotOffsetEdit{nullptr};
    QLineEdit* m_rotSizeEdit{nullptr};
    QLineEdit* m_showEdit{nullptr};
    QLineEdit* m_noshowEdit{nullptr};
    QLineEdit* m_stylesheetEdit{nullptr};
    QCheckBox* m_noStylesheetCheck{nullptr};
    QCheckBox* m_reasonCheck{nullptr};
    QCheckBox* m_interactiveCheck{nullptr};
    QCheckBox* m_nostatusCheck{nullptr};
    QCheckBox* m_ndjsonStatusCheck{nullptr};
    QCheckBox* m_jsonStatusCheck{nullptr};
    QComboBox* m_verboseCombo{nullptr};

    // Sidebar - Mode Section
    QComboBox* m_modeCombo{nullptr};

    // Sidebar - Paths Section
    QLineEdit* m_outdirEdit{nullptr};
    QLineEdit* m_binEdit{nullptr};
    QLineEdit* m_conffileEdit{nullptr};
    QLineEdit* m_datadirEdit{nullptr};
    QLineEdit* m_probesEdit{nullptr};
    QLineEdit* m_scriptEdit{nullptr};

    // Sidebar - Advanced Section
    QLineEdit* m_mssEdit{nullptr};
    QLineEdit* m_minpktEdit{nullptr};
    QLineEdit* m_bpfEdit{nullptr};
    QLineEdit* m_datalenEdit{nullptr};
    QLineEdit* m_wscaleEdit{nullptr};
    QLineEdit* m_tsechoEdit{nullptr};
    QCheckBox* m_sackokCheck{nullptr};
    QCheckBox* m_badsumCheck{nullptr};

    // Action Bar buttons
    QPushButton* m_btnStart{nullptr};
    QPushButton* m_btnStop{nullptr};
    QPushButton* m_btnResume{nullptr};
    QPushButton* m_btnOpenFolder{nullptr};
    QPushButton* m_btnStartNarrow{nullptr};
    QPushButton* m_btnStopNarrow{nullptr};
    QPushButton* m_btnResumeNarrow{nullptr};
    QPushButton* m_btnOpenFolderNarrow{nullptr};

    // Right Column - Command Preview
    TerminalCodeBox* m_terminalBox{nullptr};
    QLabel* m_commandTextLabel{nullptr};
    QAbstractButton* m_btnCopy{nullptr};
    QVBoxLayout* m_warningsLayout{nullptr};
    QWidget* m_warningsContainer{nullptr};

    // Right Column - Tabs (Results / Log)
    QPushButton* m_pillResults{nullptr};
    QPushButton* m_pillLog{nullptr};
    SlidingStackedWidget* m_tabStack{nullptr};
    QWidget* m_tabResultsWidget{nullptr};
    QWidget* m_tabLogWidget{nullptr};
    QLineEdit* m_filterResultsEdit{nullptr};
    QAction* m_filterResultsAction{nullptr};
    QPushButton* m_btnExport{nullptr};
    QPushButton* m_btnClear{nullptr};
    QTableWidget* m_resultsTable{nullptr};
    std::unordered_set<std::string> m_discoveredKeys;
    QPlainTextEdit* m_logEdit{nullptr};
    int m_currentTab{0};

    // Footer
    QWidget* m_footerWidget{nullptr};
    QLabel* m_footerDot{nullptr};
    QLabel* m_footerStateLabel{nullptr};
    QProgressBar* m_footerProgressBar{nullptr};
    QLabel* m_footerPctLabel{nullptr};
    QLabel* m_footerEtaLabel{nullptr};
    QLabel* m_footerKppsLabel{nullptr};
    QLabel* m_footerFoundLabel{nullptr};

    bool m_menuOpen{false};
    qreal m_drawerProgress{0.0};
    qreal m_drawerStart{0.0};
    qreal m_drawerTarget{0.0};
    HighHzAnimator m_drawerAnim;
    bool m_resumeMode{false};
    BuiltCommand m_currentBuilt;
    std::vector<CollapsibleSection*> m_sections;
    FluentScrollBarOverlay* m_sidebarScrollOverlay{nullptr};
    FluentScrollBarOverlay* m_countryScrollOverlay{nullptr};
    FluentScrollBarOverlay* m_targetsScrollOverlay{nullptr};
    FluentScrollBarOverlay* m_cmdScrollOverlay{nullptr};
    FluentScrollBarOverlay* m_tableScrollOverlay{nullptr};
    FluentScrollBarOverlay* m_logScrollOverlay{nullptr};

    int m_masscanState{0};
    QString m_masscanInfo{"not found"};
    int m_npcapState{0};
    QString m_npcapInfo{"not found"};

    bool m_isDownloadingMasscan{false};
    bool m_isDownloadingNpcap{false};
    QNetworkAccessManager* m_netManager{nullptr};
};

} // namespace masscan::ui
