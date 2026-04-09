#ifndef DASHBOARDGAUGE_H
#define DASHBOARDGAUGE_H

#include <QWidget>
#include <QPainter>
#include <QPainterPath>

class DashboardGauge : public QWidget
{
    Q_OBJECT
public:
    explicit DashboardGauge(const QString &title, const QString &unit, double min, double max, QColor color, QWidget *parent = nullptr);

    void setValue(double val);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString m_title;
    QString m_unit;
    double m_min;
    double m_max;
    double m_value;
    QColor m_themeColor;
};

#endif // DASHBOARDGAUGE_H
