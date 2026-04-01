#ifndef MEDICALSYSTEMDEMO_T_H__
#define MEDICALSYSTEMDEMO_T_H__

#include <stdint.h>
#include <wchar.h>
#include <stddef.h>
#include "sgx_edger8r.h" /* for sgx_ocall etc. */


#define SGX_CAST(type, item) ((type)(item))

#ifdef __cplusplus
extern "C" {
#endif

void ecall_system_init(int* result);
void ecall_system_load(int* result);
void ecall_system_flush(int* result);
void ecall_register_user(const char* username, const char* password, int role, int* result);
void ecall_login_user(const char* username, const char* password, int* role, int* result);
void ecall_logout_user(int* result);
void ecall_list_users(char* buffer, size_t buffer_size, int* result);
void ecall_upsert_patient_profile(const char* patient_username, const char* full_name, const char* id_card, const char* phone, const char* address, int* result);
void ecall_get_patient_profile(const char* patient_username, char* buffer, size_t buffer_size, int* result);
void ecall_list_patients(char* buffer, size_t buffer_size, int* result);
void ecall_delete_patient_profile(const char* patient_username, int* result);
void ecall_create_record(const char* patient_username, const char* diagnosis, const char* prescription, const char* note, int* record_id, int* result);
void ecall_list_records(const char* patient_username, char* buffer, size_t buffer_size, int* result);
void ecall_update_record(int record_id, const char* diagnosis, const char* prescription, const char* note, int* result);
void ecall_delete_record(int record_id, int* result);

sgx_status_t SGX_CDECL ocall_print_log(const char* str);
sgx_status_t SGX_CDECL ocall_save_blob(const char* file_name, const uint8_t* data, size_t data_size, int* result);
sgx_status_t SGX_CDECL ocall_load_blob(const char* file_name, uint8_t* data, size_t max_size, size_t* actual_size, int* result);
sgx_status_t SGX_CDECL ocall_get_time(char* buffer, size_t buffer_size);
sgx_status_t SGX_CDECL sgx_oc_cpuidex(int cpuinfo[4], int leaf, int subleaf);
sgx_status_t SGX_CDECL sgx_thread_wait_untrusted_event_ocall(int* retval, const void* self);
sgx_status_t SGX_CDECL sgx_thread_set_untrusted_event_ocall(int* retval, const void* waiter);
sgx_status_t SGX_CDECL sgx_thread_setwait_untrusted_events_ocall(int* retval, const void* waiter, const void* self);
sgx_status_t SGX_CDECL sgx_thread_set_multiple_untrusted_events_ocall(int* retval, const void** waiters, size_t total);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
