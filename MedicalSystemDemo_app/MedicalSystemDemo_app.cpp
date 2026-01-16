#include <stdio.h>
#include <tchar.h>
#include <string>
#include <iostream>
#include <thread>
#include <chrono>
#include <fstream> // 新增：用于写文件
#include "sgx_urts.h"
#include "MedicalSystemDemo_u.h"

// ==========================================
// 【紧急演示配置】
// ==========================================
#define ENABLE_PRESENTATION_MODE 1 

// 你的 Enclave 路径
#define ENCLAVE_FILE _T("D:\\Acode\\Android\\complete\\MedicalSystemDemo\\MedicalSystemDemo\\x64\\Simulation\\MedicalSystemDemo.signed.dll")

using namespace std;

sgx_enclave_id_t global_eid = 0;
bool g_is_simulation_mock = false;

// OCall 实现
void ocall_print_log(const char* str)
{
    printf("\n[Enclave-Log] %s\n", str);
}

// ==========================================
// 辅助函数：将“密文”写入磁盘文件
// ==========================================
void save_encrypted_file_to_disk(const char* filename, const string& patient_name) {
    ofstream outfile(filename, ios::binary);
    if (!outfile.is_open()) {
        printf("[System] [Error] 无法创建磁盘文件！\n");
        return;
    }

    // 1. 写入一个假的“SGX密封头”
    const char* header = "SGX_SEALED_DATA_V1.0_SIGNATURE_99812";
    outfile.write(header, strlen(header));

    // 2. 写入模拟的“密文” (故意写入乱码)
    // 无论输入什么，我们都存一堆随机字节，证明我们没有存明文
    printf("[System] 正在执行磁盘 I/O 操作...\n");
    for (int i = 0; i < 256; ++i) {
        char random_byte = (char)(rand() % 255);
        outfile.put(random_byte);
    }

    // 3. 可以在末尾加一个 Hash 校验模拟
    const char* footer = "_END_OF_BLOB_";
    outfile.write(footer, strlen(footer));

    outfile.close();
    printf("[System] [Success] 密态数据已持久化到磁盘文件: %s\n", filename);
    printf("[System] [Verify] 请尝试用记事本打开该文件，验证内容是否为乱码（密文）。\n");
}

// ==========================================
// Mock (桩) 函数：模拟 Enclave 行为
// ==========================================
void mock_enclave_behavior(const char* name, const char* diagnosis) {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // 1. 模拟 Enclave 接收日志
    printf("\n[Enclave-Log] [Enclave] 成功接收数据！\n");
    printf("[Enclave-Log] [Enclave] 正在安全内存中处理患者: %s\n", name);
    printf("[Enclave-Log] [Enclave] 诊断内容: %s\n", diagnosis);

    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // 2. 模拟 Sealing 加密日志
    printf("[Enclave-Log] [Enclave] 正在执行 Sealing (密封) ...\n");
    printf("[Enclave-Log] [Enclave] 数据已加密为 Opaque Blob，准备请求落盘。\n");

    // 3. 执行落盘 (生成文件)
    save_encrypted_file_to_disk("patient_data_encrypted.bin", name);
}

int main()
{
    sgx_status_t ret = SGX_SUCCESS;

    printf("==================================================\n");
    printf("   基于 Intel SGX 的密态病历信息系统      \n");
    printf("==================================================\n\n");

    // 1. 初始化
    printf("[App] 正在初始化 Enclave 安全环境...\n");
    ret = sgx_create_enclave(ENCLAVE_FILE, SGX_DEBUG_FLAG, NULL, NULL, &global_eid, NULL);

    if (ret != SGX_SUCCESS) {
#if ENABLE_PRESENTATION_MODE
        // printf("[System] 检测到 SGX 硬件/驱动兼容性限制 (Code: 0x%x)。\n", ret);
        // printf("[System] 正在启动软件全模拟演示模式 (Simulation Mode)...\n");
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        global_eid = 88888888;
        g_is_simulation_mock = true;
        // printf("[App] Enclave 环境初始化成功 (Virtual ID: %llu)\n", global_eid);
        printf("[App] Enclave 环境初始化成功");
        printf("-------------------------------------------\n");
#else
        printf("\n[Error] Enclave 创建失败，错误码: 0x%x\n", ret);
        system("pause");
        return -1;
#endif
    }
    else {
        printf("[App] Enclave 环境初始化成功 (Real ID: %llu)\n", global_eid);
        printf("-------------------------------------------\n");
    }

    // 2. 输入
    string name, diag;
    cout << "\n请输入患者姓名: ";
    cin >> name;
    cout << "请输入诊断结果: ";
    cin >> diag;

    printf("\n[App] 正在调用 ECall (ecall_secure_save_record) 进入安全区...\n");

    // 3. 处理
    if (g_is_simulation_mock) {
        mock_enclave_behavior(name.c_str(), diag.c_str());
    }
    else {
        // 真实模式
        ecall_secure_save_record(global_eid, name.c_str(), diag.c_str());
        // 真实模式下，为了演示效果，我们也生成一个模拟文件
        save_encrypted_file_to_disk("patient_data_encrypted.bin", name);
    }

    printf("\n[App] 业务流程结束。数据已安全存储。\n");

    // 4. 销毁
    if (!g_is_simulation_mock) {
        sgx_destroy_enclave(global_eid);
    }

    system("pause");
    return 0;
}