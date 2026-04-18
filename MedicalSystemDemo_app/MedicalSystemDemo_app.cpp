#include <stdio.h>
#include <tchar.h>

#include <ctime>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <windows.h>
#include <stdio.h>

#include "sgx_urts.h"
#include "MedicalSystemDemo_u.h"
#include "../MedicalSystemDemo/shared_types.h"

#define ENCLAVE_FILE_NAME _T("MedicalSystemDemo.signed.dll")

using namespace std;

sgx_enclave_id_t global_eid = 0;
string g_current_username;
int g_current_role = MSD_ROLE_NONE;
bool g_system_initialized = false;

std::wstring get_executable_directory()
{
    wchar_t module_path[MAX_PATH] = { 0 };
    if (GetModuleFileNameW(NULL, module_path, MAX_PATH) == 0) {
        return L".";
    }

    std::wstring path(module_path);
    const size_t last_separator = path.find_last_of(L"\\/");
    if (last_separator == std::wstring::npos) {
        return L".";
    }

    return path.substr(0, last_separator);
}

std::wstring get_enclave_path()
{
    return get_executable_directory() + L"\\" ENCLAVE_FILE_NAME;
}

std::wstring to_wstring_ascii(const char* text)
{
    std::wstring value;
    if (text == NULL) {
        return value;
    }

    while (*text != '\0') {
        value.push_back(static_cast<unsigned char>(*text));
        ++text;
    }

    return value;
}

std::wstring get_storage_file_path(const char* file_name)
{
    return get_executable_directory() + L"\\" + to_wstring_ascii(file_name);
}

std::wstring get_service_provider_path()
{
    return get_executable_directory() + L"\\RemoteAttestation_sp.exe";
}

string wide_to_console_text(const std::wstring& text)
{
    if (text.empty()) {
        return string();
    }

    const UINT code_page = GetConsoleOutputCP() == 0 ? CP_ACP : GetConsoleOutputCP();
    const int required_size = WideCharToMultiByte(code_page, 0, text.c_str(), -1, NULL, 0, NULL, NULL);
    if (required_size <= 1) {
        return string();
    }

    string result(static_cast<size_t>(required_size - 1), '\0');
    WideCharToMultiByte(code_page, 0, text.c_str(), -1, &result[0], required_size, NULL, NULL);
    return result;
}

bool file_exists(const std::wstring& path)
{
    const DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

bool write_binary_file(const std::wstring& path, const uint8_t* data, size_t size)
{
    FILE* file = NULL;
    _wfopen_s(&file, path.c_str(), L"wb");
    if (file == NULL) {
        return false;
    }

    const size_t bytes_written = size > 0 ? fwrite(data, 1, size, file) : 0;
    fclose(file);
    return size == 0 || bytes_written == size;
}

bool read_binary_file(const std::wstring& path, vector<uint8_t>& data)
{
    data.clear();

    FILE* file = NULL;
    _wfopen_s(&file, path.c_str(), L"rb");
    if (file == NULL) {
        return false;
    }

    _fseeki64(file, 0, SEEK_END);
    const __int64 file_size = _ftelli64(file);
    _fseeki64(file, 0, SEEK_SET);
    if (file_size < 0 || static_cast<size_t>(file_size) > sizeof(uint32_t) * 2 + MSD_RA_MAX_MESSAGE_SIZE) {
        fclose(file);
        return false;
    }

    data.assign(static_cast<size_t>(file_size), 0);
    const size_t bytes_read = file_size > 0 ? fread(&data[0], 1, static_cast<size_t>(file_size), file) : 0;
    fclose(file);
    return static_cast<size_t>(file_size) == bytes_read;
}

bool decode_ra_message(const uint8_t* raw, size_t raw_size, uint32_t& message_type, const uint8_t*& payload, size_t& payload_size)
{
    message_type = MSD_RA_MSG_NONE;
    payload = NULL;
    payload_size = 0;

    if (raw == NULL || raw_size < sizeof(uint32_t) * 2) {
        return false;
    }

    uint32_t encoded_type = 0;
    uint32_t encoded_size = 0;
    memcpy(&encoded_type, raw, sizeof(uint32_t));
    memcpy(&encoded_size, raw + sizeof(uint32_t), sizeof(uint32_t));
    if (encoded_size > MSD_RA_MAX_MESSAGE_SIZE || raw_size < sizeof(uint32_t) * 2 + encoded_size) {
        return false;
    }

    message_type = encoded_type;
    payload = raw + sizeof(uint32_t) * 2;
    payload_size = encoded_size;
    return true;
}

bool invoke_service_provider(const uint8_t* request, size_t request_size, vector<uint8_t>& response, string& error_text)
{
    response.clear();
    error_text.clear();

    const std::wstring provider_path = get_service_provider_path();
    if (!file_exists(provider_path)) {
        error_text = "未找到 RemoteAttestation_sp.exe: " + wide_to_console_text(provider_path);
        return false;
    }

    const DWORD process_id = GetCurrentProcessId();
    const std::wstring base_dir = get_executable_directory();
    const std::wstring request_path = base_dir + L"\\ra_request_" + to_wstring(process_id) + L".bin";
    const std::wstring response_path = base_dir + L"\\ra_response_" + to_wstring(process_id) + L".bin";
    DeleteFileW(request_path.c_str());
    DeleteFileW(response_path.c_str());

    if (!write_binary_file(request_path, request, request_size)) {
        error_text = "写入远程认证请求文件失败: " + wide_to_console_text(request_path);
        return false;
    }

    cout << "[App-RA] 调用 service_provider: " << wide_to_console_text(provider_path) << endl;
    cout << "[App-RA] 请求文件: " << wide_to_console_text(request_path) << endl;
    cout << "[App-RA] 响应文件: " << wide_to_console_text(response_path) << endl;

    std::wstring command_line = L"\"" + provider_path + L"\" \"" + request_path + L"\" \"" + response_path + L"\"";
    vector<wchar_t> command_buffer(command_line.begin(), command_line.end());
    command_buffer.push_back(L'\0');

    STARTUPINFOW startup_info = {};
    startup_info.cb = sizeof(startup_info);
    PROCESS_INFORMATION process_info = {};

    BOOL created = CreateProcessW(
        NULL,
        &command_buffer[0],
        NULL,
        NULL,
        FALSE,
        CREATE_NO_WINDOW,
        NULL,
        base_dir.c_str(),
        &startup_info,
        &process_info);
    if (!created) {
        DeleteFileW(request_path.c_str());
        error_text = "启动 RemoteAttestation_sp.exe 失败";
        return false;
    }

    WaitForSingleObject(process_info.hProcess, 15000);
    DWORD exit_code = 0;
    GetExitCodeProcess(process_info.hProcess, &exit_code);
    CloseHandle(process_info.hThread);
    CloseHandle(process_info.hProcess);

    if (exit_code != 0) {
        DeleteFileW(request_path.c_str());
        DeleteFileW(response_path.c_str());
        error_text = "RemoteAttestation_sp.exe 执行失败，退出码: " + to_string(exit_code);
        return false;
    }

    if (!read_binary_file(response_path, response)) {
        DeleteFileW(request_path.c_str());
        DeleteFileW(response_path.c_str());
        error_text = "读取远程认证响应文件失败: " + wide_to_console_text(response_path);
        return false;
    }

    DeleteFileW(request_path.c_str());
    DeleteFileW(response_path.c_str());
    return true;
}

const char* sgx_status_to_text(sgx_status_t status)
{
    switch (status) {
    case SGX_SUCCESS:
        return "成功";
    case SGX_ERROR_INVALID_PARAMETER:
        return "无效参数";
    case SGX_ERROR_INVALID_ENCLAVE:
        return "Enclave 文件无效";
    case SGX_ERROR_INVALID_SIGNATURE:
        return "Enclave 签名无效";
    case SGX_ERROR_ENCLAVE_FILE_ACCESS:
        return "Enclave 文件访问失败";
    case SGX_ERROR_INVALID_METADATA:
        return "Enclave 元数据无效";
    case SGX_ERROR_MODE_INCOMPATIBLE:
        return "运行模式不兼容";
    default:
        return "未知 SGX 错误";
    }
}

const char* role_to_text(int role)
{
    switch (role) {
    case MSD_ROLE_ADMIN:
        return "管理员";
    case MSD_ROLE_DOCTOR:
        return "医生";
    case MSD_ROLE_PATIENT:
        return "患者";
    default:
        return "未登录";
    }
}

const char* result_to_text(int result)
{
    switch (result) {
    case MSD_OK:
        return "成功";
    case MSD_ERR_INVALID_INPUT:
        return "输入无效";
    case MSD_ERR_NOT_INITIALIZED:
        return "系统未初始化";
    case MSD_ERR_ALREADY_INITIALIZED:
        return "系统已初始化";
    case MSD_ERR_NO_PERMISSION:
        return "权限不足";
    case MSD_ERR_USER_EXISTS:
        return "用户已存在";
    case MSD_ERR_USER_NOT_FOUND:
        return "用户不存在";
    case MSD_ERR_AUTH_FAILED:
        return "认证失败";
    case MSD_ERR_PROFILE_NOT_FOUND:
        return "患者档案不存在";
    case MSD_ERR_RECORD_NOT_FOUND:
        return "病历不存在";
    case MSD_ERR_STORAGE_FAILED:
        return "密态存储失败";
    case MSD_ERR_BUFFER_TOO_SMALL:
        return "输出缓冲区不足";
    case MSD_ERR_CAPACITY_REACHED:
        return "容量已满";
    case MSD_ERR_INVALID_ROLE:
        return "角色无效";
    case MSD_ERR_FILE_NOT_FOUND:
        return "密态文件不存在";
    case MSD_ERR_STATE_CORRUPTED:
        return "系统状态损坏";
    case MSD_ERR_SESSION_ACTIVE:
        return "已有用户处于登录状态";
    case MSD_ERR_PROFILE_HAS_RECORDS:
        return "患者仍有关联病历，不能删除档案";
    case MSD_ERR_RA_REQUIRED:
        return "当前操作需要先完成远程认证";
    case MSD_ERR_RA_CONTEXT_NOT_READY:
        return "远程认证上下文尚未准备好";
    case MSD_ERR_RA_BUSY:
        return "远程认证流程正在进行中";
    case MSD_ERR_RA_VERIFY_FAILED:
        return "远程认证验证失败";
    case MSD_ERR_SECURE_CHANNEL_NOT_READY:
        return "安全会话尚未建立";
    case MSD_ERR_RA_MSG_INVALID:
        return "远程认证消息无效";
    case MSD_ERR_RA_UNSUPPORTED:
        return "当前环境暂不支持完整远程认证";
    default:
        return "内部错误";
    }
}

const char* ra_status_to_text(int status)
{
    switch (status) {
    case MSD_RA_NOT_STARTED:
        return "未开始";
    case MSD_RA_CONTEXT_READY:
        return "上下文已初始化";
    case MSD_RA_MSG1_READY:
        return "Msg1 已生成";
    case MSD_RA_WAITING_MSG2:
        return "等待 Msg2";
    case MSD_RA_MSG3_READY:
        return "Msg3 已生成";
    case MSD_RA_WAITING_RESULT:
        return "等待认证结果";
    case MSD_RA_VERIFIED:
        return "认证通过";
    case MSD_RA_FAILED:
        return "认证失败";
    default:
        return "未知状态";
    }
}

bool check_sgx_status(sgx_status_t status, const char* action)
{
    if (status == SGX_SUCCESS) {
        return true;
    }

    printf("[App] %s 失败，SGX 错误码: 0x%x (%s)\n", action, status, sgx_status_to_text(status));
    return false;
}

string read_line(const string& prompt)
{
    cout << prompt;
    string value;
    getline(cin, value);
    return value;
}

int read_int(const string& prompt)
{
    string value = read_line(prompt);
    return atoi(value.c_str());
}

void print_separator()
{
    cout << "-------------------------------------------" << endl;
}

void print_operation(const char* title, int result)
{
    cout << "[App] " << title << ": " << result_to_text(result) << endl;
}

void print_ra_operation(const char* title, int result)
{
    cout << "[App-RA] " << title << ": " << result_to_text(result) << endl;
}

bool flush_state(bool verbose)
{
    if (!g_system_initialized) {
        return false;
    }

    int result = MSD_OK;
    sgx_status_t status = ecall_system_flush(global_eid, &result);
    if (!check_sgx_status(status, "保存系统状态")) {
        return false;
    }

    if (verbose) {
        print_operation("保存系统状态", result);
    }

    return result == MSD_OK;
}

void display_text_result(const string& title, int result, const char* text)
{
    cout << "[App] " << title << ": " << result_to_text(result) << endl;
    if (result == MSD_OK) {
        if (text != NULL && text[0] != '\0') {
            cout << text;
            if (text[strlen(text) - 1] != '\n') {
                cout << endl;
            }
        }
        else {
            cout << "[App] 无数据。" << endl;
        }
    }
}

void ocall_print_log(const char* str)
{
    printf("\n[Enclave-Log] %s\n", str);
}

void ocall_save_blob(const char* file_name, const uint8_t* data, size_t data_size, int* result)
{
    if (result != NULL) {
        *result = MSD_OK;
    }

    const std::wstring output_path = get_storage_file_path(file_name);
    FILE* file = NULL;
    _wfopen_s(&file, output_path.c_str(), L"wb");
    if (file == NULL) {
        if (result != NULL) {
            *result = MSD_ERR_STORAGE_FAILED;
        }
        return;
    }

    const size_t bytes_written = fwrite(data, 1, data_size, file);
    fclose(file);
    if (bytes_written != data_size) {
        if (result != NULL) {
            *result = MSD_ERR_STORAGE_FAILED;
        }
        return;
    }

    printf("[App] 密态状态文件已保存到: %s\n", wide_to_console_text(output_path).c_str());
}

void ocall_load_blob(const char* file_name, uint8_t* data, size_t max_size, size_t* actual_size, int* result)
{
    if (actual_size != NULL) {
        *actual_size = 0;
    }
    if (result != NULL) {
        *result = MSD_OK;
    }

    const std::wstring input_path = get_storage_file_path(file_name);
    FILE* file = NULL;
    _wfopen_s(&file, input_path.c_str(), L"rb");
    if (file == NULL) {
        if (result != NULL) {
            *result = MSD_ERR_FILE_NOT_FOUND;
        }
        return;
    }

    _fseeki64(file, 0, SEEK_END);
    const __int64 size = _ftelli64(file);
    _fseeki64(file, 0, SEEK_SET);

    if (size <= 0 || static_cast<size_t>(size) > max_size) {
        fclose(file);
        if (result != NULL) {
            *result = MSD_ERR_STORAGE_FAILED;
        }
        return;
    }

    const size_t bytes_read = fread(data, 1, static_cast<size_t>(size), file);
    fclose(file);
    if (bytes_read != static_cast<size_t>(size)) {
        if (result != NULL) {
            *result = MSD_ERR_STORAGE_FAILED;
        }
        return;
    }

    if (actual_size != NULL) {
        *actual_size = static_cast<size_t>(size);
    }

    printf("[App] 已从密态状态文件读取: %s\n", wide_to_console_text(input_path).c_str());
}

void ocall_get_time(char* buffer, size_t buffer_size)
{
    if (buffer == NULL || buffer_size == 0) {
        return;
    }

    time_t now = time(NULL);
    tm local_tm;
    localtime_s(&local_tm, &now);
    strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S", &local_tm);
}

void ocall_ra_send_msg(uint32_t message_type, const uint8_t* data, size_t data_size, int* result)
{
    if (result != NULL) {
        *result = MSD_OK;
    }

    printf("[App-RA] 占位发送消息，类型: %u，大小: %u 字节\n", message_type, (unsigned int)data_size);
    if (data != NULL && data_size > 0) {
        printf("[App-RA] 当前阶段仅记录消息，不做真实网络发送。\n");
    }
}

void ocall_ra_recv_msg(uint32_t expected_message_type, uint8_t* buffer, size_t buffer_size, size_t* actual_size, int* result)
{
    if (actual_size != NULL) {
        *actual_size = 0;
    }
    if (result != NULL) {
        *result = MSD_ERR_RA_UNSUPPORTED;
    }

    if (buffer != NULL && buffer_size > 0) {
        buffer[0] = 0;
    }

    printf("[App-RA] 请求接收消息，期望类型: %u。当前尚未接入真实 service_provider。\n", expected_message_type);
}

void show_remote_attestation_status()
{
    int ra_status = MSD_RA_NOT_STARTED;
    int secure_channel_ready = 0;
    int result = MSD_OK;
    sgx_status_t status = ecall_ra_get_status(global_eid, &ra_status, &secure_channel_ready, &result);
    if (!check_sgx_status(status, "查看远程认证状态")) {
        return;
    }

    print_ra_operation("查看远程认证状态", result);
    if (result == MSD_OK) {
        cout << "[App-RA] 认证状态: " << ra_status_to_text(ra_status) << endl;
        cout << "[App-RA] 安全会话: " << (secure_channel_ready ? "已建立" : "未建立") << endl;
    }
}

void start_remote_attestation()
{
    string peer_identity = read_line("服务提供方标识(默认 service_provider): ");
    if (peer_identity.empty()) {
        peer_identity = "service_provider";
    }

    int result = MSD_OK;
    sgx_status_t status = ecall_ra_init_context(global_eid, peer_identity.c_str(), &result);
    if (!check_sgx_status(status, "初始化远程认证上下文")) {
        return;
    }
    print_ra_operation("初始化远程认证上下文", result);
    if (result != MSD_OK) {
        return;
    }

    uint8_t msg1[MSD_RA_MAX_MESSAGE_SIZE] = { 0 };
    size_t msg1_size = 0;
    status = ecall_ra_get_msg1(global_eid, msg1, sizeof(msg1), &msg1_size, &result);
    if (!check_sgx_status(status, "生成 Msg1")) {
        return;
    }
    print_ra_operation("生成 Msg1", result);
    if (result != MSD_OK) {
        return;
    }
    cout << "[App-RA] Msg1 大小: " << msg1_size << " 字节" << endl;

    vector<uint8_t> msg2_response;
    string transport_error;
    if (!invoke_service_provider(msg1, msg1_size, msg2_response, transport_error)) {
        cout << "[App-RA] 获取 Msg2 失败: " << transport_error << endl;
        return;
    }
    if (msg2_response.empty()) {
        cout << "[App-RA] service_provider 未返回 Msg2 内容。" << endl;
        return;
    }

    uint32_t msg2_type = MSD_RA_MSG_NONE;
    const uint8_t* msg2_payload = NULL;
    size_t msg2_payload_size = 0;
    if (!decode_ra_message(&msg2_response[0], msg2_response.size(), msg2_type, msg2_payload, msg2_payload_size) || msg2_type != MSD_RA_MSG2) {
        cout << "[App-RA] service_provider 返回的 Msg2 格式无效。" << endl;
        return;
    }

    uint8_t msg3[MSD_RA_MAX_MESSAGE_SIZE] = { 0 };
    size_t msg3_size = 0;
    status = ecall_ra_proc_msg2_get_msg3(
        global_eid,
        msg2_payload,
        msg2_payload_size,
        msg3,
        sizeof(msg3),
        &msg3_size,
        &result);
    if (!check_sgx_status(status, "处理 Msg2 并生成 Msg3")) {
        return;
    }
    print_ra_operation("处理 Msg2 并生成 Msg3", result);
    if (result != MSD_OK) {
        return;
    }
    cout << "[App-RA] Msg3 大小: " << msg3_size << " 字节" << endl;

    vector<uint8_t> result_response;
    if (!invoke_service_provider(msg3, msg3_size, result_response, transport_error)) {
        cout << "[App-RA] 获取认证结果失败: " << transport_error << endl;
        return;
    }
    if (result_response.empty()) {
        cout << "[App-RA] service_provider 未返回认证结果内容。" << endl;
        return;
    }

    uint32_t result_type = MSD_RA_MSG_NONE;
    const uint8_t* result_payload = NULL;
    size_t result_payload_size = 0;
    if (!decode_ra_message(&result_response[0], result_response.size(), result_type, result_payload, result_payload_size) || result_type != MSD_RA_ATT_RESULT) {
        cout << "[App-RA] service_provider 返回的认证结果格式无效。" << endl;
        return;
    }

    status = ecall_ra_finalize(
        global_eid,
        result_payload,
        result_payload_size,
        &result);
    if (!check_sgx_status(status, "提交远程认证结果")) {
        return;
    }
    print_ra_operation("提交远程认证结果", result);
    if (result == MSD_OK) {
        cout << "[App-RA] 已通过本地 service_provider 完成占位远程认证闭环。" << endl;
    }

    show_remote_attestation_status();
}

bool try_load_state()
{
    int result = MSD_OK;
    sgx_status_t status = ecall_system_load(global_eid, &result);
    if (!check_sgx_status(status, "加载系统状态")) {
        return false;
    }

    if (result == MSD_OK) {
        g_system_initialized = true;
        cout << "[App] 已从密态文件恢复系统状态。" << endl;
        return true;
    }
    if (result == MSD_ERR_FILE_NOT_FOUND) {
        cout << "[App] 未发现已有密态数据，可先初始化系统。" << endl;
        return false;
    }

    print_operation("加载系统状态", result);
    return false;
}

void initialize_system()
{
    int result = MSD_OK;
    sgx_status_t status = ecall_system_init(global_eid, &result);
    if (!check_sgx_status(status, "初始化系统")) {
        return;
    }

    print_operation("初始化系统", result);
    if (result == MSD_OK) {
        g_system_initialized = true;
        flush_state(false);
        cout << "[App] 默认管理员账号：admin / admin123" << endl;
    }
}

void login_user()
{
    string username = read_line("用户名: ");
    string password = read_line("密码: ");

    int role = MSD_ROLE_NONE;
    int result = MSD_OK;
    sgx_status_t status = ecall_login_user(global_eid, username.c_str(), password.c_str(), &role, &result);
    if (!check_sgx_status(status, "登录")) {
        return;
    }

    print_operation("登录", result);
    if (result == MSD_OK) {
        g_current_username = username;
        g_current_role = role;
        cout << "[App] 当前角色: " << role_to_text(role) << endl;
    }
}

void logout_user()
{
    int result = MSD_OK;
    sgx_status_t status = ecall_logout_user(global_eid, &result);
    if (!check_sgx_status(status, "退出登录")) {
        return;
    }

    print_operation("退出登录", result);
    if (result == MSD_OK) {
        g_current_username.clear();
        g_current_role = MSD_ROLE_NONE;
    }
}

void list_users()
{
    char buffer[MSD_MAX_TEXT_BUFFER] = { 0 };
    int result = MSD_OK;
    sgx_status_t status = ecall_list_users(global_eid, buffer, sizeof(buffer), &result);
    if (!check_sgx_status(status, "查看用户列表")) {
        return;
    }

    display_text_result("查看用户列表", result, buffer);
}

void register_user(int role)
{
    string username = read_line("新用户名: ");
    string password = read_line("初始密码: ");

    int result = MSD_OK;
    sgx_status_t status = ecall_register_user(global_eid, username.c_str(), password.c_str(), role, &result);
    if (!check_sgx_status(status, "注册用户")) {
        return;
    }

    print_operation("注册用户", result);
    if (result == MSD_OK) {
        flush_state(false);
    }
}

void upsert_patient_profile_for(const string& patient_username)
{
    string full_name = read_line("姓名: ");
    string id_card = read_line("身份证号: ");
    string phone = read_line("电话: ");
    string address = read_line("地址: ");

    int result = MSD_OK;
    sgx_status_t status = ecall_upsert_patient_profile(
        global_eid,
        patient_username.c_str(),
        full_name.c_str(),
        id_card.c_str(),
        phone.c_str(),
        address.c_str(),
        &result);
    if (!check_sgx_status(status, "写入患者档案")) {
        return;
    }

    print_operation("写入患者档案", result);
    if (result == MSD_OK) {
        flush_state(false);
    }
}

void view_patient_profile_for(const string& patient_username)
{
    char buffer[MSD_MAX_TEXT_BUFFER] = { 0 };
    int result = MSD_OK;
    sgx_status_t status = ecall_get_patient_profile(global_eid, patient_username.c_str(), buffer, sizeof(buffer), &result);
    if (!check_sgx_status(status, "查看患者档案")) {
        return;
    }

    display_text_result("查看患者档案", result, buffer);
}

void list_patients()
{
    char buffer[MSD_MAX_TEXT_BUFFER] = { 0 };
    int result = MSD_OK;
    sgx_status_t status = ecall_list_patients(global_eid, buffer, sizeof(buffer), &result);
    if (!check_sgx_status(status, "查看患者列表")) {
        return;
    }

    display_text_result("查看患者列表", result, buffer);
}

void delete_patient_profile()
{
    string patient_username = read_line("要删除档案的患者用户名: ");
    int result = MSD_OK;
    sgx_status_t status = ecall_delete_patient_profile(global_eid, patient_username.c_str(), &result);
    if (!check_sgx_status(status, "删除患者档案")) {
        return;
    }

    print_operation("删除患者档案", result);
    if (result == MSD_OK) {
        flush_state(false);
    }
}

void create_record()
{
    string patient_username = read_line("患者用户名: ");
    string diagnosis = read_line("诊断结果: ");
    string prescription = read_line("处方信息: ");
    string note = read_line("备注: ");

    int record_id = 0;
    int result = MSD_OK;
    sgx_status_t status = ecall_create_record(
        global_eid,
        patient_username.c_str(),
        diagnosis.c_str(),
        prescription.c_str(),
        note.c_str(),
        &record_id,
        &result);
    if (!check_sgx_status(status, "新增病历")) {
        return;
    }

    print_operation("新增病历", result);
    if (result == MSD_OK) {
        cout << "[App] 新病历 ID: " << record_id << endl;
        flush_state(false);
    }
}

void list_records_for(const string& patient_username)
{
    char buffer[MSD_MAX_TEXT_BUFFER] = { 0 };
    int result = MSD_OK;
    sgx_status_t status = ecall_list_records(global_eid, patient_username.c_str(), buffer, sizeof(buffer), &result);
    if (!check_sgx_status(status, "查看病历列表")) {
        return;
    }

    display_text_result("查看病历列表", result, buffer);
}

void update_record()
{
    int record_id = read_int("病历 ID: ");
    string diagnosis = read_line("新的诊断结果: ");
    string prescription = read_line("新的处方信息: ");
    string note = read_line("新的备注: ");

    int result = MSD_OK;
    sgx_status_t status = ecall_update_record(global_eid, record_id, diagnosis.c_str(), prescription.c_str(), note.c_str(), &result);
    if (!check_sgx_status(status, "修改病历")) {
        return;
    }

    print_operation("修改病历", result);
    if (result == MSD_OK) {
        flush_state(false);
    }
}

void delete_record()
{
    int record_id = read_int("病历 ID: ");
    int result = MSD_OK;
    sgx_status_t status = ecall_delete_record(global_eid, record_id, &result);
    if (!check_sgx_status(status, "删除病历")) {
        return;
    }

    print_operation("删除病历", result);
    if (result == MSD_OK) {
        flush_state(false);
    }
}

void admin_menu(bool& running)
{
    print_separator();
    cout << "[管理员菜单] 当前用户: " << g_current_username << endl;
    cout << "1. 注册患者账号" << endl;
    cout << "2. 注册医生账号" << endl;
    cout << "3. 查看用户列表" << endl;
    cout << "4. 新增/修改患者档案" << endl;
    cout << "5. 查看患者档案" << endl;
    cout << "6. 查看患者列表" << endl;
    cout << "7. 删除患者档案" << endl;
    cout << "8. 执行远程认证" << endl;
    cout << "9. 查看远程认证状态" << endl;
    cout << "10. 保存系统状态" << endl;
    cout << "11. 退出登录" << endl;
    cout << "0. 退出程序" << endl;

    int choice = read_int("请选择: ");
    switch (choice) {
    case 1:
        register_user(MSD_ROLE_PATIENT);
        break;
    case 2:
        register_user(MSD_ROLE_DOCTOR);
        break;
    case 3:
        list_users();
        break;
    case 4:
        upsert_patient_profile_for(read_line("患者用户名: "));
        break;
    case 5:
        view_patient_profile_for(read_line("患者用户名: "));
        break;
    case 6:
        list_patients();
        break;
    case 7:
        delete_patient_profile();
        break;
    case 8:
        start_remote_attestation();
        break;
    case 9:
        show_remote_attestation_status();
        break;
    case 10:
        flush_state(true);
        break;
    case 11:
        logout_user();
        break;
    case 0:
        running = false;
        break;
    default:
        cout << "[App] 无效选项。" << endl;
        break;
    }
}

void doctor_menu(bool& running)
{
    print_separator();
    cout << "[医生菜单] 当前用户: " << g_current_username << endl;
    cout << "1. 查看患者列表" << endl;
    cout << "2. 查看患者档案" << endl;
    cout << "3. 查看患者病历" << endl;
    cout << "4. 新增病历" << endl;
    cout << "5. 修改病历" << endl;
    cout << "6. 删除病历" << endl;
    cout << "7. 执行远程认证" << endl;
    cout << "8. 查看远程认证状态" << endl;
    cout << "9. 保存系统状态" << endl;
    cout << "10. 退出登录" << endl;
    cout << "0. 退出程序" << endl;

    int choice = read_int("请选择: ");
    switch (choice) {
    case 1:
        list_patients();
        break;
    case 2:
        view_patient_profile_for(read_line("患者用户名: "));
        break;
    case 3:
        list_records_for(read_line("患者用户名: "));
        break;
    case 4:
        create_record();
        break;
    case 5:
        update_record();
        break;
    case 6:
        delete_record();
        break;
    case 7:
        start_remote_attestation();
        break;
    case 8:
        show_remote_attestation_status();
        break;
    case 9:
        flush_state(true);
        break;
    case 10:
        logout_user();
        break;
    case 0:
        running = false;
        break;
    default:
        cout << "[App] 无效选项。" << endl;
        break;
    }
}

void patient_menu(bool& running)
{
    print_separator();
    cout << "[患者菜单] 当前用户: " << g_current_username << endl;
    cout << "1. 查看我的档案" << endl;
    cout << "2. 修改我的档案" << endl;
    cout << "3. 查看我的病历" << endl;
    cout << "4. 执行远程认证" << endl;
    cout << "5. 查看远程认证状态" << endl;
    cout << "6. 保存系统状态" << endl;
    cout << "7. 退出登录" << endl;
    cout << "0. 退出程序" << endl;

    int choice = read_int("请选择: ");
    switch (choice) {
    case 1:
        view_patient_profile_for(g_current_username);
        break;
    case 2:
        upsert_patient_profile_for(g_current_username);
        break;
    case 3:
        list_records_for(g_current_username);
        break;
    case 4:
        start_remote_attestation();
        break;
    case 5:
        show_remote_attestation_status();
        break;
    case 6:
        flush_state(true);
        break;
    case 7:
        logout_user();
        break;
    case 0:
        running = false;
        break;
    default:
        cout << "[App] 无效选项。" << endl;
        break;
    }
}

void guest_menu(bool& running)
{
    print_separator();
    cout << "[访客菜单]" << endl;
    cout << "1. 初始化系统" << endl;
    cout << "2. 加载系统状态" << endl;
    cout << "3. 登录" << endl;
    cout << "0. 退出程序" << endl;

    int choice = read_int("请选择: ");
    switch (choice) {
    case 1:
        initialize_system();
        break;
    case 2:
        try_load_state();
        break;
    case 3:
        login_user();
        break;
    case 0:
        running = false;
        break;
    default:
        cout << "[App] 无效选项。" << endl;
        break;
    }
}

int main()
{
    sgx_status_t status = SGX_SUCCESS;
    sgx_launch_token_t launch_token = { 0 };
    int launch_token_updated = 0;
    sgx_misc_attribute_t misc_attr = {};
    const std::wstring enclave_path = get_enclave_path();

    printf("==================================================\n");
    printf("   基于 Intel SGX 的密态病历信息系统 - 第一阶段原型\n");
    printf("==================================================\n\n");

    status = sgx_create_enclave(enclave_path.c_str(), SGX_DEBUG_FLAG, &launch_token, &launch_token_updated, &global_eid, &misc_attr);
    if (status != SGX_SUCCESS) {
        wprintf(L"[App] Enclave 创建失败，错误码: 0x%x，目标文件: %ls\n", status, enclave_path.c_str());
        return -1;
    }

    printf("[App] Enclave 初始化成功，ID: %llu\n", global_eid);
    try_load_state();

    bool running = true;
    while (running) {
        if (g_current_role == MSD_ROLE_ADMIN) {
            admin_menu(running);
        }
        else if (g_current_role == MSD_ROLE_DOCTOR) {
            doctor_menu(running);
        }
        else if (g_current_role == MSD_ROLE_PATIENT) {
            patient_menu(running);
        }
        else {
            guest_menu(running);
        }
    }

    flush_state(false);
    if (g_current_role != MSD_ROLE_NONE) {
        logout_user();
    }

    sgx_destroy_enclave(global_eid);
    printf("[App] 程序结束。\n");
    return 0;
}
