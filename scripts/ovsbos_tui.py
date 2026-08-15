#!/usr/bin/env python3
import argparse
import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path

ZIG_VERSION = "0.16.0"


def log(msg: str) -> None:
    print(f"[ovsbos] {msg}")


def detect_environment() -> str:
    system = platform.system().lower()
    release = platform.release().lower()
    uname = platform.uname().system.lower() if hasattr(platform, "uname") else ""

    if os.environ.get("WSL_DISTRO_NAME") or "microsoft" in release or "microsoft" in uname or "wsl" in release:
        return "wsl"
    if os.name == "nt" or system.startswith("windows") or "mingw" in uname or "msys" in uname:
        return "windows"
    return "linux"


def which(name: str):
    return shutil.which(name)


def run_cmd(cmd, shell=False, capture=True):
    try:
        result = subprocess.run(
            cmd,
            shell=shell,
            capture_output=capture,
            text=True,
        )
        return result.returncode, result.stdout.strip(), result.stderr.strip()
    except FileNotFoundError as exc:
        return 127, "", str(exc)


def get_zig_version(path: str | None):
    if not path:
        return None
    code, out, _ = run_cmd([path, "version"])
    if code == 0 and out:
        return out.split()[0].strip()
    return None


def arch_name() -> str:
    machine = platform.machine().lower()
    if machine in {"x86_64", "amd64"}:
        return "x86_64"
    if machine in {"aarch64", "arm64"}:
        return "aarch64"
    if machine.startswith("arm"):
        return "armv7a"
    return machine or "x86_64"


def zig_official_urls(version: str, env: str):
    arch = arch_name()
    if env == "windows":
        return [
            f"https://ziglang.org/download/{version}/zig-windows-{arch}-{version}.zip",
            f"https://github.com/ziglang/zig/releases/download/{version}/zig-windows-{arch}-{version}.zip",
        ]
    return [
        f"https://ziglang.org/download/{version}/zig-linux-{arch}-{version}.tar.xz",
        f"https://github.com/ziglang/zig/releases/download/{version}/zig-linux-{arch}-{version}.tar.xz",
    ]


def ensure_dir(path: Path):
    path.mkdir(parents=True, exist_ok=True)


def download_file(url: str, target: Path) -> bool:
    curl = which("curl")
    wget = which("wget")

    if curl:
        code, _, err = run_cmd([curl, "-L", "-f", "--retry", "3", "--output", str(target), url])
        if code == 0:
            return True
        if err:
            log(f"curl falhou para {url}: {err}")

    if wget:
        code, _, err = run_cmd([wget, "-O", str(target), url])
        if code == 0:
            return True
        if err:
            log(f"wget falhou para {url}: {err}")

    return False


def install_zig_exact(version: str, env: str) -> bool:
    current = which("zig")
    current_version = get_zig_version(current) if current else None
    if current_version == version:
        print(f"[OK] Zig {version} ja esta instalado e funcionando.")
        return True

    local_root = Path.home() / ".local" / "share" / f"zig-{version}"
    ensure_dir(local_root)

    archive_name = "zig.zip" if env == "windows" else "zig.tar.xz"
    archive_path = local_root / archive_name

    log(f"Preparando instalacao do Zig {version} para ambiente {env}")
    for url in zig_official_urls(version, env):
        log(f"Tentando: {url}")
        if download_file(url, archive_path):
            break
    else:
        log(f"Nao foi possivel baixar Zig {version} com os URLs oficiais.")
        print("Sugestao: instale manualmente em seu ambiente ou use o pacote do sistema.")
        return False

    if env == "windows":
        code, _, err = run_cmd([
            "powershell",
            "-NoProfile",
            "-ExecutionPolicy",
            "Bypass",
            "-Command",
            f"Expand-Archive -LiteralPath '{archive_path}' -DestinationPath '{local_root}' -Force",
        ])
        if code != 0:
            log(f"Falha ao extrair o pacote do Zig: {err}")
            return False
        candidates = [
            local_root / "zig-windows-x86_64" / "zig.exe",
            local_root / "zig" / "zig.exe",
        ]
        zig_executable = next((p for p in candidates if p.exists()), None)
    else:
        code, _, err = run_cmd(["tar", "-xf", str(archive_path), "-C", str(local_root)])
        if code != 0:
            log(f"Falha ao extrair o pacote do Zig: {err}")
            return False
        candidates = [
            local_root / "zig-linux-x86_64" / "zig",
            local_root / "zig-linux-aarch64" / "zig",
        ]
        zig_executable = next((p for p in candidates if p.exists()), None)
        if zig_executable is None:
            matches = list(local_root.glob("*/zig"))
            zig_executable = matches[0] if matches else None

    if zig_executable is None or not zig_executable.exists():
        log("Nao foi possivel localizar o executavel do Zig apos a extracao.")
        return False

    bin_dir = Path.home() / ".local" / "bin"
    ensure_dir(bin_dir)
    target = bin_dir / ("zig.exe" if env == "windows" else "zig")

    if target.exists():
        target.unlink()

    try:
        os.link(zig_executable, target)
    except OSError:
        try:
            shutil.copy2(str(zig_executable), str(target))
        except Exception as exc:
            log(f"Falha ao instalar o Zig em PATH: {exc}")
            return False

    shell_profile = Path.home() / ".bashrc"
    export_line = 'export PATH="$HOME/.local/bin:$PATH"'
    if not shell_profile.exists() or export_line not in shell_profile.read_text(errors="ignore"):
        with shell_profile.open("a", encoding="utf-8") as fh:
            fh.write(f"\n{export_line}\n")

    print(f"[OK] Zig {version} foi instalado em {target}")
    print("ATENCAO: reinicie o terminal ou execute: export PATH=$HOME/.local/bin:$PATH")
    if which("zig"):
        print(f"[INFO] Versao em PATH: {get_zig_version(which('zig'))}")
    return True


def install_system_packages(env: str) -> bool:
    print("\n[setup] Verificando gerenciadores de pacotes...\n")

    managers = []
    if env == "windows":
        if which("winget"):
            managers.append("winget")
        if which("choco"):
            managers.append("choco")
    else:
        if which("apt-get"):
            managers.append("apt-get")
        if which("dnf"):
            managers.append("dnf")
        if which("pacman"):
            managers.append("pacman")
        if which("apk"):
            managers.append("apk")
        if which("brew"):
            managers.append("brew")

    if not managers:
        log("Nenhum gerenciador de pacotes foi encontrado automaticamente.")
        print("Instale as ferramentas manualmente no seu sistema antes de continuar.")
        return False

    pkgs = ["git", "make", "gcc", "clang", "nasm", "qemu-system-x86_64"]

    for manager in managers:
        if manager == "apt-get":
            cmd = "sudo apt-get update && sudo apt-get install -y " + " ".join(pkgs)
            rc, out, err = run_cmd(cmd, shell=True)
            if rc == 0:
                print("[OK] Dependencias do Linux instaladas com sucesso.")
                return True
            print(out or err)
        elif manager == "dnf":
            cmd = "sudo dnf install -y " + " ".join(pkgs)
            rc, out, err = run_cmd(cmd, shell=True)
            if rc == 0:
                print("[OK] Dependencias do Linux instaladas com sucesso.")
                return True
            print(out or err)
        elif manager == "pacman":
            cmd = "sudo pacman -S --noconfirm " + " ".join(pkgs)
            rc, out, err = run_cmd(cmd, shell=True)
            if rc == 0:
                print("[OK] Dependencias do Linux instaladas com sucesso.")
                return True
            print(out or err)
        elif manager == "apk":
            cmd = "sudo apk add --no-cache " + " ".join(pkgs)
            rc, out, err = run_cmd(cmd, shell=True)
            if rc == 0:
                print("[OK] Dependencias do Linux instaladas com sucesso.")
                return True
            print(out or err)
        elif manager == "brew":
            cmd = "brew install " + " ".join(pkgs)
            rc, out, err = run_cmd(cmd, shell=True)
            if rc == 0:
                print("[OK] Dependencias do macOS instaladas com sucesso.")
                return True
            print(out or err)
        elif manager == "winget":
            items = ["Git.Git", "GnuWin32.Make", "Microsoft.VisualStudio.2022.BuildTools"]
            cmd = "winget install --accept-source-agreements --accept-package-agreements -e --id " + " --id ".join(items)
            rc, out, err = run_cmd(cmd, shell=True)
            if rc == 0:
                print("[OK] Dependencias do Windows instaladas com sucesso.")
                return True
            print(out or err)
        elif manager == "choco":
            cmd = "choco install -y git make mingw qemu"
            rc, out, err = run_cmd(cmd, shell=True)
            if rc == 0:
                print("[OK] Dependencias do Windows instaladas com sucesso.")
                return True
            print(out or err)

    return False


def check_dependencies() -> bool:
    required = {
        "git": "Instale Git para clonar e controlar o codigo.",
        "make": "Instale make ou o pacote de build do sistema.",
        "gcc": "Instale GCC/Clang para compilar o kernel e componentes.",
        "nasm": "Instale NASM para montar o assembly.",
        "zig": "Instale o Zig 0.16.0 via instalador oficial ou via gerenciador do sistema.",
    }

    print("\n=== Verificacao de dependencias ===")
    all_ok = True
    for name, hint in required.items():
        found = which(name)
        if name == "zig":
            version = get_zig_version(found) if found else None
            if version == ZIG_VERSION:
                print(f"[OK] {name}: {version}")
                continue
            if found:
                print(f"[WARN] {name}: encontrado ({version}) mas o esperado e {ZIG_VERSION}")
            else:
                print(f"[WARN] {name}: ausente. {hint}")
            all_ok = False
            continue

        if found:
            print(f"[OK] {name}: {found}")
        else:
            print(f"[WARN] {name}: ausente. {hint}")
            all_ok = False

    if not all_ok:
        print("\nAlgumas dependencias ainda nao estao prontas. Use a opcao de instalacao no menu.")
    else:
        print("\nTudo pronto para compilar e executar o projeto.")
    return all_ok


def build_project() -> bool:
    root = Path(os.environ.get("OVSBOS_ROOT", ".")).resolve()
    print(f"\n=== Build do projeto ===")
    print(f"Diretorio: {root}")
    rc, out, err = run_cmd(["make", "-C", str(root), "all"], shell=False)
    if rc != 0:
        print(out or err)
        return False
    print(out)
    return True


def run_project() -> bool:
    root = Path(os.environ.get("OVSBOS_ROOT", ".")).resolve()
    print(f"\n=== Execucao do projeto ===")
    print(f"Diretorio: {root}")
    rc, out, err = run_cmd(["make", "-C", str(root), "run"], shell=False)
    if rc != 0:
        print(out or err)
        return False
    print(out)
    return True


def choose_environment() -> str:
    env = detect_environment()
    print("\nAmbiente detectado:", env)
    print("1) Auto (detectar)")
    print("2) WSL")
    print("3) Linux nativo")
    print("4) Windows nativo")
    try:
        choice = input("Escolha a opcao [1-4]: ").strip()
    except EOFError:
        return env

    mapping = {"1": env, "2": "wsl", "3": "linux", "4": "windows"}
    return mapping.get(choice, env)


def menu_loop() -> None:
    env = choose_environment()
    while True:
        print("\n" + "=" * 72)
        print(" OvsbOS Automation Console ")
        print("=" * 72)
        print("1) Detectar ambiente")
        print("2) Verificar dependencias")
        print("3) Instalar Zig 0.16.0")
        print("4) Instalar dependencias do sistema")
        print("5) Compilar projeto")
        print("6) Executar projeto")
        print("7) Sair")
        print("=" * 72)
        try:
            option = input("Selecione uma opcao [1-7]: ").strip()
        except EOFError:
            print("\nSaindo do menu.")
            return

        if option == "1":
            env = choose_environment()
        elif option == "2":
            check_dependencies()
        elif option == "3":
            install_zig_exact(ZIG_VERSION, env)
        elif option == "4":
            ok = install_system_packages(env)
            if ok:
                print("[OK] Processo de instalacao concluido.")
            else:
                print("[WARN] A instalacao automatica nao foi possivel neste ambiente.")
        elif option == "5":
            build_project()
        elif option == "6":
            run_project()
        elif option == "7":
            print("Até logo!")
            return
        else:
            print("Opcao invalida. Tente novamente.")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Automacao para OvsbOS com suporte a WSL, Linux e Windows."
    )
    parser.add_argument("command", nargs="?", choices=["setup", "run", "check", "menu"], default="menu")
    args = parser.parse_args()

    env = detect_environment()
    if args.command == "check":
        return 0 if check_dependencies() else 1

    if args.command == "setup":
        print("\n=== Setup do OvsbOS ===")
        print(f"Ambiente detectado: {env}")
        install_zig_exact(ZIG_VERSION, env)
        install_system_packages(env)
        check_dependencies()
        return 0

    if args.command == "run":
        print("\n=== Execucao do OvsbOS ===")
        return 0 if run_project() else 1

    menu_loop()
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except KeyboardInterrupt:
        print("\nOperacao interrompida pelo usuario.")
        raise SystemExit(130)
