# 基于可信执行环境的密态病历信息系统 - 开发指南

## 项目概览

这是一个基于 Intel SGX 的密态病历管理系统，旨在实现医疗数据的全生命周期加密保护。系统采用 C/S 架构，在 Enclave 内进行所有敏感数据处理，确保即使系统被攻破，明文病历也永不泄露。

### 核心特性

✅ **全生命周期密态保护**
- 静态保护：数据在磁盘上加密存储
- 传输保护：数据在网络上加密传输
- 使用中保护：数据在 Enclave 内处理，外部完全加密

✅ **硬件级安全隔离**
- 基于 Intel SGX 的 CPU 级隔离
- 信任根缩小到 Enclave + CPU 硬件
- 即使 OS 被完全控制，数据仍安全

✅ **多用户权限管理**
- 患者：查看个人信息和病历
- 医生：查看患者信息，编辑病历
- 管理员：系统部署和维护

✅ **高性能优化**
- Switchless Calls 技术减少上下文切换
- 性能相比传统 ECall/OCall 提升 50%+
- 支持 100+ 并发用户

✅ **完整审计追踪**
- 所有操作都被记录和审计
- 支持数据修改历史追踪
- 满足法规合规要求

---

## 快速开始

### 环境要求

**硬件要求**
- Intel CPU 支持 SGX（第 6 代 Core 及以上）
- 至少 8GB RAM
- 至少 20GB 磁盘空间

**软件要求**
- Ubuntu 20.04 LTS 或更高版本
- Intel SGX SDK 2.x
- GCC 9.0+ 或 Clang 10.0+
- CMake 3.10+
- OpenSSL 1.1.1+

### 安装步骤

#### 1. 安装 Intel SGX 驱动和 SDK

```bash
# 安装依赖
sudo apt-get update
sudo apt-get install -y build-essential python3 libcurl4-openssl-dev libprotobuf-dev

# 下载 SGX 驱动
git clone https://github.com/intel/linux-sgx-driver.git
cd linux-sgx-driver
make
sudo insmod isgx.ko

# 下载 SGX SDK
wget https://download.01.org/intel-sgx/sgx-linux/2.x/distro/ubuntu20.04-server/sgx_linux_x64_sdk_2.x.x.bin
chmod +x sgx_linux_x64_sdk_2.x.x.bin
./sgx_linux_x64_sdk_2.x.x.bin

# 设置环境变量
source $HOME/sgxsdk/environment
```

#### 2. 克隆项目

```bash
git clone <project-repo>
cd tee-medical-records
```

#### 3. 构建项目

```bash
mkdir build
cd build
cmake ..
make
```

#### 4. 运行测试

```bash
./test_enclave
./test_authentication
./test_data_management
```

---

## 项目结构

```
tee-medical-records/
├── .kiro/
│   └── specs/
│       └── tee-medical-records/
│           ├── requirements.md      # 需求规格说明书
│           ├── design.md            # 设计文档
│           ├── tasks.md             # 实现任务清单
│           └── README.md            # 本文件
├── src/
│   ├── enclave/                     # Enclave 代码（可信）
│   │   ├── enclave.cpp
│   │   ├── enclave.h
│   │   ├── auth.cpp                 # 身份认证模块
│   │   ├── auth.h
│   │   ├── rbac.cpp                 # 权限管理模块
│   │   ├── rbac.h
│   │   ├── crypto.cpp               # 加密模块
│   │   ├── crypto.h
│   │   ├── keymanagement.cpp        # 密钥管理模块
│   │   ├── keymanagement.h
│   │   ├── data_management.cpp      # 数据管理模块
│   │   ├── data_management.h
│   │   ├── attestation.cpp          # 远程证明模块
│   │   ├── attestation.h
│   │   ├── sealing.cpp              # Sealing 模块
│   │   ├── sealing.h
│   │   ├── switchless.cpp           # Switchless Calls 模块
│   │   └── switchless.h
│   ├── app/                         # 应用层代码（不可信）
│   │   ├── app.cpp
│   │   ├── app.h
│   │   ├── server.cpp               # 服务器实现
│   │   ├── server.h
│   │   ├── database.cpp             # 数据库管理
│   │   ├── database.h
│   │   ├── audit.cpp                # 审计日志
│   │   └── audit.h
│   ├── client/                      # 客户端代码
│   │   ├── patient_client.cpp       # 患者客户端
│   │   ├── doctor_client.cpp        # 医生客户端
│   │   └── admin_client.cpp         # 管理员客户端
│   └── common/                      # 公共代码
│       ├── types.h                  # 数据类型定义
│       ├── constants.h              # 常量定义
│       └── utils.h                  # 工具函数
├── test/
│   ├── unit_tests/                  # 单元测试
│   │   ├── test_auth.cpp
│   │   ├── test_rbac.cpp
│   │   ├── test_crypto.cpp
│   │   ├── test_keymanagement.cpp
│   │   └── test_data_management.cpp
│   ├── integration_tests/           # 集成测试
│   │   ├── test_end_to_end.cpp
│   │   ├── test_security.cpp
│   │   └── test_performance.cpp
│   └── CMakeLists.txt
├── docs/
│   ├── architecture.md              # 架构文档
│   ├── api.md                       # API 文档
│   ├── deployment.md                # 部署指南
│   └── troubleshooting.md           # 故障排查
├── scripts/
│   ├── build.sh                     # 构建脚本
│   ├── deploy.sh                    # 部署脚本
│   ├── test.sh                      # 测试脚本
│   └── monitor.sh                   # 监控脚本
├── CMakeLists.txt
├── enclave.edl                      # Enclave 定义文件
└── README.md
```

---

## 开发流程

### 1. 需求分析阶段

**文件**：`.kiro/specs/tee-medical-records/requirements.md`

- 理解系统需求
- 明确功能范围
- 确定验收标准

**关键问题**
- 系统需要支持哪些用户角色？
- 每个角色有哪些权限？
- 数据需要如何保护？

### 2. 设计阶段

**文件**：`.kiro/specs/tee-medical-records/design.md`

- 系统架构设计
- 模块设计
- 数据流设计
- 安全设计

**关键设计决策**
- 采用 C/S 架构
- 使用 Intel SGX 进行硬件隔离
- 采用多级密钥体系
- 使用 Switchless Calls 优化性能

### 3. 实现阶段

**文件**：`.kiro/specs/tee-medical-records/tasks.md`

按照任务清单逐步实现：

1. **第一阶段**：项目基础设置（1-2 周）
2. **第二阶段**：核心安全基础设施（3-4 周）
3. **第三阶段**：数据管理与加密（3-4 周）
4. **第四阶段**：应用层与通信（2-3 周）
5. **第五阶段**：审计与合规（1-2 周）
6. **第六阶段**：性能优化（2-3 周）
7. **第七阶段**：系统集成与测试（2-3 周）
8. **第八阶段**：文档与部署（1-2 周）

### 4. 测试阶段

- 单元测试：测试各个模块的功能
- 集成测试：测试模块之间的交互
- 系统测试：测试整个系统的功能
- 性能测试：测试系统的性能指标
- 安全测试：测试系统的安全性

### 5. 部署阶段

- 编写部署脚本
- 配置生产环境
- 进行部署前检查
- 执行部署
- 进行部署后验证

---

## 核心模块说明

### 模块 1：身份认证与权限管理

**位置**：`src/enclave/auth.cpp`, `src/enclave/rbac.cpp`

**功能**
- 基于公钥密码学的身份认证
- Challenge-Response 机制防重放
- 基于角色的访问控制（RBAC）

**关键函数**
```cpp
// 身份认证
int sgx_authenticate_user(const char* username, const uint8_t* signature);

// 权限检查
int sgx_check_permission(uint32_t user_id, const char* resource, const char* action);

// Token 生成
int sgx_generate_token(uint32_t user_id, uint8_t* token);
```

### 模块 2：数据管理与加密

**位置**：`src/enclave/data_management.cpp`, `src/enclave/crypto.cpp`

**功能**
- 患者基本信息管理
- 病历 CRUD 操作
- 数据加密与解密
- 数据完整性验证

**关键函数**
```cpp
// 患者信息操作
int sgx_create_patient(const PatientInfo* info);
int sgx_read_patient(uint32_t patient_id, PatientInfo* info);
int sgx_update_patient(const PatientInfo* info);
int sgx_delete_patient(uint32_t patient_id);

// 病历操作
int sgx_create_record(const MedicalRecord* record);
int sgx_read_record(uint32_t record_id, MedicalRecord* record);
int sgx_update_record(const MedicalRecord* record);
int sgx_delete_record(uint32_t record_id);

// 加密操作
int sgx_encrypt_data(const uint8_t* plaintext, uint32_t plaintext_len,
                     uint8_t* ciphertext, uint32_t* ciphertext_len);
int sgx_decrypt_data(const uint8_t* ciphertext, uint32_t ciphertext_len,
                     uint8_t* plaintext, uint32_t* plaintext_len);
```

### 模块 3：密钥管理

**位置**：`src/enclave/keymanagement.cpp`

**功能**
- 主密钥生成与管理
- 密钥派生
- 密钥轮换
- 密钥销毁

**关键函数**
```cpp
// 密钥初始化
int sgx_init_keys();

// 密钥派生
int sgx_derive_key(const uint8_t* master_key, const char* context,
                   uint8_t* derived_key);

// 密钥轮换
int sgx_rotate_keys();

// 密钥销毁
int sgx_destroy_keys();
```

### 模块 4：远程证明与 Sealing

**位置**：`src/enclave/attestation.cpp`, `src/enclave/sealing.cpp`

**功能**
- Quote 生成
- 远程证明验证
- 数据 Sealing
- 数据 Unsealing

**关键函数**
```cpp
// 远程证明
int sgx_generate_quote(uint8_t* quote, uint32_t* quote_len);
int sgx_verify_quote(const uint8_t* quote, uint32_t quote_len);

// Sealing
int sgx_seal_data(const uint8_t* plaintext, uint32_t plaintext_len,
                  uint8_t* sealed_data, uint32_t* sealed_data_len);
int sgx_unseal_data(const uint8_t* sealed_data, uint32_t sealed_data_len,
                    uint8_t* plaintext, uint32_t* plaintext_len);
```

### 模块 5：性能优化 - Switchless Calls

**位置**：`src/enclave/switchless.cpp`

**功能**
- 共享内存环形缓冲区管理
- Switchless ECall 实现
- Switchless OCall 实现

**关键函数**
```cpp
// 初始化 Switchless 缓冲区
int sgx_init_switchless_buffer();

// Switchless 调用
int sgx_switchless_call(uint32_t function_id, const uint8_t* params,
                        uint32_t params_len, uint8_t* result,
                        uint32_t* result_len);
```

---

## 开发建议

### 1. 开发顺序

**推荐按以下顺序开发**

1. **基础设施**（第 1-2 周）
   - SGX 环境搭建
   - 密码学库集成
   - 基础 ECall/OCall 实现

2. **安全基础**（第 3-6 周）
   - 身份认证系统
   - 权限管理系统
   - 密钥管理系统
   - 远程证明和 Sealing

3. **数据管理**（第 7-10 周）
   - 患者信息管理
   - 病历管理
   - 数据加密解密
   - 完整性验证

4. **应用层**（第 11-13 周）
   - 服务器框架
   - 数据库管理
   - 客户端实现

5. **优化与测试**（第 14-20 周）
   - 性能优化
   - 集成测试
   - 安全测试
   - 部署

### 2. 测试策略

**单元测试**
- 为每个模块编写单元测试
- 测试覆盖率目标 > 80%
- 使用 Google Test 框架

**集成测试**
- 测试模块之间的交互
- 测试完整的业务流程
- 测试错误处理

**系统测试**
- 测试整个系统的功能
- 测试性能指标
- 测试安全性

### 3. 安全最佳实践

**代码安全**
- 使用内存安全的编程实践
- 避免缓冲区溢出
- 避免整数溢出
- 定期进行代码审计

**密钥安全**
- 密钥仅在 Enclave 内生成
- 密钥不落地
- 定期密钥轮换
- 密钥销毁时完全清零

**数据安全**
- 所有敏感数据加密存储
- 使用强加密算法（AES-256-GCM）
- 验证数据完整性（MAC）
- 防止重放攻击（版本号）

### 4. 性能优化

**优化策略**
- 使用 Switchless Calls 减少上下文切换
- 在 Enclave 内缓存热数据
- 支持批量操作
- 优化内存管理

**性能监控**
- 监控 ECall/OCall 次数
- 监控内存使用
- 监控 CPU 使用率
- 监控响应时间

---

## 常见问题

### Q1: SGX 硬件不支持怎么办？

**A**: 可以使用 SGX 模拟器进行开发和测试。

```bash
# 安装 SGX 模拟器
./sgx_linux_x64_sdk_2.x.x.bin --prefix=$HOME/sgxsdk

# 设置模拟器模式
export SGX_MODE=SW
```

### Q2: 如何调试 Enclave 代码？

**A**: 使用 SGX 调试工具。

```bash
# 编译调试版本
cmake -DCMAKE_BUILD_TYPE=Debug ..
make

# 使用 gdb 调试
gdb ./app
(gdb) run
```

### Q3: 如何处理 Enclave 内存限制？

**A**: Enclave 内存通常限制在 128MB。可以通过以下方式优化：

- 使用外部内存存储大数据
- 实现数据分页机制
- 优化数据结构

### Q4: 如何提高系统性能？

**A**: 可以采用以下优化策略：

- 使用 Switchless Calls
- 实现数据缓存
- 支持批量操作
- 优化密钥派生算法

### Q5: 如何确保系统安全性？

**A**: 采用多层防护：

- 硬件级隔离（SGX）
- 密码学保护（加密、MAC）
- 访问控制（RBAC）
- 审计追踪（日志）

---

## 参考资源

### 官方文档
- [Intel SGX 官方文档](https://software.intel.com/content/www/us/en/develop/articles/intel-software-guard-extensions-intel-sgx-sdk-and-enclave-development-platform.html)
- [Intel SGX SDK 开发者指南](https://download.01.org/intel-sgx/sgx-linux/2.x/docs/)

### 学习资源
- [SGX 101](https://sgx101.gitbook.io/sgx101/)
- [SGX 安全编程指南](https://github.com/intel/linux-sgx/tree/master/SampleCode)

### 相关论文
- [Intel SGX Explained](https://eprint.iacr.org/2016/086.pdf)
- [Enclave Malware Detection](https://arxiv.org/abs/1701.01681)

---

## 联系与支持

如有问题或建议，请联系：

- **指导教师**：卢益彪
- **学生**：唐闫琪
- **学号**：22211835217

---

## 许可证

本项目采用 MIT 许可证。详见 LICENSE 文件。

---

**最后更新**：2025 年 11 月 26 日

