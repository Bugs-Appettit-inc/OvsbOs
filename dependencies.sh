#!/usr/bin/env bash
set -euo pipefail

OS="$(uname -s 2>/dev/null || echo Linux)"
ENV="linux"

if [ -n "${WSL_DISTRO_NAME:-}" ]; then
  ENV="wsl"
elif [[ "$OS" == *[Ww]indows* ]] || [[ "$OS" == MINGW* ]] || [[ "$OS" == MSYS* ]]; then
  ENV="windows"
fi

print_manual() {
  echo "=== Instalação manual ==="
  case "$ENV" in
    windows)
      echo "winget install Git.Git"
      echo "winget install GnuWin32.Make"
      echo "winget install --id Microsoft.VisualStudio.2022.BuildTools"
      echo "choco install -y mingw nasm qemu"
      ;;
    wsl|linux)
      echo "sudo apt-get update"
      echo "sudo apt-get install -y build-essential git nasm qemu-system-x86 python3-tk"
      ;;
  esac
}

check_cmd() {
  command -v "$1" >/dev/null 2>&1
}

install_linux() {
  echo "[deps] Detectado ambiente: $ENV"
  missing=()
  for cmd in git make gcc nasm python3; do
    if ! check_cmd "$cmd"; then
      missing+=("$cmd")
    fi
  done

  if ! python3 - <<'PY'
import tkinter
PY
  then
    missing+=("python3-tk")
  fi

  if [ ${#missing[@]} -eq 0 ]; then
    echo "[deps] Todas as dependências básicas já estão instaladas."
    return 0
  fi

  if check_cmd apt-get; then
    echo "[deps] Instalando: ${missing[*]}"
    sudo apt-get update
    sudo apt-get install -y "${missing[@]}"
    return 0
  fi

  if check_cmd dnf; then
    echo "[deps] Instalando: ${missing[*]}"
    sudo dnf install -y "${missing[@]}"
    return 0
  fi

  if check_cmd pacman; then
    echo "[deps] Instalando: ${missing[*]}"
    sudo pacman -S --noconfirm "${missing[@]}"
    return 0
  fi

  if check_cmd apk; then
    echo "[deps] Instalando: ${missing[*]}"
    sudo apk add --no-cache "${missing[@]}"
    return 0
  fi

  echo "[deps] Nenhum gerenciador suportado foi encontrado."
  print_manual
  return 1
}

install_windows() {
  echo "[deps] Detectado ambiente: windows"
  if check_cmd winget; then
    echo "[deps] Instalando ferramentas via winget..."
    winget install --accept-source-agreements --accept-package-agreements -e --id Git.Git --id GnuWin32.Make --id Microsoft.VisualStudio.2022.BuildTools || true
    return 0
  fi

  if check_cmd choco; then
    echo "[deps] Instalando ferramentas via choco..."
    choco install -y git make mingw nasm qemu || true
    return 0
  fi

  echo "[deps] Nenhum instalador suportado encontrado no Windows."
  print_manual
  return 1
}

case "$ENV" in
  windows)
    install_windows
    ;;
  *)
    install_linux
    ;;
 esac
