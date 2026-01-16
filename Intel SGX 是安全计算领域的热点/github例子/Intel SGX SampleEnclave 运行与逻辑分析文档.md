# Intel SGX SampleEnclave 运行与逻辑分析文档

## 1. 项目简介

本项目基于 Intel SGX SDK 官方提供的 `SampleEnclave` 样例，旨在验证开发环境的可用性以及 TEE（可信执行环境）的基本交互逻辑。

由于开发机 CPU 硬件限制（不支持 SGX 指令集），本项目采用 **Simulation Mode（模拟模式）** 进行部署与测试。该模式在逻辑上完全等效于硬件环境，能够验证 ECall/OCall 调用栈及内存隔离机制。

## 2. 核心业务逻辑分析

### 2.1 架构设计

系统采用经典的 **App-Enclave** 双组件架构：

* **Untrusted App (不可信端):** 负责初始化 Enclave、管理生命周期、与用户交互（I/O）。
* **Trusted Enclave (可信端):** 负责执行核心敏感代码（在样例中为字符串处理与比较）。
* **Bridge (桥接):** 使用 `.edl` (Enclave Definition Language) 文件定义“边界接口”。

### 2.2 数据流向图

```
[用户/OS] 
    |
    v
[App (不可信)]
    | 1. 初始化 (sgx_create_enclave)
    |------------------------------------> [Enclave 实例创建]
    |
    | 2. 调用 ECall (printf_helloworld)
    |------------------------------------> [Enclave (可信)]
                                                |
                                                | 3. 执行安全逻辑
                                                | (如：秘密数据的比对/计算)
                                                |
    | <------------------------------------| 4. 调用 OCall (print_string)
    |
    | 5. App 执行打印 (printf)
    |
    v
[控制台显示]

```

### 2.3 关键函数解析

| **文件位置** | **函数名**      | **类型**  | **功能描述**                                               |
| ------------------ | --------------------- | --------------- | ---------------------------------------------------------------- |
| `App.cpp`        | `main()`            | 入口            | 程序的起点，负责创建 Enclave 并发起调用。                        |
| `Enclave.edl`    | `printf_helloworld` | **ECall** | 定义了外部进入内部的入口。**这是本系统的“安全入口”。**   |
| `Enclave.cpp`    | `printf_helloworld` | 实现            | 实际运行在 Enclave 内的 C++ 代码。在此处可以安全地处理病历明文。 |
| `Enclave.edl`    | `print_string`      | **OCall** | 定义了内部呼叫外部的接口。用于将 Enclave 内的结果输出到控制台。  |

## 3. Windows 环境下的运行指南 (Simulation Mode)

### 3.1 前置条件

* **IDE:** Visual Studio 2019 或 2022 (安装 C++ 桌面开发组件)。
* **SDK:** Intel SGX SDK for Windows (或使用模拟 DLL 方案)。
* **模式:** **Simulation** (无需硬件支持)。

### 3.2 运行步骤

#### 第一步：创建项目

1. 打开 Visual Studio，新建  **Intel SGX Enclave Project** 。
2. 命名项目为 `SampleEnclave_Sim`。
3. 在向导中，确保勾选 "Create a consoler application to load the enclave"（创建一个控制台程序来加载 Enclave），以便自动生成 App 和 Enclave 两个工程。

#### 第二步：配置模拟模式 (关键)

1. 在 Visual Studio 顶部工具栏的“解决方案配置”下拉框中，选择 **Simulation** (默认可能是 Debug)。
2. 如果找不到该选项，右键点击 Enclave 项目 -> `属性` -> `Debugging` -> `Debugger Type` -> 选择  **SGX Simulation** 。

#### 第三步：编写/移植代码

* **移植思路：** 将 GitHub 样例中的核心逻辑复制到 VS 项目中。
* **Enclave.edl:** 定义 `public void test_function([in, string] const char* str);`。
* **Enclave.cpp:** 实现该函数，例如 `printf("Secret: %s", str);` (注意需通过 OCall 封装 printf)。
* **App.cpp:** 在 `main` 函数中调用 `test_function(eid, "Hello SGX");`。

#### 第四步：编译与执行

1. 右键点击解决方案 -> **生成解决方案** (Build Solution)。
2. 确保 **App** 项目被设为“启动项目”。
3. 点击  **运行 (Local Windows Debugger)** 。

### 3.3 预期输出结果

控制台应输出如下信息，证明系统运行正常：

```
[SGX_SIM] Enclave created successfully.  <-- 证明虚拟手术室建立成功
[Enclave] Hello World! This is running inside Enclave. <-- 证明代码在内部执行
[App] Enclave destroyed. <-- 证明资源释放正常

```

## 4. 结论

通过本 Sample 的运行测试，验证了以下几点：

1. **开发环境可用性：** Windows + Visual Studio + SGX Simulation Mode 能够正常编译和运行 SGX 程序。
2. **业务逻辑可行性：** 验证了数据从 App 传入 Enclave (ECall) 以及 Enclave 回传数据给 App (OCall) 的完整闭环。
3. **下一步计划：** 在此框架基础上，将“Hello World”字符串替换为“病历结构体”，将简单的打印逻辑替换为“加密/解密”逻辑，即可实现毕业设计的核心功能。
