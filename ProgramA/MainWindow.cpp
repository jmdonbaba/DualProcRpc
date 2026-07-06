#include "MainWindow.h"
#include "HttpClient.h"
#include "WebSocketClient.h"
#include "shared/Protocol.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QButtonGroup>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QApplication>
#include <QUuid>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Program A - RPC Caller");
    resize(750, 650);

    m_httpClient = new HttpClient(this);
    m_wsClient   = new WebSocketClient(this);

    setupUi();
    setupConnections();
    onModeChanged(); // initial state
}

MainWindow::~MainWindow() {}

void MainWindow::setupUi()
{
    auto *central = new QWidget(this);
    setCentralWidget(central);
    auto *mainLayout = new QVBoxLayout(central);

    // ===== Connection Config Group =====
    auto *connGroup = new QGroupBox("Connection Configuration", central);
    auto *connLayout = new QFormLayout(connGroup);

    m_ipEdit = new QLineEdit("127.0.0.1");
    connLayout->addRow("Program B IP:", m_ipEdit);

    m_httpPortSpin = new QSpinBox();
    m_httpPortSpin->setRange(1024, 65535);
    m_httpPortSpin->setValue(8080);
    connLayout->addRow("HTTP Port:", m_httpPortSpin);

    m_wsPortSpin = new QSpinBox();
    m_wsPortSpin->setRange(1024, 65535);
    m_wsPortSpin->setValue(8081);
    connLayout->addRow("WebSocket Port:", m_wsPortSpin);

    // Mode selector
    auto *modeLayout = new QHBoxLayout();
    auto *modeGroup = new QButtonGroup(this);
    m_syncRadio  = new QRadioButton("HTTP Sync");
    m_asyncRadio = new QRadioButton("WebSocket Async");
    modeGroup->addButton(m_syncRadio);
    modeGroup->addButton(m_asyncRadio);
    modeLayout->addWidget(m_syncRadio);
    modeLayout->addWidget(m_asyncRadio);
    modeLayout->addStretch();
    m_syncRadio->setChecked(true);
    connLayout->addRow("Mode:", modeLayout);

    m_connStatus = new QLabel("Not connected");
    connLayout->addRow("WS Status:", m_connStatus);

    mainLayout->addWidget(connGroup);

    // ===== Function Call Group =====
    auto *funcGroup = new QGroupBox("Function Call", central);
    auto *funcLayout = new QFormLayout(funcGroup);

    m_funcCombo = new QComboBox();
    // Functions must match ProgramB's FunctionRegistry
    m_funcCombo->addItems({
        "add", "subtract", "multiply", "divide",
        "echo", "getTime", "getVersion", "arraySum", "toUpperCase"
    });
    funcLayout->addRow("Function:", m_funcCombo);

    m_paramsEdit = new QTextEdit();
    m_paramsEdit->setFont(QFont("Consolas", 10));
    m_paramsEdit->setMaximumHeight(120);
    funcLayout->addRow("Parameters (JSON):", m_paramsEdit);

    m_callBtn = new QPushButton("Call Function");
    m_callBtn->setMinimumHeight(32);
    m_callBtn->setStyleSheet(
        "QPushButton { background-color: #3498db; color: white; font-weight: bold; "
        "border-radius: 4px; } "
        "QPushButton:hover { background-color: #2980b9; } "
        "QPushButton:disabled { background-color: #bdc3c7; }");
    funcLayout->addRow("", m_callBtn);

    mainLayout->addWidget(funcGroup);

    // ===== Result Group =====
    auto *resultGroup = new QGroupBox("Result", central);
    auto *resultLayout = new QVBoxLayout(resultGroup);

    m_resultView = new QTextEdit();
    m_resultView->setReadOnly(true);
    m_resultView->setFont(QFont("Consolas", 10));
    m_resultView->setMaximumHeight(150);
    resultLayout->addWidget(m_resultView);

    mainLayout->addWidget(resultGroup);

    // ===== Log Group =====
    auto *logGroup = new QGroupBox("Communication Log", central);
    auto *logLayout = new QVBoxLayout(logGroup);

    m_logView = new QTextEdit();
    m_logView->setReadOnly(true);
    m_logView->setFont(QFont("Consolas", 9));
    logLayout->addWidget(m_logView);

    mainLayout->addWidget(logGroup, 1);

    // Initialize parameter templates
    m_paramTemplates = {
        {"add",         "{\n  \"a\": 10,\n  \"b\": 20\n}"},
        {"subtract",    "{\n  \"a\": 50,\n  \"b\": 30\n}"},
        {"multiply",    "{\n  \"a\": 7,\n  \"b\": 8\n}"},
        {"divide",      "{\n  \"a\": 100,\n  \"b\": 5\n}"},
        {"echo",        "{\n  \"message\": \"Hello from Program A\"\n}"},
        {"getTime",     "{}"},
        {"getVersion",  "{}"},
        {"arraySum",    "{\n  \"array\": [1, 2, 3, 4, 5]\n}"},
        {"toUpperCase", "{\n  \"text\": \"hello world\"\n}"}
    };

    // Load template for initial function
    onFunctionChanged(m_funcCombo->currentText());
}

void MainWindow::setupConnections()
{
    connect(m_syncRadio, &QRadioButton::toggled,
            this, &MainWindow::onModeChanged);
    connect(m_funcCombo, &QComboBox::currentTextChanged,
            this, &MainWindow::onFunctionChanged);
    connect(m_callBtn, &QPushButton::clicked,
            this, &MainWindow::onCallClicked);

    // HTTP client signals
    connect(m_httpClient, &HttpClient::responseReceived,
            this, &MainWindow::onHttpResponse);
    connect(m_httpClient, &HttpClient::errorOccurred,
            this, &MainWindow::onError);
    connect(m_httpClient, &HttpClient::logMessage,
            this, &MainWindow::appendLog);

    // WebSocket client signals
    connect(m_wsClient, &WebSocketClient::connected, this, [this]() {
        m_wsConnected = true;
        m_connStatus->setText("WS Connected");
        m_connStatus->setStyleSheet("font-weight: bold; color: #27ae60;");
    });
    connect(m_wsClient, &WebSocketClient::disconnected, this, [this]() {
        m_wsConnected = false;
        m_connStatus->setText("WS Disconnected");
        m_connStatus->setStyleSheet("font-weight: bold; color: #c0392b;");
    });
    connect(m_wsClient, &WebSocketClient::responseReceived,
            this, &MainWindow::onWsResponse);
    connect(m_wsClient, &WebSocketClient::errorOccurred,
            this, &MainWindow::onError);
    connect(m_wsClient, &WebSocketClient::logMessage,
            this, &MainWindow::appendLog);
}

void MainWindow::onModeChanged()
{
    bool sync = m_syncRadio->isChecked();
    if (sync) {
        appendLog("[Mode] Switched to HTTP Sync mode");
    } else {
        appendLog("[Mode] Switched to WebSocket Async mode");
        if (!m_wsConnected) {
            connectWebSocket();
        }
    }
}

void MainWindow::onFunctionChanged(const QString &name)
{
    if (m_paramTemplates.contains(name)) {
        m_paramsEdit->setPlainText(m_paramTemplates[name]);
    }
}

void MainWindow::onCallClicked()
{
    if (m_asyncRadio->isChecked() && !m_wsConnected) {
        appendLog("[WARN] WebSocket not connected, attempting to connect first...");
        connectWebSocket();
        // Proceed with sync fallback — the WS result won't arrive this time
        // but the HTTP sync response still works
    }
    sendRequest();
}

void MainWindow::connectWebSocket()
{
    QString host = m_ipEdit->text().trimmed();
    if (host.isEmpty())
        host = "127.0.0.1";
    m_wsClient->connectToServer(host, static_cast<quint16>(m_wsPortSpin->value()));
}

void MainWindow::sendRequest()
{
    // Parse params JSON
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(
        m_paramsEdit->toPlainText().toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        m_resultView->setHtml(
            QString("<p style='color:red;'><b>Parameter JSON Error:</b> %1</p>")
                .arg(parseError.errorString()));
        return;
    }

    RpcRequest req;
    req.function  = m_funcCombo->currentText();
    req.params    = doc.object();
    req.requestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    req.clientId  = m_wsClient->clientId();
    req.mode      = m_asyncRadio->isChecked() ? "async" : "sync";

    m_resultView->clear();
    appendLog(QString(">> Calling: %1 (mode=%2)").arg(req.function, req.mode));

    QString host = m_ipEdit->text().trimmed();
    if (host.isEmpty())
        host = "127.0.0.1";

    m_httpClient->setBaseUrl(host, static_cast<quint16>(m_httpPortSpin->value()));
    m_httpClient->sendRequest(req);
}

void MainWindow::onHttpResponse(const RpcResponse &response)
{
    displayResponse(response);
}

void MainWindow::onWsResponse(const RpcResponse &response)
{
    displayResponse(response);
}

void MainWindow::onError(const QString &error)
{
    m_resultView->setHtml(
        QString("<p style='color:red;'><b>Error:</b> %1</p>").arg(error));
    appendLog(QString("[ERROR] %1").arg(error));
}

void MainWindow::displayResponse(const RpcResponse &response)
{
    QString html;
    if (response.status == "success") {
        html += "<h3 style='color:#27ae60;'>Success</h3>";
        if (response.data.isObject()) {
            QJsonObject obj = response.data.toObject();
            html += "<pre>" + QString::fromUtf8(
                        QJsonDocument(obj).toJson(QJsonDocument::Indented))
                    + "</pre>";
        } else if (response.data.isArray()) {
            html += "<pre>" + QString::fromUtf8(
                        QJsonDocument(response.data.toArray())
                            .toJson(QJsonDocument::Indented))
                    + "</pre>";
        } else if (response.data.isString()) {
            html += "<p>" + response.data.toString() + "</p>";
        } else if (response.data.isDouble()) {
            html += "<p>" + QString::number(response.data.toDouble()) + "</p>";
        } else if (response.data.isBool()) {
            html += "<p>" + QString(response.data.toBool() ? "true" : "false")
                    + "</p>";
        }
    } else {
        html += "<h3 style='color:#e74c3c;'>Error</h3>";
        html += "<p>" + response.error + "</p>";
    }
    html += "<p><small>Request ID: " + response.requestId + "</small></p>";
    m_resultView->setHtml(html);
}

void MainWindow::appendLog(const QString &msg)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    m_logView->append(QString("[%1] %2").arg(timestamp, msg));
}
