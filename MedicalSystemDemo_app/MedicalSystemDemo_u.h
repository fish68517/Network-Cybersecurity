#ifndef MEDICALSYSTEMDEMO_U_H__
#define MEDICALSYSTEMDEMO_U_H__

#include <stdint.h>
#include <wchar.h>
#include <stddef.h>
#include <string.h>
#include "sgx_edger8r.h" /* for sgx_status_t etc. */


#define SGX_CAST(type, item) ((type)(item))

#ifdef __cplusplus
extern "C" {
#endif

#ifndef OCALL_PRINT_LOG_DEFINED__
#define OCALL_PRINT_LOG_DEFINED__
void SGX_UBRIDGE(SGX_NOCONVENTION, ocall_print_log, (const char* str));
#endif
#ifndef OCALL_SAVE_BLOB_DEFINED__
#define OCALL_SAVE_BLOB_DEFINED__
void SGX_UBRIDGE(SGX_NOCONVENTION, ocall_save_blob, (const char* file_name, const uint8_t* data, size_t data_size, int* result));
#endif
#ifndef OCALL_LOAD_BLOB_DEFINED__
#define OCALL_LOAD_BLOB_DEFINED__
void SGX_UBRIDGE(SGX_NOCONVENTION, ocall_load_blob, (const char* file_name, uint8_t* data, size_t max_size, size_t* actual_size, int* result));
#endif
#ifndef OCALL_GET_TIME_DEFINED__
#define OCALL_GET_TIME_DEFINED__
void SGX_UBRIDGE(SGX_NOCONVENTION, ocall_get_time, (char* buffer, size_t buffer_size));
#endif
#ifndef OCALL_RA_SEND_MSG_DEFINED__
#define OCALL_RA_SEND_MSG_DEFINED__
void SGX_UBRIDGE(SGX_NOCONVENTION, ocall_ra_send_msg, (uint32_t message_type, const uint8_t* data, size_t data_size, int* result));
#endif
#ifndef OCALL_RA_RECV_MSG_DEFINED__
#define OCALL_RA_RECV_MSG_DEFINED__
void SGX_UBRIDGE(SGX_NOCONVENTION, ocall_ra_recv_msg, (uint32_t expected_message_type, uint8_t* buffer, size_t buffer_size, size_t* actual_size, int* result));
#endif
#ifndef SGX_OC_CPUIDEX_DEFINED__
#define SGX_OC_CPUIDEX_DEFINED__
void SGX_UBRIDGE(SGX_CDECL, sgx_oc_cpuidex, (int cpuinfo[4], int leaf, int subleaf));
#endif
#ifndef SGX_THREAD_WAIT_UNTRUSTED_EVENT_OCALL_DEFINED__
#define SGX_THREAD_WAIT_UNTRUSTED_EVENT_OCALL_DEFINED__
int SGX_UBRIDGE(SGX_CDECL, sgx_thread_wait_untrusted_event_ocall, (const void* self));
#endif
#ifndef SGX_THREAD_SET_UNTRUSTED_EVENT_OCALL_DEFINED__
#define SGX_THREAD_SET_UNTRUSTED_EVENT_OCALL_DEFINED__
int SGX_UBRIDGE(SGX_CDECL, sgx_thread_set_untrusted_event_ocall, (const void* waiter));
#endif
#ifndef SGX_THREAD_SETWAIT_UNTRUSTED_EVENTS_OCALL_DEFINED__
#define SGX_THREAD_SETWAIT_UNTRUSTED_EVENTS_OCALL_DEFINED__
int SGX_UBRIDGE(SGX_CDECL, sgx_thread_setwait_untrusted_events_ocall, (const void* waiter, const void* self));
#endif
#ifndef SGX_THREAD_SET_MULTIPLE_UNTRUSTED_EVENTS_OCALL_DEFINED__
#define SGX_THREAD_SET_MULTIPLE_UNTRUSTED_EVENTS_OCALL_DEFINED__
int SGX_UBRIDGE(SGX_CDECL, sgx_thread_set_multiple_untrusted_events_ocall, (const void** waiters, size_t total));
#endif

sgx_status_t ecall_system_init(sgx_enclave_id_t eid, int* result);
sgx_status_t ecall_system_load(sgx_enclave_id_t eid, int* result);
sgx_status_t ecall_system_flush(sgx_enclave_id_t eid, int* result);
sgx_status_t ecall_ra_init_context(sgx_enclave_id_t eid, const char* peer_identity, int* result);
sgx_status_t ecall_ra_get_msg1(sgx_enclave_id_t eid, uint8_t* buffer, size_t buffer_size, size_t* actual_size, int* result);
sgx_status_t ecall_ra_proc_msg2_get_msg3(sgx_enclave_id_t eid, const uint8_t* msg2, size_t msg2_size, uint8_t* buffer, size_t buffer_size, size_t* actual_size, int* result);
sgx_status_t ecall_ra_finalize(sgx_enclave_id_t eid, const uint8_t* attestation_result, size_t attestation_size, int* result);
sgx_status_t ecall_ra_get_status(sgx_enclave_id_t eid, int* ra_status, int* secure_channel_ready, int* result);
sgx_status_t ecall_register_user(sgx_enclave_id_t eid, const char* username, const char* password, int role, int* result);
sgx_status_t ecall_login_user(sgx_enclave_id_t eid, const char* username, const char* password, int* role, int* result);
sgx_status_t ecall_logout_user(sgx_enclave_id_t eid, int* result);
sgx_status_t ecall_list_users(sgx_enclave_id_t eid, char* buffer, size_t buffer_size, int* result);
sgx_status_t ecall_upsert_patient_profile(sgx_enclave_id_t eid, const char* patient_username, const char* full_name, const char* id_card, const char* phone, const char* address, int* result);
sgx_status_t ecall_get_patient_profile(sgx_enclave_id_t eid, const char* patient_username, char* buffer, size_t buffer_size, int* result);
sgx_status_t ecall_list_patients(sgx_enclave_id_t eid, char* buffer, size_t buffer_size, int* result);
sgx_status_t ecall_delete_patient_profile(sgx_enclave_id_t eid, const char* patient_username, int* result);
sgx_status_t ecall_create_record(sgx_enclave_id_t eid, const char* patient_username, const char* diagnosis, const char* prescription, const char* note, int* record_id, int* result);
sgx_status_t ecall_list_records(sgx_enclave_id_t eid, const char* patient_username, char* buffer, size_t buffer_size, int* result);
sgx_status_t ecall_update_record(sgx_enclave_id_t eid, int record_id, const char* diagnosis, const char* prescription, const char* note, int* result);
sgx_status_t ecall_delete_record(sgx_enclave_id_t eid, int record_id, int* result);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
