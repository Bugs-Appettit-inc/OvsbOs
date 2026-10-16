#!/bin/bash
# Wrapper: registra exit code do QEMU (0/1=guest saiu; 137=morto de fora).
/home/gabicnt/bin/qemu-system-x86_64 "$@"
echo "QEMU_RC=$? $(date +%H:%M:%S)" >> /tmp/qemu-rc.log
