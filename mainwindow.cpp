#include "mainwindow.h"
#include <QDebug>
#include <QDateTime>
#include <QMessageBox>
#include <QDir>
#include <QApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // 设置窗口基本属性
    this->setWindowTitle("双体水下机器人控制终端");
    this->resize(800, 600);

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
    leIp = new QLineEdit("192.168.1.138");
    layNetwork->addWidget(leIp);

    layNetwork->addWidget(new QLabel("端口:"));
    lePort = new QLineEdit("8888");
    lePort->setFixedWidth(80);
    layNetwork->addWidget(lePort);

    btnConnect = new QPushButton("连接下位机");
    connect(btnConnect, &QPushButton::clicked, this, &MainWindow::onConnectButtonClicked);
    layNetwork->addWidget(btnConnect);
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
        // 直接使用 qrc 协议读取编译进程序内部的 html 文件
    mapView->setUrl(QUrl("qrc:/map.html"));
    layMap->addWidget(mapView);

    layMiddle->addWidget(grpMap, 5);

    // ------------------------------------------
    // 右侧：数据与控制面板 (垂直堆叠)
    // ------------------------------------------
    QVBoxLayout *layRightPanel = new QVBoxLayout();

    // 模块 1: 实时仪表盘展示 (横向排列 3 个仪表盘)
    QGroupBox *grpGauges = new QGroupBox("实时核心状态");
    QHBoxLayout *layGauges = new QHBoxLayout(grpGauges);

    // 参数依次为：标题，单位，量程最小值，量程最大值，主题颜色
    gaugeTemp = new DashboardGauge("水温", "℃", 0, 40, QColor(255, 87, 34)); // 橙色
    gaugeSalinity = new DashboardGauge("盐度", "PSU", 0, 40, QColor(33, 150, 243)); // 蓝色
    gaugeDoConc = new DashboardGauge("溶解氧", "mg/L", 0, 20, QColor(76, 175, 80)); // 绿色

    layGauges->addWidget(gaugeTemp);
    layGauges->addWidget(gaugeSalinity);
    layGauges->addWidget(gaugeDoConc);
    layRightPanel->addWidget(grpGauges, 1);

    // 模块 2: 动态历史曲线图 (显示最近 60 帧)
    QGroupBox *grpChart = new QGroupBox("环境趋势曲线");
    QVBoxLayout *layChart = new QVBoxLayout(grpChart);

    trendChart = new QChart();
    seriesTemp = new QLineSeries();
    seriesTemp->setName("水温 (℃)");
    seriesDoConc = new QLineSeries();
    seriesDoConc->setName("溶氧 (mg/L)");

    trendChart->addSeries(seriesTemp);
    trendChart->addSeries(seriesDoConc);

    axisX = new QValueAxis();
    axisX->setRange(0, 60); // 初始显示 0-60 帧
    axisX->setLabelFormat("%d");
    axisX->setTitleText("时间 (帧)");

    axisY = new QValueAxis();
    axisY->setRange(0, 40); // Y轴范围，包容水温(0-40)和溶氧(0-20)
    axisY->setTitleText("数值");

    trendChart->addAxis(axisX, Qt::AlignBottom);
    trendChart->addAxis(axisY, Qt::AlignLeft);
    seriesTemp->attachAxis(axisX);
    seriesTemp->attachAxis(axisY);
    seriesDoConc->attachAxis(axisX);
    seriesDoConc->attachAxis(axisY);

    // 隐藏图表自带的外边距，更紧凑
    trendChart->layout()->setContentsMargins(0, 0, 0, 0);
    trendChart->setBackgroundRoundness(0);

    chartView = new QChartView(trendChart);
    chartView->setRenderHint(QPainter::Antialiasing); // 抗锯齿
    layChart->addWidget(chartView);

    timeFrameIndex = 0; // 初始化帧计数器
    layRightPanel->addWidget(grpChart, 2); // 曲线图给大一点的权重

    // 模块 3: GPS 简易状态
    QGroupBox *grpGps = new QGroupBox("定位数据");
    QHBoxLayout *layGps = new QHBoxLayout(grpGps);
    lblGpsStatus = new QLabel("未定位");
    lblGpsLat = new QLabel("纬度: 0");
    lblGpsLon = new QLabel("经度: 0");
    layGps->addWidget(lblGpsStatus);
    layGps->addWidget(lblGpsLat);
    layGps->addWidget(lblGpsLon);
    layRightPanel->addWidget(grpGps);

    // 模块 4: 设备控制面板 (保持原有 Lambda 生成逻辑不变)
    QGroupBox *grpControl = new QGroupBox("设备控制面板");
    QGridLayout *layControl = new QGridLayout(grpControl);
    struct RelayInfo { int id; QString name; };
    RelayInfo relays[] = { {1, "增氧机"}, {2, "投喂器"}, {3, "补温器"}, {4, "换水器"} };
    for (int i = 0; i < 4; ++i) {
        QLabel *lblName = new QLabel(QString("继电器 %1: <b>%2</b>").arg(relays[i].id).arg(relays[i].name));
        QPushButton *btnOn = new QPushButton("打开");
        QPushButton *btnOff = new QPushButton("关闭");
        btnOn->setFixedWidth(60); btnOff->setFixedWidth(60);
        btnOn->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; border-radius: 3px; padding: 5px; }");
        btnOff->setStyleSheet("QPushButton { background-color: #f44336; color: white; border-radius: 3px; padding: 5px; }");
        layControl->addWidget(lblName, i, 0);
        layControl->addWidget(btnOn, i, 1);
        layControl->addWidget(btnOff, i, 2);
        int id = relays[i].id; QString name = relays[i].name;
        connect(btnOn, &QPushButton::clicked, this, [this, id, name]() { sendRelayCommand(id, "ON"); });
        connect(btnOff, &QPushButton::clicked, this, [this, id, name]() { sendRelayCommand(id, "OFF"); });
    }
    layRightPanel->addWidget(grpControl);

    // 把右侧装入主布局
    layMiddle->addLayout(layRightPanel, 3); // 给右侧布局稍微多一点权重，因为塞了图表

    // ==========================================
    // 区域 3: 运行日志区
    // ==========================================
    QGroupBox *grpLog = new QGroupBox("运行日志");
    QVBoxLayout *layLog = new QVBoxLayout(grpLog);
    txtLog = new QTextEdit();
    txtLog->setReadOnly(true);
    txtLog->setMaximumHeight(120);
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
    Q_UNUSED(error); // 消除警告
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

void MainWindow::sendRelayCommand(int relayId, const QString &action)
{
    if (tcpSocket->state() == QAbstractSocket::ConnectedState) {
        QString cmd = QString("RELAY %1 %2\n").arg(relayId).arg(action);
        tcpSocket->write(cmd.toUtf8());
        tcpSocket->flush();
    } else {
        QMessageBox::warning(this, "发送失败", "未连接到下位机，请先连接网络！");
    }
}

// ==========================================
// 数据解析与地图联动 (唯一正确的解析函数)
// ==========================================
void MainWindow::parseSensorData(const QByteArray &jsonData)
{
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) return;

    if (jsonDoc.isObject()) {
        QJsonObject rootObj = jsonDoc.object();
        double currentTemp = 0.0, currentDo = 0.0;
        bool hasData = false;

        // 1. 更新 CTD
        if (rootObj.contains("ctd")) {
            QJsonObject ctdObj = rootObj.value("ctd").toObject();
            currentTemp = ctdObj.value("t").toDouble();
            gaugeTemp->setValue(currentTemp); // 更新仪表盘
            gaugeSalinity->setValue(ctdObj.value("s").toDouble());
            hasData = true;
        }

        // 2. 更新 DO
        if (rootObj.contains("do")) {
            QJsonObject doObj = rootObj.value("do").toObject();
            currentDo = doObj.value("conc").toDouble();
            gaugeDoConc->setValue(currentDo); // 更新仪表盘
            hasData = true;
        }

        // 3. 核心：更新折线图 (超过 60 帧则滚动 X 轴)
        if (hasData) {
            timeFrameIndex++;
            seriesTemp->append(timeFrameIndex, currentTemp);
            seriesDoConc->append(timeFrameIndex, currentDo);

            if (timeFrameIndex > 60) {
                // X 轴整体向右平移 1 帧，实现滚动效果
                axisX->setRange(timeFrameIndex - 60, timeFrameIndex);
                // 为了防止内存无限增长，移除最旧的数据点
                seriesTemp->remove(0);
                seriesDoConc->remove(0);
            }
        }

        // 4. 解析 GPS 驱动地图 (原有逻辑不变)
        if (rootObj.contains("gps")) {
            QJsonObject gpsObj = rootObj.value("gps").toObject();
            QString status = gpsObj.value("st").toString();
            QString latStr = gpsObj.value("lat").toString();
            QString lonStr = gpsObj.value("lon").toString();

            lblGpsLat->setText("纬度: " + latStr);
            lblGpsLon->setText("经度: " + lonStr);

            if (status == "A") {
                lblGpsStatus->setText("已定位");
                lblGpsStatus->setStyleSheet("color: green; font-weight: bold;");
                double decLat = convertNmeaToDecimal(latStr);
                double decLon = convertNmeaToDecimal(lonStr);
                if (mapView && mapView->page()) {
                    QString jsCode = QString("updateRobotPosition(%1, %2);").arg(decLon, 0, 'f', 6).arg(decLat, 0, 'f', 6);
                    mapView->page()->runJavaScript(jsCode);
                }
            } else {
                lblGpsStatus->setText("搜索中 (V)");
                lblGpsStatus->setStyleSheet("color: red; font-weight: bold;");
            }
        }
    }
}

// ==========================================
// NMEA 格式转十进制度数
// ==========================================
double MainWindow::convertNmeaToDecimal(const QString &nmeaStr)
{
    QStringList parts = nmeaStr.split(" ", QString::SkipEmptyParts);
    if (parts.size() < 1) return 0.0;

    QString valStr = parts[0];
    if (valStr.isEmpty() || valStr == "0") return 0.0;

    double val = valStr.toDouble();
    int degrees = 0;
    double minutes = 0.0;

    if (val < 10000) {
        degrees = val / 100;
        minutes = val - (degrees * 100);
    } else {
        degrees = val / 100;
        minutes = val - (degrees * 100);
    }

    double decimal = degrees + (minutes / 60.0);

    if (parts.size() == 2) {
        QString dir = parts[1].toUpper();
        if (dir == "S" || dir == "W") {
            decimal = -decimal;
        }
    }

    return decimal;
}
