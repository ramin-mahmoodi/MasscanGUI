#pragma once

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>

namespace masscan::ui {

class HighHzAnimator;
class ChevronWidget;

class CollapsibleSection : public QWidget {
    Q_OBJECT

public:
    explicit CollapsibleSection(const QString& title, const char* iconPath = nullptr, QWidget* parent = nullptr, bool expanded = true);

    void setContentLayout(QLayout* layout);
    void setExpanded(bool expanded, bool animate = true);
    void applyTheme(bool isDark);
    [[nodiscard]] bool isExpanded() const { return m_expanded; }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    bool hasHeightForWidth() const override;
    int heightForWidth(int w) const override;

    void onContentContainerResized(const QSize& sz);

signals:
    void toggled(bool expanded);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void updateHeaderIcons();
    void updateAnimationStep(double p);
    void finishAnimation();

    class ContentContainer : public QWidget {
    public:
        explicit ContentContainer(CollapsibleSection* section);
        void setDark(bool isDark) { m_isDark = isDark; update(); }
        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;
    protected:
        void paintEvent(QPaintEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
    private:
        CollapsibleSection* m_section{nullptr};
        bool m_isDark{true};
    };

    QLabel* m_iconLabel{nullptr};
    QLabel* m_titleLabel{nullptr};
    ChevronWidget* m_toggleIcon{nullptr};
    QWidget* m_headerWidget{nullptr};
    ContentContainer* m_contentWidget{nullptr};
    QWidget* m_innerWidget{nullptr};
    QLayout* m_contentLayout{nullptr};
    QVBoxLayout* m_mainLayout{nullptr};

    HighHzAnimator* m_animator{nullptr};

    bool m_expanded{true};
    bool m_isDark{true};
    bool m_headerHovered{false};
    const char* m_iconPath{nullptr};

    double m_animProgress{1.0};
    double m_startProgress{1.0};
    double m_targetProgress{1.0};
    int m_fullContentHeight{0};
};

} // namespace masscan::ui
