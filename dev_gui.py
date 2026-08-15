#!/usr/bin/env python3
import os
import shutil
import subprocess
import sys
import threading
import time
from pathlib import Path

try:
    from tkinter import END, BooleanVar, StringVar, Tk, ttk
    from tkinter.scrolledtext import ScrolledText
except ModuleNotFoundError:
    print("[gui] tkinter nao encontrado.")
    print("[gui] Instale com: sudo apt-get install -y python3-tk")
    print("[gui] ou no WSL: sudo apt-get install -y python3-tk")
    raise SystemExit(1)


class DevLauncher:
    def __init__(self, root: Tk):
        self.root = root
        self.root.title("OvsbOS Dev Launcher")
        self.root.geometry("980x620")
        self.root.minsize(820, 500)

        self.project_root = Path(__file__).resolve().parent
        self.system_name = self.detect_system()
        self.python_cmd = self.detect_python_command()
        self.status_var = StringVar(value="Pronto")
        self.auto_reload = BooleanVar(value=False)
        self.log_box = None
        self.last_scan = {}
        self.watch_thread = None
        self.build_lock = threading.Lock()

        self.build_ui()
        self.scan_files()
        self.start_watch()

    def detect_system(self):
        if os.environ.get("WSL_DISTRO_NAME"):
            return "wsl"
        if os.name == "nt" or sys.platform.startswith("win"):
            return "windows"
        return "linux"

    def detect_python_command(self):
        candidates = []
        if self.system_name == "windows":
            candidates = ["py", "python", "python3"]
        else:
            candidates = ["python3", "python", "py"]

        for candidate in candidates:
            resolved = shutil.which(candidate)
            if resolved:
                if candidate == "py":
                    return ["py", "-3"]
                return [candidate]

        return ["python3"]

    def build_ui(self):
        frame = ttk.Frame(self.root, padding=12)
        frame.pack(fill="both", expand=True)

        top = ttk.Frame(frame)
        top.pack(fill="x", pady=(0, 10))

        ttk.Label(top, text="Sistema:", font=("Segoe UI", 10, "bold")).pack(side="left")
        ttk.Label(top, text=self.system_name.upper(), foreground="#0b7cff", font=("Segoe UI", 10, "bold")).pack(side="left", padx=(4, 12))
        ttk.Label(top, text="Projeto:", font=("Segoe UI", 10, "bold")).pack(side="left")
        ttk.Label(top, text=str(self.project_root), foreground="#0b7cff").pack(side="left", padx=(8, 0))
        ttk.Checkbutton(top, text="Auto reload", variable=self.auto_reload).pack(side="right")

        actions = ttk.Frame(frame)
        actions.pack(fill="x", pady=(0, 12))

        ttk.Button(actions, text="Instalar deps", command=self.install_deps).pack(side="left", padx=(0, 8))
        ttk.Button(actions, text="Testar", command=self.run_test).pack(side="left", padx=(0, 8))
        ttk.Button(actions, text="Build", command=self.run_build).pack(side="left", padx=(0, 8))
        ttk.Button(actions, text="Run", command=self.run_project).pack(side="left", padx=(0, 8))
        ttk.Button(actions, text="Fazer tudo", command=self.do_everything).pack(side="left")

        status = ttk.Frame(frame)
        status.pack(fill="x", pady=(0, 8))
        ttk.Label(status, text="Status:").pack(side="left")
        ttk.Label(status, textvariable=self.status_var, foreground="#1f7a1f", font=("Segoe UI", 10, "bold")).pack(side="left", padx=(8, 0))

        self.log_box = ScrolledText(frame, wrap="word", height=30, font=("Consolas", 10))
        self.log_box.pack(fill="both", expand=True)
        self.log_box.insert(END, "OvsbOS Dev Launcher pronto.\n")
        self.log_box.see(END)

    def log(self, text: str):
        self.log_box.insert(END, text)
        self.log_box.see(END)
        self.root.update_idletasks()

    def set_status(self, text: str, color: str = "#1f7a1f"):
        self.status_var.set(text)
        self.root.update_idletasks()

    def run_shell(self, command: str):
        try:
            process = subprocess.run(
                command,
                shell=True,
                cwd=str(self.project_root),
                capture_output=True,
                text=True,
                timeout=600,
            )
            return process.returncode, (process.stdout or "") + (process.stderr or "")
        except Exception as exc:
            return 1, f"Erro: {exc}\n"

    def install_deps(self):
        if self.system_name == "windows":
            command = "powershell -ExecutionPolicy Bypass -File ./dependencies.ps1"
            label = "./dependencies.ps1"
        else:
            command = "bash ./dependencies.sh"
            label = "./dependencies.sh"

        self.log(f"\n>>> {label}\n")
        self.set_status("Instalando deps...")
        code, output = self.run_shell(command)
        self.log(output)
        if code == 0:
            self.set_status("Dependencias OK", "#1f7a1f")
        else:
            self.set_status("Falha nas deps", "#b42318")

    def run_python_script(self, *args):
        command = self.python_cmd + list(args)
        try:
            process = subprocess.run(
                command,
                cwd=str(self.project_root),
                capture_output=True,
                text=True,
                timeout=600,
            )
            return process.returncode, (process.stdout or "") + (process.stderr or "")
        except Exception as exc:
            return 1, f"Erro: {exc}\n"

    def run_test(self):
        self.log(f"\n>>> {' '.join(self.python_cmd + ['dev.py', 'test'])}\n")
        self.set_status("Testando...")
        code, output = self.run_python_script("dev.py", "test")
        self.log(output)
        if code == 0:
            self.set_status("Teste OK", "#1f7a1f")
        else:
            self.set_status("Teste falhou", "#b42318")

    def run_build(self):
        if not self.build_lock.acquire(blocking=False):
            self.log("[gui] build ja em andamento; ignorando nova disparada.\n")
            return
        try:
            self.log(f"\n>>> {' '.join(self.python_cmd + ['dev.py', 'build'])}\n")
            self.set_status("Buildando...")
            code, output = self.run_python_script("dev.py", "build")
            self.log(output)
            if code == 0:
                self.set_status("Build OK", "#1f7a1f")
            else:
                self.set_status("Build falhou", "#b42318")
        finally:
            self.build_lock.release()

    def run_project(self):
        self.log(f"\n>>> {' '.join(self.python_cmd + ['dev.py', 'run'])}\n")
        self.set_status("Executando QEMU...")
        code, output = self.run_python_script("dev.py", "run")
        self.log(output)
        if code == 0:
            self.set_status("Exec OK", "#1f7a1f")
        else:
            self.set_status("Exec falhou", "#b42318")

    def do_everything(self):
        self.log("\n>>> Fluxo completo: instalar + testar + build + executar\n")
        self.set_status("Executando tudo...")
        for action in (self.install_deps, self.run_test, self.run_build, self.run_project):
            action()
            if self.status_var.get() in {"Teste falhou", "Build falhou", "Exec falhou", "Falha nas deps"}:
                self.set_status("Fluxo interrompido", "#b42318")
                return
        self.set_status("Tudo OK", "#1f7a1f")

    def scan_files(self):
        self.last_scan = {}
        watch_roots = [
            self.project_root / "kernel",
            self.project_root / "system",
            self.project_root / "tests",
            self.project_root / "tools",
        ]
        for folder in watch_roots:
            if not folder.exists():
                continue
            for file in folder.rglob("*"):
                if file.is_file() and not file.name.startswith(".") and "build" not in file.parts and ".git" not in file.parts:
                    try:
                        self.last_scan[str(file)] = file.stat().st_mtime_ns
                    except OSError:
                        pass

    def has_changed(self):
        current = {}
        watch_roots = [
            self.project_root / "kernel",
            self.project_root / "system",
            self.project_root / "tests",
            self.project_root / "tools",
        ]
        for folder in watch_roots:
            if not folder.exists():
                continue
            for file in folder.rglob("*"):
                if file.is_file() and not file.name.startswith(".") and "build" not in file.parts and ".git" not in file.parts:
                    try:
                        current[str(file)] = file.stat().st_mtime_ns
                    except OSError:
                        pass
        changed = current != self.last_scan
        if changed:
            self.last_scan = current
        return changed

    def start_watch(self):
        def loop():
            while True:
                if self.auto_reload.get() and self.has_changed():
                    if self.build_lock.acquire(blocking=False):
                        try:
                            self.root.after(0, self.run_build)
                        finally:
                            self.build_lock.release()
                time.sleep(1.0)

        self.watch_thread = threading.Thread(target=loop, daemon=True)
        self.watch_thread.start()


def main():
    root = Tk()
    DevLauncher(root)
    root.mainloop()


if __name__ == "__main__":
    main()
