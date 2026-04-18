# RemoteAttestation_sp

## 1. 目录用途

该目录用于放置当前项目的“远程认证服务提供方（service provider）”原型代码。

它的设计参考：
- `intel/confidential-computing.sgx/SampleCode/RemoteAttestation/service_provider`

## 2. 当前阶段定位

当前阶段该目录只承担“结构预留 + 原型代码占位”作用，目标是：

1. 给后续远程认证功能提供独立的服务提供方模块
2. 与当前 `MedicalSystemDemo_app + MedicalSystemDemo enclave` 架构形成三段式结构
3. 便于后续逐步接入：
   - 认证消息收发
   - enclave 身份验证
   - 认证策略检查
   - 安全会话建立

## 3. 当前不包含的能力

当前原型目录还没有实现：

- 真实网络通信
- quote 验证
- IAS / DCAP 对接
- 真实 secure session key 协商

## 4. 后续演进方向

后续建议按以下顺序扩展：

1. 先完成本地进程内模拟消息交互
2. 再完成本机回环地址 `127.0.0.1` 的 socket 通信
3. 最后再接入真实的 quote 验证链路
