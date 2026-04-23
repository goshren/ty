#include "mainwindow.h"
#include <QDebug>
#include <QDateTime>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // 设置窗口基本属性
    this->setWindowTitle("辐射与GPS轨迹监控终端");
    this->resize(1000, 650);

    // 1. 初始化界面
    initUI();

    // 2. 初始化网络
    tcpSocket = new QTcpSocket(this);
    connect(tcpSocket, &QTcpSocket::connected, this, &MainWindow::onConnected);
    connect(tcpSocket, &QTcpSocket::disconnected, this, &MainWindow::onDisconnected);
    connect(tcpSocket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
            this, &MainWindow::onSocketError);
    connect(tcpSocket, &QTcpSocket::readyRead, this, &MainWindow::onReadyRead);
}

MainWindow::~MainWindow()
{
    if (tcpSocket->isOpen()) {
        tcpSocket->disconnectFromHost();
    }
}

void MainWindow::initUI()
{
    QWidget *centralWidget = new QWidget(this);
    this->setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // ==========================================
    // 区域 1: 网络连接设置
    // ==========================================
    QGroupBox *grpNetwork = new QGroupBox("通信设置");
    QHBoxLayout *layNetwork = new QHBoxLayout(grpNetwork);

    layNetwork->addWidget(new QLabel("IP 地址:"));
    leIp = new QLineEdit("196.168.1.190"); // 替换为您下位机的默认IP
    layNetwork->addWidget(leIp);

    layNetwork->addWidget(new QLabel("端口:"));
    lePort = new QLineEdit("8888");
    lePort->setFixedWidth(80);
    layNetwork->addWidget(lePort);

    btnConnect = new QPushButton("连接下位机");
    connect(btnConnect, &QPushButton::clicked, this, &MainWindow::onConnectButtonClicked);
    layNetwork->addWidget(btnConnect);

    lblTime = new QLabel("数据时间: 暂无");
    lblTime->setStyleSheet("margin-left: 20px; color: #555;");
    layNetwork->addWidget(lblTime);
    layNetwork->addStretch();

    mainLayout->addWidget(grpNetwork);

    // ==========================================
    // 区域 2: 核心内容区 (左右并排布局)
    // ==========================================
    QHBoxLayout *layMiddle = new QHBoxLayout();

    // ------------------------------------------
    // 左侧：百度地图展示区
    // ------------------------------------------
    QGroupBox *grpMap = new QGroupBox("实时轨迹地图");
    QVBoxLayout *layMap = new QVBoxLayout(grpMap);

    mapView = new QWebEngineView(this);
    mapView->setUrl(QUrl("qrc:/map.html"));
    layMap->addWidget(mapView);

    layMiddle->addWidget(grpMap, 5);

    // ------------------------------------------
    // 右侧：数据面板 (垂直堆叠)
    // ------------------------------------------
    QVBoxLayout *layRightPanel = new QVBoxLayout();

    // 模块 1: GPS 简易状态
    QGroupBox *grpGps = new QGroupBox("定位数据");
    QHBoxLayout *layGps = new QHBoxLayout(grpGps);
    lblGpsStatus = new QLabel("未定位");
    lblGpsLat = new QLabel("纬度: 0.000000");
    lblGpsLon = new QLabel("经度: 0.000000");
    layGps->addWidget(lblGpsStatus);
    layGps->addWidget(lblGpsLat);
    layGps->addWidget(lblGpsLon);
    layRightPanel->addWidget(grpGps);

    // 模块 2: 实时仪表盘展示
    QGroupBox *grpGauges = new QGroupBox("实时辐射监测");
    QHBoxLayout *layGauges = new QHBoxLayout(grpGauges);

    // 创建辐射仪表盘：标题，单位，最小值，最大值(假设5.0为常规量程上限)，主题颜色(红色警告色)
    gaugeRadiation = new DashboardGauge("辐射剂量", "μSv", 0, 5.0, QColor(244, 67, 54));
    layGauges->addWidget(gaugeRadiation);
    layRightPanel->addWidget(grpGauges, 1);

    // 模块 3: 动态历史曲线图 (显示最近 60 帧)
    QGroupBox *grpChart = new QGroupBox("辐射趋势曲线");
    QVBoxLayout *layChart = new QVBoxLayout(grpChart);

    trendChart = new QChart();
    seriesRadiation = new QLineSeries();
    seriesRadiation->setName("辐射剂量 (μSv)");

    trendChart->addSeries(seriesRadiation);

    axisX = new QValueAxis();
    axisX->setRange(0, 60); // 初始显示 0-60 帧
    axisX->setLabelFormat("%d");
    axisX->setTitleText("时间 (帧)");

    axisY = new QValueAxis();
    axisY->setRange(0, 5.0); // Y轴范围
    axisY->setTitleText("μSv");

    trendChart->addAxis(axisX, Qt::AlignBottom);
    trendChart->addAxis(axisY, Qt::AlignLeft);
    seriesRadiation->attachAxis(axisX);
    seriesRadiation->attachAxis(axisY);

    trendChart->layout()->setContentsMargins(0, 0, 0, 0);
    trendChart->setBackgroundRoundness(0);

    chartView = new QChartView(trendChart);
    chartView->setRenderHint(QPainter::Antialiasing);
    layChart->addWidget(chartView);

    timeFrameIndex = 0;
    layRightPanel->addWidget(grpChart, 2);

    layMiddle->addLayout(layRightPanel, 3);

    // ==========================================
    // 区域 3: 运行日志区
    // ==========================================
    QGroupBox *grpLog = new QGroupBox("运行日志");
    QVBoxLayout *layLog = new QVBoxLayout(grpLog);
    txtLog = new QTextEdit();
    txtLog->setReadOnly(true);
    txtLog->setMaximumHeight(100);
    layLog->addWidget(txtLog);

    mainLayout->addWidget(grpLog);

    appendLog("系统初始化完成，等待连接...");
}

void MainWindow::appendLog(const QString &msg)
{
    QString timeStr = QDateTime::currentDateTime().toString("[HH:mm:ss] ");
    txtLog->append(timeStr + msg);
}

void MainWindow::onConnectButtonClicked()
{
    if (tcpSocket->state() == QAbstractSocket::UnconnectedState) {
        QString ip = leIp->text();
        quint16 port = lePort->text().toUShort();
        tcpSocket->connectToHost(ip, port);
        appendLog("正在连接到 " + ip + ":" + QString::number(port) + "...");
    } else {
        tcpSocket->disconnectFromHost();
    }
}

void MainWindow::onConnected()
{
    btnConnect->setText("断开连接");
    appendLog("成功连接到下位机！");
}

void MainWindow::onDisconnected()
{
    btnConnect->setText("连接下位机");
    appendLog("已与下位机断开。");
}

void MainWindow::onSocketError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    appendLog("网络错误: " + tcpSocket->errorString());
}

void MainWindow::onReadyRead()
{
    while (tcpSocket->canReadLine()) {
        QByteArray lineData = tcpSocket->readLine().trimmed();
        if (!lineData.isEmpty()) {
            parseSensorData(lineData);
        }
    }
}

// ==========================================
// 核心解析逻辑：适配最新的 JSON 格式
// ==========================================
void MainWindow::parseSensorData(const QByteArray &jsonData)
{
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) return;

    if (jsonDoc.isObject()) {
        QJsonObject rootObj = jsonDoc.object();

        // 1. 解析时间
        if (rootObj.contains("time")) {
            lblTime->setText("数据时间: " + rootObj.value("time").toString());
        }

        // 2. 解析辐射数据并更新仪表盘和折线图
        if (rootObj.contains("radiation")) {
            QJsonObject radObj = rootObj.value("radiation").toObject();
            if (radObj.contains("dose_usv")) {
                double dose = radObj.value("dose_usv").toDouble();

                // 更新仪表盘
                gaugeRadiation->setValue(dose);

                // 更新历史折线图
                timeFrameIndex++;
                seriesRadiation->append(timeFrameIndex, dose);

                // 超过60帧后，X轴向右平移，实现滚动效果
                if (timeFrameIndex > 60) {
                    axisX->setRange(timeFrameIndex - 60, timeFrameIndex);
                    seriesRadiation->remove(0); // 移除旧数据防止内存泄漏
                }
            }
        }

        // 3. 解析 GPS 数据并驱动地图更新
        if (rootObj.contains("gps")) {
            QJsonObject gpsObj = rootObj.value("gps").toObject();
            int valid = gpsObj.value("valid").toInt();
            double lat = gpsObj.value("lat").toDouble();
            double lon = gpsObj.value("lon").toDouble();

            lblGpsLat->setText(QString("纬度: %1").arg(lat, 0, 'f', 6));
            lblGpsLon->setText(QString("经度: %1").arg(lon, 0, 'f', 6));

            if (valid == 1) {
                lblGpsStatus->setText("已定位");
                lblGpsStatus->setStyleSheet("color: green; font-weight: bold;");

                // 调用 map.html 中的 updateRobotPosition 函数
                if (mapView && mapView->page()) {
                    QString jsCode = QString("updateRobotPosition(%1, %2);").arg(lon, 0, 'f', 6).arg(lat, 0, 'f', 6);
                    mapView->page()->runJavaScript(jsCode);
                }
            } else {
                lblGpsStatus->setText("未定位 (valid: 0)");
                lblGpsStatus->setStyleSheet("color: red; font-weight: bold;");
            }
        }
    }
}
