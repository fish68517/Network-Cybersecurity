#include "MedicalSystemDemo_t.h"

#include "sgx_trts.h" /* for sgx_ocalloc, sgx_is_outside_enclave */
#include "sgx_lfence.h" /* for sgx_lfence */

#include <errno.h>
#include <mbusafecrt.h> /* for memcpy_s etc */
#include <stdlib.h> /* for malloc/free etc */

#define CHECK_REF_POINTER(ptr, siz) do {	\
	if (!(ptr) || ! sgx_is_outside_enclave((ptr), (siz)))	\
		return SGX_ERROR_INVALID_PARAMETER;\
} while (0)

#define CHECK_UNIQUE_POINTER(ptr, siz) do {	\
	if ((ptr) && ! sgx_is_outside_enclave((ptr), (siz)))	\
		return SGX_ERROR_INVALID_PARAMETER;\
} while (0)

#define CHECK_ENCLAVE_POINTER(ptr, siz) do {	\
	if ((ptr) && ! sgx_is_within_enclave((ptr), (siz)))	\
		return SGX_ERROR_INVALID_PARAMETER;\
} while (0)

#define ADD_ASSIGN_OVERFLOW(a, b) (	\
	((a) += (b)) < (b)	\
)


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

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4127)
#pragma warning(disable: 4200)
#pragma warning(disable: 4090)
#endif

static sgx_status_t SGX_CDECL sgx_ecall_system_init(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_system_init_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_system_init_t* ms = SGX_CAST(ms_ecall_system_init_t*, pms);
	ms_ecall_system_init_t __in_ms;
	if (memcpy_s(&__in_ms, sizeof(ms_ecall_system_init_t), ms, sizeof(ms_ecall_system_init_t))) {
		return SGX_ERROR_UNEXPECTED;
	}
	sgx_status_t status = SGX_SUCCESS;
	int* _tmp_result = __in_ms.ms_result;
	size_t _len_result = sizeof(int);
	int* _in_result = NULL;

	CHECK_UNIQUE_POINTER(_tmp_result, _len_result);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_result != NULL && _len_result != 0) {
		if ( _len_result % sizeof(*_tmp_result) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		if ((_in_result = (int*)malloc(_len_result)) == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memset((void*)_in_result, 0, _len_result);
	}
	ecall_system_init(_in_result);
	if (_in_result) {
		if (memcpy_verw_s(_tmp_result, _len_result, _in_result, _len_result)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}

err:
	if (_in_result) free(_in_result);
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_system_load(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_system_load_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_system_load_t* ms = SGX_CAST(ms_ecall_system_load_t*, pms);
	ms_ecall_system_load_t __in_ms;
	if (memcpy_s(&__in_ms, sizeof(ms_ecall_system_load_t), ms, sizeof(ms_ecall_system_load_t))) {
		return SGX_ERROR_UNEXPECTED;
	}
	sgx_status_t status = SGX_SUCCESS;
	int* _tmp_result = __in_ms.ms_result;
	size_t _len_result = sizeof(int);
	int* _in_result = NULL;

	CHECK_UNIQUE_POINTER(_tmp_result, _len_result);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_result != NULL && _len_result != 0) {
		if ( _len_result % sizeof(*_tmp_result) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		if ((_in_result = (int*)malloc(_len_result)) == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memset((void*)_in_result, 0, _len_result);
	}
	ecall_system_load(_in_result);
	if (_in_result) {
		if (memcpy_verw_s(_tmp_result, _len_result, _in_result, _len_result)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}

err:
	if (_in_result) free(_in_result);
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_system_flush(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_system_flush_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_system_flush_t* ms = SGX_CAST(ms_ecall_system_flush_t*, pms);
	ms_ecall_system_flush_t __in_ms;
	if (memcpy_s(&__in_ms, sizeof(ms_ecall_system_flush_t), ms, sizeof(ms_ecall_system_flush_t))) {
		return SGX_ERROR_UNEXPECTED;
	}
	sgx_status_t status = SGX_SUCCESS;
	int* _tmp_result = __in_ms.ms_result;
	size_t _len_result = sizeof(int);
	int* _in_result = NULL;

	CHECK_UNIQUE_POINTER(_tmp_result, _len_result);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_result != NULL && _len_result != 0) {
		if ( _len_result % sizeof(*_tmp_result) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		if ((_in_result = (int*)malloc(_len_result)) == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memset((void*)_in_result, 0, _len_result);
	}
	ecall_system_flush(_in_result);
	if (_in_result) {
		if (memcpy_verw_s(_tmp_result, _len_result, _in_result, _len_result)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}

err:
	if (_in_result) free(_in_result);
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_register_user(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_register_user_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_register_user_t* ms = SGX_CAST(ms_ecall_register_user_t*, pms);
	ms_ecall_register_user_t __in_ms;
	if (memcpy_s(&__in_ms, sizeof(ms_ecall_register_user_t), ms, sizeof(ms_ecall_register_user_t))) {
		return SGX_ERROR_UNEXPECTED;
	}
	sgx_status_t status = SGX_SUCCESS;
	const char* _tmp_username = __in_ms.ms_username;
	size_t _len_username = __in_ms.ms_username_len ;
	char* _in_username = NULL;
	const char* _tmp_password = __in_ms.ms_password;
	size_t _len_password = __in_ms.ms_password_len ;
	char* _in_password = NULL;
	int* _tmp_result = __in_ms.ms_result;
	size_t _len_result = sizeof(int);
	int* _in_result = NULL;

	CHECK_UNIQUE_POINTER(_tmp_username, _len_username);
	CHECK_UNIQUE_POINTER(_tmp_password, _len_password);
	CHECK_UNIQUE_POINTER(_tmp_result, _len_result);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_username != NULL && _len_username != 0) {
		_in_username = (char*)malloc(_len_username);
		if (_in_username == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_username, _len_username, _tmp_username, _len_username)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

		_in_username[_len_username - 1] = '\0';
		if (_len_username != strlen(_in_username) + 1)
		{
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}
	if (_tmp_password != NULL && _len_password != 0) {
		_in_password = (char*)malloc(_len_password);
		if (_in_password == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_password, _len_password, _tmp_password, _len_password)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

		_in_password[_len_password - 1] = '\0';
		if (_len_password != strlen(_in_password) + 1)
		{
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}
	if (_tmp_result != NULL && _len_result != 0) {
		if ( _len_result % sizeof(*_tmp_result) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		if ((_in_result = (int*)malloc(_len_result)) == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memset((void*)_in_result, 0, _len_result);
	}
	ecall_register_user((const char*)_in_username, (const char*)_in_password, __in_ms.ms_role, _in_result);
	if (_in_result) {
		if (memcpy_verw_s(_tmp_result, _len_result, _in_result, _len_result)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}

err:
	if (_in_username) free(_in_username);
	if (_in_password) free(_in_password);
	if (_in_result) free(_in_result);
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_login_user(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_login_user_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_login_user_t* ms = SGX_CAST(ms_ecall_login_user_t*, pms);
	ms_ecall_login_user_t __in_ms;
	if (memcpy_s(&__in_ms, sizeof(ms_ecall_login_user_t), ms, sizeof(ms_ecall_login_user_t))) {
		return SGX_ERROR_UNEXPECTED;
	}
	sgx_status_t status = SGX_SUCCESS;
	const char* _tmp_username = __in_ms.ms_username;
	size_t _len_username = __in_ms.ms_username_len ;
	char* _in_username = NULL;
	const char* _tmp_password = __in_ms.ms_password;
	size_t _len_password = __in_ms.ms_password_len ;
	char* _in_password = NULL;
	int* _tmp_role = __in_ms.ms_role;
	size_t _len_role = sizeof(int);
	int* _in_role = NULL;
	int* _tmp_result = __in_ms.ms_result;
	size_t _len_result = sizeof(int);
	int* _in_result = NULL;

	CHECK_UNIQUE_POINTER(_tmp_username, _len_username);
	CHECK_UNIQUE_POINTER(_tmp_password, _len_password);
	CHECK_UNIQUE_POINTER(_tmp_role, _len_role);
	CHECK_UNIQUE_POINTER(_tmp_result, _len_result);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_username != NULL && _len_username != 0) {
		_in_username = (char*)malloc(_len_username);
		if (_in_username == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_username, _len_username, _tmp_username, _len_username)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

		_in_username[_len_username - 1] = '\0';
		if (_len_username != strlen(_in_username) + 1)
		{
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}
	if (_tmp_password != NULL && _len_password != 0) {
		_in_password = (char*)malloc(_len_password);
		if (_in_password == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_password, _len_password, _tmp_password, _len_password)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

		_in_password[_len_password - 1] = '\0';
		if (_len_password != strlen(_in_password) + 1)
		{
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}
	if (_tmp_role != NULL && _len_role != 0) {
		if ( _len_role % sizeof(*_tmp_role) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		if ((_in_role = (int*)malloc(_len_role)) == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memset((void*)_in_role, 0, _len_role);
	}
	if (_tmp_result != NULL && _len_result != 0) {
		if ( _len_result % sizeof(*_tmp_result) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		if ((_in_result = (int*)malloc(_len_result)) == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memset((void*)_in_result, 0, _len_result);
	}
	ecall_login_user((const char*)_in_username, (const char*)_in_password, _in_role, _in_result);
	if (_in_role) {
		if (memcpy_verw_s(_tmp_role, _len_role, _in_role, _len_role)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}
	if (_in_result) {
		if (memcpy_verw_s(_tmp_result, _len_result, _in_result, _len_result)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}

err:
	if (_in_username) free(_in_username);
	if (_in_password) free(_in_password);
	if (_in_role) free(_in_role);
	if (_in_result) free(_in_result);
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_logout_user(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_logout_user_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_logout_user_t* ms = SGX_CAST(ms_ecall_logout_user_t*, pms);
	ms_ecall_logout_user_t __in_ms;
	if (memcpy_s(&__in_ms, sizeof(ms_ecall_logout_user_t), ms, sizeof(ms_ecall_logout_user_t))) {
		return SGX_ERROR_UNEXPECTED;
	}
	sgx_status_t status = SGX_SUCCESS;
	int* _tmp_result = __in_ms.ms_result;
	size_t _len_result = sizeof(int);
	int* _in_result = NULL;

	CHECK_UNIQUE_POINTER(_tmp_result, _len_result);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_result != NULL && _len_result != 0) {
		if ( _len_result % sizeof(*_tmp_result) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		if ((_in_result = (int*)malloc(_len_result)) == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memset((void*)_in_result, 0, _len_result);
	}
	ecall_logout_user(_in_result);
	if (_in_result) {
		if (memcpy_verw_s(_tmp_result, _len_result, _in_result, _len_result)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}

err:
	if (_in_result) free(_in_result);
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_list_users(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_list_users_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_list_users_t* ms = SGX_CAST(ms_ecall_list_users_t*, pms);
	ms_ecall_list_users_t __in_ms;
	if (memcpy_s(&__in_ms, sizeof(ms_ecall_list_users_t), ms, sizeof(ms_ecall_list_users_t))) {
		return SGX_ERROR_UNEXPECTED;
	}
	sgx_status_t status = SGX_SUCCESS;
	char* _tmp_buffer = __in_ms.ms_buffer;
	size_t _tmp_buffer_size = __in_ms.ms_buffer_size;
	size_t _len_buffer = _tmp_buffer_size;
	char* _in_buffer = NULL;
	int* _tmp_result = __in_ms.ms_result;
	size_t _len_result = sizeof(int);
	int* _in_result = NULL;

	CHECK_UNIQUE_POINTER(_tmp_buffer, _len_buffer);
	CHECK_UNIQUE_POINTER(_tmp_result, _len_result);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_buffer != NULL && _len_buffer != 0) {
		if ( _len_buffer % sizeof(*_tmp_buffer) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		if ((_in_buffer = (char*)malloc(_len_buffer)) == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memset((void*)_in_buffer, 0, _len_buffer);
	}
	if (_tmp_result != NULL && _len_result != 0) {
		if ( _len_result % sizeof(*_tmp_result) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		if ((_in_result = (int*)malloc(_len_result)) == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memset((void*)_in_result, 0, _len_result);
	}
	ecall_list_users(_in_buffer, _tmp_buffer_size, _in_result);
	if (_in_buffer) {
		if (memcpy_verw_s(_tmp_buffer, _len_buffer, _in_buffer, _len_buffer)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}
	if (_in_result) {
		if (memcpy_verw_s(_tmp_result, _len_result, _in_result, _len_result)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}

err:
	if (_in_buffer) free(_in_buffer);
	if (_in_result) free(_in_result);
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_upsert_patient_profile(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_upsert_patient_profile_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_upsert_patient_profile_t* ms = SGX_CAST(ms_ecall_upsert_patient_profile_t*, pms);
	ms_ecall_upsert_patient_profile_t __in_ms;
	if (memcpy_s(&__in_ms, sizeof(ms_ecall_upsert_patient_profile_t), ms, sizeof(ms_ecall_upsert_patient_profile_t))) {
		return SGX_ERROR_UNEXPECTED;
	}
	sgx_status_t status = SGX_SUCCESS;
	const char* _tmp_patient_username = __in_ms.ms_patient_username;
	size_t _len_patient_username = __in_ms.ms_patient_username_len ;
	char* _in_patient_username = NULL;
	const char* _tmp_full_name = __in_ms.ms_full_name;
	size_t _len_full_name = __in_ms.ms_full_name_len ;
	char* _in_full_name = NULL;
	const char* _tmp_id_card = __in_ms.ms_id_card;
	size_t _len_id_card = __in_ms.ms_id_card_len ;
	char* _in_id_card = NULL;
	const char* _tmp_phone = __in_ms.ms_phone;
	size_t _len_phone = __in_ms.ms_phone_len ;
	char* _in_phone = NULL;
	const char* _tmp_address = __in_ms.ms_address;
	size_t _len_address = __in_ms.ms_address_len ;
	char* _in_address = NULL;
	int* _tmp_result = __in_ms.ms_result;
	size_t _len_result = sizeof(int);
	int* _in_result = NULL;

	CHECK_UNIQUE_POINTER(_tmp_patient_username, _len_patient_username);
	CHECK_UNIQUE_POINTER(_tmp_full_name, _len_full_name);
	CHECK_UNIQUE_POINTER(_tmp_id_card, _len_id_card);
	CHECK_UNIQUE_POINTER(_tmp_phone, _len_phone);
	CHECK_UNIQUE_POINTER(_tmp_address, _len_address);
	CHECK_UNIQUE_POINTER(_tmp_result, _len_result);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_patient_username != NULL && _len_patient_username != 0) {
		_in_patient_username = (char*)malloc(_len_patient_username);
		if (_in_patient_username == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_patient_username, _len_patient_username, _tmp_patient_username, _len_patient_username)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

		_in_patient_username[_len_patient_username - 1] = '\0';
		if (_len_patient_username != strlen(_in_patient_username) + 1)
		{
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}
	if (_tmp_full_name != NULL && _len_full_name != 0) {
		_in_full_name = (char*)malloc(_len_full_name);
		if (_in_full_name == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_full_name, _len_full_name, _tmp_full_name, _len_full_name)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

		_in_full_name[_len_full_name - 1] = '\0';
		if (_len_full_name != strlen(_in_full_name) + 1)
		{
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}
	if (_tmp_id_card != NULL && _len_id_card != 0) {
		_in_id_card = (char*)malloc(_len_id_card);
		if (_in_id_card == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_id_card, _len_id_card, _tmp_id_card, _len_id_card)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

		_in_id_card[_len_id_card - 1] = '\0';
		if (_len_id_card != strlen(_in_id_card) + 1)
		{
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}
	if (_tmp_phone != NULL && _len_phone != 0) {
		_in_phone = (char*)malloc(_len_phone);
		if (_in_phone == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_phone, _len_phone, _tmp_phone, _len_phone)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

		_in_phone[_len_phone - 1] = '\0';
		if (_len_phone != strlen(_in_phone) + 1)
		{
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}
	if (_tmp_address != NULL && _len_address != 0) {
		_in_address = (char*)malloc(_len_address);
		if (_in_address == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_address, _len_address, _tmp_address, _len_address)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

		_in_address[_len_address - 1] = '\0';
		if (_len_address != strlen(_in_address) + 1)
		{
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}
	if (_tmp_result != NULL && _len_result != 0) {
		if ( _len_result % sizeof(*_tmp_result) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		if ((_in_result = (int*)malloc(_len_result)) == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memset((void*)_in_result, 0, _len_result);
	}
	ecall_upsert_patient_profile((const char*)_in_patient_username, (const char*)_in_full_name, (const char*)_in_id_card, (const char*)_in_phone, (const char*)_in_address, _in_result);
	if (_in_result) {
		if (memcpy_verw_s(_tmp_result, _len_result, _in_result, _len_result)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}

err:
	if (_in_patient_username) free(_in_patient_username);
	if (_in_full_name) free(_in_full_name);
	if (_in_id_card) free(_in_id_card);
	if (_in_phone) free(_in_phone);
	if (_in_address) free(_in_address);
	if (_in_result) free(_in_result);
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_get_patient_profile(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_get_patient_profile_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_get_patient_profile_t* ms = SGX_CAST(ms_ecall_get_patient_profile_t*, pms);
	ms_ecall_get_patient_profile_t __in_ms;
	if (memcpy_s(&__in_ms, sizeof(ms_ecall_get_patient_profile_t), ms, sizeof(ms_ecall_get_patient_profile_t))) {
		return SGX_ERROR_UNEXPECTED;
	}
	sgx_status_t status = SGX_SUCCESS;
	const char* _tmp_patient_username = __in_ms.ms_patient_username;
	size_t _len_patient_username = __in_ms.ms_patient_username_len ;
	char* _in_patient_username = NULL;
	char* _tmp_buffer = __in_ms.ms_buffer;
	size_t _tmp_buffer_size = __in_ms.ms_buffer_size;
	size_t _len_buffer = _tmp_buffer_size;
	char* _in_buffer = NULL;
	int* _tmp_result = __in_ms.ms_result;
	size_t _len_result = sizeof(int);
	int* _in_result = NULL;

	CHECK_UNIQUE_POINTER(_tmp_patient_username, _len_patient_username);
	CHECK_UNIQUE_POINTER(_tmp_buffer, _len_buffer);
	CHECK_UNIQUE_POINTER(_tmp_result, _len_result);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_patient_username != NULL && _len_patient_username != 0) {
		_in_patient_username = (char*)malloc(_len_patient_username);
		if (_in_patient_username == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_patient_username, _len_patient_username, _tmp_patient_username, _len_patient_username)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

		_in_patient_username[_len_patient_username - 1] = '\0';
		if (_len_patient_username != strlen(_in_patient_username) + 1)
		{
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}
	if (_tmp_buffer != NULL && _len_buffer != 0) {
		if ( _len_buffer % sizeof(*_tmp_buffer) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		if ((_in_buffer = (char*)malloc(_len_buffer)) == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memset((void*)_in_buffer, 0, _len_buffer);
	}
	if (_tmp_result != NULL && _len_result != 0) {
		if ( _len_result % sizeof(*_tmp_result) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		if ((_in_result = (int*)malloc(_len_result)) == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memset((void*)_in_result, 0, _len_result);
	}
	ecall_get_patient_profile((const char*)_in_patient_username, _in_buffer, _tmp_buffer_size, _in_result);
	if (_in_buffer) {
		if (memcpy_verw_s(_tmp_buffer, _len_buffer, _in_buffer, _len_buffer)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}
	if (_in_result) {
		if (memcpy_verw_s(_tmp_result, _len_result, _in_result, _len_result)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}

err:
	if (_in_patient_username) free(_in_patient_username);
	if (_in_buffer) free(_in_buffer);
	if (_in_result) free(_in_result);
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_list_patients(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_list_patients_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_list_patients_t* ms = SGX_CAST(ms_ecall_list_patients_t*, pms);
	ms_ecall_list_patients_t __in_ms;
	if (memcpy_s(&__in_ms, sizeof(ms_ecall_list_patients_t), ms, sizeof(ms_ecall_list_patients_t))) {
		return SGX_ERROR_UNEXPECTED;
	}
	sgx_status_t status = SGX_SUCCESS;
	char* _tmp_buffer = __in_ms.ms_buffer;
	size_t _tmp_buffer_size = __in_ms.ms_buffer_size;
	size_t _len_buffer = _tmp_buffer_size;
	char* _in_buffer = NULL;
	int* _tmp_result = __in_ms.ms_result;
	size_t _len_result = sizeof(int);
	int* _in_result = NULL;

	CHECK_UNIQUE_POINTER(_tmp_buffer, _len_buffer);
	CHECK_UNIQUE_POINTER(_tmp_result, _len_result);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_buffer != NULL && _len_buffer != 0) {
		if ( _len_buffer % sizeof(*_tmp_buffer) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		if ((_in_buffer = (char*)malloc(_len_buffer)) == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memset((void*)_in_buffer, 0, _len_buffer);
	}
	if (_tmp_result != NULL && _len_result != 0) {
		if ( _len_result % sizeof(*_tmp_result) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		if ((_in_result = (int*)malloc(_len_result)) == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memset((void*)_in_result, 0, _len_result);
	}
	ecall_list_patients(_in_buffer, _tmp_buffer_size, _in_result);
	if (_in_buffer) {
		if (memcpy_verw_s(_tmp_buffer, _len_buffer, _in_buffer, _len_buffer)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}
	if (_in_result) {
		if (memcpy_verw_s(_tmp_result, _len_result, _in_result, _len_result)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}

err:
	if (_in_buffer) free(_in_buffer);
	if (_in_result) free(_in_result);
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_delete_patient_profile(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_delete_patient_profile_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_delete_patient_profile_t* ms = SGX_CAST(ms_ecall_delete_patient_profile_t*, pms);
	ms_ecall_delete_patient_profile_t __in_ms;
	if (memcpy_s(&__in_ms, sizeof(ms_ecall_delete_patient_profile_t), ms, sizeof(ms_ecall_delete_patient_profile_t))) {
		return SGX_ERROR_UNEXPECTED;
	}
	sgx_status_t status = SGX_SUCCESS;
	const char* _tmp_patient_username = __in_ms.ms_patient_username;
	size_t _len_patient_username = __in_ms.ms_patient_username_len ;
	char* _in_patient_username = NULL;
	int* _tmp_result = __in_ms.ms_result;
	size_t _len_result = sizeof(int);
	int* _in_result = NULL;

	CHECK_UNIQUE_POINTER(_tmp_patient_username, _len_patient_username);
	CHECK_UNIQUE_POINTER(_tmp_result, _len_result);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_patient_username != NULL && _len_patient_username != 0) {
		_in_patient_username = (char*)malloc(_len_patient_username);
		if (_in_patient_username == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_patient_username, _len_patient_username, _tmp_patient_username, _len_patient_username)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

		_in_patient_username[_len_patient_username - 1] = '\0';
		if (_len_patient_username != strlen(_in_patient_username) + 1)
		{
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}
	if (_tmp_result != NULL && _len_result != 0) {
		if ( _len_result % sizeof(*_tmp_result) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		if ((_in_result = (int*)malloc(_len_result)) == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memset((void*)_in_result, 0, _len_result);
	}
	ecall_delete_patient_profile((const char*)_in_patient_username, _in_result);
	if (_in_result) {
		if (memcpy_verw_s(_tmp_result, _len_result, _in_result, _len_result)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}

err:
	if (_in_patient_username) free(_in_patient_username);
	if (_in_result) free(_in_result);
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_create_record(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_create_record_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_create_record_t* ms = SGX_CAST(ms_ecall_create_record_t*, pms);
	ms_ecall_create_record_t __in_ms;
	if (memcpy_s(&__in_ms, sizeof(ms_ecall_create_record_t), ms, sizeof(ms_ecall_create_record_t))) {
		return SGX_ERROR_UNEXPECTED;
	}
	sgx_status_t status = SGX_SUCCESS;
	const char* _tmp_patient_username = __in_ms.ms_patient_username;
	size_t _len_patient_username = __in_ms.ms_patient_username_len ;
	char* _in_patient_username = NULL;
	const char* _tmp_diagnosis = __in_ms.ms_diagnosis;
	size_t _len_diagnosis = __in_ms.ms_diagnosis_len ;
	char* _in_diagnosis = NULL;
	const char* _tmp_prescription = __in_ms.ms_prescription;
	size_t _len_prescription = __in_ms.ms_prescription_len ;
	char* _in_prescription = NULL;
	const char* _tmp_note = __in_ms.ms_note;
	size_t _len_note = __in_ms.ms_note_len ;
	char* _in_note = NULL;
	int* _tmp_record_id = __in_ms.ms_record_id;
	size_t _len_record_id = sizeof(int);
	int* _in_record_id = NULL;
	int* _tmp_result = __in_ms.ms_result;
	size_t _len_result = sizeof(int);
	int* _in_result = NULL;

	CHECK_UNIQUE_POINTER(_tmp_patient_username, _len_patient_username);
	CHECK_UNIQUE_POINTER(_tmp_diagnosis, _len_diagnosis);
	CHECK_UNIQUE_POINTER(_tmp_prescription, _len_prescription);
	CHECK_UNIQUE_POINTER(_tmp_note, _len_note);
	CHECK_UNIQUE_POINTER(_tmp_record_id, _len_record_id);
	CHECK_UNIQUE_POINTER(_tmp_result, _len_result);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_patient_username != NULL && _len_patient_username != 0) {
		_in_patient_username = (char*)malloc(_len_patient_username);
		if (_in_patient_username == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_patient_username, _len_patient_username, _tmp_patient_username, _len_patient_username)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

		_in_patient_username[_len_patient_username - 1] = '\0';
		if (_len_patient_username != strlen(_in_patient_username) + 1)
		{
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}
	if (_tmp_diagnosis != NULL && _len_diagnosis != 0) {
		_in_diagnosis = (char*)malloc(_len_diagnosis);
		if (_in_diagnosis == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_diagnosis, _len_diagnosis, _tmp_diagnosis, _len_diagnosis)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

		_in_diagnosis[_len_diagnosis - 1] = '\0';
		if (_len_diagnosis != strlen(_in_diagnosis) + 1)
		{
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}
	if (_tmp_prescription != NULL && _len_prescription != 0) {
		_in_prescription = (char*)malloc(_len_prescription);
		if (_in_prescription == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_prescription, _len_prescription, _tmp_prescription, _len_prescription)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

		_in_prescription[_len_prescription - 1] = '\0';
		if (_len_prescription != strlen(_in_prescription) + 1)
		{
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}
	if (_tmp_note != NULL && _len_note != 0) {
		_in_note = (char*)malloc(_len_note);
		if (_in_note == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_note, _len_note, _tmp_note, _len_note)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

		_in_note[_len_note - 1] = '\0';
		if (_len_note != strlen(_in_note) + 1)
		{
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}
	if (_tmp_record_id != NULL && _len_record_id != 0) {
		if ( _len_record_id % sizeof(*_tmp_record_id) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		if ((_in_record_id = (int*)malloc(_len_record_id)) == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memset((void*)_in_record_id, 0, _len_record_id);
	}
	if (_tmp_result != NULL && _len_result != 0) {
		if ( _len_result % sizeof(*_tmp_result) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		if ((_in_result = (int*)malloc(_len_result)) == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memset((void*)_in_result, 0, _len_result);
	}
	ecall_create_record((const char*)_in_patient_username, (const char*)_in_diagnosis, (const char*)_in_prescription, (const char*)_in_note, _in_record_id, _in_result);
	if (_in_record_id) {
		if (memcpy_verw_s(_tmp_record_id, _len_record_id, _in_record_id, _len_record_id)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}
	if (_in_result) {
		if (memcpy_verw_s(_tmp_result, _len_result, _in_result, _len_result)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}

err:
	if (_in_patient_username) free(_in_patient_username);
	if (_in_diagnosis) free(_in_diagnosis);
	if (_in_prescription) free(_in_prescription);
	if (_in_note) free(_in_note);
	if (_in_record_id) free(_in_record_id);
	if (_in_result) free(_in_result);
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_list_records(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_list_records_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_list_records_t* ms = SGX_CAST(ms_ecall_list_records_t*, pms);
	ms_ecall_list_records_t __in_ms;
	if (memcpy_s(&__in_ms, sizeof(ms_ecall_list_records_t), ms, sizeof(ms_ecall_list_records_t))) {
		return SGX_ERROR_UNEXPECTED;
	}
	sgx_status_t status = SGX_SUCCESS;
	const char* _tmp_patient_username = __in_ms.ms_patient_username;
	size_t _len_patient_username = __in_ms.ms_patient_username_len ;
	char* _in_patient_username = NULL;
	char* _tmp_buffer = __in_ms.ms_buffer;
	size_t _tmp_buffer_size = __in_ms.ms_buffer_size;
	size_t _len_buffer = _tmp_buffer_size;
	char* _in_buffer = NULL;
	int* _tmp_result = __in_ms.ms_result;
	size_t _len_result = sizeof(int);
	int* _in_result = NULL;

	CHECK_UNIQUE_POINTER(_tmp_patient_username, _len_patient_username);
	CHECK_UNIQUE_POINTER(_tmp_buffer, _len_buffer);
	CHECK_UNIQUE_POINTER(_tmp_result, _len_result);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_patient_username != NULL && _len_patient_username != 0) {
		_in_patient_username = (char*)malloc(_len_patient_username);
		if (_in_patient_username == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_patient_username, _len_patient_username, _tmp_patient_username, _len_patient_username)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

		_in_patient_username[_len_patient_username - 1] = '\0';
		if (_len_patient_username != strlen(_in_patient_username) + 1)
		{
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}
	if (_tmp_buffer != NULL && _len_buffer != 0) {
		if ( _len_buffer % sizeof(*_tmp_buffer) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		if ((_in_buffer = (char*)malloc(_len_buffer)) == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memset((void*)_in_buffer, 0, _len_buffer);
	}
	if (_tmp_result != NULL && _len_result != 0) {
		if ( _len_result % sizeof(*_tmp_result) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		if ((_in_result = (int*)malloc(_len_result)) == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memset((void*)_in_result, 0, _len_result);
	}
	ecall_list_records((const char*)_in_patient_username, _in_buffer, _tmp_buffer_size, _in_result);
	if (_in_buffer) {
		if (memcpy_verw_s(_tmp_buffer, _len_buffer, _in_buffer, _len_buffer)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}
	if (_in_result) {
		if (memcpy_verw_s(_tmp_result, _len_result, _in_result, _len_result)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}

err:
	if (_in_patient_username) free(_in_patient_username);
	if (_in_buffer) free(_in_buffer);
	if (_in_result) free(_in_result);
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_update_record(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_update_record_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_update_record_t* ms = SGX_CAST(ms_ecall_update_record_t*, pms);
	ms_ecall_update_record_t __in_ms;
	if (memcpy_s(&__in_ms, sizeof(ms_ecall_update_record_t), ms, sizeof(ms_ecall_update_record_t))) {
		return SGX_ERROR_UNEXPECTED;
	}
	sgx_status_t status = SGX_SUCCESS;
	const char* _tmp_diagnosis = __in_ms.ms_diagnosis;
	size_t _len_diagnosis = __in_ms.ms_diagnosis_len ;
	char* _in_diagnosis = NULL;
	const char* _tmp_prescription = __in_ms.ms_prescription;
	size_t _len_prescription = __in_ms.ms_prescription_len ;
	char* _in_prescription = NULL;
	const char* _tmp_note = __in_ms.ms_note;
	size_t _len_note = __in_ms.ms_note_len ;
	char* _in_note = NULL;
	int* _tmp_result = __in_ms.ms_result;
	size_t _len_result = sizeof(int);
	int* _in_result = NULL;

	CHECK_UNIQUE_POINTER(_tmp_diagnosis, _len_diagnosis);
	CHECK_UNIQUE_POINTER(_tmp_prescription, _len_prescription);
	CHECK_UNIQUE_POINTER(_tmp_note, _len_note);
	CHECK_UNIQUE_POINTER(_tmp_result, _len_result);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_diagnosis != NULL && _len_diagnosis != 0) {
		_in_diagnosis = (char*)malloc(_len_diagnosis);
		if (_in_diagnosis == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_diagnosis, _len_diagnosis, _tmp_diagnosis, _len_diagnosis)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

		_in_diagnosis[_len_diagnosis - 1] = '\0';
		if (_len_diagnosis != strlen(_in_diagnosis) + 1)
		{
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}
	if (_tmp_prescription != NULL && _len_prescription != 0) {
		_in_prescription = (char*)malloc(_len_prescription);
		if (_in_prescription == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_prescription, _len_prescription, _tmp_prescription, _len_prescription)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

		_in_prescription[_len_prescription - 1] = '\0';
		if (_len_prescription != strlen(_in_prescription) + 1)
		{
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}
	if (_tmp_note != NULL && _len_note != 0) {
		_in_note = (char*)malloc(_len_note);
		if (_in_note == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_note, _len_note, _tmp_note, _len_note)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

		_in_note[_len_note - 1] = '\0';
		if (_len_note != strlen(_in_note) + 1)
		{
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}
	if (_tmp_result != NULL && _len_result != 0) {
		if ( _len_result % sizeof(*_tmp_result) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		if ((_in_result = (int*)malloc(_len_result)) == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memset((void*)_in_result, 0, _len_result);
	}
	ecall_update_record(__in_ms.ms_record_id, (const char*)_in_diagnosis, (const char*)_in_prescription, (const char*)_in_note, _in_result);
	if (_in_result) {
		if (memcpy_verw_s(_tmp_result, _len_result, _in_result, _len_result)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}

err:
	if (_in_diagnosis) free(_in_diagnosis);
	if (_in_prescription) free(_in_prescription);
	if (_in_note) free(_in_note);
	if (_in_result) free(_in_result);
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_delete_record(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_delete_record_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_delete_record_t* ms = SGX_CAST(ms_ecall_delete_record_t*, pms);
	ms_ecall_delete_record_t __in_ms;
	if (memcpy_s(&__in_ms, sizeof(ms_ecall_delete_record_t), ms, sizeof(ms_ecall_delete_record_t))) {
		return SGX_ERROR_UNEXPECTED;
	}
	sgx_status_t status = SGX_SUCCESS;
	int* _tmp_result = __in_ms.ms_result;
	size_t _len_result = sizeof(int);
	int* _in_result = NULL;

	CHECK_UNIQUE_POINTER(_tmp_result, _len_result);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_result != NULL && _len_result != 0) {
		if ( _len_result % sizeof(*_tmp_result) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		if ((_in_result = (int*)malloc(_len_result)) == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memset((void*)_in_result, 0, _len_result);
	}
	ecall_delete_record(__in_ms.ms_record_id, _in_result);
	if (_in_result) {
		if (memcpy_verw_s(_tmp_result, _len_result, _in_result, _len_result)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}

err:
	if (_in_result) free(_in_result);
	return status;
}

SGX_EXTERNC const struct {
	size_t nr_ecall;
	struct {void* call_addr; uint8_t is_priv; uint8_t is_switchless;} ecall_table[15];
} g_ecall_table = {
	15,
	{
		{(void*)(uintptr_t)sgx_ecall_system_init, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_system_load, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_system_flush, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_register_user, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_login_user, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_logout_user, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_list_users, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_upsert_patient_profile, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_get_patient_profile, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_list_patients, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_delete_patient_profile, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_create_record, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_list_records, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_update_record, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_delete_record, 0, 0},
	}
};

SGX_EXTERNC const struct {
	size_t nr_ocall;
	uint8_t entry_table[9][15];
} g_dyn_entry_table = {
	9,
	{
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
	}
};


sgx_status_t SGX_CDECL ocall_print_log(const char* str)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_str = str ? strlen(str) + 1 : 0;

	ms_ocall_print_log_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_ocall_print_log_t);
	void *__tmp = NULL;


	CHECK_ENCLAVE_POINTER(str, _len_str);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (str != NULL) ? _len_str : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_ocall_print_log_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_ocall_print_log_t));
	ocalloc_size -= sizeof(ms_ocall_print_log_t);

	if (str != NULL) {
		if (memcpy_verw_s(&ms->ms_str, sizeof(const char*), &__tmp, sizeof(const char*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (_len_str % sizeof(*str) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_verw_s(__tmp, ocalloc_size, str, _len_str)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_str);
		ocalloc_size -= _len_str;
	} else {
		ms->ms_str = NULL;
	}

	status = sgx_ocall(0, ms);

	if (status == SGX_SUCCESS) {
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL ocall_save_blob(const char* file_name, const uint8_t* data, size_t data_size, int* result)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_file_name = file_name ? strlen(file_name) + 1 : 0;
	size_t _len_data = data_size;
	size_t _len_result = sizeof(int);

	ms_ocall_save_blob_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_ocall_save_blob_t);
	void *__tmp = NULL;

	void *__tmp_result = NULL;

	CHECK_ENCLAVE_POINTER(file_name, _len_file_name);
	CHECK_ENCLAVE_POINTER(data, _len_data);
	CHECK_ENCLAVE_POINTER(result, _len_result);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (file_name != NULL) ? _len_file_name : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (data != NULL) ? _len_data : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (result != NULL) ? _len_result : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_ocall_save_blob_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_ocall_save_blob_t));
	ocalloc_size -= sizeof(ms_ocall_save_blob_t);

	if (file_name != NULL) {
		if (memcpy_verw_s(&ms->ms_file_name, sizeof(const char*), &__tmp, sizeof(const char*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (_len_file_name % sizeof(*file_name) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_verw_s(__tmp, ocalloc_size, file_name, _len_file_name)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_file_name);
		ocalloc_size -= _len_file_name;
	} else {
		ms->ms_file_name = NULL;
	}

	if (data != NULL) {
		if (memcpy_verw_s(&ms->ms_data, sizeof(const uint8_t*), &__tmp, sizeof(const uint8_t*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (_len_data % sizeof(*data) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_verw_s(__tmp, ocalloc_size, data, _len_data)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_data);
		ocalloc_size -= _len_data;
	} else {
		ms->ms_data = NULL;
	}

	if (memcpy_verw_s(&ms->ms_data_size, sizeof(ms->ms_data_size), &data_size, sizeof(data_size))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (result != NULL) {
		if (memcpy_verw_s(&ms->ms_result, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_result = __tmp;
		if (_len_result % sizeof(*result) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_result, 0, _len_result);
		__tmp = (void *)((size_t)__tmp + _len_result);
		ocalloc_size -= _len_result;
	} else {
		ms->ms_result = NULL;
	}

	status = sgx_ocall(1, ms);

	if (status == SGX_SUCCESS) {
		if (result) {
			if (memcpy_s((void*)result, _len_result, __tmp_result, _len_result)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL ocall_load_blob(const char* file_name, uint8_t* data, size_t max_size, size_t* actual_size, int* result)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_file_name = file_name ? strlen(file_name) + 1 : 0;
	size_t _len_data = max_size;
	size_t _len_actual_size = sizeof(size_t);
	size_t _len_result = sizeof(int);

	ms_ocall_load_blob_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_ocall_load_blob_t);
	void *__tmp = NULL;

	void *__tmp_data = NULL;
	void *__tmp_actual_size = NULL;
	void *__tmp_result = NULL;

	CHECK_ENCLAVE_POINTER(file_name, _len_file_name);
	CHECK_ENCLAVE_POINTER(data, _len_data);
	CHECK_ENCLAVE_POINTER(actual_size, _len_actual_size);
	CHECK_ENCLAVE_POINTER(result, _len_result);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (file_name != NULL) ? _len_file_name : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (data != NULL) ? _len_data : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (actual_size != NULL) ? _len_actual_size : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (result != NULL) ? _len_result : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_ocall_load_blob_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_ocall_load_blob_t));
	ocalloc_size -= sizeof(ms_ocall_load_blob_t);

	if (file_name != NULL) {
		if (memcpy_verw_s(&ms->ms_file_name, sizeof(const char*), &__tmp, sizeof(const char*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (_len_file_name % sizeof(*file_name) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_verw_s(__tmp, ocalloc_size, file_name, _len_file_name)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_file_name);
		ocalloc_size -= _len_file_name;
	} else {
		ms->ms_file_name = NULL;
	}

	if (data != NULL) {
		if (memcpy_verw_s(&ms->ms_data, sizeof(uint8_t*), &__tmp, sizeof(uint8_t*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_data = __tmp;
		if (_len_data % sizeof(*data) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_data, 0, _len_data);
		__tmp = (void *)((size_t)__tmp + _len_data);
		ocalloc_size -= _len_data;
	} else {
		ms->ms_data = NULL;
	}

	if (memcpy_verw_s(&ms->ms_max_size, sizeof(ms->ms_max_size), &max_size, sizeof(max_size))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (actual_size != NULL) {
		if (memcpy_verw_s(&ms->ms_actual_size, sizeof(size_t*), &__tmp, sizeof(size_t*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_actual_size = __tmp;
		if (_len_actual_size % sizeof(*actual_size) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_actual_size, 0, _len_actual_size);
		__tmp = (void *)((size_t)__tmp + _len_actual_size);
		ocalloc_size -= _len_actual_size;
	} else {
		ms->ms_actual_size = NULL;
	}

	if (result != NULL) {
		if (memcpy_verw_s(&ms->ms_result, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_result = __tmp;
		if (_len_result % sizeof(*result) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_result, 0, _len_result);
		__tmp = (void *)((size_t)__tmp + _len_result);
		ocalloc_size -= _len_result;
	} else {
		ms->ms_result = NULL;
	}

	status = sgx_ocall(2, ms);

	if (status == SGX_SUCCESS) {
		if (data) {
			if (memcpy_s((void*)data, _len_data, __tmp_data, _len_data)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (actual_size) {
			if (memcpy_s((void*)actual_size, _len_actual_size, __tmp_actual_size, _len_actual_size)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (result) {
			if (memcpy_s((void*)result, _len_result, __tmp_result, _len_result)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL ocall_get_time(char* buffer, size_t buffer_size)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_buffer = buffer_size;

	ms_ocall_get_time_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_ocall_get_time_t);
	void *__tmp = NULL;

	void *__tmp_buffer = NULL;

	CHECK_ENCLAVE_POINTER(buffer, _len_buffer);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (buffer != NULL) ? _len_buffer : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_ocall_get_time_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_ocall_get_time_t));
	ocalloc_size -= sizeof(ms_ocall_get_time_t);

	if (buffer != NULL) {
		if (memcpy_verw_s(&ms->ms_buffer, sizeof(char*), &__tmp, sizeof(char*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_buffer = __tmp;
		if (_len_buffer % sizeof(*buffer) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_buffer, 0, _len_buffer);
		__tmp = (void *)((size_t)__tmp + _len_buffer);
		ocalloc_size -= _len_buffer;
	} else {
		ms->ms_buffer = NULL;
	}

	if (memcpy_verw_s(&ms->ms_buffer_size, sizeof(ms->ms_buffer_size), &buffer_size, sizeof(buffer_size))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(3, ms);

	if (status == SGX_SUCCESS) {
		if (buffer) {
			if (memcpy_s((void*)buffer, _len_buffer, __tmp_buffer, _len_buffer)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL sgx_oc_cpuidex(int cpuinfo[4], int leaf, int subleaf)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_cpuinfo = 4 * sizeof(int);

	ms_sgx_oc_cpuidex_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_sgx_oc_cpuidex_t);
	void *__tmp = NULL;

	void *__tmp_cpuinfo = NULL;

	CHECK_ENCLAVE_POINTER(cpuinfo, _len_cpuinfo);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (cpuinfo != NULL) ? _len_cpuinfo : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_sgx_oc_cpuidex_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_sgx_oc_cpuidex_t));
	ocalloc_size -= sizeof(ms_sgx_oc_cpuidex_t);

	if (cpuinfo != NULL) {
		if (memcpy_verw_s(&ms->ms_cpuinfo, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_cpuinfo = __tmp;
		if (_len_cpuinfo % sizeof(*cpuinfo) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_cpuinfo, 0, _len_cpuinfo);
		__tmp = (void *)((size_t)__tmp + _len_cpuinfo);
		ocalloc_size -= _len_cpuinfo;
	} else {
		ms->ms_cpuinfo = NULL;
	}

	if (memcpy_verw_s(&ms->ms_leaf, sizeof(ms->ms_leaf), &leaf, sizeof(leaf))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (memcpy_verw_s(&ms->ms_subleaf, sizeof(ms->ms_subleaf), &subleaf, sizeof(subleaf))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(4, ms);

	if (status == SGX_SUCCESS) {
		if (cpuinfo) {
			if (memcpy_s((void*)cpuinfo, _len_cpuinfo, __tmp_cpuinfo, _len_cpuinfo)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL sgx_thread_wait_untrusted_event_ocall(int* retval, const void* self)
{
	sgx_status_t status = SGX_SUCCESS;

	ms_sgx_thread_wait_untrusted_event_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_sgx_thread_wait_untrusted_event_ocall_t);
	void *__tmp = NULL;


	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_sgx_thread_wait_untrusted_event_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_sgx_thread_wait_untrusted_event_ocall_t));
	ocalloc_size -= sizeof(ms_sgx_thread_wait_untrusted_event_ocall_t);

	if (memcpy_verw_s(&ms->ms_self, sizeof(ms->ms_self), &self, sizeof(self))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(5, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL sgx_thread_set_untrusted_event_ocall(int* retval, const void* waiter)
{
	sgx_status_t status = SGX_SUCCESS;

	ms_sgx_thread_set_untrusted_event_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_sgx_thread_set_untrusted_event_ocall_t);
	void *__tmp = NULL;


	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_sgx_thread_set_untrusted_event_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_sgx_thread_set_untrusted_event_ocall_t));
	ocalloc_size -= sizeof(ms_sgx_thread_set_untrusted_event_ocall_t);

	if (memcpy_verw_s(&ms->ms_waiter, sizeof(ms->ms_waiter), &waiter, sizeof(waiter))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(6, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL sgx_thread_setwait_untrusted_events_ocall(int* retval, const void* waiter, const void* self)
{
	sgx_status_t status = SGX_SUCCESS;

	ms_sgx_thread_setwait_untrusted_events_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_sgx_thread_setwait_untrusted_events_ocall_t);
	void *__tmp = NULL;


	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_sgx_thread_setwait_untrusted_events_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_sgx_thread_setwait_untrusted_events_ocall_t));
	ocalloc_size -= sizeof(ms_sgx_thread_setwait_untrusted_events_ocall_t);

	if (memcpy_verw_s(&ms->ms_waiter, sizeof(ms->ms_waiter), &waiter, sizeof(waiter))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (memcpy_verw_s(&ms->ms_self, sizeof(ms->ms_self), &self, sizeof(self))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(7, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL sgx_thread_set_multiple_untrusted_events_ocall(int* retval, const void** waiters, size_t total)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_waiters = total * sizeof(void*);

	ms_sgx_thread_set_multiple_untrusted_events_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_sgx_thread_set_multiple_untrusted_events_ocall_t);
	void *__tmp = NULL;


	CHECK_ENCLAVE_POINTER(waiters, _len_waiters);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (waiters != NULL) ? _len_waiters : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_sgx_thread_set_multiple_untrusted_events_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_sgx_thread_set_multiple_untrusted_events_ocall_t));
	ocalloc_size -= sizeof(ms_sgx_thread_set_multiple_untrusted_events_ocall_t);

	if (waiters != NULL) {
		if (memcpy_verw_s(&ms->ms_waiters, sizeof(const void**), &__tmp, sizeof(const void**))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (_len_waiters % sizeof(*waiters) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_verw_s(__tmp, ocalloc_size, waiters, _len_waiters)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_waiters);
		ocalloc_size -= _len_waiters;
	} else {
		ms->ms_waiters = NULL;
	}

	if (memcpy_verw_s(&ms->ms_total, sizeof(ms->ms_total), &total, sizeof(total))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(8, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
