#!/usr/bin/env python3
import os, sys
def daemon(args, errlog):
    p = os.fork()
    if p > 0:
        os.waitpid(p, 0)
        return
    os.setsid()
    p2 = os.fork()
    if p2 > 0:
        os._exit(0)
    os.chdir("/home/gabicnt/OvsbOS")
    si = os.open("/dev/null", os.O_RDONLY)
    so = os.open(errlog, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o644)
    os.dup2(si, 0); os.dup2(so, 1); os.dup2(so, 2)
    os.execv("/home/gabicnt/OvsbOS/scripts/qemu-wrap.sh", ["wrap"] + args)
if __name__ == "__main__":
    import time
    which = sys.argv[1] if len(sys.argv) > 1 else "gl"
    base = ["-enable-kvm", "-boot", "order=d", "-cdrom", "OvsbOS.iso", "-m", "512M",
            "-drive", "file=disk.img,format=raw,index=0,cache=none",
            "-serial", "file:/tmp/vga.log", "-rtc", "base=localtime",
            "-monitor", "unix:/tmp/qmon,server,nowait"]
    if which == "gl":
        base += ["-device", "virtio-vga-gl", "-display", "sdl,gl=on"]
    elif which == "virtio":
        base += ["-device", "virtio-vga", "-display", "sdl"]
    else:
        base += ["-display", "sdl"]
    daemon(base, "/tmp/qemu-err.log")
    time.sleep(1)
