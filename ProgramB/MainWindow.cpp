#include "MainWindow.h"
#include "HttpServer.h"
#include "WebSocketServer.h"
#include "FunctionRegistry.h"
#include "BusinessFunctions.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QDateTime>
#include <QApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Program B - Function Executor");
    resize(700, 550);

    m_registry = new FunctionRegistry();
    registerFunctions();

    setupUi();
    setupServers();
}

MainWindow::~MainWindow() {}

void MainWindow::setupUi()
{
    auto *central = new QWidget(this);
    setCentralWidget(central);
    auto *mainLayout = new QVBoxLayout(central);

    // --- Service Control Group ---
    auto *ctrlGroup = new QGroupBox("Service Control", central);
    auto *ctrlLayout = new QHBoxLayout(ctrlGroup);

    ctrlLayout->addWidget(new QLabel("Port:"));

    m_portSpinBox = new QSpinBox();
    m_portSpinBox->setRange(1024, 65535);
    m_portSpinBox->setValue(8080);
    ctrlLayout->addWidget(m_portSpinBox);

    m_startStopBtn = new QPushButton("Start Services");
    m_startStopBtn->setMinimumWidth(120);
    connect(m_startStopBtn, &QPushButton::clicked,
            this, &MainWindow::onStartStop);
    ctrlLayout->addWidget(m_startStopBtn);

    m_statusLabel = new QLabel("Status: Stopped");
    m_statusLabel->setStyleSheet("font-weight: bold; color: #c0392b;");
    ctrlLayout->addWidget(m_statusLabel);

    ctrlLayout->addStretch();

    m_httpPortLabel = new QLabel("HTTP: --");
    ctrlLayout->addWidget(m_httpPortLabel);

    m_wsPortLabel = new QLabel("WebSocket: --");
    ctrlLayout->addWidget(m_wsPortLabel);

    mainLayout->addWidget(ctrlGroup);

    // --- Log Group ---
    auto *logGroup = new QGroupBox("Call Log", central);
    auto *logLayout = new QVBoxLayout(logGroup);

    m_logView = new QTextEdit();
    m_logView->setReadOnly(true);
    m_logView->setFont(QFont("Consolas", 10));
    logLayout->addWidget(m_logView);

    mainLayout->addWidget(logGroup, 1);
}

void MainWindow::setupServers()
{
    m_httpServer = new HttpServer(this);
    m_wsServer   = new WebSocketServer(this);

    m_httpServer->setFunctionRegistry(m_registry);
    m_wsServer->setFunctionRegistry(m_registry);
    m_httpServer->setWebSocketServer(m_wsServer);

    connect(m_httpServer, &HttpServer::logMessage,
            this, &MainWindow::appendLog);
    connect(m_wsServer, &WebSocketServer::logMessage,
            this, &MainWindow::appendLog);

    // Forward HTTP request info to log
    connect(m_httpServer, &HttpServer::requestReceived, this,
            [this](const QString &clientId, const QString &function,
                   const QString &params) {
                appendLog(QString("Request: client=%1 func=%2 params=%3")
                              .arg(clientId, function, params));
            });
}

void MainWindow::registerFunctions()
{
    m_registry->registerFunction("add",         BusinessFunctions::add);
    m_registry->registerFunction("subtract",    BusinessFunctions::subtract);
    m_registry->registerFunction("multiply",    BusinessFunctions::multiply);
    m_registry->registerFunction("divide",      BusinessFunctions::divide);
    m_registry->registerFunction("echo",        BusinessFunctions::echo);
    m_registry->registerFunction("getTime",     BusinessFunctions::getTime);
    m_registry->registerFunction("getVersion",  BusinessFunctions::getVersion);
    m_registry->registerFunction("arraySum",    BusinessFunctions::arraySum);
    m_registry->registerFunction("toUpperCase", BusinessFunctions::toUpperCase);
}

void MainWindow::onStartStop()
{
    if (m_running) {
        m_httpServer->stop();
        m_wsServer->stop();
        m_running = false;
        m_startStopBtn->setText("Start Services");
        updateServiceStatus();
    } else {
        quint16 port = static_cast<quint16>(m_portSpinBox->value());

        // Start HTTP server on specified port
        if (!m_httpServer->start(port)) {
            appendLog("ERROR: Failed to start HTTP server");
            return;
        }

        // Start WebSocket server on port + 1
        if (!m_wsServer->start(port + 1)) {
            appendLog("ERROR: Failed to start WebSocket server");
            m_httpServer->stop();
            return;
        }

        m_running = true;
        m_startStopBtn->setText("Stop Services");
        updateServiceStatus();
    }
}

void MainWindow::appendLog(const QString &msg)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    m_logView->append(QString("[%1] %2").arg(timestamp, msg));
}

void MainWindow::updateServiceStatus()
{
    if (m_running) {
        m_statusLabel->setText("Status: Running");
        m_statusLabel->setStyleSheet("font-weight: bold; color: #27ae60;");
        m_httpPortLabel->setText(
            QString("HTTP: %1").arg(m_httpServer->port()));
        m_wsPortLabel->setText(
            QString("WebSocket: %1").arg(m_wsServer->port()));
        m_portSpinBox->setEnabled(false);
    } else {
        m_statusLabel->setText("Status: Stopped");
        m_statusLabel->setStyleSheet("font-weight: bold; color: #c0392b;");
        m_httpPortLabel->setText("HTTP: --");
        m_wsPortLabel->setText("WebSocket: --");
        m_portSpinBox->setEnabled(true);
    }
}
