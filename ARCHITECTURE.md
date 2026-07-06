# DualProcRpc 代码说明文档

## 一、项目概述

本项目实现了两个独立进程（程序A与程序B）之间基于网络协议的远程过程调用（RPC）。

- **程序A（调用发起端）**：通过HTTP/WebSocket向程序B发起函数调用并接收结果
- **程序B（功能执行端）**：监听HTTP/WebSocket端口，解析请求并执行业务函数，回传结果

## 二、目录结构

```
DualProcRpc/
├── CMakeLists.txt                  # CMake 构建配置
├── 项目需求说明书.md                # 原始需求文档
├── 代码说明文档.md                  # 本文档
├── shared/                         # 共享模块
│   ├── Protocol.h                  # 通信协议数据结构定义
│   └── Protocol.cpp                # JSON序列化/反序列化实现
├── ProgramA/                       # 程序A源码（调用端）
│   ├── main.cpp                    # 入口
│   ├── MainWindow.h/.cpp           # 主界面：连接配置、函数调用、结果展示
│   ├── HttpClient.h/.cpp           # HTTP同步调用客户端
│   └── WebSocketClient.h/.cpp      # WebSocket异步接收客户端
├── ProgramB/                       # 程序B源码（执行端）
│   ├── main.cpp                    # 入口
│   ├── MainWindow.h/.cpp           # 主界面：服务控制、调用日志
│   ├── HttpServer.h/.cpp           # HTTP/1.1服务端（基于QTcpServer）
│   ├── WebSocketServer.h/.cpp      # WebSocket推送服务端
│   ├── FunctionRegistry.h/.cpp     # 函数注册与调度中心
│   └── BusinessFunctions.h/.cpp    # 业务函数实现（9个内置函数）
└── build/                          # 编译输出目录
```

## 三、系统架构

### 3.1 通信架构图

```
┌──────────────────┐        HTTP POST (JSON)        ┌──────────────────┐
│                  │ ──────────────────────────────> │                  │
│   程序A (Caller)  │                                │  程序B (Executor) │
│                  │ <────────────────────────────── │                  │
│                  │     HTTP Response (JSON)        │                  │
│                  │                                 │                  │
│                  │ <══════════════════════════════ │                  │
│                  │    WebSocket Push (JSON)        │                  │
└──────────────────┘                                 └──────────────────┘
   HTTP Client                                         HTTP Server
   WebSocket Client                                    WebSocket Server
   (port: 8080 for HTTP)                               (port: 8081 for WS)
```

### 3.2 调用模式

| 模式 | 正向链路 | 反向链路 | 适用场景 |
|------|---------|---------|---------|
| **HTTP同步** | A → HTTP POST → B | B → HTTP Response → A | 快速请求/响应 |
| **WebSocket异步** | A → HTTP POST → B | B → WebSocket Push → A | 长时间计算、多次推送 |

## 四、核心模块详解

### 4.1 通信协议（shared/Protocol）

#### RpcRequest（请求报文）
```json
{
  "function": "add",
  "params": {"a": 10, "b": 20},
  "requestId": "uuid-string",
  "clientId": "client-uuid",
  "mode": "sync|async"
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| function | string | 目标函数名称 |
| params | object | 函数入参 |
| requestId | string | 请求唯一ID (UUID) |
| clientId | string | 客户端标识，用于WebSocket回推匹配 |
| mode | string | `sync`(HTTP响应) 或 `async`(WebSocket推送) |

#### RpcResponse（响应报文）
```json
{
  "status": "success|error",
  "data": {返回值},
  "error": "错误描述",
  "requestId": "uuid-string"
}
```

### 4.2 程序A核心模块

#### HttpClient — HTTP同步调用
- 基于 `QNetworkAccessManager` 实现
- `setBaseUrl(host, port)`: 配置目标地址，路径固定为 `/rpc`
- `sendRequest(RpcRequest)`: 发送POST请求，Body为JSON
- 信号 `responseReceived(RpcResponse)`: 收到响应时触发
- 信号 `errorOccurred(QString)`: 网络错误或JSON解析错误时触发

#### WebSocketClient — 异步推送接收
- 基于 `QWebSocket` 实现
- 连接成功后自动发送 `{"type":"register", "clientId":"xxx"}` 注册
- 接收到的消息若不带type字段，则解析为RpcResponse
- 具备自动重连机制（3秒间隔）
- `clientId()`: 返回唯一客户端标识符（UUID）

### 4.3 程序B核心模块

#### HttpServer — HTTP/1.1服务端
- 基于 `QTcpServer` + `QTcpSocket` 手动解析HTTP协议
- 请求解析状态机：ReadingRequest → ReadingBody → Complete
- 支持标准HTTP/1.1请求格式（request line + headers + body）
- 自动提取Content-Length以确定body长度
- 仅接受POST方法，返回Content-Type: application/json

**关键实现细节**：
- 每个TCP连接维护独立的接收缓冲区（`QMap<QTcpSocket*, ClientState>`）
- 粘包/拆包处理：持续缓冲直到接收到完整的HTTP消息
- 连接断开时自动清理缓冲区

#### WebSocketServer — WebSocket推送服务
- 基于 `QWebSocketServer` + `QWebSocket`
- 维护 clientId → QWebSocket 的映射表
- 支持WebSocket直接RPC调用（通过 `{"type":"rpc"}` 消息）
- `pushToClient(clientId, RpcResponse)`: 向指定客户端推送结果

#### FunctionRegistry — 函数注册中心
- 内部维护 `std::map<QString, RpcFunction>` 映射表
- `registerFunction(name, handler)`: 注册函数
- `invoke(name, params)`: 执行函数并返回结果
- `hasFunction(name)`: 检查函数是否存在
- 新增业务函数只需调用 `registerFunction()`，无需修改通信层代码

#### BusinessFunctions — 业务函数库
| 函数名 | 参数 | 返回值 | 说明 |
|--------|------|--------|------|
| add | a, b (number) | result | 加法运算 |
| subtract | a, b (number) | result | 减法运算 |
| multiply | a, b (number) | result | 乘法运算 |
| divide | a, b (number) | result/error | 除法运算（除零返回error） |
| echo | 任意参数 | echo | 参数原样回传 |
| getTime | 无 | time, timestamp | 获取服务器当前时间 |
| getVersion | 无 | program, version, qtVersion | 获取版本信息 |
| arraySum | array (number[]) | result, count | 数组求和 |
| toUpperCase | text (string) | result | 字符串转大写 |

### 4.4 消息格式示例

**注册WebSocket客户端（A→B）**:
```json
{"type": "register", "clientId": "a1b2c3d4..."}
```

**注册确认（B→A）**:
```json
{"type": "registered", "clientId": "a1b2c3d4..."}
```

**成功响应**:
```json
{"status": "success", "data": {"result": 30}, "requestId": "req-001"}
```

**错误响应**:
```json
{"status": "error", "error": "Function 'foo' not found", "requestId": "req-002"}
```

## 五、UI界面说明

### 5.1 程序A界面
- **连接配置区**：IP地址、HTTP端口（默认8080）、WebSocket端口（默认8081）、模式选择（HTTP同步/WebSocket异步）
- **函数调用区**：函数下拉列表、JSON参数编辑器（切换函数时自动填充模板）、调用按钮
- **结果展示区**：成功结果（绿色）与错误信息（红色）
- **通信日志区**：带时间戳的完整通信日志

### 5.2 程序B界面
- **服务控制区**：端口设置、启动/停止按钮、运行状态指示（绿色/红色）
- **端口信息**：显示实际监听的HTTP和WebSocket端口
- **调用日志区**：记录所有请求、响应、连接事件、错误信息

## 六、编译与运行

### 6.1 环境要求
- **操作系统**: Windows 10/11
- **编译器**: MinGW 8.1.0 64-bit（随Qt安装的 `mingw810_64`）
- **CMake**: 3.16+
- **Qt**: 6.5.3 (mingw_64)

### 6.2 编译命令
```bash
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=D:/Qt/6.5.3/mingw_64
cmake --build .
```

### 6.3 运行方法
1. 先启动 **ProgramB.exe**，点击"Start Services"启动HTTP和WebSocket服务
2. 再启动 **ProgramA.exe**，配置连接参数后选择函数并点击"Call Function"
3. 同步模式：结果直接显示在HTTP响应中
4. 异步模式：结果通过WebSocket连接推送到程序A

## 七、异常处理

| 场景 | 处理方式 |
|------|---------|
| 连接失败 | 程序A显示网络错误提示，WebSocket自动重连 |
| 函数不存在 | 返回HTTP 404 + JSON错误对象，界面红色高亮 |
| 参数JSON格式错误 | 返回HTTP 400 + JSON解析错误描述 |
| 除零错误 | 业务函数返回error字段，通信层透传 |
| TCP粘包/拆包 | 服务端缓冲区机制，按Content-Length精确读取 |
| 客户端断开 | WebSocket服务端自动清理clientId映射表 |

## 八、扩展指南

### 新增业务函数
只需在 `ProgramB/BusinessFunctions.cpp` 中添加实现，然后在 `ProgramB/MainWindow.cpp` 的 `registerFunctions()` 中注册：

```cpp
// 1. 在 BusinessFunctions.h 中声明
static QJsonObject myFunction(const QJsonObject &params);

// 2. 在 BusinessFunctions.cpp 中实现
QJsonObject BusinessFunctions::myFunction(const QJsonObject &params) {
    QJsonObject result;
    result["result"] = /* 业务逻辑 */;
    return result;
}

// 3. 在 MainWindow.cpp 中注册
m_registry->registerFunction("myFunction", BusinessFunctions::myFunction);

// 4. (可选) 在程序A的 m_paramTemplates 中添加参数模板
```

通信框架代码无需任何修改。
