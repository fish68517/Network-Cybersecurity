#include "MedicalSystemDemo_u.h"
#include <errno.h>

typedef struct ms_ecall_system_init_t {
	int* ms_result;
} ms_ecall_system_init_t;

typedef struct ms_ecall_system_load_t {
	int* ms_result;
} ms_ecall_system_load_t;

typedef struct ms_ecall_system_flush_t {
	int* ms_result;
} ms_ecall_system_flush_t;

typedef struct ms_ecall_register_user_t {
	const char* ms_username;
	size_t ms_username_len;
	const char* ms_password;
	size_t ms_password_len;
	int ms_role;
	int* ms_result;
} ms_ecall_register_user_t;

typedef struct ms_ecall_login_user_t {
	const char* ms_username;
	size_t ms_username_len;
	const char* ms_password;
	size_t ms_password_len;
	int* ms_role;
	int* ms_result;
} ms_ecall_login_user_t;

typedef struct ms_ecall_logout_user_t {
	int* ms_result;
} ms_ecall_logout_user_t;

typedef struct ms_ecall_list_users_t {
	char* ms_buffer;
	size_t ms_buffer_size;
	int* ms_result;
} ms_ecall_list_users_t;

typedef struct ms_ecall_upsert_patient_profile_t {
	const char* ms_patient_username;
	size_t ms_patient_username_len;
	const char* ms_full_name;
	size_t ms_full_name_len;
	const char* ms_id_card;
	size_t ms_id_card_len;
	const char* ms_phone;
	size_t ms_phone_len;
	const char* ms_address;
	size_t ms_address_len;
	int* ms_result;
} ms_ecall_upsert_patient_profile_t;

typedef struct ms_ecall_get_patient_profile_t {
	const char* ms_patient_username;
	size_t ms_patient_username_len;
	char* ms_buffer;
	size_t ms_buffer_size;
	int* ms_result;
} ms_ecall_get_patient_profile_t;

typedef struct ms_ecall_list_patients_t {
	char* ms_buffer;
	size_t ms_buffer_size;
	int* ms_result;
} ms_ecall_list_patients_t;

typedef struct ms_ecall_delete_patient_profile_t {
	const char* ms_patient_username;
	size_t ms_patient_username_len;
	int* ms_result;
} ms_ecall_delete_patient_profile_t;

typedef struct ms_ecall_create_record_t {
	const char* ms_patient_username;
	size_t ms_patient_username_len;
	const char* ms_diagnosis;
	size_t ms_diagnosis_len;
	const char* ms_prescription;
	size_t ms_prescription_len;
	const char* ms_note;
	size_t ms_note_len;
	int* ms_record_id;
	int* ms_result;
} ms_ecall_create_record_t;

typedef struct ms_ecall_list_records_t {
	const char* ms_patient_username;
	size_t ms_patient_username_len;
	char* ms_buffer;
	size_t ms_buffer_size;
	int* ms_result;
} ms_ecall_list_records_t;

typedef struct ms_ecall_update_record_t {
	int ms_record_id;
	const char* ms_diagnosis;
	size_t ms_diagnosis_len;
	const char* ms_prescription;
	size_t ms_prescription_len;
	const char* ms_note;
	size_t ms_note_len;
	int* ms_result;
} ms_ecall_update_record_t;

typedef struct ms_ecall_delete_record_t {
	int ms_record_id;
	int* ms_result;
} ms_ecall_delete_record_t;

typedef struct ms_ocall_print_log_t {
	const char* ms_str;
} ms_ocall_print_log_t;

typedef struct ms_ocall_save_blob_t {
	const char* ms_file_name;
	const uint8_t* ms_data;
	size_t ms_data_size;
	int* ms_result;
} ms_ocall_save_blob_t;

typedef struct ms_ocall_load_blob_t {
	const char* ms_file_name;
	uint8_t* ms_data;
	size_t ms_max_size;
	size_t* ms_actual_size;
	int* ms_result;
} ms_ocall_load_blob_t;

typedef struct ms_ocall_get_time_t {
	char* ms_buffer;
	size_t ms_buffer_size;
} ms_ocall_get_time_t;

typedef struct ms_sgx_oc_cpuidex_t {
	int* ms_cpuinfo;
	int ms_leaf;
	int ms_subleaf;
} ms_sgx_oc_cpuidex_t;

typedef struct ms_sgx_thread_wait_untrusted_event_ocall_t {
	int ms_retval;
	const void* ms_self;
} ms_sgx_thread_wait_untrusted_event_ocall_t;

typedef struct ms_sgx_thread_set_untrusted_event_ocall_t {
	int ms_retval;
	const void* ms_waiter;
} ms_sgx_thread_set_untrusted_event_ocall_t;

typedef struct ms_sgx_thread_setwait_untrusted_events_ocall_t {
	int ms_retval;
	const void* ms_waiter;
	const void* ms_self;
} ms_sgx_thread_setwait_untrusted_events_ocall_t;

typedef struct ms_sgx_thread_set_multiple_untrusted_events_ocall_t {
	int ms_retval;
	const void** ms_waiters;
	size_t ms_total;
} ms_sgx_thread_set_multiple_untrusted_events_ocall_t;

static sgx_status_t SGX_CDECL MedicalSystemDemo_ocall_print_log(void* pms)
{
	ms_ocall_print_log_t* ms = SGX_CAST(ms_ocall_print_log_t*, pms);
	ocall_print_log(ms->ms_str);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL MedicalSystemDemo_ocall_save_blob(void* pms)
{
	ms_ocall_save_blob_t* ms = SGX_CAST(ms_ocall_save_blob_t*, pms);
	ocall_save_blob(ms->ms_file_name, ms->ms_data, ms->ms_data_size, ms->ms_result);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL MedicalSystemDemo_ocall_load_blob(void* pms)
{
	ms_ocall_load_blob_t* ms = SGX_CAST(ms_ocall_load_blob_t*, pms);
	ocall_load_blob(ms->ms_file_name, ms->ms_data, ms->ms_max_size, ms->ms_actual_size, ms->ms_result);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL MedicalSystemDemo_ocall_get_time(void* pms)
{
	ms_ocall_get_time_t* ms = SGX_CAST(ms_ocall_get_time_t*, pms);
	ocall_get_time(ms->ms_buffer, ms->ms_buffer_size);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL MedicalSystemDemo_sgx_oc_cpuidex(void* pms)
{
	ms_sgx_oc_cpuidex_t* ms = SGX_CAST(ms_sgx_oc_cpuidex_t*, pms);
	sgx_oc_cpuidex(ms->ms_cpuinfo, ms->ms_leaf, ms->ms_subleaf);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL MedicalSystemDemo_sgx_thread_wait_untrusted_event_ocall(void* pms)
{
	ms_sgx_thread_wait_untrusted_event_ocall_t* ms = SGX_CAST(ms_sgx_thread_wait_untrusted_event_ocall_t*, pms);
	ms->ms_retval = sgx_thread_wait_untrusted_event_ocall(ms->ms_self);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL MedicalSystemDemo_sgx_thread_set_untrusted_event_ocall(void* pms)
{
	ms_sgx_thread_set_untrusted_event_ocall_t* ms = SGX_CAST(ms_sgx_thread_set_untrusted_event_ocall_t*, pms);
	ms->ms_retval = sgx_thread_set_untrusted_event_ocall(ms->ms_waiter);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL MedicalSystemDemo_sgx_thread_setwait_untrusted_events_ocall(void* pms)
{
	ms_sgx_thread_setwait_untrusted_events_ocall_t* ms = SGX_CAST(ms_sgx_thread_setwait_untrusted_events_ocall_t*, pms);
	ms->ms_retval = sgx_thread_setwait_untrusted_events_ocall(ms->ms_waiter, ms->ms_self);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL MedicalSystemDemo_sgx_thread_set_multiple_untrusted_events_ocall(void* pms)
{
	ms_sgx_thread_set_multiple_untrusted_events_ocall_t* ms = SGX_CAST(ms_sgx_thread_set_multiple_untrusted_events_ocall_t*, pms);
	ms->ms_retval = sgx_thread_set_multiple_untrusted_events_ocall(ms->ms_waiters, ms->ms_total);

	return SGX_SUCCESS;
}

static const struct {
	size_t nr_ocall;
	void * func_addr[9];
} ocall_table_MedicalSystemDemo = {
	9,
	{
		(void*)(uintptr_t)MedicalSystemDemo_ocall_print_log,
		(void*)(uintptr_t)MedicalSystemDemo_ocall_save_blob,
		(void*)(uintptr_t)MedicalSystemDemo_ocall_load_blob,
		(void*)(uintptr_t)MedicalSystemDemo_ocall_get_time,
		(void*)(uintptr_t)MedicalSystemDemo_sgx_oc_cpuidex,
		(void*)(uintptr_t)MedicalSystemDemo_sgx_thread_wait_untrusted_event_ocall,
		(void*)(uintptr_t)MedicalSystemDemo_sgx_thread_set_untrusted_event_ocall,
		(void*)(uintptr_t)MedicalSystemDemo_sgx_thread_setwait_untrusted_events_ocall,
		(void*)(uintptr_t)MedicalSystemDemo_sgx_thread_set_multiple_untrusted_events_ocall,
	}
};

sgx_status_t ecall_system_init(sgx_enclave_id_t eid, int* result)
{
	sgx_status_t status;
	ms_ecall_system_init_t ms;
	ms.ms_result = result;
	status = sgx_ecall(eid, 0, &ocall_table_MedicalSystemDemo, &ms);
	return status;
}

sgx_status_t ecall_system_load(sgx_enclave_id_t eid, int* result)
{
	sgx_status_t status;
	ms_ecall_system_load_t ms;
	ms.ms_result = result;
	status = sgx_ecall(eid, 1, &ocall_table_MedicalSystemDemo, &ms);
	return status;
}

sgx_status_t ecall_system_flush(sgx_enclave_id_t eid, int* result)
{
	sgx_status_t status;
	ms_ecall_system_flush_t ms;
	ms.ms_result = result;
	status = sgx_ecall(eid, 2, &ocall_table_MedicalSystemDemo, &ms);
	return status;
}

sgx_status_t ecall_register_user(sgx_enclave_id_t eid, const char* username, const char* password, int role, int* result)
{
	sgx_status_t status;
	ms_ecall_register_user_t ms;
	ms.ms_username = username;
	ms.ms_username_len = username ? strlen(username) + 1 : 0;
	ms.ms_password = password;
	ms.ms_password_len = password ? strlen(password) + 1 : 0;
	ms.ms_role = role;
	ms.ms_result = result;
	status = sgx_ecall(eid, 3, &ocall_table_MedicalSystemDemo, &ms);
	return status;
}

sgx_status_t ecall_login_user(sgx_enclave_id_t eid, const char* username, const char* password, int* role, int* result)
{
	sgx_status_t status;
	ms_ecall_login_user_t ms;
	ms.ms_username = username;
	ms.ms_username_len = username ? strlen(username) + 1 : 0;
	ms.ms_password = password;
	ms.ms_password_len = password ? strlen(password) + 1 : 0;
	ms.ms_role = role;
	ms.ms_result = result;
	status = sgx_ecall(eid, 4, &ocall_table_MedicalSystemDemo, &ms);
	return status;
}

sgx_status_t ecall_logout_user(sgx_enclave_id_t eid, int* result)
{
	sgx_status_t status;
	ms_ecall_logout_user_t ms;
	ms.ms_result = result;
	status = sgx_ecall(eid, 5, &ocall_table_MedicalSystemDemo, &ms);
	return status;
}

sgx_status_t ecall_list_users(sgx_enclave_id_t eid, char* buffer, size_t buffer_size, int* result)
{
	sgx_status_t status;
	ms_ecall_list_users_t ms;
	ms.ms_buffer = buffer;
	ms.ms_buffer_size = buffer_size;
	ms.ms_result = result;
	status = sgx_ecall(eid, 6, &ocall_table_MedicalSystemDemo, &ms);
	return status;
}

sgx_status_t ecall_upsert_patient_profile(sgx_enclave_id_t eid, const char* patient_username, const char* full_name, const char* id_card, const char* phone, const char* address, int* result)
{
	sgx_status_t status;
	ms_ecall_upsert_patient_profile_t ms;
	ms.ms_patient_username = patient_username;
	ms.ms_patient_username_len = patient_username ? strlen(patient_username) + 1 : 0;
	ms.ms_full_name = full_name;
	ms.ms_full_name_len = full_name ? strlen(full_name) + 1 : 0;
	ms.ms_id_card = id_card;
	ms.ms_id_card_len = id_card ? strlen(id_card) + 1 : 0;
	ms.ms_phone = phone;
	ms.ms_phone_len = phone ? strlen(phone) + 1 : 0;
	ms.ms_address = address;
	ms.ms_address_len = address ? strlen(address) + 1 : 0;
	ms.ms_result = result;
	status = sgx_ecall(eid, 7, &ocall_table_MedicalSystemDemo, &ms);
	return status;
}

sgx_status_t ecall_get_patient_profile(sgx_enclave_id_t eid, const char* patient_username, char* buffer, size_t buffer_size, int* result)
{
	sgx_status_t status;
	ms_ecall_get_patient_profile_t ms;
	ms.ms_patient_username = patient_username;
	ms.ms_patient_username_len = patient_username ? strlen(patient_username) + 1 : 0;
	ms.ms_buffer = buffer;
	ms.ms_buffer_size = buffer_size;
	ms.ms_result = result;
	status = sgx_ecall(eid, 8, &ocall_table_MedicalSystemDemo, &ms);
	return status;
}

sgx_status_t ecall_list_patients(sgx_enclave_id_t eid, char* buffer, size_t buffer_size, int* result)
{
	sgx_status_t status;
	ms_ecall_list_patients_t ms;
	ms.ms_buffer = buffer;
	ms.ms_buffer_size = buffer_size;
	ms.ms_result = result;
	status = sgx_ecall(eid, 9, &ocall_table_MedicalSystemDemo, &ms);
	return status;
}

sgx_status_t ecall_delete_patient_profile(sgx_enclave_id_t eid, const char* patient_username, int* result)
{
	sgx_status_t status;
	ms_ecall_delete_patient_profile_t ms;
	ms.ms_patient_username = patient_username;
	ms.ms_patient_username_len = patient_username ? strlen(patient_username) + 1 : 0;
	ms.ms_result = result;
	status = sgx_ecall(eid, 10, &ocall_table_MedicalSystemDemo, &ms);
	return status;
}

sgx_status_t ecall_create_record(sgx_enclave_id_t eid, const char* patient_username, const char* diagnosis, const char* prescription, const char* note, int* record_id, int* result)
{
	sgx_status_t status;
	ms_ecall_create_record_t ms;
	ms.ms_patient_username = patient_username;
	ms.ms_patient_username_len = patient_username ? strlen(patient_username) + 1 : 0;
	ms.ms_diagnosis = diagnosis;
	ms.ms_diagnosis_len = diagnosis ? strlen(diagnosis) + 1 : 0;
	ms.ms_prescription = prescription;
	ms.ms_prescription_len = prescription ? strlen(prescription) + 1 : 0;
	ms.ms_note = note;
	ms.ms_note_len = note ? strlen(note) + 1 : 0;
	ms.ms_record_id = record_id;
	ms.ms_result = result;
	status = sgx_ecall(eid, 11, &ocall_table_MedicalSystemDemo, &ms);
	return status;
}

sgx_status_t ecall_list_records(sgx_enclave_id_t eid, const char* patient_username, char* buffer, size_t buffer_size, int* result)
{
	sgx_status_t status;
	ms_ecall_list_records_t ms;
	ms.ms_patient_username = patient_username;
	ms.ms_patient_username_len = patient_username ? strlen(patient_username) + 1 : 0;
	ms.ms_buffer = buffer;
	ms.ms_buffer_size = buffer_size;
	ms.ms_result = result;
	status = sgx_ecall(eid, 12, &ocall_table_MedicalSystemDemo, &ms);
	return status;
}

sgx_status_t ecall_update_record(sgx_enclave_id_t eid, int record_id, const char* diagnosis, const char* prescription, const char* note, int* result)
{
	sgx_status_t status;
	ms_ecall_update_record_t ms;
	ms.ms_record_id = record_id;
	ms.ms_diagnosis = diagnosis;
	ms.ms_diagnosis_len = diagnosis ? strlen(diagnosis) + 1 : 0;
	ms.ms_prescription = prescription;
	ms.ms_prescription_len = prescription ? strlen(prescription) + 1 : 0;
	ms.ms_note = note;
	ms.ms_note_len = note ? strlen(note) + 1 : 0;
	ms.ms_result = result;
	status = sgx_ecall(eid, 13, &ocall_table_MedicalSystemDemo, &ms);
	return status;
}

sgx_status_t ecall_delete_record(sgx_enclave_id_t eid, int record_id, int* result)
{
	sgx_status_t status;
	ms_ecall_delete_record_t ms;
	ms.ms_record_id = record_id;
	ms.ms_result = result;
	status = sgx_ecall(eid, 14, &ocall_table_MedicalSystemDemo, &ms);
	return status;
}

