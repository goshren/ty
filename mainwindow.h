#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QTextEdit>

// 引入 WebEngine (用于百度地图)
#include <QWebEngineView>

// 引入仪表盘与图表头文件
#include "dashboardgauge.h"
#include <QtCharts>
QT_CHARTS_USE_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onConnected();
    void onDisconnected();
    void onSocketError(QAbstractSocket::SocketError error);
    void onReadyRead();
    void onConnectButtonClicked();

private:
    QTcpSocket *tcpSocket;

    // 网络与基础 UI 控件
    QLineEdit *leIp;
    QLineEdit *lePort;
    QPushButton *btnConnect;
    QTextEdit *txtLog;

    // 地图视图
    QWebEngineView *mapView;

    // GPS 状态标签
    QLabel *lblGpsStatus;
    QLabel *lblGpsLat;
    QLabel *lblGpsLon;
    QLabel *lblTime; // 新增：用于显示下位机时间

    // 辐射仪表盘
    DashboardGauge *gaugeRadiation;

    // 动态折线图相关对象
    QChart *trendChart;
    QChartView *chartView;
    QLineSeries *seriesRadiation;  // 辐射历史曲线
    QValueAxis *axisX;
    QValueAxis *axisY;
    int timeFrameIndex; // 用于记录当前接收了多少帧数据

    // 内部函数
    void initUI();
    void parseSensorData(const QByteArray &jsonData);
    void appendLog(const QString &msg);
};

#endif // MAINWINDOW_H
