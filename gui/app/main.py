import tkinter as tk
from tkinter import ttk, messagebox

from services.backend_client import (
    BackendClient,
    BridgeError,
    MSD_OK,
    MSD_ROLE_ADMIN,
    MSD_ROLE_DOCTOR,
    MSD_ROLE_PATIENT,
)


class MedicalSystemGuiApp:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title('Intel SGX 密态病历系统 - GUI版')
        self.root.geometry('1380x860')
        self.root.minsize(1200, 760)

        self.backend = BackendClient()
        self.role = self.backend.get_current_role()
        self.ra_status_code = 0
        self.secure_channel_ready = 0

        self.status_text = tk.StringVar(value='Bridge 已连接')
        self.session_text = tk.StringVar(value='当前用户: 未登录')
        self.ra_text = tk.StringVar(value='远程认证: 未开始 | 安全会话: 未建立')
        self.storage_text = tk.StringVar(value=f'密态文件: {self.backend.get_storage_path()}')

        self._build_ui()
        self.refresh_all()
        self.root.protocol('WM_DELETE_WINDOW', self.on_close)

    def _build_ui(self):
        style = ttk.Style()
        style.theme_use('clam')
        style.configure('Title.TLabel', font=('Microsoft YaHei', 18, 'bold'))
        style.configure('Section.TLabelframe.Label', font=('Microsoft YaHei', 10, 'bold'))

        root_frame = ttk.Frame(self.root, padding=12)
        root_frame.pack(fill='both', expand=True)
        root_frame.columnconfigure(0, weight=3)
        root_frame.columnconfigure(1, weight=2)
        root_frame.rowconfigure(1, weight=1)

        header = ttk.Frame(root_frame)
        header.grid(row=0, column=0, columnspan=2, sticky='ew', pady=(0, 10))
        header.columnconfigure(0, weight=1)
        ttk.Label(header, text='Intel SGX 密态病历系统 - GUI版', style='Title.TLabel').grid(row=0, column=0, sticky='w')
        ttk.Label(header, textvariable=self.status_text).grid(row=1, column=0, sticky='w', pady=(4, 0))
        ttk.Label(header, textvariable=self.session_text).grid(row=2, column=0, sticky='w', pady=(2, 0))
        ttk.Label(header, textvariable=self.ra_text).grid(row=3, column=0, sticky='w', pady=(2, 0))
        ttk.Label(header, textvariable=self.storage_text, wraplength=1100).grid(row=4, column=0, sticky='w', pady=(2, 0))

        left = ttk.Frame(root_frame)
        left.grid(row=1, column=0, sticky='nsew', padx=(0, 10))
        left.rowconfigure(1, weight=1)
        left.columnconfigure(0, weight=1)

        self.login_frame = ttk.LabelFrame(left, text='系统入口', style='Section.TLabelframe', padding=12)
        self.login_frame.grid(row=0, column=0, sticky='ew', pady=(0, 10))
        self._build_login_panel(self.login_frame)

        self.notebook = ttk.Notebook(left)
        self.notebook.grid(row=1, column=0, sticky='nsew')

        self.admin_tab = ttk.Frame(self.notebook, padding=10)
        self.doctor_tab = ttk.Frame(self.notebook, padding=10)
        self.patient_tab = ttk.Frame(self.notebook, padding=10)
        self.system_tab = ttk.Frame(self.notebook, padding=10)
        self.notebook.add(self.admin_tab, text='管理员')
        self.notebook.add(self.doctor_tab, text='医生')
        self.notebook.add(self.patient_tab, text='患者')
        self.notebook.add(self.system_tab, text='系统信息')

        self._build_admin_tab()
        self._build_doctor_tab()
        self._build_patient_tab()
        self._build_system_tab()

        right = ttk.Frame(root_frame)
        right.grid(row=1, column=1, sticky='nsew')
        right.rowconfigure(0, weight=1)
        right.columnconfigure(0, weight=1)
        self._build_log_panel(right)

    def _build_login_panel(self, parent):
        parent.columnconfigure(1, weight=1)
        ttk.Button(parent, text='初始化系统', command=self.handle_init_system).grid(row=0, column=0, sticky='ew', padx=(0, 8), pady=4)
        ttk.Button(parent, text='加载密态状态', command=self.handle_load_system).grid(row=0, column=1, sticky='ew', pady=4)

        ttk.Label(parent, text='用户名').grid(row=1, column=0, sticky='w', pady=(10, 4))
        self.login_username = ttk.Entry(parent)
        self.login_username.grid(row=1, column=1, sticky='ew', pady=(10, 4))
        ttk.Label(parent, text='密码').grid(row=2, column=0, sticky='w', pady=4)
        self.login_password = ttk.Entry(parent, show='*')
        self.login_password.grid(row=2, column=1, sticky='ew', pady=4)
        button_bar = ttk.Frame(parent)
        button_bar.grid(row=3, column=0, columnspan=2, sticky='ew', pady=(10, 0))
        for column in range(3):
            button_bar.columnconfigure(column, weight=1)
        ttk.Button(button_bar, text='登录', command=self.handle_login).grid(row=0, column=0, sticky='ew', padx=(0, 6))
        ttk.Button(button_bar, text='退出登录', command=self.handle_logout).grid(row=0, column=1, sticky='ew', padx=3)
        ttk.Button(button_bar, text='保存密态状态', command=self.handle_flush).grid(row=0, column=2, sticky='ew', padx=(6, 0))

    def _build_admin_tab(self):
        self.admin_tab.columnconfigure(0, weight=1)
        self.admin_tab.columnconfigure(1, weight=1)
        self.admin_tab.rowconfigure(1, weight=1)

        account_box = ttk.LabelFrame(self.admin_tab, text='账号管理', style='Section.TLabelframe', padding=10)
        account_box.grid(row=0, column=0, sticky='nsew', padx=(0, 8), pady=(0, 8))
        account_box.columnconfigure(1, weight=1)
        ttk.Label(account_box, text='用户名').grid(row=0, column=0, sticky='w', pady=4)
        self.admin_new_user = ttk.Entry(account_box)
        self.admin_new_user.grid(row=0, column=1, sticky='ew', pady=4)
        ttk.Label(account_box, text='密码').grid(row=1, column=0, sticky='w', pady=4)
        self.admin_new_password = ttk.Entry(account_box)
        self.admin_new_password.grid(row=1, column=1, sticky='ew', pady=4)
        ttk.Button(account_box, text='注册患者', command=lambda: self.handle_register_user(MSD_ROLE_PATIENT)).grid(row=2, column=0, sticky='ew', pady=(8, 0), padx=(0, 6))
        ttk.Button(account_box, text='注册医生', command=lambda: self.handle_register_user(MSD_ROLE_DOCTOR)).grid(row=2, column=1, sticky='ew', pady=(8, 0))
        ttk.Button(account_box, text='查看用户列表', command=self.handle_list_users).grid(row=3, column=0, columnspan=2, sticky='ew', pady=(8, 0))

        patient_box = ttk.LabelFrame(self.admin_tab, text='患者档案管理', style='Section.TLabelframe', padding=10)
        patient_box.grid(row=0, column=1, sticky='nsew', pady=(0, 8))
        patient_box.columnconfigure(1, weight=1)
        labels = ['患者用户名', '姓名', '身份证号', '电话', '地址']
        self.admin_patient_entries = {}
        for idx, label in enumerate(labels):
            ttk.Label(patient_box, text=label).grid(row=idx, column=0, sticky='w', pady=4)
            entry = ttk.Entry(patient_box)
            entry.grid(row=idx, column=1, sticky='ew', pady=4)
            self.admin_patient_entries[label] = entry
        ttk.Button(patient_box, text='新增/更新档案', command=self.handle_upsert_patient).grid(row=5, column=0, sticky='ew', pady=(8, 0), padx=(0, 6))
        ttk.Button(patient_box, text='查看单个档案', command=self.handle_get_patient).grid(row=5, column=1, sticky='ew', pady=(8, 0))
        ttk.Button(patient_box, text='查看患者列表', command=self.handle_list_patients).grid(row=6, column=0, sticky='ew', pady=(8, 0), padx=(0, 6))
        ttk.Button(patient_box, text='删除档案', command=self.handle_delete_patient).grid(row=6, column=1, sticky='ew', pady=(8, 0))

        self.admin_output = self._create_scrollable_text(self.admin_tab, row=1, column=0, columnspan=2)

    def _build_doctor_tab(self):
        self.doctor_tab.columnconfigure(0, weight=1)
        self.doctor_tab.columnconfigure(1, weight=1)
        self.doctor_tab.rowconfigure(1, weight=1)

        ra_box = ttk.LabelFrame(self.doctor_tab, text='远程认证', style='Section.TLabelframe', padding=10)
        ra_box.grid(row=0, column=0, sticky='nsew', padx=(0, 8), pady=(0, 8))
        ra_box.columnconfigure(1, weight=1)
        ttk.Label(ra_box, text='对端标识').grid(row=0, column=0, sticky='w', pady=4)
        self.doctor_peer = ttk.Entry(ra_box)
        self.doctor_peer.insert(0, 'service_provider')
        self.doctor_peer.grid(row=0, column=1, sticky='ew', pady=4)
        ttk.Button(ra_box, text='执行远程认证', command=self.handle_start_ra).grid(row=1, column=0, sticky='ew', pady=(8, 0), padx=(0, 6))
        ttk.Button(ra_box, text='查看认证状态', command=self.handle_get_ra_status).grid(row=1, column=1, sticky='ew', pady=(8, 0))
        ttk.Button(ra_box, text='查看患者列表', command=self.handle_list_patients).grid(row=2, column=0, columnspan=2, sticky='ew', pady=(8, 0))

        record_box = ttk.LabelFrame(self.doctor_tab, text='病历管理', style='Section.TLabelframe', padding=10)
        record_box.grid(row=0, column=1, sticky='nsew', pady=(0, 8))
        record_box.columnconfigure(1, weight=1)
        labels = ['患者用户名', '病历ID', '诊断', '处方', '备注']
        self.doctor_entries = {}
        for idx, label in enumerate(labels):
            ttk.Label(record_box, text=label).grid(row=idx, column=0, sticky='w', pady=4)
            entry = ttk.Entry(record_box)
            entry.grid(row=idx, column=1, sticky='ew', pady=4)
            self.doctor_entries[label] = entry
        ttk.Button(record_box, text='新增病历', command=self.handle_create_record).grid(row=5, column=0, sticky='ew', pady=(8, 0), padx=(0, 6))
        ttk.Button(record_box, text='查看病历列表', command=self.handle_list_records_for_target).grid(row=5, column=1, sticky='ew', pady=(8, 0))
        ttk.Button(record_box, text='修改病历', command=self.handle_update_record).grid(row=6, column=0, sticky='ew', pady=(8, 0), padx=(0, 6))
        ttk.Button(record_box, text='删除病历', command=self.handle_delete_record).grid(row=6, column=1, sticky='ew', pady=(8, 0))

        self.doctor_output = self._create_scrollable_text(self.doctor_tab, row=1, column=0, columnspan=2)

    def _build_patient_tab(self):
        self.patient_tab.columnconfigure(0, weight=1)
        self.patient_tab.columnconfigure(1, weight=1)
        self.patient_tab.rowconfigure(1, weight=1)

        self_box = ttk.LabelFrame(self.patient_tab, text='本人信息', style='Section.TLabelframe', padding=10)
        self_box.grid(row=0, column=0, sticky='nsew', padx=(0, 8), pady=(0, 8))
        self_box.columnconfigure(1, weight=1)
        labels = ['姓名', '身份证号', '电话', '地址']
        self.patient_entries = {}
        for idx, label in enumerate(labels):
            ttk.Label(self_box, text=label).grid(row=idx, column=0, sticky='w', pady=4)
            entry = ttk.Entry(self_box)
            entry.grid(row=idx, column=1, sticky='ew', pady=4)
            self.patient_entries[label] = entry
        ttk.Button(self_box, text='查看本人档案', command=self.handle_view_self_profile).grid(row=4, column=0, sticky='ew', pady=(8, 0), padx=(0, 6))
        ttk.Button(self_box, text='更新本人档案', command=self.handle_update_self_profile).grid(row=4, column=1, sticky='ew', pady=(8, 0))
        ttk.Button(self_box, text='查看本人病历', command=self.handle_view_self_records).grid(row=5, column=0, columnspan=2, sticky='ew', pady=(8, 0))

        ra_box = ttk.LabelFrame(self.patient_tab, text='远程认证', style='Section.TLabelframe', padding=10)
        ra_box.grid(row=0, column=1, sticky='nsew', pady=(0, 8))
        ra_box.columnconfigure(1, weight=1)
        ttk.Label(ra_box, text='对端标识').grid(row=0, column=0, sticky='w', pady=4)
        self.patient_peer = ttk.Entry(ra_box)
        self.patient_peer.insert(0, 'service_provider')
        self.patient_peer.grid(row=0, column=1, sticky='ew', pady=4)
        ttk.Button(ra_box, text='执行远程认证', command=self.handle_start_ra_patient).grid(row=1, column=0, sticky='ew', pady=(8, 0), padx=(0, 6))
        ttk.Button(ra_box, text='查看认证状态', command=self.handle_get_ra_status).grid(row=1, column=1, sticky='ew', pady=(8, 0))

        self.patient_output = self._create_scrollable_text(self.patient_tab, row=1, column=0, columnspan=2)

    def _build_system_tab(self):
        self.system_tab.columnconfigure(0, weight=1)
        info = ttk.LabelFrame(self.system_tab, text='运行信息', style='Section.TLabelframe', padding=10)
        info.grid(row=0, column=0, sticky='nsew')
        info.columnconfigure(1, weight=1)
        self.runtime_value = ttk.Label(info, text='')
        self.storage_value = ttk.Label(info, text='', wraplength=900)
        ttk.Label(info, text='Bridge目录').grid(row=0, column=0, sticky='w', pady=4)
        ttk.Label(info, text=str(self.backend.bridge_dir)).grid(row=0, column=1, sticky='w', pady=4)
        ttk.Label(info, text='运行目录').grid(row=1, column=0, sticky='w', pady=4)
        self.runtime_value.grid(row=1, column=1, sticky='w', pady=4)
        ttk.Label(info, text='密态文件').grid(row=2, column=0, sticky='w', pady=4)
        self.storage_value.grid(row=2, column=1, sticky='w', pady=4)
        ttk.Button(info, text='刷新页面', command=self.refresh_all).grid(row=3, column=0, columnspan=2, sticky='ew', pady=(10, 0))

    def _build_log_panel(self, parent):
        box = ttk.LabelFrame(parent, text='运行日志', style='Section.TLabelframe', padding=10)
        box.pack(fill='both', expand=True)
        box.rowconfigure(1, weight=1)
        box.columnconfigure(0, weight=1)
        button_row = ttk.Frame(box)
        button_row.grid(row=0, column=0, sticky='ew', pady=(0, 8))
        ttk.Button(button_row, text='刷新日志', command=self.refresh_logs).pack(side='left')
        ttk.Button(button_row, text='清空日志', command=self.clear_logs).pack(side='left', padx=6)
        self.log_text = self._create_scrollable_text(
            box,
            row=1,
            column=0,
            background='#111827',
            foreground='#F3F4F6',
        )

    def _create_scrollable_text(self, parent, row: int, column: int, columnspan: int = 1, **text_kwargs):
        container = ttk.Frame(parent)
        container.grid(row=row, column=column, columnspan=columnspan, sticky='nsew')
        container.rowconfigure(0, weight=1)
        container.columnconfigure(0, weight=1)

        text_widget = tk.Text(
            container,
            height=16,
            wrap='none',
            font=('Consolas', 10),
            **text_kwargs,
        )
        text_widget.grid(row=0, column=0, sticky='nsew')

        y_scroll = ttk.Scrollbar(container, orient='vertical', command=text_widget.yview)
        y_scroll.grid(row=0, column=1, sticky='ns')
        x_scroll = ttk.Scrollbar(container, orient='horizontal', command=text_widget.xview)
        x_scroll.grid(row=1, column=0, sticky='ew')

        text_widget.configure(yscrollcommand=y_scroll.set, xscrollcommand=x_scroll.set)
        return text_widget

    def on_close(self):
        try:
            self.backend.close()
        finally:
            self.root.destroy()

    def show_result(self, title: str, result: int, extra: str = ''):
        text = self.backend.result_text(result)
        message = f'{title}: {text}'
        if extra:
            message += f'\n{extra}'
        if result == MSD_OK:
            messagebox.showinfo('操作结果', message)
        else:
            detail = self.backend.get_last_error()
            if detail:
                message += f'\n\nBridge错误: {detail}'
            messagebox.showwarning('操作结果', message)
        self.refresh_all()

    def set_output(self, widget: tk.Text, content: str):
        widget.delete('1.0', 'end')
        widget.insert('1.0', content or '[暂无数据]')

    def refresh_logs(self):
        logs = self.backend.get_logs()
        self.log_text.delete('1.0', 'end')
        self.log_text.insert('1.0', logs or '[暂无日志]')
        self.log_text.see('end')

    def clear_logs(self):
        self.backend.clear_logs()
        self.refresh_logs()

    def refresh_all(self):
        self.role = self.backend.get_current_role()
        current_user = self.backend.get_current_username() or '未登录'
        self.session_text.set(f'当前用户: {current_user} | 角色: {self.backend.role_text(self.role)}')
        ra_label = self.backend.ra_status_text(self.ra_status_code)
        secure_text = '已建立' if self.secure_channel_ready else '未建立'
        self.ra_text.set(f'远程认证: {ra_label} | 安全会话: {secure_text}')
        self.runtime_value.config(text=self.backend.get_runtime_directory())
        self.storage_value.config(text=self.backend.get_storage_path())
        self.storage_text.set(f'密态文件: {self.backend.get_storage_path()}')
        self.status_text.set('Bridge 已连接，GUI 正在通过 SGX 后端运行')
        self.refresh_logs()
        self.update_tab_state()

    def update_tab_state(self):
        enabled = {
            MSD_ROLE_ADMIN: [self.admin_tab],
            MSD_ROLE_DOCTOR: [self.doctor_tab],
            MSD_ROLE_PATIENT: [self.patient_tab],
        }
        for tab in (self.admin_tab, self.doctor_tab, self.patient_tab):
            state = 'normal' if tab in enabled.get(self.role, []) else 'hidden'
            self.notebook.tab(tab, state=state)
        self.notebook.tab(self.system_tab, state='normal')
        if self.role == MSD_ROLE_ADMIN:
            self.notebook.select(self.admin_tab)
        elif self.role == MSD_ROLE_DOCTOR:
            self.notebook.select(self.doctor_tab)
        elif self.role == MSD_ROLE_PATIENT:
            self.notebook.select(self.patient_tab)
        else:
            self.notebook.select(self.system_tab)

    def handle_init_system(self):
        result = self.backend.init_system()
        if result == MSD_OK:
            self.ra_status_code = 0
            self.secure_channel_ready = 0
        self.show_result('初始化系统', result)

    def handle_load_system(self):
        result = self.backend.load_system()
        if result == MSD_OK:
            self.ra_status_code = 0
            self.secure_channel_ready = 0
        self.show_result('加载密态状态', result)

    def handle_flush(self):
        result = self.backend.flush_system()
        self.show_result('保存密态状态', result)

    def handle_login(self):
        username = self.login_username.get().strip()
        password = self.login_password.get().strip()
        result, role = self.backend.login(username, password)
        extra = f'登录角色: {self.backend.role_text(role)}' if result == MSD_OK else ''
        self.show_result('登录', result, extra)

    def handle_logout(self):
        result = self.backend.logout()
        if result == MSD_OK:
            self.ra_status_code = 0
            self.secure_channel_ready = 0
        self.show_result('退出登录', result)

    def handle_register_user(self, role: int):
        username = self.admin_new_user.get().strip()
        password = self.admin_new_password.get().strip()
        result = self.backend.register_user(username, password, role)
        self.show_result('注册用户', result)

    def handle_list_users(self):
        result, text = self.backend.list_users()
        self.set_output(self.admin_output, text)
        self.show_result('查看用户列表', result)

    def handle_upsert_patient(self):
        result = self.backend.upsert_patient_profile(
            self.admin_patient_entries['患者用户名'].get().strip(),
            self.admin_patient_entries['姓名'].get().strip(),
            self.admin_patient_entries['身份证号'].get().strip(),
            self.admin_patient_entries['电话'].get().strip(),
            self.admin_patient_entries['地址'].get().strip(),
        )
        self.show_result('新增/更新档案', result)

    def handle_get_patient(self):
        username = self.admin_patient_entries['患者用户名'].get().strip()
        result, text = self.backend.get_patient_profile(username)
        self.set_output(self.admin_output, text)
        self.show_result('查看单个档案', result)

    def handle_list_patients(self):
        result, text = self.backend.list_patients()
        target = self.admin_output if self.role == MSD_ROLE_ADMIN else self.doctor_output
        self.set_output(target, text)
        self.show_result('查看患者列表', result)

    def handle_delete_patient(self):
        username = self.admin_patient_entries['患者用户名'].get().strip()
        result = self.backend.delete_patient_profile(username)
        self.show_result('删除档案', result)

    def handle_start_ra(self):
        result, ra_status, secure = self.backend.start_ra(self.doctor_peer.get().strip() or 'service_provider')
        self.ra_status_code = ra_status
        self.secure_channel_ready = secure
        secure_text = '已建立' if secure else '未建立'
        extra = f'认证状态: {self.backend.ra_status_text(ra_status)}\n安全会话: {secure_text}'
        self.show_result('执行远程认证', result, extra if result == MSD_OK else '')

    def handle_start_ra_patient(self):
        result, ra_status, secure = self.backend.start_ra(self.patient_peer.get().strip() or 'service_provider')
        self.ra_status_code = ra_status
        self.secure_channel_ready = secure
        secure_text = '已建立' if secure else '未建立'
        extra = f'认证状态: {self.backend.ra_status_text(ra_status)}\n安全会话: {secure_text}'
        self.show_result('执行远程认证', result, extra if result == MSD_OK else '')

    def handle_get_ra_status(self):
        result, ra_status, secure = self.backend.get_ra_status()
        self.ra_status_code = ra_status
        self.secure_channel_ready = secure
        secure_text = '已建立' if secure else '未建立'
        extra = f'认证状态: {self.backend.ra_status_text(ra_status)}\n安全会话: {secure_text}'
        self.show_result('查看远程认证状态', result, extra if result == MSD_OK else '')

    def handle_create_record(self):
        result, record_id = self.backend.create_record(
            self.doctor_entries['患者用户名'].get().strip(),
            self.doctor_entries['诊断'].get().strip(),
            self.doctor_entries['处方'].get().strip(),
            self.doctor_entries['备注'].get().strip(),
        )
        extra = f'病历ID: {record_id}' if result == MSD_OK else ''
        self.show_result('新增病历', result, extra)

    def handle_list_records_for_target(self):
        result, text = self.backend.list_records(self.doctor_entries['患者用户名'].get().strip())
        self.set_output(self.doctor_output, text)
        self.show_result('查看病历列表', result)

    def handle_update_record(self):
        record_id = self.doctor_entries['病历ID'].get().strip() or '0'
        result = self.backend.update_record(
            int(record_id),
            self.doctor_entries['诊断'].get().strip(),
            self.doctor_entries['处方'].get().strip(),
            self.doctor_entries['备注'].get().strip(),
        )
        self.show_result('修改病历', result)

    def handle_delete_record(self):
        record_id = self.doctor_entries['病历ID'].get().strip() or '0'
        result = self.backend.delete_record(int(record_id))
        self.show_result('删除病历', result)

    def handle_view_self_profile(self):
        username = self.backend.get_current_username()
        result, text = self.backend.get_patient_profile(username)
        self.set_output(self.patient_output, text)
        self.show_result('查看本人档案', result)

    def handle_update_self_profile(self):
        username = self.backend.get_current_username()
        result = self.backend.upsert_patient_profile(
            username,
            self.patient_entries['姓名'].get().strip(),
            self.patient_entries['身份证号'].get().strip(),
            self.patient_entries['电话'].get().strip(),
            self.patient_entries['地址'].get().strip(),
        )
        self.show_result('更新本人档案', result)

    def handle_view_self_records(self):
        username = self.backend.get_current_username()
        result, text = self.backend.list_records(username)
        self.set_output(self.patient_output, text)
        self.show_result('查看本人病历', result)


if __name__ == '__main__':
    root = tk.Tk()
    try:
        app = MedicalSystemGuiApp(root)
    except BridgeError as exc:
        root.withdraw()
        messagebox.showerror('启动失败', str(exc))
        raise SystemExit(1)
    root.mainloop()
