#include "dashboardgauge.h"
#include <QFontMetrics>

DashboardGauge::DashboardGauge(const QString &title, const QString &unit, double min, double max, QColor color, QWidget *parent)
    : QWidget(parent), m_title(title), m_unit(unit), m_min(min), m_max(max), m_value(min), m_themeColor(color)
{
    setMinimumSize(120, 120); // 保证仪表盘最小尺寸
}

void DashboardGauge::setValue(double val)
{
    // 限制在最大最小值范围内
    if (val < m_min) val = m_min;
    if (val > m_max) val = m_max;
    m_value = val;
    update(); // 触发重绘
}

void DashboardGauge::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing); // 抗锯齿，使边缘平滑

    int width = this->width();
    int height = this->height();
    int side = qMin(width, height);

    // 将坐标系平移到中心，并缩放
    painter.translate(width / 2.0, height / 2.0);
    painter.scale(side / 200.0, side / 200.0);

    // 1. 绘制背景圆弧 (灰色)
    QRectF rect(-80, -80, 160, 160);
    QPen bgPen(QColor(220, 220, 220), 12, Qt::SolidLine, Qt::RoundCap);
    painter.setPen(bgPen);
    painter.drawArc(rect, -45 * 16, 270 * 16); // Qt中角度以 1/16 度为单位

    // 2. 绘制当前值圆弧 (主题色)
    double ratio = (m_value - m_min) / (m_max - m_min);
    int spanAngle = 270 * ratio * 16;
    QPen valPen(m_themeColor, 12, Qt::SolidLine, Qt::RoundCap);
    painter.setPen(valPen);
    painter.drawArc(rect, 225 * 16, -spanAngle); // 从左下角开始逆时针画（负角度代表顺时针）

    // 3. 绘制数值和单位
    painter.setPen(Qt::black);
    QFont valFont("Arial", 22, QFont::Bold);
    painter.setFont(valFont);
    QString valStr = QString::number(m_value, 'f', 2);
    painter.drawText(QRectF(-80, -20, 160, 40), Qt::AlignCenter, valStr);

    QFont unitFont("Arial", 10);
    painter.setFont(unitFont);
    painter.drawText(QRectF(-80, 20, 160, 30), Qt::AlignCenter, m_unit);

    // 4. 绘制标题
    painter.setPen(QColor(100, 100, 100));
    painter.drawText(QRectF(-80, 50, 160, 30), Qt::AlignCenter, m_title);
}
