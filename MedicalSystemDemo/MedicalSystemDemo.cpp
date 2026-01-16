#include "MedicalSystemDemo_t.h" // 自动生成的头文件

#include "sgx_trts.h"
#include <stdio.h>
#include <string.h>

// 1. 实现核心业务：安全保存病历
void ecall_secure_save_record(const char* name, const char* diagnosis)
{
    char buffer[512];

    // 步骤 A: 验证数据已进入 Enclave
    // 此时数据位于受保护的内存（模拟模式下为模拟的受保护内存）
    snprintf(buffer, 512, "[Enclave] 成功接收数据！\n[Enclave] 正在处理患者: %s\n[Enclave] 诊断内容: %s", name, diagnosis);
    ocall_print_log(buffer);

    // 步骤 B: 模拟加密/Sealing
    // 在真实场景中，这里会调用 sgx_seal_data
    // 这里我们简单拼接，模拟“加密打包”的过程
    snprintf(buffer, 512, "[Enclave] 正在执行 Sealing (密封) ...\n[Enclave] 数据已加密为 Opaque Blob，准备落盘。");
    ocall_print_log(buffer);
}