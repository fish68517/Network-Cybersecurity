#ifndef MSD_GUI_BRIDGE_H__
#define MSD_GUI_BRIDGE_H__

#include <stddef.h>

#ifdef _WIN32
#define MSD_GUI_API extern "C" __declspec(dllexport)
#else
#define MSD_GUI_API extern "C"
#endif

MSD_GUI_API int msd_gui_open();
MSD_GUI_API void msd_gui_close();

MSD_GUI_API int msd_gui_init_system();
MSD_GUI_API int msd_gui_load_system();
MSD_GUI_API int msd_gui_flush_system();

MSD_GUI_API int msd_gui_login(const char* username, const char* password, int* out_role);
MSD_GUI_API int msd_gui_logout();
MSD_GUI_API int msd_gui_get_current_role(int* out_role);
MSD_GUI_API int msd_gui_get_current_username(char* buffer, size_t buffer_size);

MSD_GUI_API int msd_gui_register_user(const char* username, const char* password, int role);
MSD_GUI_API int msd_gui_list_users(char* buffer, size_t buffer_size);

MSD_GUI_API int msd_gui_upsert_patient_profile(
    const char* patient_username,
    const char* full_name,
    const char* id_card,
    const char* phone,
    const char* address);
MSD_GUI_API int msd_gui_get_patient_profile(const char* patient_username, char* buffer, size_t buffer_size);
MSD_GUI_API int msd_gui_list_patients(char* buffer, size_t buffer_size);
MSD_GUI_API int msd_gui_delete_patient_profile(const char* patient_username);

MSD_GUI_API int msd_gui_create_record(
    const char* patient_username,
    const char* diagnosis,
    const char* prescription,
    const char* note,
    int* out_record_id);
MSD_GUI_API int msd_gui_list_records(const char* patient_username, char* buffer, size_t buffer_size);
MSD_GUI_API int msd_gui_update_record(int record_id, const char* diagnosis, const char* prescription, const char* note);
MSD_GUI_API int msd_gui_delete_record(int record_id);

MSD_GUI_API int msd_gui_ra_start(const char* peer_identity, int* out_ra_status, int* out_secure_channel);
MSD_GUI_API int msd_gui_ra_get_status(int* out_ra_status, int* out_secure_channel);

MSD_GUI_API int msd_gui_get_log_text(char* buffer, size_t buffer_size);
MSD_GUI_API void msd_gui_clear_log();
MSD_GUI_API int msd_gui_get_last_error(char* buffer, size_t buffer_size);
MSD_GUI_API int msd_gui_get_runtime_directory(char* buffer, size_t buffer_size);
MSD_GUI_API int msd_gui_get_storage_path(char* buffer, size_t buffer_size);
MSD_GUI_API int msd_gui_set_runtime_directory_utf8(const char* directory_utf8);

MSD_GUI_API int msd_gui_result_text(int result_code, char* buffer, size_t buffer_size);
MSD_GUI_API int msd_gui_role_text(int role, char* buffer, size_t buffer_size);
MSD_GUI_API int msd_gui_ra_status_text(int ra_status, char* buffer, size_t buffer_size);

#endif
