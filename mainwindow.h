#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

// 引入 UI 控件和布局头文件
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout> // 新增：网格布局用于对齐按钮
#include <QGroupBox>
#include <QFormLayout>
#include <QTextEdit>

// 引入 WebEngine (用于百度地图)
#include <QWebEngineView>
#include <QWebChannel>
// --- 新增：仪表盘与图表头文件 ---
#include "dashboardgauge.h"
#include <QtCharts>
QT_CHARTS_USE_NAMESPACE // 必须加上这句，引入 QtCharts 命名空间

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

    // --- UI 控件指针声明 ---
    QLineEdit *leIp;
    QLineEdit *lePort;
    QPushButton *btnConnect;

    // 新增：地图视图
    QWebEngineView *mapView;

    QLabel *lblTemp;
    QLabel *lblSalinity;

    QLabel *lblDoSat;
    QLabel *lblDoConc;


    QLabel *lblGpsStatus;
    QLabel *lblGpsLat;
    QLabel *lblGpsLon;

    QTextEdit *txtLog;
    // --- 新增：仪表盘控件 ---
    DashboardGauge *gaugeTemp;
    DashboardGauge *gaugeSalinity;
    DashboardGauge *gaugeDoConc;

    // --- 新增：动态折线图相关对象 ---
    QChart *trendChart;
    QChartView *chartView;
    QLineSeries *seriesTemp;     // 温度曲线
    QLineSeries *seriesDoConc;   // 溶氧曲线
    QValueAxis *axisX;
    QValueAxis *axisY;
    int timeFrameIndex; // 用于记录当前接收了多少帧数据
    // --- 内部函数 ---
    void initUI();
    void parseSensorData(const QByteArray &jsonData);
    void appendLog(const QString &msg);

    // 通用的继电器控制指令下发函数
    void sendRelayCommand(int relayId, const QString &action);
    // NMEA 经纬度转换工具函数
    double convertNmeaToDecimal(const QString &nmeaStr);
};

#endif // MAINWINDOW_H
