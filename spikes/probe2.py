"""Second throwaway probe: HID++ 1.0 registers with the right encoding
(short read 0x81, long read 0x83, no software id), pings with longer
timeouts and long reports, then a passive listen for notifications."""
import os, select, sys, time

fd = os.open(sys.argv[1] if len(sys.argv) > 1 else "/dev/hidraw3", os.O_RDWR | os.O_NONBLOCK)

def drain(t=0.05):
    while True:
        r, _, _ = select.select([fd], [], [], t)
        if not r: return
        os.read(fd, 64)

def xfer(report, timeout=1.0):
    drain()
    os.write(fd, bytes(report).ljust(20 if report[0] == 0x11 else 7, b"\0"))
    out = []
    end = time.monotonic() + timeout
    while time.monotonic() < end:
        r, _, _ = select.select([fd], [], [], end - time.monotonic())
        if not r: break
        out.append(os.read(fd, 64).hex())
    return out or None

ERR = {1:"invalid subid",2:"invalid address",3:"invalid value",4:"connect fail",5:"too many devices",6:"already exists",7:"busy",8:"unknown device",9:"resource error",10:"request unavailable",11:"invalid param value",12:"wrong pin"}
def show(label, replies):
    if not replies: print(f"{label}: no answer"); return
    for h in replies:
        b = bytes.fromhex(h)
        if len(b) >= 6 and b[2] == 0x8f: print(f"{label}: error {ERR.get(b[5], b[5])} (0x{b[3]:02x}/0x{b[4]:02x})")
        else: print(f"{label}: {h}")

print("--- receiver registers, short read 0x81")
for reg in (0x00, 0x02, 0x03):
    show(f"  0x{reg:02x}", xfer([0x10, 0xff, 0x81, reg]))
print("--- receiver register 0xB5, long read 0x83")
for sub in (0x03, 0x20, 0x21, 0x22, 0x30, 0x40, 0x41, 0x50, 0x51, 0x52, 0x60, 0x61, 0x62):
    show(f"  0xB5/0x{sub:02x}", xfer([0x10, 0xff, 0x83, 0xb5, sub]))
print("--- device pings, short and long report, 2 s")
for dev in (1, 2, 3):
    show(f"  dev {dev} short", xfer([0x10, dev, 0x00, 0x1a, 0, 0, 0x5a], 2.0))
    show(f"  dev {dev} long ", xfer([0x11, dev, 0x00, 0x1a, 0, 0, 0x5a], 2.0))
print("--- passive listen 6 s (move the mouse or press a key if you are there)")
end = time.monotonic() + 6
while time.monotonic() < end:
    r, _, _ = select.select([fd], [], [], end - time.monotonic())
    if r: print("  ", os.read(fd, 64).hex())
