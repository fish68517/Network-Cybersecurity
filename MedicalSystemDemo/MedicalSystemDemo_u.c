#include "MedicalSystemDemo_u.h"
#include <errno.h>

typedef struct ms_ecall_secure_save_record_t {
	const char* ms_name;
	size_t ms_name_len;
	const char* ms_diagnosis;
	size_t ms_diagnosis_len;
} ms_ecall_secure_save_record_t;

typedef struct ms_ocall_print_log_t {
	const char* ms_str;
} ms_ocall_print_log_t;

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
	void * func_addr[6];
} ocall_table_MedicalSystemDemo = {
	6,
	{
		(void*)(uintptr_t)MedicalSystemDemo_ocall_print_log,
		(void*)(uintptr_t)MedicalSystemDemo_sgx_oc_cpuidex,
		(void*)(uintptr_t)MedicalSystemDemo_sgx_thread_wait_untrusted_event_ocall,
		(void*)(uintptr_t)MedicalSystemDemo_sgx_thread_set_untrusted_event_ocall,
		(void*)(uintptr_t)MedicalSystemDemo_sgx_thread_setwait_untrusted_events_ocall,
		(void*)(uintptr_t)MedicalSystemDemo_sgx_thread_set_multiple_untrusted_events_ocall,
	}
};

sgx_status_t ecall_secure_save_record(sgx_enclave_id_t eid, const char* name, const char* diagnosis)
{
	sgx_status_t status;
	ms_ecall_secure_save_record_t ms;
	ms.ms_name = name;
	ms.ms_name_len = name ? strlen(name) + 1 : 0;
	ms.ms_diagnosis = diagnosis;
	ms.ms_diagnosis_len = diagnosis ? strlen(diagnosis) + 1 : 0;
	status = sgx_ecall(eid, 0, &ocall_table_MedicalSystemDemo, &ms);
	return status;
}

