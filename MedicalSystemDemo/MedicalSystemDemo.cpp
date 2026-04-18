#include "MedicalSystemDemo_t.h"

#include "shared_types.h"
#include "sgx_tseal.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

namespace {

MsdSystemState g_state;
MsdRaSessionState g_ra_state;
uint32_t g_ra_next_session_id = 1;

size_t safe_strlen_limit(const char* text, size_t limit)
{
    size_t index = 0;
    if (text == NULL) {
        return 0;
    }

    while (index < limit && text[index] != '\0') {
        ++index;
    }

    return index;
}

bool text_fits(const char* text, size_t limit)
{
    return text != NULL && safe_strlen_limit(text, limit) < limit;
}

bool text_empty(const char* text)
{
    return text == NULL || text[0] == '\0';
}

void write_result(int* result, int code);

void copy_text(char* destination, size_t destination_size, const char* source)
{
    if (destination == NULL || destination_size == 0) {
        return;
    }

    if (source == NULL) {
        destination[0] = '\0';
        return;
    }

    snprintf(destination, destination_size, "%s", source);
}

void reset_ra_state()
{
    memset(&g_ra_state, 0, sizeof(g_ra_state));
    g_ra_state.status = MSD_RA_NOT_STARTED;
}

void clear_state()
{
    memset(&g_state, 0, sizeof(g_state));
    g_state.magic = MSD_STATE_MAGIC;
    g_state.version = MSD_STATE_VERSION;
    g_state.next_record_id = 1;
    g_state.active_session_index = -1;
    reset_ra_state();
}

void reset_session()
{
    g_state.active_session_index = -1;
    reset_ra_state();
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
        return "未知";
    }
}

bool valid_role(int role)
{
    return role == MSD_ROLE_ADMIN || role == MSD_ROLE_DOCTOR || role == MSD_ROLE_PATIENT;
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

void enclave_log(const char* format, ...)
{
    char buffer[512] = { 0 };
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    ocall_print_log(buffer);
}

bool append_format(char* buffer, size_t buffer_size, size_t* offset, const char* format, ...)
{
    if (buffer == NULL || offset == NULL || *offset >= buffer_size) {
        return false;
    }

    va_list args;
    va_start(args, format);
    int written = vsnprintf(buffer + *offset, buffer_size - *offset, format, args);
    va_end(args);

    if (written < 0) {
        return false;
    }

    if ((size_t)written >= buffer_size - *offset) {
        *offset = buffer_size - 1;
        buffer[buffer_size - 1] = '\0';
        return false;
    }

    *offset += (size_t)written;
    return true;
}

int find_user_index(const char* username)
{
    for (int index = 0; index < MSD_MAX_USERS; ++index) {
        if (g_state.users[index].active && strcmp(g_state.users[index].username, username) == 0) {
            return index;
        }
    }
    return -1;
}

int find_patient_index(const char* username)
{
    for (int index = 0; index < MSD_MAX_PATIENTS; ++index) {
        if (g_state.patients[index].active && strcmp(g_state.patients[index].patient_username, username) == 0) {
            return index;
        }
    }
    return -1;
}

int find_record_index(int record_id)
{
    for (int index = 0; index < MSD_MAX_RECORDS; ++index) {
        if (g_state.records[index].active && g_state.records[index].record_id == record_id) {
            return index;
        }
    }
    return -1;
}

MsdUserAccount* current_user()
{
    if (g_state.active_session_index < 0 || g_state.active_session_index >= MSD_MAX_USERS) {
        return NULL;
    }

    if (!g_state.users[g_state.active_session_index].active) {
        return NULL;
    }

    return &g_state.users[g_state.active_session_index];
}

int allocate_user_slot()
{
    for (int index = 0; index < MSD_MAX_USERS; ++index) {
        if (!g_state.users[index].active) {
            return index;
        }
    }
    return -1;
}

int allocate_patient_slot()
{
    for (int index = 0; index < MSD_MAX_PATIENTS; ++index) {
        if (!g_state.patients[index].active) {
            return index;
        }
    }
    return -1;
}

int allocate_record_slot()
{
    for (int index = 0; index < MSD_MAX_RECORDS; ++index) {
        if (!g_state.records[index].active) {
            return index;
        }
    }
    return -1;
}

bool has_role(int role)
{
    MsdUserAccount* user = current_user();
    return user != NULL && user->role == role;
}

bool has_active_session()
{
    return current_user() != NULL;
}

bool can_view_patient(const char* patient_username)
{
    MsdUserAccount* user = current_user();
    if (user == NULL) {
        return false;
    }

    if (user->role == MSD_ROLE_ADMIN || user->role == MSD_ROLE_DOCTOR) {
        return true;
    }

    return user->role == MSD_ROLE_PATIENT && strcmp(user->username, patient_username) == 0;
}

bool can_edit_patient(const char* patient_username)
{
    MsdUserAccount* user = current_user();
    if (user == NULL) {
        return false;
    }

    if (user->role == MSD_ROLE_ADMIN) {
        return true;
    }

    return user->role == MSD_ROLE_PATIENT && strcmp(user->username, patient_username) == 0;
}

bool patient_user_exists(const char* username)
{
    int user_index = find_user_index(username);
    return user_index >= 0 && g_state.users[user_index].role == MSD_ROLE_PATIENT;
}

bool patient_profile_exists(const char* username)
{
    return find_patient_index(username) >= 0;
}

bool patient_has_records(const char* username)
{
    for (int index = 0; index < MSD_MAX_RECORDS; ++index) {
        if (g_state.records[index].active && strcmp(g_state.records[index].patient_username, username) == 0) {
            return true;
        }
    }
    return false;
}

bool secure_channel_ready()
{
    return g_ra_state.status == MSD_RA_VERIFIED && g_ra_state.secure_channel_ready != 0;
}

bool attested_channel_required_for_user(const MsdUserAccount* user)
{
    return user != NULL && (user->role == MSD_ROLE_DOCTOR || user->role == MSD_ROLE_PATIENT);
}

bool ensure_attested_channel_for_user(int* result)
{
    MsdUserAccount* user = current_user();
    if (!attested_channel_required_for_user(user)) {
        return true;
    }

    if (secure_channel_ready()) {
        return true;
    }

    enclave_log("[Enclave-RA] 用户 %s 执行敏感操作前尚未完成远程认证。", user->username);
    write_result(result, MSD_ERR_RA_REQUIRED);
    return false;
}

bool build_ra_message(
    uint32_t message_type,
    const char* payload_text,
    uint8_t* buffer,
    size_t buffer_size,
    size_t* actual_size)
{
    if (actual_size != NULL) {
        *actual_size = 0;
    }

    if (buffer == NULL || actual_size == NULL) {
        return false;
    }

    const char* payload = payload_text != NULL ? payload_text : "";
    const uint32_t payload_size = (uint32_t)safe_strlen_limit(payload, MSD_RA_MAX_MESSAGE_SIZE);
    const size_t total_size = sizeof(uint32_t) * 2 + payload_size;
    if (total_size > buffer_size) {
        return false;
    }

    memcpy(buffer, &message_type, sizeof(uint32_t));
    memcpy(buffer + sizeof(uint32_t), &payload_size, sizeof(uint32_t));
    if (payload_size > 0) {
        memcpy(buffer + sizeof(uint32_t) * 2, payload, payload_size);
    }

    *actual_size = total_size;
    return true;
}

bool attestation_result_is_success(const uint8_t* data, size_t data_size)
{
    if (data == NULL || data_size == 0) {
        return false;
    }

    if (data_size == 1 && data[0] == 1) {
        return true;
    }

    char text[32] = { 0 };
    size_t copy_size = data_size < sizeof(text) - 1 ? data_size : sizeof(text) - 1;
    memcpy(text, data, copy_size);
    text[copy_size] = '\0';

    return strcmp(text, "OK") == 0 || strcmp(text, "TRUSTED") == 0;
}

int create_default_admin()
{
    int slot = allocate_user_slot();
    if (slot < 0) {
        return MSD_ERR_CAPACITY_REACHED;
    }

    MsdUserAccount* user = &g_state.users[slot];
    memset(user, 0, sizeof(*user));
    copy_text(user->username, sizeof(user->username), "admin");
    copy_text(user->password, sizeof(user->password), "admin123");
    user->role = MSD_ROLE_ADMIN;
    user->active = 1;
    return MSD_OK;
}

int flush_state_internal()
{
    if (!g_state.initialized) {
        return MSD_ERR_NOT_INITIALIZED;
    }

    MsdSystemState* snapshot = static_cast<MsdSystemState*>(malloc(sizeof(MsdSystemState)));
    if (snapshot == NULL) {
        return MSD_ERR_INTERNAL;
    }

    *snapshot = g_state;
    snapshot->active_session_index = -1;

    const uint32_t sealed_size = sgx_calc_sealed_data_size(0, (uint32_t)sizeof(MsdSystemState));
    if (sealed_size == UINT32_MAX) {
        free(snapshot);
        return MSD_ERR_INTERNAL;
    }

    uint8_t* sealed_blob = static_cast<uint8_t*>(malloc(sealed_size));
    if (sealed_blob == NULL) {
        free(snapshot);
        return MSD_ERR_INTERNAL;
    }
    memset(sealed_blob, 0, sealed_size);

    sgx_status_t seal_status = sgx_seal_data(
        0,
        NULL,
        (uint32_t)sizeof(MsdSystemState),
        reinterpret_cast<const uint8_t*>(snapshot),
        sealed_size,
        reinterpret_cast<sgx_sealed_data_t*>(sealed_blob));

    if (seal_status != SGX_SUCCESS) {
        free(sealed_blob);
        free(snapshot);
        enclave_log("[Enclave] 数据密封失败，错误码: 0x%x", seal_status);
        return MSD_ERR_STORAGE_FAILED;
    }

    int ocall_result = MSD_OK;
    ocall_save_blob(MSD_STATE_FILE_NAME, sealed_blob, sealed_size, &ocall_result);
    free(sealed_blob);
    free(snapshot);
    if (ocall_result != MSD_OK) {
        enclave_log("[Enclave] 密封数据写盘失败，错误码: %d", ocall_result);
        return ocall_result;
    }

    enclave_log("[Enclave] 系统状态已密封并请求写入文件: %s", MSD_STATE_FILE_NAME);
    return MSD_OK;
}

int load_state_internal()
{
    const size_t max_sealed_size = sgx_calc_sealed_data_size(0, (uint32_t)sizeof(MsdSystemState));
    if (max_sealed_size == UINT32_MAX) {
        return MSD_ERR_INTERNAL;
    }

    uint8_t* sealed_blob = static_cast<uint8_t*>(malloc(max_sealed_size));
    if (sealed_blob == NULL) {
        return MSD_ERR_INTERNAL;
    }
    memset(sealed_blob, 0, max_sealed_size);

    size_t actual_size = 0;
    int ocall_result = MSD_OK;

    ocall_load_blob(MSD_STATE_FILE_NAME, sealed_blob, max_sealed_size, &actual_size, &ocall_result);
    if (ocall_result != MSD_OK) {
        free(sealed_blob);
        return ocall_result;
    }

    if (actual_size < sizeof(sgx_sealed_data_t)) {
        free(sealed_blob);
        return MSD_ERR_STATE_CORRUPTED;
    }

    uint32_t expected_text_size = sgx_get_encrypt_txt_len(reinterpret_cast<const sgx_sealed_data_t*>(sealed_blob));
    if (expected_text_size != sizeof(MsdSystemState)) {
        free(sealed_blob);
        return MSD_ERR_STATE_CORRUPTED;
    }

    MsdSystemState* loaded_state = static_cast<MsdSystemState*>(malloc(sizeof(MsdSystemState)));
    if (loaded_state == NULL) {
        free(sealed_blob);
        return MSD_ERR_INTERNAL;
    }
    memset(loaded_state, 0, sizeof(MsdSystemState));

    uint32_t output_size = sizeof(MsdSystemState);
    sgx_status_t unseal_status = sgx_unseal_data(
        reinterpret_cast<sgx_sealed_data_t*>(sealed_blob),
        NULL,
        0,
        reinterpret_cast<uint8_t*>(loaded_state),
        &output_size);
    free(sealed_blob);

    if (unseal_status != SGX_SUCCESS || output_size != sizeof(MsdSystemState)) {
        free(loaded_state);
        enclave_log("[Enclave] 密封数据解封失败，错误码: 0x%x", unseal_status);
        return MSD_ERR_STORAGE_FAILED;
    }

    if (loaded_state->magic != MSD_STATE_MAGIC || loaded_state->version != MSD_STATE_VERSION) {
        free(loaded_state);
        return MSD_ERR_STATE_CORRUPTED;
    }

    g_state = *loaded_state;
    free(loaded_state);
    g_state.active_session_index = -1;
    return MSD_OK;
}

void write_result(int* result, int code)
{
    if (result != NULL) {
        *result = code;
    }
}

bool validate_common_string(const char* text, size_t limit)
{
    return text_fits(text, limit) && !text_empty(text);
}

} // namespace

void ecall_system_init(int* result)
{
    if (g_state.initialized) {
        write_result(result, MSD_ERR_ALREADY_INITIALIZED);
        return;
    }

    clear_state();
    int init_result = create_default_admin();
    if (init_result != MSD_OK) {
        write_result(result, init_result);
        return;
    }

    g_state.initialized = 1;
    enclave_log("[Enclave] 系统初始化完成，已创建默认管理员账号 admin/admin123。");
    write_result(result, MSD_OK);
}

void ecall_system_load(int* result)
{
    int load_result = load_state_internal();
    if (load_result == MSD_OK) {
        enclave_log("[Enclave] 系统状态加载成功。当前记录号起点: %d", g_state.next_record_id);
    }
    write_result(result, load_result);
}

void ecall_system_flush(int* result)
{
    write_result(result, flush_state_internal());
}

void ecall_ra_init_context(const char* peer_identity, int* result)
{
    if (!g_state.initialized) {
        write_result(result, MSD_ERR_NOT_INITIALIZED);
        return;
    }

    if (!has_active_session()) {
        write_result(result, MSD_ERR_NO_PERMISSION);
        return;
    }

    if (!validate_common_string(peer_identity, MSD_RA_MAX_IDENTITY)) {
        write_result(result, MSD_ERR_INVALID_INPUT);
        return;
    }

    reset_ra_state();
    g_ra_state.status = MSD_RA_CONTEXT_READY;
    g_ra_state.session_id = g_ra_next_session_id++;
    copy_text(g_ra_state.peer_identity, sizeof(g_ra_state.peer_identity), peer_identity);

    enclave_log("[Enclave-RA] 远程认证上下文已创建，会话ID: %u，对端: %s", g_ra_state.session_id, g_ra_state.peer_identity);
    write_result(result, MSD_OK);
}

void ecall_ra_get_msg1(uint8_t* buffer, size_t buffer_size, size_t* actual_size, int* result)
{
    if (actual_size != NULL) {
        *actual_size = 0;
    }

    if (!g_state.initialized) {
        write_result(result, MSD_ERR_NOT_INITIALIZED);
        return;
    }

    if (!has_active_session()) {
        write_result(result, MSD_ERR_NO_PERMISSION);
        return;
    }

    if (buffer == NULL || actual_size == NULL || buffer_size < sizeof(uint32_t) * 2) {
        write_result(result, MSD_ERR_INVALID_INPUT);
        return;
    }

    if (g_ra_state.status != MSD_RA_CONTEXT_READY) {
        write_result(result, MSD_ERR_RA_CONTEXT_NOT_READY);
        return;
    }

    char payload[256] = { 0 };
    MsdUserAccount* user = current_user();
    snprintf(payload,
        sizeof(payload),
        "MSD_RA_MSG1_PLACEHOLDER|session=%u|user=%s|peer=%s",
        g_ra_state.session_id,
        user != NULL ? user->username : "unknown",
        g_ra_state.peer_identity);

    if (!build_ra_message(MSD_RA_MSG1, payload, buffer, buffer_size, actual_size)) {
        write_result(result, MSD_ERR_BUFFER_TOO_SMALL);
        return;
    }

    g_ra_state.status = MSD_RA_WAITING_MSG2;
    g_ra_state.last_message_type = MSD_RA_MSG1;
    enclave_log("[Enclave-RA] 已生成 Msg1，占位远程认证流程已启动。");
    write_result(result, MSD_OK);
}

void ecall_ra_proc_msg2_get_msg3(const uint8_t* msg2, size_t msg2_size, uint8_t* buffer, size_t buffer_size, size_t* actual_size, int* result)
{
    if (actual_size != NULL) {
        *actual_size = 0;
    }

    if (!g_state.initialized) {
        write_result(result, MSD_ERR_NOT_INITIALIZED);
        return;
    }

    if (!has_active_session()) {
        write_result(result, MSD_ERR_NO_PERMISSION);
        return;
    }

    if (msg2 == NULL || msg2_size == 0 || msg2_size > MSD_RA_MAX_MESSAGE_SIZE || buffer == NULL || actual_size == NULL) {
        write_result(result, MSD_ERR_RA_MSG_INVALID);
        return;
    }

    if (g_ra_state.status != MSD_RA_WAITING_MSG2) {
        write_result(result, MSD_ERR_RA_CONTEXT_NOT_READY);
        return;
    }

    char payload[256] = { 0 };
    snprintf(payload,
        sizeof(payload),
        "MSD_RA_MSG3_PLACEHOLDER|session=%u|peer=%s|msg2_size=%u",
        g_ra_state.session_id,
        g_ra_state.peer_identity,
        (unsigned int)msg2_size);

    if (!build_ra_message(MSD_RA_MSG3, payload, buffer, buffer_size, actual_size)) {
        write_result(result, MSD_ERR_BUFFER_TOO_SMALL);
        return;
    }

    g_ra_state.status = MSD_RA_WAITING_RESULT;
    g_ra_state.last_message_type = MSD_RA_MSG3;
    enclave_log("[Enclave-RA] 已处理 Msg2，并生成 Msg3。");
    write_result(result, MSD_OK);
}

void ecall_ra_finalize(const uint8_t* attestation_result, size_t attestation_size, int* result)
{
    if (!g_state.initialized) {
        write_result(result, MSD_ERR_NOT_INITIALIZED);
        return;
    }

    if (!has_active_session()) {
        write_result(result, MSD_ERR_NO_PERMISSION);
        return;
    }

    if (attestation_result == NULL || attestation_size == 0) {
        write_result(result, MSD_ERR_RA_MSG_INVALID);
        return;
    }

    if (g_ra_state.status != MSD_RA_WAITING_RESULT) {
        write_result(result, MSD_ERR_RA_CONTEXT_NOT_READY);
        return;
    }

    if (!attestation_result_is_success(attestation_result, attestation_size)) {
        g_ra_state.status = MSD_RA_FAILED;
        g_ra_state.secure_channel_ready = 0;
        enclave_log("[Enclave-RA] 远程认证结果为失败。");
        write_result(result, MSD_ERR_RA_VERIFY_FAILED);
        return;
    }

    g_ra_state.status = MSD_RA_VERIFIED;
    g_ra_state.secure_channel_ready = 1;
    g_ra_state.last_message_type = MSD_RA_ATT_RESULT;
    g_ra_state.session_key_size = MSD_RA_SESSION_KEY_SIZE;
    for (uint32_t index = 0; index < MSD_RA_SESSION_KEY_SIZE; ++index) {
        g_ra_state.session_key[index] = static_cast<uint8_t>((g_ra_state.session_id + index * 13u) & 0xFFu);
    }
    enclave_log("[Enclave-RA] 远程认证通过，已建立占位安全会话并派生会话密钥。");
    write_result(result, MSD_OK);
}

void ecall_ra_get_status(int* ra_status, int* secure_channel_flag, int* result)
{
    if (ra_status != NULL) {
        *ra_status = g_ra_state.status;
    }
    if (secure_channel_flag != NULL) {
        *secure_channel_flag = g_ra_state.secure_channel_ready;
    }

    if (!g_state.initialized) {
        write_result(result, MSD_ERR_NOT_INITIALIZED);
        return;
    }

    enclave_log("[Enclave-RA] 当前认证状态: %s", ra_status_to_text(g_ra_state.status));
    write_result(result, MSD_OK);
}

void ecall_register_user(const char* username, const char* password, int role, int* result)
{
    if (!g_state.initialized) {
        write_result(result, MSD_ERR_NOT_INITIALIZED);
        return;
    }

    if (!has_role(MSD_ROLE_ADMIN)) {
        write_result(result, MSD_ERR_NO_PERMISSION);
        return;
    }

    if (!validate_common_string(username, MSD_MAX_USERNAME) || !validate_common_string(password, MSD_MAX_PASSWORD)) {
        write_result(result, MSD_ERR_INVALID_INPUT);
        return;
    }

    if (role != MSD_ROLE_DOCTOR && role != MSD_ROLE_PATIENT) {
        write_result(result, MSD_ERR_INVALID_ROLE);
        return;
    }

    if (find_user_index(username) >= 0) {
        write_result(result, MSD_ERR_USER_EXISTS);
        return;
    }

    int slot = allocate_user_slot();
    if (slot < 0) {
        write_result(result, MSD_ERR_CAPACITY_REACHED);
        return;
    }

    MsdUserAccount* user = &g_state.users[slot];
    memset(user, 0, sizeof(*user));
    copy_text(user->username, sizeof(user->username), username);
    copy_text(user->password, sizeof(user->password), password);
    user->role = role;
    user->active = 1;

    enclave_log("[Enclave] 已注册用户 %s，角色: %s", username, role_to_text(role));
    write_result(result, MSD_OK);
}

void ecall_login_user(const char* username, const char* password, int* role, int* result)
{
    if (role != NULL) {
        *role = MSD_ROLE_NONE;
    }

    if (!g_state.initialized) {
        write_result(result, MSD_ERR_NOT_INITIALIZED);
        return;
    }

    if (!validate_common_string(username, MSD_MAX_USERNAME) || !validate_common_string(password, MSD_MAX_PASSWORD)) {
        write_result(result, MSD_ERR_INVALID_INPUT);
        return;
    }

    if (has_active_session()) {
        write_result(result, MSD_ERR_SESSION_ACTIVE);
        return;
    }

    int user_index = find_user_index(username);
    if (user_index < 0) {
        write_result(result, MSD_ERR_USER_NOT_FOUND);
        return;
    }

    if (strcmp(g_state.users[user_index].password, password) != 0) {
        write_result(result, MSD_ERR_AUTH_FAILED);
        return;
    }

    g_state.active_session_index = user_index;
    if (role != NULL) {
        *role = g_state.users[user_index].role;
    }

    enclave_log("[Enclave] 用户 %s 登录成功，角色: %s", username, role_to_text(g_state.users[user_index].role));
    write_result(result, MSD_OK);
}

void ecall_logout_user(int* result)
{
    if (!g_state.initialized) {
        write_result(result, MSD_ERR_NOT_INITIALIZED);
        return;
    }

    MsdUserAccount* user = current_user();
    if (user != NULL) {
        enclave_log("[Enclave] 用户 %s 已退出登录。", user->username);
    }

    reset_session();
    write_result(result, MSD_OK);
}

void ecall_list_users(char* buffer, size_t buffer_size, int* result)
{
    if (buffer == NULL || buffer_size == 0) {
        write_result(result, MSD_ERR_INVALID_INPUT);
        return;
    }

    buffer[0] = '\0';

    if (!g_state.initialized) {
        write_result(result, MSD_ERR_NOT_INITIALIZED);
        return;
    }

    if (!has_role(MSD_ROLE_ADMIN)) {
        write_result(result, MSD_ERR_NO_PERMISSION);
        return;
    }

    size_t offset = 0;
    for (int index = 0; index < MSD_MAX_USERS; ++index) {
        if (!g_state.users[index].active) {
            continue;
        }

        if (!append_format(buffer, buffer_size, &offset, "用户名: %s | 角色: %s\n", g_state.users[index].username, role_to_text(g_state.users[index].role))) {
            write_result(result, MSD_ERR_BUFFER_TOO_SMALL);
            return;
        }
    }

    write_result(result, MSD_OK);
}

void ecall_upsert_patient_profile(const char* patient_username, const char* full_name, const char* id_card, const char* phone, const char* address, int* result)
{
    if (!g_state.initialized) {
        write_result(result, MSD_ERR_NOT_INITIALIZED);
        return;
    }

    if (!validate_common_string(patient_username, MSD_MAX_USERNAME)
        || !validate_common_string(full_name, MSD_MAX_NAME)
        || !validate_common_string(id_card, MSD_MAX_ID_CARD)
        || !validate_common_string(phone, MSD_MAX_PHONE)
        || !validate_common_string(address, MSD_MAX_ADDRESS)) {
        write_result(result, MSD_ERR_INVALID_INPUT);
        return;
    }

    if (!patient_user_exists(patient_username)) {
        write_result(result, MSD_ERR_USER_NOT_FOUND);
        return;
    }

    if (!can_edit_patient(patient_username)) {
        write_result(result, MSD_ERR_NO_PERMISSION);
        return;
    }

    if (!ensure_attested_channel_for_user(result)) {
        return;
    }

    int patient_index = find_patient_index(patient_username);
    if (patient_index < 0) {
        patient_index = allocate_patient_slot();
        if (patient_index < 0) {
            write_result(result, MSD_ERR_CAPACITY_REACHED);
            return;
        }
        memset(&g_state.patients[patient_index], 0, sizeof(g_state.patients[patient_index]));
    }

    MsdPatientProfile* profile = &g_state.patients[patient_index];
    copy_text(profile->patient_username, sizeof(profile->patient_username), patient_username);
    copy_text(profile->full_name, sizeof(profile->full_name), full_name);
    copy_text(profile->id_card, sizeof(profile->id_card), id_card);
    copy_text(profile->phone, sizeof(profile->phone), phone);
    copy_text(profile->address, sizeof(profile->address), address);
    profile->active = 1;

    enclave_log("[Enclave] 患者档案已写入: %s", patient_username);
    write_result(result, MSD_OK);
}

void ecall_get_patient_profile(const char* patient_username, char* buffer, size_t buffer_size, int* result)
{
    if (buffer == NULL || buffer_size == 0 || !validate_common_string(patient_username, MSD_MAX_USERNAME)) {
        write_result(result, MSD_ERR_INVALID_INPUT);
        return;
    }

    buffer[0] = '\0';

    if (!g_state.initialized) {
        write_result(result, MSD_ERR_NOT_INITIALIZED);
        return;
    }

    if (!can_view_patient(patient_username)) {
        write_result(result, MSD_ERR_NO_PERMISSION);
        return;
    }

    if (!ensure_attested_channel_for_user(result)) {
        return;
    }

    int patient_index = find_patient_index(patient_username);
    if (patient_index < 0) {
        write_result(result, MSD_ERR_PROFILE_NOT_FOUND);
        return;
    }

    MsdPatientProfile* profile = &g_state.patients[patient_index];
    snprintf(buffer,
        buffer_size,
        "患者用户名: %s\n姓名: %s\n身份证号: %s\n电话: %s\n地址: %s\n",
        profile->patient_username,
        profile->full_name,
        profile->id_card,
        profile->phone,
        profile->address);

    write_result(result, MSD_OK);
}

void ecall_list_patients(char* buffer, size_t buffer_size, int* result)
{
    if (buffer == NULL || buffer_size == 0) {
        write_result(result, MSD_ERR_INVALID_INPUT);
        return;
    }

    buffer[0] = '\0';

    if (!g_state.initialized) {
        write_result(result, MSD_ERR_NOT_INITIALIZED);
        return;
    }

    MsdUserAccount* user = current_user();
    if (user == NULL) {
        write_result(result, MSD_ERR_NO_PERMISSION);
        return;
    }

    size_t offset = 0;
    if (user->role == MSD_ROLE_PATIENT) {
        if (!ensure_attested_channel_for_user(result)) {
            return;
        }

        int patient_index = find_patient_index(user->username);
        if (patient_index < 0) {
            write_result(result, MSD_ERR_PROFILE_NOT_FOUND);
            return;
        }

        MsdPatientProfile* profile = &g_state.patients[patient_index];
        if (!append_format(buffer, buffer_size, &offset, "%s | %s | %s\n", profile->patient_username, profile->full_name, profile->phone)) {
            write_result(result, MSD_ERR_BUFFER_TOO_SMALL);
            return;
        }
        write_result(result, MSD_OK);
        return;
    }

    if (user->role != MSD_ROLE_ADMIN && user->role != MSD_ROLE_DOCTOR) {
        write_result(result, MSD_ERR_NO_PERMISSION);
        return;
    }

    if (!ensure_attested_channel_for_user(result)) {
        return;
    }

    for (int index = 0; index < MSD_MAX_PATIENTS; ++index) {
        if (!g_state.patients[index].active) {
            continue;
        }

        if (!append_format(buffer, buffer_size, &offset, "%s | %s | %s\n", g_state.patients[index].patient_username, g_state.patients[index].full_name, g_state.patients[index].phone)) {
            write_result(result, MSD_ERR_BUFFER_TOO_SMALL);
            return;
        }
    }

    write_result(result, MSD_OK);
}

void ecall_delete_patient_profile(const char* patient_username, int* result)
{
    if (!g_state.initialized) {
        write_result(result, MSD_ERR_NOT_INITIALIZED);
        return;
    }

    if (!has_role(MSD_ROLE_ADMIN)) {
        write_result(result, MSD_ERR_NO_PERMISSION);
        return;
    }

    if (!validate_common_string(patient_username, MSD_MAX_USERNAME)) {
        write_result(result, MSD_ERR_INVALID_INPUT);
        return;
    }

    int patient_index = find_patient_index(patient_username);
    if (patient_index < 0) {
        write_result(result, MSD_ERR_PROFILE_NOT_FOUND);
        return;
    }

    if (patient_has_records(patient_username)) {
        write_result(result, MSD_ERR_PROFILE_HAS_RECORDS);
        return;
    }

    memset(&g_state.patients[patient_index], 0, sizeof(g_state.patients[patient_index]));
    enclave_log("[Enclave] 已删除患者档案: %s", patient_username);
    write_result(result, MSD_OK);
}

void ecall_create_record(const char* patient_username, const char* diagnosis, const char* prescription, const char* note, int* record_id, int* result)
{
    if (record_id != NULL) {
        *record_id = 0;
    }

    if (!g_state.initialized) {
        write_result(result, MSD_ERR_NOT_INITIALIZED);
        return;
    }

    MsdUserAccount* user = current_user();
    if (user == NULL || user->role != MSD_ROLE_DOCTOR) {
        write_result(result, MSD_ERR_NO_PERMISSION);
        return;
    }

    if (!ensure_attested_channel_for_user(result)) {
        return;
    }

    if (!validate_common_string(patient_username, MSD_MAX_USERNAME)
        || !validate_common_string(diagnosis, MSD_MAX_DIAGNOSIS)
        || !validate_common_string(prescription, MSD_MAX_PRESCRIPTION)
        || !validate_common_string(note, MSD_MAX_NOTE)) {
        write_result(result, MSD_ERR_INVALID_INPUT);
        return;
    }

    if (find_patient_index(patient_username) < 0) {
        write_result(result, MSD_ERR_PROFILE_NOT_FOUND);
        return;
    }

    int slot = allocate_record_slot();
    if (slot < 0) {
        write_result(result, MSD_ERR_CAPACITY_REACHED);
        return;
    }

    MsdMedicalRecord* record = &g_state.records[slot];
    memset(record, 0, sizeof(*record));
    record->record_id = g_state.next_record_id++;
    copy_text(record->patient_username, sizeof(record->patient_username), patient_username);
    copy_text(record->doctor_username, sizeof(record->doctor_username), user->username);
    copy_text(record->diagnosis, sizeof(record->diagnosis), diagnosis);
    copy_text(record->prescription, sizeof(record->prescription), prescription);
    copy_text(record->note, sizeof(record->note), note);
    ocall_get_time(record->created_at, sizeof(record->created_at));
    record->active = 1;

    enclave_log("[Enclave] 医生 %s 新增病历，患者: %s，病历ID: %d", user->username, patient_username, record->record_id);
    if (record_id != NULL) {
        *record_id = record->record_id;
    }
    write_result(result, MSD_OK);
}

void ecall_list_records(const char* patient_username, char* buffer, size_t buffer_size, int* result)
{
    if (buffer == NULL || buffer_size == 0 || !validate_common_string(patient_username, MSD_MAX_USERNAME)) {
        write_result(result, MSD_ERR_INVALID_INPUT);
        return;
    }

    buffer[0] = '\0';

    if (!g_state.initialized) {
        write_result(result, MSD_ERR_NOT_INITIALIZED);
        return;
    }

    MsdUserAccount* user = current_user();
    if (user == NULL) {
        write_result(result, MSD_ERR_NO_PERMISSION);
        return;
    }

    if (user->role == MSD_ROLE_PATIENT && strcmp(user->username, patient_username) != 0) {
        write_result(result, MSD_ERR_NO_PERMISSION);
        return;
    }

    if (user->role != MSD_ROLE_PATIENT && user->role != MSD_ROLE_DOCTOR) {
        write_result(result, MSD_ERR_NO_PERMISSION);
        return;
    }

    if (!ensure_attested_channel_for_user(result)) {
        return;
    }

    if (!patient_profile_exists(patient_username)) {
        write_result(result, MSD_ERR_PROFILE_NOT_FOUND);
        return;
    }

    size_t offset = 0;
    for (int index = 0; index < MSD_MAX_RECORDS; ++index) {
        if (!g_state.records[index].active || strcmp(g_state.records[index].patient_username, patient_username) != 0) {
            continue;
        }

        if (!append_format(buffer,
                buffer_size,
                &offset,
                "病历ID: %d\n患者: %s\n医生: %s\n时间: %s\n诊断: %s\n处方: %s\n备注: %s\n------------------------------\n",
                g_state.records[index].record_id,
                g_state.records[index].patient_username,
                g_state.records[index].doctor_username,
                g_state.records[index].created_at,
                g_state.records[index].diagnosis,
                g_state.records[index].prescription,
                g_state.records[index].note)) {
            write_result(result, MSD_ERR_BUFFER_TOO_SMALL);
            return;
        }
    }

    write_result(result, MSD_OK);
}

void ecall_update_record(int record_id, const char* diagnosis, const char* prescription, const char* note, int* result)
{
    if (!g_state.initialized) {
        write_result(result, MSD_ERR_NOT_INITIALIZED);
        return;
    }

    MsdUserAccount* user = current_user();
    if (user == NULL || user->role != MSD_ROLE_DOCTOR) {
        write_result(result, MSD_ERR_NO_PERMISSION);
        return;
    }

    if (!ensure_attested_channel_for_user(result)) {
        return;
    }

    if (!validate_common_string(diagnosis, MSD_MAX_DIAGNOSIS)
        || !validate_common_string(prescription, MSD_MAX_PRESCRIPTION)
        || !validate_common_string(note, MSD_MAX_NOTE)) {
        write_result(result, MSD_ERR_INVALID_INPUT);
        return;
    }

    int record_index = find_record_index(record_id);
    if (record_index < 0) {
        write_result(result, MSD_ERR_RECORD_NOT_FOUND);
        return;
    }

    MsdMedicalRecord* record = &g_state.records[record_index];
    if (strcmp(record->doctor_username, user->username) != 0) {
        write_result(result, MSD_ERR_NO_PERMISSION);
        return;
    }

    copy_text(record->diagnosis, sizeof(record->diagnosis), diagnosis);
    copy_text(record->prescription, sizeof(record->prescription), prescription);
    copy_text(record->note, sizeof(record->note), note);
    enclave_log("[Enclave] 医生 %s 更新病历 %d", user->username, record_id);
    write_result(result, MSD_OK);
}

void ecall_delete_record(int record_id, int* result)
{
    if (!g_state.initialized) {
        write_result(result, MSD_ERR_NOT_INITIALIZED);
        return;
    }

    MsdUserAccount* user = current_user();
    if (user == NULL || user->role != MSD_ROLE_DOCTOR) {
        write_result(result, MSD_ERR_NO_PERMISSION);
        return;
    }

    if (!ensure_attested_channel_for_user(result)) {
        return;
    }

    int record_index = find_record_index(record_id);
    if (record_index < 0) {
        write_result(result, MSD_ERR_RECORD_NOT_FOUND);
        return;
    }

    if (strcmp(g_state.records[record_index].doctor_username, user->username) != 0) {
        write_result(result, MSD_ERR_NO_PERMISSION);
        return;
    }

    memset(&g_state.records[record_index], 0, sizeof(g_state.records[record_index]));
    enclave_log("[Enclave] 医生 %s 删除病历 %d", user->username, record_id);
    write_result(result, MSD_OK);
}
