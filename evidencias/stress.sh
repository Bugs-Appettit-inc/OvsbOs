#!/bin/bash
# stress.sh — teste de churn dos apps base (+ modo guest opcional).
#
# Host (padrao): exercita fm/editor/paint/shell em loop com churn de
# memoria em paralelo; qualquer rc!=0 vira evidencias/crashes/.
#   STRESS_N=1000 ./evidencias/stress.sh   # loop longo
#   STRESS_N=5 ./evidencias/stress.sh      # fumaca rapida
# Guest (manual): exige QEMU + ISO; ver PROCEDIMENTO-GUEST no final.
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
APPS="$ROOT/userspace/apps"
CRASHDIR="$ROOT/evidencias/crashes"
N="${STRESS_N:-50}"
TMPD="$(mktemp -d /tmp/ovsb-stress.XXXXXX)"
CHURN_PIDS=""
cleanup() {
  for p in $CHURN_PIDS; do kill "$p" 2>/dev/null; done
  rm -rf "$TMPD"
}
trap cleanup EXIT

fails=0
log_crash() { # $1=app $2=rc $3=detalhe
  fails=$((fails+1))
  ts="$(date +%Y%m%d-%H%M%S)"
  { echo "app=$1 rc=$2 ts=$ts"; echo "detail=$3"; } \
    > "$CRASHDIR/crash-$ts-$1.log"
  echo "[CRASH] $1 rc=$2 (salvo)"
}

churn_mem() {
  python3 -c '
a=[]
for i in range(20000):
    a.append(bytearray((i%97+1)*1024))
    if len(a)>40: a=a[10:]
    if i%500==0: a=[]
' 2>/dev/null
}

echo "[stress] N=$N apps=$APPS"
( while :; do churn_mem; done ) &
CHURN_PIDS="$CHURN_PIDS $!"
( while :; do churn_mem; done ) &
CHURN_PIDS="$CHURN_PIDS $!"

i=0
while [ "$i" -lt "$N" ]; do
  i=$((i+1))
  printf 'q\n' | timeout 20 "$APPS/fm/fm" /tmp >/dev/null 2>&1 \
    || log_crash fm $? "iter $i list /tmp"
  printf 'q\n' | timeout 20 "$APPS/fm/fm" / >/dev/null 2>&1 \
    || log_crash fm $? "iter $i list /"
  python3 -c "print('Lorem ipsum dolor sit amet. '*500)" > "$TMPD/big.txt"
  printf '\x13\x11' | timeout 20 "$APPS/editor/editor" "$TMPD/big.txt" \
    >/dev/null 2>&1 || log_crash editor $? "iter $i edit 10KB"
  cmd="$APPS/paint/paint --cli new 96 64 0"
  for s in $(seq 1 25); do
    cmd="$cmd line 0 0 95 63 15 rect $s $s $((95-s)) $((63-s)) 12"
    cmd="$cmd ell $s 0 $((95-s)) 63 11 fill 48 32 14"
  done
  cmd="$cmd save $TMPD/p.ppm"
  timeout 30 $cmd >/dev/null 2>&1 || log_crash paint $? "iter $i 100 formas"
  printf 'echo t\necho a | tr a-z A-Z\nexit\n' \
    | timeout 20 "$APPS/shell/shell" >/dev/null 2>&1 \
    || log_crash shell $? "iter $i shell"
  [ $((i % 10)) -eq 0 ] && echo "[stress] $i/$N fails=$fails"
done

echo "[stress] fim: $N iters, fails=$fails"
exit $fails

# PROCEDIMENTO-GUEST (QEMU interativo, ~2h):
#  1. make run  (ou: qemu-system-x86_64 -enable-kvm -cdrom OvsbOS.iso \
#       -m 512M -serial stdio -drive file=disk.img,format=raw,index=0)
#  2. No console: exec /BIN/FM  (abre/fecha 1000x via script do guest,
#     ou manualmente: INICIAR -> clicar nos apps repetidamente)
#  3. Em outro terminal: tail -f /tmp/ovsbos.log | grep -E "PF@|PF-KILL|RIP="
#  4. Cada crash: salvar bloco PF@...ate [PROC] exit em
#     evidencias/crashes/crash-<timestamp>.log
#  5. Experimento 4.1 (int3 antes do iretq): aplicar
#     evidencias/iretq-probe.diff, rebuild, repetir; se o RIP do crash
#     mudar para o endereco do int3 => culpa ANTES do iretq.
