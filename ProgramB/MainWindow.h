#pragma once

#include <QMainWindow>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>

class HttpServer;
class WebSocketServer;
class FunctionRegistry;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onStartStop();
    void appendLog(const QString &msg);

private:
    void setupUi();
    void setupServers();
    void registerFunctions();
    void updateServiceStatus();

    // Servers
    HttpServer      *m_httpServer  = nullptr;
    WebSocketServer *m_wsServer    = nullptr;
    FunctionRegistry *m_registry   = nullptr;

    // UI - Service control
    QLabel    *m_statusLabel   = nullptr;
    QLabel    *m_httpPortLabel = nullptr;
    QLabel    *m_wsPortLabel   = nullptr;
    QSpinBox  *m_portSpinBox   = nullptr;
    QPushButton *m_startStopBtn = nullptr;

    // UI - Log
    QTextEdit *m_logView = nullptr;

    bool m_running = false;
};
