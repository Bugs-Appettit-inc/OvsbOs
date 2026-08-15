#!/usr/bin/env python3
import argparse
import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent


def log(msg: str):
    print(f"[dev] {msg}")


def run_cmd(cmd, shell=False):
    try:
        proc = subprocess.run(cmd, shell=shell, cwd=str(ROOT), text=True, capture_output=True)
        return proc.returncode, proc.stdout.strip(), proc.stderr.strip()
    except FileNotFoundError as exc:
        return 127, "", str(exc)


def which(name):
    return shutil.which(name)


def detect_env():
    system = platform.system().lower()
    release = platform.release().lower()
    uname = platform.uname().system.lower() if hasattr(platform, "uname") else ""
    if os.environ.get("WSL_DISTRO_NAME") or "microsoft" in release or "microsoft" in uname or "wsl" in release:
        return "wsl"
    if os.name == "nt" or system.startswith("windows") or "mingw" in uname or "msys" in uname:
        return "windows"
    return "linux"


def print_manual_guide(env):
    print("\n=== Instalação manual ===")
    if env == "windows":
        print("- winget install Git.Git")
        print("- winget install GnuWin32.Make")
        print("- winget install --id Microsoft.VisualStudio.2022.BuildTools")
        print("- Instale GCC/MinGW e NASM manualmente.")
        print("- Opcional: choco install git make mingw nasm qemu")
    elif env == "wsl":
        print("- sudo apt-get update")
        print("- sudo apt-get install -y build-essential nasm git qemu-system-x86 python3-tk")
    else:
        print("- sudo apt-get update")
        print("- sudo apt-get install -y build-essential nasm git qemu-system-x86 python3-tk")


def find_qemu():
    for binary in ["qemu-system-x86_64", "qemu-system-x86_64.exe", "qemu-system-i386", "qemu-system-i386.exe"]:
        if which(binary):
            return binary
    return None


def prepare_boot_iso():
    iso_root = ROOT / "iso"
    boot_dir = iso_root / "boot"
    kernel_src = ROOT / "build" / "kernel" / "kernel.elf"
    kernel_dst = boot_dir / "kernel.elf"
    iso_output = ROOT / "build" / "ovsbos.iso"

    if kernel_src.exists():
        boot_dir.mkdir(parents=True, exist_ok=True)
        try:
            shutil.copy2(kernel_src, kernel_dst)
        except OSError:
            pass

    if not iso_root.exists():
        log("Diretorio ISO nao encontrado para boot do GRUB.")
        return 1

    if which("grub-mkrescue"):
        rc, out, err = run_cmd(["grub-mkrescue", "-o", str(iso_output), str(iso_root)])
        if rc == 0:
            log(f"ISO de boot criada em {iso_output}")
            return 0
        print(out or err)
        return rc

    log("grub-mkrescue nao encontrado. Instale grub-pc-bin para gerar a ISO.")
    return 1


def install_auto_dependencies():
    env = detect_env()
    if env == "windows":
        log("Windows detectado: tentando instalar via winget/choco, se disponível.")
        for manager in ["winget", "choco"]:
            if which(manager):
                if manager == "winget":
                    cmd = "winget install --accept-source-agreements --accept-package-agreements -e --id Git.Git --id GnuWin32.Make --id Microsoft.VisualStudio.2022.BuildTools"
                    rc, out, err = run_cmd(cmd, shell=True)
                    if rc == 0:
                        log("Dependencias Windows instaladas via winget.")
                        return 0
                    print(out or err)
                else:
                    cmd = "choco install -y git make mingw nasm qemu"
                    rc, out, err = run_cmd(cmd, shell=True)
                    if rc == 0:
                        log("Dependencias Windows instaladas via choco.")
                        return 0
                    print(out or err)
        print_manual_guide(env)
        return 1

    needed = ["git", "make", "gcc", "nasm", "python3"]
    missing = [name for name in needed if not which(name)]
    try:
        import tkinter  # noqa: F401
    except ModuleNotFoundError:
        missing.append("python3-tk")

    if not missing:
        log("Dependencias basicas ja estao presentes.")
        return 0

    if which("apt-get"):
        cmd = "sudo apt-get update && sudo apt-get install -y " + " ".join(missing)
        rc, out, err = run_cmd(cmd, shell=True)
        if rc == 0:
            log("Dependencias instaladas com sucesso.")
            if out:
                print(out)
            return 0
        print(out or err)
    elif which("dnf"):
        cmd = "sudo dnf install -y " + " ".join(missing)
        rc, out, err = run_cmd(cmd, shell=True)
        if rc == 0:
            log("Dependencias instaladas com sucesso.")
            if out:
                print(out)
            return 0
        print(out or err)
    elif which("pacman"):
        cmd = "sudo pacman -S --noconfirm " + " ".join(missing)
        rc, out, err = run_cmd(cmd, shell=True)
        if rc == 0:
            log("Dependencias instaladas com sucesso.")
            if out:
                print(out)
            return 0
        print(out or err)
    elif which("apk"):
        cmd = "sudo apk add --no-cache " + " ".join(missing)
        rc, out, err = run_cmd(cmd, shell=True)
        if rc == 0:
            log("Dependencias instaladas com sucesso.")
            if out:
                print(out)
            return 0
        print(out or err)

    print_manual_guide(env)
    return 1


def build_all():
    log(f"Ambiente detectado: {detect_env()}")
    steps = [
        ["make", "-C", str(ROOT), "kernel"],
        ["make", "-C", str(ROOT), "system"],
    ]
    for step in steps:
        rc, out, err = run_cmd(step)
        if rc != 0:
            print(out or err)
            return rc
        if out:
            print(out)

    rc = prepare_boot_iso()
    if rc != 0:
        return rc

    log("Build concluido.")
    return 0


def run_project():
    rc = build_all()
    if rc != 0:
        return rc

    qemu = find_qemu()
    if not qemu:
        log("QEMU nao encontrado. Instale qemu-system-x86 para abrir a janela de teste.")
        print("\nQEMU nao encontrado. Instale com:")
        print("  sudo apt-get install -y qemu-system-x86")
        return 1

    iso_path = ROOT / "build" / "ovsbos.iso"
    if not iso_path.exists():
        log("ISO de boot nao foi gerada.")
        return 1

    display_mode = "gtk" if os.environ.get("DISPLAY") or os.environ.get("WAYLAND_DISPLAY") else "none"
    cmd = [
        qemu,
        "-cdrom", str(iso_path),
        "-m", "512",
        "-display", display_mode,
        "-serial", "stdio",
        "-no-reboot",
        "-no-shutdown",
        "-vga", "std",
    ]
    log(f"Executando QEMU: {' '.join(cmd)}")
    return subprocess.run(cmd, cwd=str(ROOT)).returncode


def test_project():
    missing = [name for name in ["git", "make", "gcc", "nasm", "python3"] if not which(name)]
    try:
        import tkinter  # noqa: F401
    except ModuleNotFoundError:
        missing.append("python3-tk")

    if missing:
        log(f"Dependencias faltando: {', '.join(missing)}")
        return 1
    log("Teste basico OK: todas as dependencias essenciais foram encontradas.")
    return 0


def interactive_menu():
    while True:
        print("\n=== OvsbOS Dev Menu ===")
        print("1) Instalar dependencias")
        print("2) Testar ambiente")
        print("3) Compilar tudo")
        print("4) Executar build/run")
        print("5) Mostrar guia manual")
        print("6) Fazer tudo")
        print("7) Sair")
        try:
            choice = input("Escolha uma opcao [1-7]: ").strip()
        except EOFError:
            print("\nSaindo do menu.")
            return 0

        env = detect_env()

        if choice == "1":
            rc = install_auto_dependencies()
            if rc != 0:
                print("\nInstalacao automatica falhou; use a opcao 5 para manual.")
        elif choice == "2":
            rc = test_project()
            if rc != 0:
                print("\nTeste falhou. Verifique dependencias.")
        elif choice == "3":
            rc = build_all()
            if rc != 0:
                print("\nBuild falhou.")
        elif choice == "4":
            rc = run_project()
            if rc != 0:
                print("\nExecucao falhou.")
        elif choice == "5":
            print_manual_guide(env)
        elif choice == "6":
            print("Executando fluxo completo: instalar -> testar -> compilar -> abrir QEMU")
            rc = install_auto_dependencies()
            if rc == 0:
                rc = test_project()
            if rc == 0:
                rc = build_all()
            if rc == 0:
                rc = run_project()
            if rc != 0:
                print("\nFluxo completo falhou. Revise dependencias e a build.")
        elif choice == "7":
            print("Ate logo!")
            return 0
        else:
            print("Opcao invalida.")


def main():
    parser = argparse.ArgumentParser(description="Script unico de desenvolvimento para OvsbOS.")
    parser.add_argument("command", nargs="?", choices=["install", "build", "run", "test", "all", "manual", "menu"], default="menu")
    args = parser.parse_args()

    env = detect_env()

    if args.command == "menu":
        return interactive_menu()

    if args.command == "manual":
        print_manual_guide(env)
        return 0

    if args.command in {"install", "all"}:
        rc = install_auto_dependencies()
        if rc != 0:
            print("\nInstalacao automatica falhou; use: python3 dev.py manual")
            return rc

    if args.command in {"build", "all", "test"}:
        rc = build_all()
        if rc != 0:
            return rc

    if args.command in {"test", "all"}:
        rc = test_project()
        if rc != 0:
            return rc

    if args.command in {"run", "all"}:
        rc = run_project()
        if rc != 0:
            return rc

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
