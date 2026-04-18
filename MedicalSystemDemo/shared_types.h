#ifndef SHARED_TYPES_H__
#define SHARED_TYPES_H__

#include <stdint.h>

#define MSD_STATE_MAGIC 0x4D534430u
#define MSD_STATE_VERSION 1u
#define MSD_STATE_FILE_NAME "sealed_system_state.bin"

#define MSD_MAX_USERS 32
#define MSD_MAX_PATIENTS 32
#define MSD_MAX_RECORDS 128

#define MSD_MAX_USERNAME 32
#define MSD_MAX_PASSWORD 32
#define MSD_MAX_NAME 64
#define MSD_MAX_ID_CARD 32
#define MSD_MAX_PHONE 32
#define MSD_MAX_ADDRESS 128
#define MSD_MAX_DIAGNOSIS 256
#define MSD_MAX_PRESCRIPTION 256
#define MSD_MAX_NOTE 256
#define MSD_MAX_TIMESTAMP 32
#define MSD_MAX_TEXT_BUFFER 8192
#define MSD_RA_MAX_MESSAGE_SIZE 8192
#define MSD_RA_SESSION_KEY_SIZE 32
#define MSD_RA_MAX_IDENTITY 64

enum MsdRole {
    MSD_ROLE_NONE = 0,
    MSD_ROLE_ADMIN = 1,
    MSD_ROLE_DOCTOR = 2,
    MSD_ROLE_PATIENT = 3
};

enum MsdResult {
    MSD_OK = 0,
    MSD_ERR_INVALID_INPUT = -1,
    MSD_ERR_NOT_INITIALIZED = -2,
    MSD_ERR_ALREADY_INITIALIZED = -3,
    MSD_ERR_NO_PERMISSION = -4,
    MSD_ERR_USER_EXISTS = -5,
    MSD_ERR_USER_NOT_FOUND = -6,
    MSD_ERR_AUTH_FAILED = -7,
    MSD_ERR_PROFILE_NOT_FOUND = -8,
    MSD_ERR_RECORD_NOT_FOUND = -9,
    MSD_ERR_STORAGE_FAILED = -10,
    MSD_ERR_BUFFER_TOO_SMALL = -11,
    MSD_ERR_CAPACITY_REACHED = -12,
    MSD_ERR_INVALID_ROLE = -13,
    MSD_ERR_FILE_NOT_FOUND = -14,
    MSD_ERR_STATE_CORRUPTED = -15,
    MSD_ERR_INTERNAL = -16,
    MSD_ERR_SESSION_ACTIVE = -17,
    MSD_ERR_PROFILE_HAS_RECORDS = -18,
    MSD_ERR_RA_REQUIRED = -19,
    MSD_ERR_RA_CONTEXT_NOT_READY = -20,
    MSD_ERR_RA_BUSY = -21,
    MSD_ERR_RA_VERIFY_FAILED = -22,
    MSD_ERR_SECURE_CHANNEL_NOT_READY = -23,
    MSD_ERR_RA_MSG_INVALID = -24,
    MSD_ERR_RA_UNSUPPORTED = -25
};

enum MsdRaStatus {
    MSD_RA_NOT_STARTED = 0,
    MSD_RA_CONTEXT_READY = 1,
    MSD_RA_MSG1_READY = 2,
    MSD_RA_WAITING_MSG2 = 3,
    MSD_RA_MSG3_READY = 4,
    MSD_RA_WAITING_RESULT = 5,
    MSD_RA_VERIFIED = 6,
    MSD_RA_FAILED = 7
};

enum MsdRaMessageType {
    MSD_RA_MSG_NONE = 0,
    MSD_RA_MSG0 = 1,
    MSD_RA_MSG1 = 2,
    MSD_RA_MSG2 = 3,
    MSD_RA_MSG3 = 4,
    MSD_RA_ATT_RESULT = 5,
    MSD_RA_SECURE_CHANNEL = 6
};

typedef struct MsdUserAccount {
    char username[MSD_MAX_USERNAME];
    char password[MSD_MAX_PASSWORD];
    int role;
    int active;
} MsdUserAccount;

typedef struct MsdPatientProfile {
    char patient_username[MSD_MAX_USERNAME];
    char full_name[MSD_MAX_NAME];
    char id_card[MSD_MAX_ID_CARD];
    char phone[MSD_MAX_PHONE];
    char address[MSD_MAX_ADDRESS];
    int active;
} MsdPatientProfile;

typedef struct MsdMedicalRecord {
    int record_id;
    char patient_username[MSD_MAX_USERNAME];
    char doctor_username[MSD_MAX_USERNAME];
    char diagnosis[MSD_MAX_DIAGNOSIS];
    char prescription[MSD_MAX_PRESCRIPTION];
    char note[MSD_MAX_NOTE];
    char created_at[MSD_MAX_TIMESTAMP];
    int active;
} MsdMedicalRecord;

typedef struct MsdRaMessage {
    uint32_t message_type;
    uint32_t payload_size;
    uint8_t payload[MSD_RA_MAX_MESSAGE_SIZE];
} MsdRaMessage;

typedef struct MsdRaSessionState {
    int status;
    int secure_channel_ready;
    uint32_t session_id;
    uint32_t last_message_type;
    uint32_t session_key_size;
    char peer_identity[MSD_RA_MAX_IDENTITY];
    uint8_t session_key[MSD_RA_SESSION_KEY_SIZE];
} MsdRaSessionState;

typedef struct MsdSystemState {
    uint32_t magic;
    uint32_t version;
    int initialized;
    int next_record_id;
    int active_session_index;
    int reserved;
    MsdUserAccount users[MSD_MAX_USERS];
    MsdPatientProfile patients[MSD_MAX_PATIENTS];
    MsdMedicalRecord records[MSD_MAX_RECORDS];
} MsdSystemState;

#endif
