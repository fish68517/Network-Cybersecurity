import ctypes
from ctypes import c_char_p, c_int, c_size_t, create_string_buffer, byref
from pathlib import Path
import sys

MSD_OK = 0
MSD_ROLE_NONE = 0
MSD_ROLE_ADMIN = 1
MSD_ROLE_DOCTOR = 2
MSD_ROLE_PATIENT = 3
MSD_TEXT_BUFFER = 8192

RESULT_TEXT = {
    0: '成功',
    -1: '输入无效',
    -2: '系统未初始化',
    -3: '系统已初始化',
    -4: '权限不足',
    -5: '用户已存在',
    -6: '用户不存在',
    -7: '认证失败',
    -8: '患者档案不存在',
    -9: '病历不存在',
    -10: '密态存储失败',
    -11: '缓冲区不足',
    -12: '容量已满',
    -13: '角色无效',
    -14: '密态文件不存在',
    -15: '系统状态损坏',
    -16: '内部错误',
    -17: '已有用户处于登录状态',
    -18: '患者仍有关联病历',
    -19: '当前操作需要先完成远程认证',
    -20: '远程认证上下文尚未准备好',
    -21: '远程认证流程正在进行中',
    -22: '远程认证验证失败',
    -23: '安全会话尚未建立',
    -24: '远程认证消息无效',
    -25: '当前环境暂不支持完整远程认证',
}

ROLE_TEXT = {
    MSD_ROLE_NONE: '未登录',
    MSD_ROLE_ADMIN: '管理员',
    MSD_ROLE_DOCTOR: '医生',
    MSD_ROLE_PATIENT: '患者',
}

RA_STATUS_TEXT = {
    0: '未开始',
    1: '上下文已初始化',
    2: 'Msg1 已生成',
    3: '等待 Msg2',
    4: 'Msg3 已生成',
    5: '等待认证结果',
    6: '认证通过',
    7: '认证失败',
}


class BridgeError(RuntimeError):
    pass


class BackendClient:
    def __init__(self, bridge_dir: str | None = None):
        app_dir = Path(__file__).resolve().parents[2]
        exe_dir = Path(sys.executable).resolve().parent

        candidates = []
        if bridge_dir:
            candidates.append(Path(bridge_dir))
        candidates.extend([
            app_dir / 'bridge' / 'x64' / 'Release',
            app_dir,
            exe_dir,
        ])

        self.bridge_dir = None
        self.dll_path = None
        for candidate in candidates:
            dll_path = candidate / 'sgx_gui_bridge.dll'
            if dll_path.exists():
                self.bridge_dir = candidate
                self.dll_path = dll_path
                break

        if self.dll_path is None:
            raise BridgeError(f'未找到 bridge DLL，检查路径: {candidates}')

        self._dll = ctypes.CDLL(str(self.dll_path))
        self._configure_signatures()
        self._open()

    def _configure_signatures(self):
        d = self._dll
        d.msd_gui_open.restype = c_int
        d.msd_gui_close.restype = None
        d.msd_gui_init_system.restype = c_int
        d.msd_gui_load_system.restype = c_int
        d.msd_gui_flush_system.restype = c_int

        d.msd_gui_login.argtypes = [c_char_p, c_char_p, ctypes.POINTER(c_int)]
        d.msd_gui_login.restype = c_int
        d.msd_gui_logout.restype = c_int
        d.msd_gui_get_current_role.argtypes = [ctypes.POINTER(c_int)]
        d.msd_gui_get_current_role.restype = c_int
        d.msd_gui_get_current_username.argtypes = [c_char_p, c_size_t]
        d.msd_gui_get_current_username.restype = c_int

        d.msd_gui_register_user.argtypes = [c_char_p, c_char_p, c_int]
        d.msd_gui_register_user.restype = c_int
        d.msd_gui_list_users.argtypes = [c_char_p, c_size_t]
        d.msd_gui_list_users.restype = c_int

        d.msd_gui_upsert_patient_profile.argtypes = [c_char_p, c_char_p, c_char_p, c_char_p, c_char_p]
        d.msd_gui_upsert_patient_profile.restype = c_int
        d.msd_gui_get_patient_profile.argtypes = [c_char_p, c_char_p, c_size_t]
        d.msd_gui_get_patient_profile.restype = c_int
        d.msd_gui_list_patients.argtypes = [c_char_p, c_size_t]
        d.msd_gui_list_patients.restype = c_int
        d.msd_gui_delete_patient_profile.argtypes = [c_char_p]
        d.msd_gui_delete_patient_profile.restype = c_int

        d.msd_gui_create_record.argtypes = [c_char_p, c_char_p, c_char_p, c_char_p, ctypes.POINTER(c_int)]
        d.msd_gui_create_record.restype = c_int
        d.msd_gui_list_records.argtypes = [c_char_p, c_char_p, c_size_t]
        d.msd_gui_list_records.restype = c_int
        d.msd_gui_update_record.argtypes = [c_int, c_char_p, c_char_p, c_char_p]
        d.msd_gui_update_record.restype = c_int
        d.msd_gui_delete_record.argtypes = [c_int]
        d.msd_gui_delete_record.restype = c_int

        d.msd_gui_ra_start.argtypes = [c_char_p, ctypes.POINTER(c_int), ctypes.POINTER(c_int)]
        d.msd_gui_ra_start.restype = c_int
        d.msd_gui_ra_get_status.argtypes = [ctypes.POINTER(c_int), ctypes.POINTER(c_int)]
        d.msd_gui_ra_get_status.restype = c_int

        d.msd_gui_get_log_text.argtypes = [c_char_p, c_size_t]
        d.msd_gui_get_log_text.restype = c_int
        d.msd_gui_clear_log.restype = None
        d.msd_gui_get_last_error.argtypes = [c_char_p, c_size_t]
        d.msd_gui_get_last_error.restype = c_int
        d.msd_gui_get_runtime_directory.argtypes = [c_char_p, c_size_t]
        d.msd_gui_get_runtime_directory.restype = c_int
        d.msd_gui_get_storage_path.argtypes = [c_char_p, c_size_t]
        d.msd_gui_get_storage_path.restype = c_int
        d.msd_gui_set_runtime_directory_utf8.argtypes = [c_char_p]
        d.msd_gui_set_runtime_directory_utf8.restype = c_int

        d.msd_gui_result_text.argtypes = [c_int, c_char_p, c_size_t]
        d.msd_gui_result_text.restype = c_int
        d.msd_gui_role_text.argtypes = [c_int, c_char_p, c_size_t]
        d.msd_gui_role_text.restype = c_int
        d.msd_gui_ra_status_text.argtypes = [c_int, c_char_p, c_size_t]
        d.msd_gui_ra_status_text.restype = c_int

    def _open(self):
        result = self._dll.msd_gui_open()
        if result != MSD_OK:
            raise BridgeError(self.get_last_error() or f'Bridge 打开失败: {result}')

    def close(self):
        self._dll.msd_gui_close()

    def _call_text(self, func_name: str, *args, size: int = MSD_TEXT_BUFFER):
        buffer = create_string_buffer(size)
        func = getattr(self._dll, func_name)
        result = func(*args, buffer, size)
        return result, buffer.value.decode('utf-8', errors='replace')

    def result_text(self, code: int) -> str:
        if code in RESULT_TEXT:
            return RESULT_TEXT[code]
        _, text = self._call_text('msd_gui_result_text', code)
        return text

    def role_text(self, role: int) -> str:
        if role in ROLE_TEXT:
            return ROLE_TEXT[role]
        _, text = self._call_text('msd_gui_role_text', role)
        return text

    def ra_status_text(self, status: int) -> str:
        if status in RA_STATUS_TEXT:
            return RA_STATUS_TEXT[status]
        _, text = self._call_text('msd_gui_ra_status_text', status)
        return text

    def get_last_error(self) -> str:
        _, text = self._call_text('msd_gui_get_last_error')
        return text

    def get_logs(self) -> str:
        _, text = self._call_text('msd_gui_get_log_text', size=65536)
        return text

    def clear_logs(self):
        self._dll.msd_gui_clear_log()

    def get_runtime_directory(self) -> str:
        _, text = self._call_text('msd_gui_get_runtime_directory')
        return text

    def get_storage_path(self) -> str:
        _, text = self._call_text('msd_gui_get_storage_path')
        return text

    def set_runtime_directory(self, directory: str) -> int:
        return self._dll.msd_gui_set_runtime_directory_utf8(directory.encode('utf-8'))

    def init_system(self) -> int:
        return self._dll.msd_gui_init_system()

    def load_system(self) -> int:
        return self._dll.msd_gui_load_system()

    def flush_system(self) -> int:
        return self._dll.msd_gui_flush_system()

    def login(self, username: str, password: str):
        role = c_int(MSD_ROLE_NONE)
        result = self._dll.msd_gui_login(username.encode('utf-8'), password.encode('utf-8'), byref(role))
        return result, role.value

    def logout(self) -> int:
        return self._dll.msd_gui_logout()

    def get_current_role(self) -> int:
        role = c_int(MSD_ROLE_NONE)
        self._dll.msd_gui_get_current_role(byref(role))
        return role.value

    def get_current_username(self) -> str:
        _, text = self._call_text('msd_gui_get_current_username')
        return text

    def register_user(self, username: str, password: str, role: int) -> int:
        return self._dll.msd_gui_register_user(username.encode('utf-8'), password.encode('utf-8'), role)

    def list_users(self):
        return self._call_text('msd_gui_list_users')

    def upsert_patient_profile(self, patient_username: str, full_name: str, id_card: str, phone: str, address: str) -> int:
        return self._dll.msd_gui_upsert_patient_profile(
            patient_username.encode('utf-8'),
            full_name.encode('utf-8'),
            id_card.encode('utf-8'),
            phone.encode('utf-8'),
            address.encode('utf-8'),
        )

    def get_patient_profile(self, patient_username: str):
        return self._call_text('msd_gui_get_patient_profile', patient_username.encode('utf-8'))

    def list_patients(self):
        return self._call_text('msd_gui_list_patients')

    def delete_patient_profile(self, patient_username: str) -> int:
        return self._dll.msd_gui_delete_patient_profile(patient_username.encode('utf-8'))

    def create_record(self, patient_username: str, diagnosis: str, prescription: str, note: str):
        record_id = c_int(0)
        result = self._dll.msd_gui_create_record(
            patient_username.encode('utf-8'),
            diagnosis.encode('utf-8'),
            prescription.encode('utf-8'),
            note.encode('utf-8'),
            byref(record_id),
        )
        return result, record_id.value

    def list_records(self, patient_username: str):
        return self._call_text('msd_gui_list_records', patient_username.encode('utf-8'))

    def update_record(self, record_id: int, diagnosis: str, prescription: str, note: str) -> int:
        return self._dll.msd_gui_update_record(
            int(record_id),
            diagnosis.encode('utf-8'),
            prescription.encode('utf-8'),
            note.encode('utf-8'),
        )

    def delete_record(self, record_id: int) -> int:
        return self._dll.msd_gui_delete_record(int(record_id))

    def start_ra(self, peer_identity: str = 'service_provider'):
        ra_status = c_int(0)
        secure = c_int(0)
        result = self._dll.msd_gui_ra_start(peer_identity.encode('utf-8'), byref(ra_status), byref(secure))
        return result, ra_status.value, secure.value

    def get_ra_status(self):
        ra_status = c_int(0)
        secure = c_int(0)
        result = self._dll.msd_gui_ra_get_status(byref(ra_status), byref(secure))
        return result, ra_status.value, secure.value
