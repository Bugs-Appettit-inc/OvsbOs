#!/bin/bash
# gen-menu-cache.sh — gera menu_cache.h: tabela estática dos apps .oep.
# Uso: gen-menu-cache.sh <disk.img> <saida.h>
#
# Por quê: o scan em runtime (fork + getdents + open/read/close em FAT32)
# era o churn que corrompia frames de usuário (+0x68) e resetava o guest
# (diagnóstico diag-visual 2026-10-16). O cache é o MESMO resultado do
# scan, só que congelado no build — runtime faz 0 getdents/0 open de scan.
#
# Regenere quando apps .oep forem adicionados/removidos do disk.img
# (a regra do Makefile já depende de ../disk.img).
set -eu

DISK="$1"
OUT="$2"
DIR="$(cd "$(dirname "$0")" && pwd)"
OEP_INFO="$DIR/../../../userspace/tools/oep-pack/oep-info"

tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

for d in BIN APPS; do
    mkdir -p "$tmp/$d"
    mcopy -o -i "$DISK" "::$d/*.oep" "$tmp/$d/" 2>/dev/null || true
done

{
    echo "/* gerado por gen-menu-cache.sh a partir de $DISK — NAO EDITAR */"
    echo "/* dirs escaneados: /BIN /APPS (so *.oep) */"
    echo "static const struct { const char *path; const char *name; const char *cat; }"
    echo "menu_cache[] = {"
    for d in BIN APPS; do
        for f in "$tmp/$d"/*.oep "$tmp/$d"/*.OEP; do
            [ -f "$f" ] || continue
            base="$(basename "$f")"
            # nomes minusculos: o readdir do kernel emite 8.3 em minusculas e
            # o lookup FAT e case-insensitive — o caminho resolve igual.
            low="$(printf '%s' "$base" | tr 'A-Z' 'a-z')"
            info="$("$OEP_INFO" "$f")"
            name="$(printf '%s\n' "$info" | sed -n 's/^nome: *//p' | head -1)"
            cat="$(printf '%s\n' "$info" | sed -n 's/^categoria: *//p' | head -1)"
            [ -n "$name" ] || name="$low"
            printf '    { "/%s/%s", "%s", "%s" },\n' "$d" "$low" "$name" "$cat"
        done
    done
    echo "};"
    echo "#define MENU_CACHE_N ((int)(sizeof(menu_cache)/sizeof(menu_cache[0])))"
} > "$OUT"

echo "menu_cache: $(grep -c '^    { ' "$OUT") apps -> $OUT"
