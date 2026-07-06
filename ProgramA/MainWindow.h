#pragma once

#include <QMainWindow>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QRadioButton>
#include <QMap>

class HttpClient;
class WebSocketClient;
struct RpcRequest;
struct RpcResponse;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onModeChanged();
    void onFunctionChanged(const QString &name);
    void onCallClicked();
    void onHttpResponse(const RpcResponse &response);
    void onWsResponse(const RpcResponse &response);
    void onError(const QString &error);
    void appendLog(const QString &msg);

private:
    void setupUi();
    void setupConnections();
    void connectWebSocket();
    void sendRequest();
    void displayResponse(const RpcResponse &response);

    // Communication
    HttpClient      *m_httpClient      = nullptr;
    WebSocketClient *m_wsClient        = nullptr;

    // UI - Connection
    QLineEdit   *m_ipEdit       = nullptr;
    QSpinBox    *m_httpPortSpin = nullptr;
    QSpinBox    *m_wsPortSpin   = nullptr;
    QRadioButton *m_syncRadio   = nullptr;
    QRadioButton *m_asyncRadio  = nullptr;
    QLabel      *m_connStatus   = nullptr;

    // UI - Function call
    QComboBox   *m_funcCombo    = nullptr;
    QTextEdit   *m_paramsEdit   = nullptr;
    QPushButton *m_callBtn      = nullptr;

    // UI - Results
    QTextEdit   *m_resultView   = nullptr;
    QTextEdit   *m_logView      = nullptr;

    // Function parameter templates
    QMap<QString, QString> m_paramTemplates;
    bool m_wsConnected = false;
};
