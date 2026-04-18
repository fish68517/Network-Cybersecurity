# GUI 实现方案分析

## 1. 目标

老师提出的新要求不是简单地把控制台窗口换成一个壳，而是要把当前基于 Intel SGX 的密态病历系统，改造成**具备图形界面交互能力的可演示桌面应用**，同时继续保留以下核心特征：

1. 仍然使用现有 SGX Enclave、Sealing、远程认证原型流程。
2. GUI 代码尽量隔离，放在新目录 `D:\Acode\Android\complete\MedicalSystemDemo\gui`。
3. 不大幅污染现有 `MedicalSystemDemo`、`MedicalSystemDemo_app`、`RemoteAttestation_sp` 代码结构。
4. 最终仍然要产出老师机器可直接运行的 `exe` 包。

---

## 2. 当前项目现状分析

当前项目已经具备以下基础：

1. `MedicalSystemDemo`：Enclave 侧核心逻辑，包含用户、患者档案、病历、Sealing、远程认证状态机。
2. `MedicalSystemDemo_app`：宿主侧控制台程序，已经完成 Enclave 创建、ECall/OCall、远程认证菜单流程、文件保存与日志打印。
3. `RemoteAttestation_sp`：本地 service provider 原型，可独立生成 `RemoteAttestation_sp.exe`，并参与远程认证占位闭环。
4. `交付/runtime/MedicalSystemDemo_Portable_x64`：已经有一套可以在无开发环境机器上直接运行的 `exe + SGX Simulation DLL` 运行包。

因此，GUI 实现**不应该推翻现有核心逻辑**，而应当复用当前已经跑通的 SGX 后端能力。

---

## 3. GUI 技术路线候选分析

### 3.1 方案一：Python + Tkinter

#### 优点

1. 开发速度快，界面原型容易落地。
2. Tkinter 是 Python 内置库，不依赖额外 GUI 框架。
3. 可通过 `PyInstaller` 打包为 `exe`，方便老师机器直接运行。
4. GUI 代码可以完全放在 `gui` 目录内，和原 C++ 工程隔离较好。

#### 缺点

1. Tkinter 视觉风格比较基础，界面质感一般。
2. 如果直接让 Python 去“模拟输入输出控制台程序”，稳定性差，不适合答辩现场。
3. 如果 Python 直接调用 SGX 宿主接口，需要额外设计桥接层，不能直接复用当前控制台菜单逻辑。

#### 适配性判断

适合做**快速答辩版 GUI**，但前提是：
- 不能采用“驱动控制台菜单”的方式。
- 必须增加一个**独立的后端桥接层**，让 GUI 调用明确的业务接口。

---

### 3.2 方案二：Qt（C++）

#### 优点

1. 与当前 C++ 项目语言一致，长期来看最规范。
2. 界面能力更强，控件丰富，做出的桌面程序更像正式软件。
3. 可直接与现有 C++ 宿主侧逻辑集成。

#### 缺点

1. Qt 环境较重，答辩前新增开发和打包成本较高。
2. 需要新建 Qt 工程、处理部署工具、处理运行库和插件，时间成本明显大于 Tkinter。
3. 当前项目已经有一套能跑的 SGX 工程，若切到 Qt，容易把工作量集中在框架搭建而不是业务整合上。

#### 适配性判断

适合做“正式产品化版本”，但对当前毕设阶段来说，**风险偏高、性价比偏低**。

---

### 3.3 方案三：C# WinForms / WPF

#### 优点

1. Windows 桌面 GUI 开发效率高。
2. 打包 `exe` 方便，老师机器运行体验好。
3. 页面外观比 Tkinter 更容易做好。

#### 缺点

1. 需要设计 .NET 与现有 C++ SGX 宿主程序之间的桥接。
2. 如果要复用现有代码，通常要增加 native DLL 或 C++/CLI 包装层。
3. 跨语言桥接复杂度不一定比 Python 更低。

#### 适配性判断

可行，但会引入新的运行时和桥接复杂度。对当前项目不如 Python Tkinter 方案直接。

---

## 4. 不建议采用的实现方式

### 4.1 不建议：GUI 直接驱动当前命令行菜单

即：GUI 通过 `subprocess` 启动 `MedicalSystemDemo_app.exe`，再向标准输入写菜单编号、再解析标准输出文本。

#### 原因

1. 当前控制台程序是面向人工输入设计的，不是稳定 API。
2. 一旦菜单编号、输出文案、日志顺序变化，GUI 就会失效。
3. 中文日志与控制台编码问题已经在现有运行中出现过路径显示乱码现象，GUI 若依赖文本解析，会更脆弱。
4. 远程认证流程已经增加 service provider 交互，继续走“文本驱动”会让错误定位非常困难。

#### 结论

**不能把 GUI 建在命令行菜单自动化之上。**

---

## 5. 推荐方案

## 推荐方案：`Python Tkinter + 新增独立后端桥接层` 

这是结合当前项目实际情况后，最稳妥、最容易在毕设周期内完成的方案。

### 5.1 核心思路

保持现有 C++ SGX 核心不变，在 `gui` 目录下新建两层：

1. **GUI 前端层**：使用 Python Tkinter 实现页面。
2. **后端桥接层**：新增一个轻量的 native bridge，向 GUI 暴露清晰的业务接口。

GUI 不再操作控制台菜单，而是调用桥接层提供的函数，例如：

- 初始化系统
- 登录/退出
- 执行远程认证
- 查看远程认证状态
- 注册用户
- 写入患者档案
- 查询患者档案
- 创建病历
- 查询病历

这样做的好处是：

1. 现有 SGX 核心逻辑保留。
2. GUI 与控制台菜单解耦。
3. 未来答辩既能演示 GUI，也能继续保留命令行版作为备份。

---

## 6. 推荐目录结构

建议后续新增如下结构：

```text
D:\Acode\Android\complete\MedicalSystemDemo\gui
├─ app
│  ├─ main.py
│  ├─ ui_main.py
│  ├─ pages
│  │  ├─ login_page.py
│  │  ├─ admin_page.py
│  │  ├─ doctor_page.py
│  │  ├─ patient_page.py
│  │  └─ ra_page.py
│  ├─ services
│  │  ├─ backend_client.py
│  │  └─ models.py
│  └─ assets
│     └─ icons
├─ bridge
│  ├─ sgx_gui_bridge.cpp
│  ├─ sgx_gui_bridge.h
│  └─ sgx_gui_bridge.vcxproj
├─ package
│  └─ build_gui_release.ps1
└─ README.md
```

### 说明

1. `gui/app`：纯 GUI 代码。
2. `gui/bridge`：新建宿主桥接层，负责复用现有 SGX ECall/OCall 能力。
3. `gui/package`：打包脚本。
4. 不直接改动原有控制台程序主菜单逻辑。

---

## 7. 后端桥接层建议职责

后端桥接层建议做成一个**新的 native DLL**，供 Python 使用 `ctypes` 调用。

### 7.1 为什么需要桥接 DLL

因为当前 `MedicalSystemDemo_app.exe` 的逻辑都集中在控制台交互里，不适合作为 GUI 直接复用。

如果增加一个 `sgx_gui_bridge.dll`，那么 GUI 可以调用如下导出接口：

```text
msd_gui_init_system()
msd_gui_load_state()
msd_gui_login(username, password)
msd_gui_logout()
msd_gui_ra_start(peer_identity)
msd_gui_ra_get_status()
msd_gui_register_user(username, password, role)
msd_gui_upsert_patient(...)
msd_gui_get_patient(...)
msd_gui_list_patients(...)
msd_gui_create_record(...)
msd_gui_list_records(...)
```

### 7.2 桥接层复用内容

桥接层应尽量复用：

1. `MedicalSystemDemo_u.h`
2. `MedicalSystemDemo_u.c`
3. SGX Enclave 初始化逻辑
4. 现有文件 OCall
5. 现有 `RemoteAttestation_sp.exe` 调用逻辑

### 7.3 桥接层不要做的事

1. 不要实现 GUI。
2. 不要再写一套业务规则。
3. 不要复制 Enclave 业务代码。

桥接层只做：**函数化封装 + 状态维持 + 字符串/缓冲区适配**。

---

## 8. GUI 页面建议

### 8.1 登录页

功能：
1. 初始化系统
2. 加载系统状态
3. 用户名密码登录
4. 显示当前 Enclave 初始化状态

### 8.2 管理员页

功能：
1. 注册患者账号
2. 注册医生账号
3. 查看用户列表
4. 新增/修改患者档案
5. 查询患者档案
6. 查看患者列表
7. 删除患者档案
8. 查看系统日志区

### 8.3 医生页

功能：
1. 查看患者列表
2. 查看患者档案
3. 执行远程认证
4. 查看远程认证状态
5. 新增病历
6. 查看患者病历
7. 修改病历
8. 删除病历

### 8.4 患者页

功能：
1. 执行远程认证
2. 查看远程认证状态
3. 查看我的档案
4. 修改我的档案
5. 查看我的病历

### 8.5 日志显示区

建议 GUI 页面右侧保留日志面板，用于展示：
1. App 日志
2. Enclave 日志
3. 远程认证流程日志
4. sealed 文件保存路径

这对答辩非常重要，因为老师会直接观察“SGX 是否真的参与了流程”。

---

## 9. GUI 实现时必须保留的 SGX 展示点

如果做 GUI，只做表单是不够的。必须把 SGX 特性也展示出来。

建议在 GUI 中明确显示以下状态：

1. **Enclave 状态**：是否创建成功
2. **远程认证状态**：未开始、处理中、认证通过、认证失败
3. **安全会话状态**：未建立 / 已建立
4. **密态状态文件路径**：`sealed_system_state.bin` 的实际位置
5. **操作日志**：显示 Msg1、Msg2、Msg3、认证结果等关键阶段

这样老师在看 GUI 时，不会觉得只是一个普通病历管理界面。

---

## 10. 最终 exe 打包建议

### 10.1 推荐打包方式

如果采用 Python Tkinter：

1. 使用 `PyInstaller` 打包 GUI 主程序为 `MedicalSystemDemo_GUI.exe`
2. 与以下文件放在同一目录：
   - `MedicalSystemDemo.signed.dll`
   - `sgx_*.dll`
   - `RemoteAttestation_sp.exe`
   - `sgx_gui_bridge.dll`

### 10.2 交付目录建议

```text
MedicalSystemDemo_GUI_Portable_x64
├─ MedicalSystemDemo_GUI.exe
├─ sgx_gui_bridge.dll
├─ MedicalSystemDemo.signed.dll
├─ RemoteAttestation_sp.exe
├─ sgx_capable.dll
├─ sgx_dbghelp.dll
├─ sgx_enclave_common.dll
├─ sgx_epid_sim.dll
├─ sgx_launch_sim.dll
├─ sgx_platform_sim.dll
├─ sgx_quote_ex_sim.dll
├─ sgx_uae_service_sim.dll
├─ sgx_urtsd.dll
├─ sgx_urts_simd.dll
└─ 运行说明.txt
```

### 10.3 这样做的意义

1. 老师机器无需 Python 环境。
2. 老师机器无需 Visual Studio。
3. GUI 版和控制台版都可以同时保留，互为备份。

---

## 11. 实施复杂度评估

### 11.1 开发工作量

中等偏上，但可控。

主要工作量来自：
1. 把当前宿主程序逻辑抽成桥接接口
2. Tkinter 页面搭建
3. GUI 与桥接层的数据交互
4. 打包与运行验证

### 11.2 风险点

1. 如果直接驱动现有控制台菜单，项目会非常脆弱。
2. 如果 GUI 页面过多、样式追求过高，会拖慢实现。
3. Python 与 native DLL 交互时，要注意字符串编码和缓冲区长度。
4. 打包后要确保 `RemoteAttestation_sp.exe` 与 SGX DLL 能在同目录被发现。

### 11.3 风险控制建议

1. 第一版 GUI 只覆盖核心答辩流程，不追求复杂皮肤。
2. 先完成功能闭环，再考虑界面美化。
3. 保留现有控制台版运行包作为兜底方案。

---

## 12. 建议的实施顺序

如果你审核通过，后续建议按下面顺序做：

1. 在 `gui/bridge` 新建 `sgx_gui_bridge` 工程
2. 把现有宿主逻辑改造成可导出函数
3. 在 `gui/app` 用 Tkinter 实现登录页和主页面框架
4. 先接管理员页
5. 再接医生页和远程认证页
6. 再接患者页
7. 最后做 PyInstaller 打包和便携运行验证

---

## 13. 最终建议结论

### 结论

结合当前项目状态、时间成本、老师对 GUI 的要求、以及“尽量不要污染旧代码”的前提，**最推荐的实现方案是：**

**`Python Tkinter + 新建 native bridge DLL + 继续复用现有 SGX Enclave 与 RemoteAttestation_sp`**

### 原因

1. 改动隔离性最好，符合“新目录 gui”要求。
2. 相比 Qt，开发和打包成本更低。
3. 相比直接驱动命令行，稳定性更高。
4. 最终仍可交付老师机器直接运行的 GUI `exe`。

### 不建议的路线

1. 不建议把 GUI 建在当前控制台菜单脚本驱动之上。
2. 不建议现在切到 Qt 重做整个宿主程序。
3. 不建议为了 GUI 去重写 Enclave 业务逻辑。
