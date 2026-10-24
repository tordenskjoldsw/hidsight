"""Throwaway spike: which reports arrive on the HID++ node without Hidsight
enabling anything? Reads receiver register 0x00 (notification flags) once,
then only listens and prints every report with a timestamp. Sends no write.
Run with: python3 -I listen.py /dev/hidrawN SECONDS
"""
import os
import select
import sys
import time

path = sys.argv[1] if len(sys.argv) > 1 else "/dev/hidraw3"
duration = float(sys.argv[2]) if len(sys.argv) > 2 else 600
fd = os.open(path, os.O_RDWR | os.O_NONBLOCK)


def stamp():
    return time.strftime("%H:%M:%S") + f".{int(time.time() * 1000) % 1000:03d}"


# Short register read (sub id 0x81) of register 0x00, no software id.
os.write(fd, bytes([0x10, 0xFF, 0x81, 0x00, 0, 0, 0]))
print(f"{stamp()} requested register 0x00", flush=True)
end = time.monotonic() + duration
while time.monotonic() < end:
    ready, _, _ = select.select([fd], [], [], end - time.monotonic())
    if ready:
        print(f"{stamp()} < {os.read(fd, 64).hex(' ')}", flush=True)
print(f"{stamp()} done", flush=True)
