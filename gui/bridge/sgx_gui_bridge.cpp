#include "sgx_gui_bridge.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctime>
#include <string>
#include <vector>

#include <windows.h>

#include "sgx_urts.h"
#include "../../MedicalSystemDemo/shared_types.h"
#include "../../MedicalSystemDemo_app/MedicalSystemDemo_u.h"

namespace {

const wchar_t* kEnclaveFileName = L"MedicalSystemDemo.signed.dll";
const wchar_t* kServiceProviderFileName = L"RemoteAttestation_sp.exe";
const size_t kMaxLogBytes = 64 * 1024;

sgx_enclave_id_t g_eid = 0;
std::string g_current_username;
int g_current_role = MSD_ROLE_NONE;
bool g_system_initialized = false;
std::wstring g_runtime_directory;
std::string g_last_error;
std::string g_log_buffer;

std::string wide_to_utf8(const std::wstring& text)
{
    if (text.empty()) {
        return std::string();
    }

    const int required = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, NULL, 0, NULL, NULL);
    if (required <= 1) {
        return std::string();
    }

    std::string output(static_cast<size_t>(required - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, &output[0], required, NULL, NULL);
    return output;
}

bool is_valid_utf8(const char* text)
{
    if (text == NULL) {
        return false;
    }

    const unsigned char* bytes = reinterpret_cast<const unsigned char*>(text);
    while (*bytes != 0) {
        if (*bytes <= 0x7F) {
            ++bytes;
            continue;
        }

        int expected = 0;
        if ((*bytes & 0xE0) == 0xC0) {
            expected = 1;
            if (*bytes < 0xC2) {
                return false;
            }
        }
        else if ((*bytes & 0xF0) == 0xE0) {
            expected = 2;
        }
        else if ((*bytes & 0xF8) == 0xF0) {
            expected = 3;
            if (*bytes > 0xF4) {
                return false;
            }
        }
        else {
            return false;
        }

        ++bytes;
        for (int index = 0; index < expected; ++index, ++bytes) {
            if ((*bytes & 0xC0) != 0x80) {
                return false;
            }
        }
    }

    return true;
}

std::string acp_to_utf8(const char* text)
{
    if (text == NULL || text[0] == '\0') {
        return std::string();
    }

    const int wide_required = MultiByteToWideChar(CP_ACP, 0, text, -1, NULL, 0);
    if (wide_required <= 1) {
        return std::string();
    }

    std::wstring wide(static_cast<size_t>(wide_required - 1), L'\0');
    MultiByteToWideChar(CP_ACP, 0, text, -1, &wide[0], wide_required);
    return wide_to_utf8(wide);
}

std::string utf8_to_acp(const char* text)
{
    if (text == NULL || text[0] == '\0') {
        return std::string();
    }

    const int wide_required = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
    if (wide_required <= 1) {
        return std::string(text);
    }

    std::wstring wide(static_cast<size_t>(wide_required - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text, -1, &wide[0], wide_required);

    const int acp_required = WideCharToMultiByte(CP_ACP, 0, wide.c_str(), -1, NULL, 0, NULL, NULL);
    if (acp_required <= 1) {
        return std::string();
    }

    std::string output(static_cast<size_t>(acp_required - 1), '\0');
    WideCharToMultiByte(CP_ACP, 0, wide.c_str(), -1, &output[0], acp_required, NULL, NULL);
    return output;
}

std::string normalize_text_to_utf8(const char* text)
{
    if (text == NULL || text[0] == '\0') {
        return std::string();
    }

    if (is_valid_utf8(text)) {
        return std::string(text);
    }

    return acp_to_utf8(text);
}

std::wstring utf8_to_wide(const char* text)
{
    if (text == NULL || text[0] == '\0') {
        return std::wstring();
    }

    const int required = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
    if (required <= 1) {
        return std::wstring();
    }

    std::wstring output(static_cast<size_t>(required - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text, -1, &output[0], required);
    return output;
}

std::wstring get_module_directory()
{
    HMODULE module = NULL;
    if (!GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&msd_gui_open),
            &module)) {
        return L".";
    }

    wchar_t module_path[MAX_PATH] = { 0 };
    if (GetModuleFileNameW(module, module_path, MAX_PATH) == 0) {
        return L".";
    }

    std::wstring path(module_path);
    const size_t last_separator = path.find_last_of(L"\\/");
    if (last_separator == std::wstring::npos) {
        return L".";
    }

    return path.substr(0, last_separator);
}

const std::wstring& runtime_directory()
{
    if (g_runtime_directory.empty()) {
        g_runtime_directory = get_module_directory();
    }
    return g_runtime_directory;
}

std::wstring build_runtime_path(const wchar_t* filename)
{
    return runtime_directory() + L"\\" + filename;
}

std::wstring ascii_to_wide(const char* text)
{
    std::wstring output;
    if (text == NULL) {
        return output;
    }

    while (*text != '\0') {
        output.push_back(static_cast<unsigned char>(*text));
        ++text;
    }
    return output;
}

std::wstring storage_path_for(const char* file_name)
{
    return runtime_directory() + L"\\" + ascii_to_wide(file_name);
}

void append_log_line(const char* prefix, const char* message)
{
    const char* safe_prefix = prefix != NULL ? prefix : "";
    const char* safe_message = message != NULL ? message : "";
    g_log_buffer.append(safe_prefix);
    g_log_buffer.append(safe_message);
    g_log_buffer.push_back('\n');
    if (g_log_buffer.size() > kMaxLogBytes) {
        g_log_buffer.erase(0, g_log_buffer.size() - kMaxLogBytes);
    }
}

void append_log_format(const char* prefix, const char* format, ...)
{
    char buffer[1024] = { 0 };
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    append_log_line(prefix, buffer);
}

void set_last_error_format(const char* format, ...)
{
    char buffer[1024] = { 0 };
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    g_last_error = buffer;
    append_log_line("[Bridge-Error] ", g_last_error.c_str());
}

void clear_last_error()
{
    g_last_error.clear();
}

int copy_text_to_buffer(const std::string& text, char* buffer, size_t buffer_size)
{
    if (buffer == NULL || buffer_size == 0) {
        return MSD_ERR_INVALID_INPUT;
    }

    const size_t required = text.size() + 1;
    if (required > buffer_size) {
        memcpy(buffer, text.c_str(), buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
        return MSD_ERR_BUFFER_TOO_SMALL;
    }

    memcpy(buffer, text.c_str(), required);
    return MSD_OK;
}

int normalize_output_buffer(char* buffer, size_t buffer_size)
{
    if (buffer == NULL || buffer_size == 0) {
        return MSD_ERR_INVALID_INPUT;
    }

    const std::string normalized = normalize_text_to_utf8(buffer);
    return copy_text_to_buffer(normalized, buffer, buffer_size);
}

const char* sgx_status_to_text(sgx_status_t status)
{
    switch (status) {
    case SGX_SUCCESS:
        return "success";
    case SGX_ERROR_INVALID_PARAMETER:
        return "invalid parameter";
    case SGX_ERROR_INVALID_ENCLAVE:
        return "invalid enclave file";
    case SGX_ERROR_INVALID_SIGNATURE:
        return "invalid enclave signature";
    case SGX_ERROR_ENCLAVE_FILE_ACCESS:
        return "enclave file access failed";
    case SGX_ERROR_INVALID_METADATA:
        return "invalid enclave metadata";
    case SGX_ERROR_MODE_INCOMPATIBLE:
        return "mode incompatible";
    default:
        return "unknown sgx error";
    }
}

const char* result_to_text(int result)
{
    switch (result) {
    case MSD_OK:
        return "success";
    case MSD_ERR_INVALID_INPUT:
        return "invalid input";
    case MSD_ERR_NOT_INITIALIZED:
        return "system not initialized";
    case MSD_ERR_ALREADY_INITIALIZED:
        return "system already initialized";
    case MSD_ERR_NO_PERMISSION:
        return "permission denied";
    case MSD_ERR_USER_EXISTS:
        return "user already exists";
    case MSD_ERR_USER_NOT_FOUND:
        return "user not found";
    case MSD_ERR_AUTH_FAILED:
        return "authentication failed";
    case MSD_ERR_PROFILE_NOT_FOUND:
        return "patient profile not found";
    case MSD_ERR_RECORD_NOT_FOUND:
        return "record not found";
    case MSD_ERR_STORAGE_FAILED:
        return "secure storage failed";
    case MSD_ERR_BUFFER_TOO_SMALL:
        return "buffer too small";
    case MSD_ERR_CAPACITY_REACHED:
        return "capacity reached";
    case MSD_ERR_INVALID_ROLE:
        return "invalid role";
    case MSD_ERR_FILE_NOT_FOUND:
        return "sealed state file not found";
    case MSD_ERR_STATE_CORRUPTED:
        return "sealed state corrupted";
    case MSD_ERR_INTERNAL:
        return "internal error";
    case MSD_ERR_SESSION_ACTIVE:
        return "session already active";
    case MSD_ERR_PROFILE_HAS_RECORDS:
        return "patient profile still has records";
    case MSD_ERR_RA_REQUIRED:
        return "remote attestation required";
    case MSD_ERR_RA_CONTEXT_NOT_READY:
        return "ra context not ready";
    case MSD_ERR_RA_BUSY:
        return "ra busy";
    case MSD_ERR_RA_VERIFY_FAILED:
        return "ra verification failed";
    case MSD_ERR_SECURE_CHANNEL_NOT_READY:
        return "secure channel not ready";
    case MSD_ERR_RA_MSG_INVALID:
        return "invalid ra message";
    case MSD_ERR_RA_UNSUPPORTED:
        return "ra unsupported";
    default:
        return "unknown error";
    }
}

const char* role_to_text(int role)
{
    switch (role) {
    case MSD_ROLE_ADMIN:
        return "admin";
    case MSD_ROLE_DOCTOR:
        return "doctor";
    case MSD_ROLE_PATIENT:
        return "patient";
    default:
        return "guest";
    }
}

const char* ra_status_to_text(int status)
{
    switch (status) {
    case MSD_RA_NOT_STARTED:
        return "not started";
    case MSD_RA_CONTEXT_READY:
        return "context ready";
    case MSD_RA_MSG1_READY:
        return "msg1 ready";
    case MSD_RA_WAITING_MSG2:
        return "waiting msg2";
    case MSD_RA_MSG3_READY:
        return "msg3 ready";
    case MSD_RA_WAITING_RESULT:
        return "waiting result";
    case MSD_RA_VERIFIED:
        return "verified";
    case MSD_RA_FAILED:
        return "failed";
    default:
        return "unknown";
    }
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

bool read_binary_file(const std::wstring& path, std::vector<uint8_t>& data)
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

bool file_exists(const std::wstring& path)
{
    const DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

bool invoke_service_provider(const uint8_t* request, size_t request_size, std::vector<uint8_t>& response, std::string& error_text)
{
    response.clear();
    error_text.clear();

    const std::wstring provider_path = build_runtime_path(kServiceProviderFileName);
    if (!file_exists(provider_path)) {
        error_text = "RemoteAttestation_sp.exe not found: " + wide_to_utf8(provider_path);
        return false;
    }

    const DWORD process_id = GetCurrentProcessId();
    const std::wstring request_path = runtime_directory() + L"\\ra_request_" + std::to_wstring(process_id) + L".bin";
    const std::wstring response_path = runtime_directory() + L"\\ra_response_" + std::to_wstring(process_id) + L".bin";
    DeleteFileW(request_path.c_str());
    DeleteFileW(response_path.c_str());

    if (!write_binary_file(request_path, request, request_size)) {
        error_text = "Failed to write RA request file: " + wide_to_utf8(request_path);
        return false;
    }

    append_log_format("[Bridge-RA] ", "Invoke service_provider: %s", wide_to_utf8(provider_path).c_str());
    append_log_format("[Bridge-RA] ", "Request file: %s", wide_to_utf8(request_path).c_str());
    append_log_format("[Bridge-RA] ", "Response file: %s", wide_to_utf8(response_path).c_str());

    std::wstring command_line = L"\"" + provider_path + L"\" \"" + request_path + L"\" \"" + response_path + L"\"";
    std::vector<wchar_t> command_buffer(command_line.begin(), command_line.end());
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
        runtime_directory().c_str(),
        &startup_info,
        &process_info);
    if (!created) {
        DeleteFileW(request_path.c_str());
        error_text = "Failed to launch RemoteAttestation_sp.exe";
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
        error_text = "RemoteAttestation_sp.exe exited with code: " + std::to_string(exit_code);
        return false;
    }

    if (!read_binary_file(response_path, response)) {
        DeleteFileW(request_path.c_str());
        DeleteFileW(response_path.c_str());
        error_text = "Failed to read RA response file: " + wide_to_utf8(response_path);
        return false;
    }

    DeleteFileW(request_path.c_str());
    DeleteFileW(response_path.c_str());
    return true;
}

bool ensure_enclave_ready()
{
    clear_last_error();
    if (g_eid != 0) {
        return true;
    }

    const std::wstring enclave_path = build_runtime_path(kEnclaveFileName);
    sgx_launch_token_t launch_token = { 0 };
    int launch_token_updated = 0;
    sgx_misc_attribute_t misc_attr = {};

    const sgx_status_t status = sgx_create_enclave(
        enclave_path.c_str(),
        SGX_DEBUG_FLAG,
        &launch_token,
        &launch_token_updated,
        &g_eid,
        &misc_attr);
    if (status != SGX_SUCCESS) {
        set_last_error_format("Failed to create enclave: 0x%x (%s)", status, sgx_status_to_text(status));
        return false;
    }

    append_log_format("[Bridge] ", "Enclave created, id=%llu", static_cast<unsigned long long>(g_eid));
    return true;
}

int run_simple_ecall(const char* action, sgx_status_t (*ecall)(sgx_enclave_id_t, int*))
{
    if (!ensure_enclave_ready()) {
        return MSD_ERR_INTERNAL;
    }

    int result = MSD_OK;
    const sgx_status_t status = ecall(g_eid, &result);
    if (status != SGX_SUCCESS) {
        set_last_error_format("%s failed: 0x%x (%s)", action, status, sgx_status_to_text(status));
        return MSD_ERR_INTERNAL;
    }

    append_log_format("[Bridge] ", "%s: %s", action, result_to_text(result));
    return result;
}

} // namespace

void ocall_print_log(const char* str)
{
    const std::string converted = normalize_text_to_utf8(str);
    append_log_line("[Enclave-Log] ", converted.c_str());
}

void ocall_save_blob(const char* file_name, const uint8_t* data, size_t data_size, int* result)
{
    if (result != NULL) {
        *result = MSD_OK;
    }

    const std::wstring output_path = storage_path_for(file_name);
    if (!write_binary_file(output_path, data, data_size)) {
        if (result != NULL) {
            *result = MSD_ERR_STORAGE_FAILED;
        }
        append_log_format("[Bridge-Storage] ", "Failed to write sealed state: %s", wide_to_utf8(output_path).c_str());
        return;
    }

    append_log_format("[Bridge-Storage] ", "Sealed state saved to: %s", wide_to_utf8(output_path).c_str());
}

void ocall_load_blob(const char* file_name, uint8_t* data, size_t max_size, size_t* actual_size, int* result)
{
    if (actual_size != NULL) {
        *actual_size = 0;
    }
    if (result != NULL) {
        *result = MSD_OK;
    }

    const std::wstring input_path = storage_path_for(file_name);
    FILE* file = NULL;
    _wfopen_s(&file, input_path.c_str(), L"rb");
    if (file == NULL) {
        if (result != NULL) {
            *result = MSD_ERR_FILE_NOT_FOUND;
        }
        append_log_format("[Bridge-Storage] ", "Sealed state file not found: %s", wide_to_utf8(input_path).c_str());
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
        append_log_format("[Bridge-Storage] ", "Invalid sealed state size: %s", wide_to_utf8(input_path).c_str());
        return;
    }

    const size_t bytes_read = fread(data, 1, static_cast<size_t>(size), file);
    fclose(file);
    if (bytes_read != static_cast<size_t>(size)) {
        if (result != NULL) {
            *result = MSD_ERR_STORAGE_FAILED;
        }
        append_log_format("[Bridge-Storage] ", "Failed to read sealed state: %s", wide_to_utf8(input_path).c_str());
        return;
    }

    if (actual_size != NULL) {
        *actual_size = static_cast<size_t>(size);
    }

    append_log_format("[Bridge-Storage] ", "Sealed state loaded from: %s", wide_to_utf8(input_path).c_str());
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
    append_log_format("[Bridge-RA] ", "Enclave requested send, type=%u, size=%u", message_type, static_cast<unsigned int>(data_size));
    (void)data;
}

void ocall_ra_recv_msg(uint32_t expected_message_type, uint8_t* buffer, size_t buffer_size, size_t* actual_size, int* result)
{
    if (actual_size != NULL) {
        *actual_size = 0;
    }
    if (buffer != NULL && buffer_size > 0) {
        buffer[0] = 0;
    }
    if (result != NULL) {
        *result = MSD_ERR_RA_UNSUPPORTED;
    }

    append_log_format("[Bridge-RA] ", "ocall_ra_recv_msg not used directly, expected type=%u", expected_message_type);
}

int msd_gui_open()
{
    return ensure_enclave_ready() ? MSD_OK : MSD_ERR_INTERNAL;
}

void msd_gui_close()
{
    if (g_eid != 0) {
        sgx_destroy_enclave(g_eid);
        append_log_line("[Bridge] ", "Enclave destroyed.");
    }
    g_eid = 0;
    g_current_username.clear();
    g_current_role = MSD_ROLE_NONE;
    g_system_initialized = false;
    clear_last_error();
}

int msd_gui_init_system()
{
    const int result = run_simple_ecall("Init system", ecall_system_init);
    if (result == MSD_OK || result == MSD_ERR_ALREADY_INITIALIZED) {
        g_system_initialized = true;
    }
    return result;
}

int msd_gui_load_system()
{
    const int result = run_simple_ecall("Load system state", ecall_system_load);
    if (result == MSD_OK) {
        g_system_initialized = true;
    }
    return result;
}

int msd_gui_flush_system()
{
    return run_simple_ecall("Flush system state", ecall_system_flush);
}

int msd_gui_login(const char* username, const char* password, int* out_role)
{
    if (out_role != NULL) {
        *out_role = MSD_ROLE_NONE;
    }
    if (!ensure_enclave_ready()) {
        return MSD_ERR_INTERNAL;
    }

    const std::string username_acp = utf8_to_acp(username);
    const std::string password_acp = utf8_to_acp(password);
    int role = MSD_ROLE_NONE;
    int result = MSD_OK;
    const sgx_status_t status = ecall_login_user(g_eid, username_acp.c_str(), password_acp.c_str(), &role, &result);
    if (status != SGX_SUCCESS) {
        set_last_error_format("Login failed: 0x%x (%s)", status, sgx_status_to_text(status));
        return MSD_ERR_INTERNAL;
    }

    if (result == MSD_OK) {
        g_current_username = username != NULL ? username : "";
        g_current_role = role;
        if (out_role != NULL) {
            *out_role = role;
        }
    }
    append_log_format("[Bridge] ", "Login result: %s", result_to_text(result));
    return result;
}

int msd_gui_logout()
{
    const int result = run_simple_ecall("Logout", ecall_logout_user);
    if (result == MSD_OK) {
        g_current_username.clear();
        g_current_role = MSD_ROLE_NONE;
    }
    return result;
}

int msd_gui_get_current_role(int* out_role)
{
    if (out_role == NULL) {
        return MSD_ERR_INVALID_INPUT;
    }
    *out_role = g_current_role;
    return MSD_OK;
}

int msd_gui_get_current_username(char* buffer, size_t buffer_size)
{
    return copy_text_to_buffer(g_current_username, buffer, buffer_size);
}

int msd_gui_register_user(const char* username, const char* password, int role)
{
    if (!ensure_enclave_ready()) {
        return MSD_ERR_INTERNAL;
    }

    const std::string username_acp = utf8_to_acp(username);
    const std::string password_acp = utf8_to_acp(password);
    int result = MSD_OK;
    const sgx_status_t status = ecall_register_user(g_eid, username_acp.c_str(), password_acp.c_str(), role, &result);
    if (status != SGX_SUCCESS) {
        set_last_error_format("Register user failed: 0x%x (%s)", status, sgx_status_to_text(status));
        return MSD_ERR_INTERNAL;
    }
    append_log_format("[Bridge] ", "Register user result: %s", result_to_text(result));
    return result;
}

int msd_gui_list_users(char* buffer, size_t buffer_size)
{
    if (!ensure_enclave_ready()) {
        return MSD_ERR_INTERNAL;
    }

    int result = MSD_OK;
    const sgx_status_t status = ecall_list_users(g_eid, buffer, buffer_size, &result);
    if (status != SGX_SUCCESS) {
        set_last_error_format("List users failed: 0x%x (%s)", status, sgx_status_to_text(status));
        return MSD_ERR_INTERNAL;
    }
    normalize_output_buffer(buffer, buffer_size);
    return result;
}

int msd_gui_upsert_patient_profile(const char* patient_username, const char* full_name, const char* id_card, const char* phone, const char* address)
{
    if (!ensure_enclave_ready()) {
        return MSD_ERR_INTERNAL;
    }

    const std::string patient_username_acp = utf8_to_acp(patient_username);
    const std::string full_name_acp = utf8_to_acp(full_name);
    const std::string id_card_acp = utf8_to_acp(id_card);
    const std::string phone_acp = utf8_to_acp(phone);
    const std::string address_acp = utf8_to_acp(address);
    int result = MSD_OK;
    const sgx_status_t status = ecall_upsert_patient_profile(
        g_eid,
        patient_username_acp.c_str(),
        full_name_acp.c_str(),
        id_card_acp.c_str(),
        phone_acp.c_str(),
        address_acp.c_str(),
        &result);
    if (status != SGX_SUCCESS) {
        set_last_error_format("Upsert patient profile failed: 0x%x (%s)", status, sgx_status_to_text(status));
        return MSD_ERR_INTERNAL;
    }
    append_log_format("[Bridge] ", "Upsert patient result: %s", result_to_text(result));
    return result;
}

int msd_gui_get_patient_profile(const char* patient_username, char* buffer, size_t buffer_size)
{
    if (!ensure_enclave_ready()) {
        return MSD_ERR_INTERNAL;
    }

    const std::string patient_username_acp = utf8_to_acp(patient_username);
    int result = MSD_OK;
    const sgx_status_t status = ecall_get_patient_profile(g_eid, patient_username_acp.c_str(), buffer, buffer_size, &result);
    if (status != SGX_SUCCESS) {
        set_last_error_format("Get patient profile failed: 0x%x (%s)", status, sgx_status_to_text(status));
        return MSD_ERR_INTERNAL;
    }
    normalize_output_buffer(buffer, buffer_size);
    return result;
}

int msd_gui_list_patients(char* buffer, size_t buffer_size)
{
    if (!ensure_enclave_ready()) {
        return MSD_ERR_INTERNAL;
    }

    int result = MSD_OK;
    const sgx_status_t status = ecall_list_patients(g_eid, buffer, buffer_size, &result);
    if (status != SGX_SUCCESS) {
        set_last_error_format("List patients failed: 0x%x (%s)", status, sgx_status_to_text(status));
        return MSD_ERR_INTERNAL;
    }
    normalize_output_buffer(buffer, buffer_size);
    return result;
}

int msd_gui_delete_patient_profile(const char* patient_username)
{
    if (!ensure_enclave_ready()) {
        return MSD_ERR_INTERNAL;
    }

    const std::string patient_username_acp = utf8_to_acp(patient_username);
    int result = MSD_OK;
    const sgx_status_t status = ecall_delete_patient_profile(g_eid, patient_username_acp.c_str(), &result);
    if (status != SGX_SUCCESS) {
        set_last_error_format("Delete patient profile failed: 0x%x (%s)", status, sgx_status_to_text(status));
        return MSD_ERR_INTERNAL;
    }
    append_log_format("[Bridge] ", "Delete patient result: %s", result_to_text(result));
    return result;
}

int msd_gui_create_record(const char* patient_username, const char* diagnosis, const char* prescription, const char* note, int* out_record_id)
{
    if (out_record_id != NULL) {
        *out_record_id = 0;
    }
    if (!ensure_enclave_ready()) {
        return MSD_ERR_INTERNAL;
    }

    const std::string patient_username_acp = utf8_to_acp(patient_username);
    const std::string diagnosis_acp = utf8_to_acp(diagnosis);
    const std::string prescription_acp = utf8_to_acp(prescription);
    const std::string note_acp = utf8_to_acp(note);
    int record_id = 0;
    int result = MSD_OK;
    const sgx_status_t status = ecall_create_record(
        g_eid,
        patient_username_acp.c_str(),
        diagnosis_acp.c_str(),
        prescription_acp.c_str(),
        note_acp.c_str(),
        &record_id,
        &result);
    if (status != SGX_SUCCESS) {
        set_last_error_format("Create record failed: 0x%x (%s)", status, sgx_status_to_text(status));
        return MSD_ERR_INTERNAL;
    }

    if (result == MSD_OK && out_record_id != NULL) {
        *out_record_id = record_id;
    }
    append_log_format("[Bridge] ", "Create record result: %s", result_to_text(result));
    return result;
}

int msd_gui_list_records(const char* patient_username, char* buffer, size_t buffer_size)
{
    if (!ensure_enclave_ready()) {
        return MSD_ERR_INTERNAL;
    }

    const std::string patient_username_acp = utf8_to_acp(patient_username);
    int result = MSD_OK;
    const sgx_status_t status = ecall_list_records(g_eid, patient_username_acp.c_str(), buffer, buffer_size, &result);
    if (status != SGX_SUCCESS) {
        set_last_error_format("List records failed: 0x%x (%s)", status, sgx_status_to_text(status));
        return MSD_ERR_INTERNAL;
    }
    normalize_output_buffer(buffer, buffer_size);
    return result;
}

int msd_gui_update_record(int record_id, const char* diagnosis, const char* prescription, const char* note)
{
    if (!ensure_enclave_ready()) {
        return MSD_ERR_INTERNAL;
    }

    const std::string diagnosis_acp = utf8_to_acp(diagnosis);
    const std::string prescription_acp = utf8_to_acp(prescription);
    const std::string note_acp = utf8_to_acp(note);
    int result = MSD_OK;
    const sgx_status_t status = ecall_update_record(
        g_eid,
        record_id,
        diagnosis_acp.c_str(),
        prescription_acp.c_str(),
        note_acp.c_str(),
        &result);
    if (status != SGX_SUCCESS) {
        set_last_error_format("Update record failed: 0x%x (%s)", status, sgx_status_to_text(status));
        return MSD_ERR_INTERNAL;
    }
    append_log_format("[Bridge] ", "Update record result: %s", result_to_text(result));
    return result;
}

int msd_gui_delete_record(int record_id)
{
    if (!ensure_enclave_ready()) {
        return MSD_ERR_INTERNAL;
    }

    int result = MSD_OK;
    const sgx_status_t status = ecall_delete_record(g_eid, record_id, &result);
    if (status != SGX_SUCCESS) {
        set_last_error_format("Delete record failed: 0x%x (%s)", status, sgx_status_to_text(status));
        return MSD_ERR_INTERNAL;
    }
    append_log_format("[Bridge] ", "Delete record result: %s", result_to_text(result));
    return result;
}

int msd_gui_ra_start(const char* peer_identity, int* out_ra_status, int* out_secure_channel)
{
    if (out_ra_status != NULL) {
        *out_ra_status = MSD_RA_NOT_STARTED;
    }
    if (out_secure_channel != NULL) {
        *out_secure_channel = 0;
    }
    if (!ensure_enclave_ready()) {
        return MSD_ERR_INTERNAL;
    }

    const char* safe_peer_identity = (peer_identity != NULL && peer_identity[0] != '\0') ? peer_identity : "service_provider";
    int result = MSD_OK;

    sgx_status_t status = ecall_ra_init_context(g_eid, safe_peer_identity, &result);
    if (status != SGX_SUCCESS) {
        set_last_error_format("RA init failed: 0x%x (%s)", status, sgx_status_to_text(status));
        return MSD_ERR_INTERNAL;
    }
    if (result != MSD_OK) {
        append_log_format("[Bridge-RA] ", "RA init result: %s", result_to_text(result));
        return result;
    }

    uint8_t msg1[MSD_RA_MAX_MESSAGE_SIZE] = { 0 };
    size_t msg1_size = 0;
    status = ecall_ra_get_msg1(g_eid, msg1, sizeof(msg1), &msg1_size, &result);
    if (status != SGX_SUCCESS) {
        set_last_error_format("Generate msg1 failed: 0x%x (%s)", status, sgx_status_to_text(status));
        return MSD_ERR_INTERNAL;
    }
    if (result != MSD_OK) {
        append_log_format("[Bridge-RA] ", "Generate msg1 result: %s", result_to_text(result));
        return result;
    }

    std::vector<uint8_t> msg2_response;
    std::string transport_error;
    if (!invoke_service_provider(msg1, msg1_size, msg2_response, transport_error)) {
        set_last_error_format("%s", transport_error.c_str());
        return MSD_ERR_INTERNAL;
    }
    if (msg2_response.empty()) {
        set_last_error_format("Service provider returned empty msg2");
        return MSD_ERR_INTERNAL;
    }

    uint32_t msg2_type = MSD_RA_MSG_NONE;
    const uint8_t* msg2_payload = NULL;
    size_t msg2_payload_size = 0;
    if (!decode_ra_message(&msg2_response[0], msg2_response.size(), msg2_type, msg2_payload, msg2_payload_size) || msg2_type != MSD_RA_MSG2) {
        set_last_error_format("Invalid msg2 format from service provider");
        return MSD_ERR_INTERNAL;
    }

    uint8_t msg3[MSD_RA_MAX_MESSAGE_SIZE] = { 0 };
    size_t msg3_size = 0;
    status = ecall_ra_proc_msg2_get_msg3(g_eid, msg2_payload, msg2_payload_size, msg3, sizeof(msg3), &msg3_size, &result);
    if (status != SGX_SUCCESS) {
        set_last_error_format("Process msg2/get msg3 failed: 0x%x (%s)", status, sgx_status_to_text(status));
        return MSD_ERR_INTERNAL;
    }
    if (result != MSD_OK) {
        append_log_format("[Bridge-RA] ", "Process msg2/get msg3 result: %s", result_to_text(result));
        return result;
    }

    std::vector<uint8_t> result_response;
    if (!invoke_service_provider(msg3, msg3_size, result_response, transport_error)) {
        set_last_error_format("%s", transport_error.c_str());
        return MSD_ERR_INTERNAL;
    }
    if (result_response.empty()) {
        set_last_error_format("Service provider returned empty attestation result");
        return MSD_ERR_INTERNAL;
    }

    uint32_t result_type = MSD_RA_MSG_NONE;
    const uint8_t* result_payload = NULL;
    size_t result_payload_size = 0;
    if (!decode_ra_message(&result_response[0], result_response.size(), result_type, result_payload, result_payload_size) || result_type != MSD_RA_ATT_RESULT) {
        set_last_error_format("Invalid attestation result format from service provider");
        return MSD_ERR_INTERNAL;
    }

    status = ecall_ra_finalize(g_eid, result_payload, result_payload_size, &result);
    if (status != SGX_SUCCESS) {
        set_last_error_format("Finalize RA failed: 0x%x (%s)", status, sgx_status_to_text(status));
        return MSD_ERR_INTERNAL;
    }
    if (result != MSD_OK) {
        append_log_format("[Bridge-RA] ", "Finalize RA result: %s", result_to_text(result));
        return result;
    }

    return msd_gui_ra_get_status(out_ra_status, out_secure_channel);
}

int msd_gui_ra_get_status(int* out_ra_status, int* out_secure_channel)
{
    if (!ensure_enclave_ready()) {
        return MSD_ERR_INTERNAL;
    }

    int ra_status = MSD_RA_NOT_STARTED;
    int secure_channel = 0;
    int result = MSD_OK;
    const sgx_status_t status = ecall_ra_get_status(g_eid, &ra_status, &secure_channel, &result);
    if (status != SGX_SUCCESS) {
        set_last_error_format("Get RA status failed: 0x%x (%s)", status, sgx_status_to_text(status));
        return MSD_ERR_INTERNAL;
    }

    if (out_ra_status != NULL) {
        *out_ra_status = ra_status;
    }
    if (out_secure_channel != NULL) {
        *out_secure_channel = secure_channel;
    }

    append_log_format("[Bridge-RA] ", "RA status=%s, secure_channel=%s", ra_status_to_text(ra_status), secure_channel ? "ready" : "not_ready");
    return result;
}

int msd_gui_get_log_text(char* buffer, size_t buffer_size)
{
    return copy_text_to_buffer(g_log_buffer, buffer, buffer_size);
}

void msd_gui_clear_log()
{
    g_log_buffer.clear();
}

int msd_gui_get_last_error(char* buffer, size_t buffer_size)
{
    return copy_text_to_buffer(g_last_error, buffer, buffer_size);
}

int msd_gui_get_runtime_directory(char* buffer, size_t buffer_size)
{
    return copy_text_to_buffer(wide_to_utf8(runtime_directory()), buffer, buffer_size);
}

int msd_gui_get_storage_path(char* buffer, size_t buffer_size)
{
    return copy_text_to_buffer(wide_to_utf8(storage_path_for(MSD_STATE_FILE_NAME)), buffer, buffer_size);
}

int msd_gui_set_runtime_directory_utf8(const char* directory_utf8)
{
    std::wstring value = utf8_to_wide(directory_utf8);
    if (value.empty()) {
        return MSD_ERR_INVALID_INPUT;
    }

    g_runtime_directory = value;
    append_log_format("[Bridge] ", "Runtime directory set to: %s", wide_to_utf8(g_runtime_directory).c_str());
    return MSD_OK;
}

int msd_gui_result_text(int result_code, char* buffer, size_t buffer_size)
{
    return copy_text_to_buffer(result_to_text(result_code), buffer, buffer_size);
}

int msd_gui_role_text(int role, char* buffer, size_t buffer_size)
{
    return copy_text_to_buffer(role_to_text(role), buffer, buffer_size);
}

int msd_gui_ra_status_text(int ra_status, char* buffer, size_t buffer_size)
{
    return copy_text_to_buffer(ra_status_to_text(ra_status), buffer, buffer_size);
}
