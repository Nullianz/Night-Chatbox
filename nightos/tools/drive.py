#!/usr/bin/env python3
"""Boot NightOS in QEMU headless, type commands over serial, check output, optionally take screenshots."""
import os, socket, subprocess, sys, time, tempfile

ELF = sys.argv[1] if len(sys.argv) > 1 else "build/nightos.elf"
SHOTS = os.environ.get("SHOTS")  # directory for screenshots, optional
mon_path = os.path.join(tempfile.mkdtemp(), "mon.sock")
q = subprocess.Popen(
    ["qemu-system-i386", "-m", "128", "-kernel", ELF, "-display", "none", "-no-reboot",
     "-serial", "stdio", "-monitor", f"unix:{mon_path},server,nowait"],
    stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
os.set_blocking(q.stdout.fileno(), False)
log = b""

pos = 0
def read_until(token, timeout=15):
    """Return output from the read cursor up to and including the next `token`."""
    global log, pos
    end = time.time() + timeout
    while True:
        i = log.find(token.encode(), pos)
        if i >= 0:
            out = log[pos:i + len(token)].decode(errors="replace")
            pos = i + len(token)
            return out
        if time.time() > end or q.poll() is not None:
            raise SystemExit(f"TIMEOUT waiting for {token!r}. Output so far:\n{log[pos:].decode(errors='replace')}")
        try:
            chunk = q.stdout.read()
        except BlockingIOError:
            chunk = None
        if chunk: log += chunk
        else: time.sleep(0.05)

def send(text):
    for ch in text:  # type slowly enough for the 16-byte UART FIFO
        q.stdin.write(ch.encode()); q.stdin.flush(); time.sleep(0.01)

def shot(name):
    if not SHOTS: return
    os.makedirs(SHOTS, exist_ok=True)
    s = socket.socket(socket.AF_UNIX); s.connect(mon_path)
    time.sleep(0.2); s.recv(4096)
    path = os.path.abspath(os.path.join(SHOTS, name + ".ppm"))
    s.sendall(f"screendump {path}\n".encode()); time.sleep(0.6); s.close()
    from PIL import Image
    Image.open(path).save(path[:-4] + ".png"); os.remove(path)

PROMPT = "~> "
fails = 0
def check(cmd, *expect, wait=PROMPT, feed=None):
    global fails
    send(cmd + "\r")
    out = "" if feed else read_until(wait)
    if feed:
        for f_wait, f_text in feed:
            if f_wait: out += read_until(f_wait)
            send(f_text)
        out += read_until(PROMPT)
    missing = [e for e in expect if e not in out]
    status = "PASS" if not missing else "FAIL"
    if missing: fails += 1
    print(f"[{status}] {cmd!r}" + (f"  missing: {missing}" if missing else ""))
    return out

read_until("Type help")
read_until(PROMPT)
shot("01-boot")
check("ver", "NightOS 0.1")
check("dir", "hello.exe", "guess.exe", "readme.txt")
check("hello world", "Hello from a Windows .exe running on NightOS!", "Command line: hello world", "Loaded at address 0x00400000")
shot("02-hello")
check("info hello", "32-bit Windows console program", "uses kernel32.dll", "functions supported, 0 missing")
check("sysinfo", "Computer : NIGHTBOX", "100 alloc/free cycles passed", "Sleep    : asked for 250 ms")
check("notes", "Saved. notes.txt is now", "first line", "second line",
      feed=[("note> ", "first line\r"), ("note> ", "second line\r"), ("note> ", "\r")])
check("type notes.txt", "first line\nsecond line".replace("\n", "\r\n"))
check("dir", "notes.txt")
# guess: binary search to win
send("guess\r"); out = read_until("Your guess")
lo, hi, won = 1, 100, False
for _ in range(10):
    g = (lo + hi) // 2
    send(f"{g}\r")
    out = read_until("\n", 5); out += read_until("\n", 5)
    if "Correct" in out: won = True; break
    if "Higher" in out: lo = g + 1
    else: hi = g - 1
    read_until("Your guess")
read_until(PROMPT)
print(f"[{'PASS' if won else 'FAIL'}] guess game won by binary search"); fails += not won
# message box: answer Yes, then dismiss second box
send("msgbox\r"); read_until("[MessageBox] Hello!")
time.sleep(0.3); shot("03-msgbox")
send("y"); read_until("[MessageBox] Your answer: Thanks!")
send("\r"); out = read_until(PROMPT)
ok = "You clicked Yes." in out
print(f"[{'PASS' if ok else 'FAIL'}] msgbox (MessageBoxA Yes/No)"); fails += not ok
check("crash", "The program stopped working: Divide by zero", "Your system is fine")
shot("04-crash")
check("threads", "needs 'kernel32.dll!CreateThread'")
check("hello again", "Command line: hello again")  # OS still healthy after crash + missing import
check("mem", "heap:")
check("nonsense", "is not a command or program")
send("shutdown\r")
try:
    q.wait(timeout=10); print("[PASS] shutdown powered off QEMU")
except subprocess.TimeoutExpired:
    print("[FAIL] shutdown did not power off"); fails += 1; q.kill()
open(os.environ.get("LOG", os.devnull), "wb").write(log)
print("ALL TESTS PASSED" if not fails else f"{fails} FAILED")
sys.exit(1 if fails else 0)
