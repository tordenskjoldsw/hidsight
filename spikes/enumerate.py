"""Throwaway spike: enumerate the devices on a Logitech receiver over HID++.

Reads, never writes settings. Protocol from Logitech's public HID++ 2.0
documents: short report 0x10 (7 bytes), long report 0x11 (20 bytes),
layout [report id, device index, feature index, function << 4 | sw id,
params...]. Error replies carry feature index 0xFF (HID++ 2.0) or sub id
0x8F (HID++ 1.0). Run with: python3 -I enumerate.py /dev/hidrawN
"""
import os
import select
import sys

SW_ID = 0x0A
RECEIVER = 0xFF
TIMEOUT = 2.0

ROOT = 0x0000
FEATURE_SET = 0x0001
DEVICE_FW = 0x0003
DEVICE_NAME = 0x0005
BATTERY_STATUS = 0x1000
BATTERY_VOLTAGE = 0x1001
UNIFIED_BATTERY = 0x1004
ADJUSTABLE_DPI = 0x2201
REPORT_RATE = 0x8060
EXTENDED_REPORT_RATE = 0x8061
KNOWN = {
    0x0000: "IRoot", 0x0001: "IFeatureSet", 0x0003: "DeviceFwVersion",
    0x0005: "DeviceName", 0x0007: "DeviceFriendlyName", 0x0020: "ConfigChange",
    0x0021: "UniqueIdentifier", 0x1000: "BatteryStatus", 0x1001: "BatteryVoltage",
    0x1004: "UnifiedBattery", 0x1814: "ChangeHost", 0x1815: "HostsInfo",
    0x1b04: "ReprogControlsV4", 0x1d4b: "WirelessDeviceStatus",
    0x1e00: "EnableHiddenFeatures", 0x2100: "VerticalScrolling",
    0x2110: "SmartShift", 0x2111: "SmartShiftEnhanced", 0x2121: "HiResWheel",
    0x2150: "Thumbwheel", 0x2201: "AdjustableDpi", 0x2250: "ExtendedAdjustableDpi",
    0x40a3: "FnInversion", 0x4521: "DisableKeys", 0x4531: "MultiPlatform",
    0x4540: "KeyboardLayout2", 0x8060: "ReportRate", 0x8061: "ExtendedReportRate",
    0x8100: "OnboardProfiles", 0x1982: "Backlight2", 0x1990: "Illumination",
    0x00c2: "DfuControl", 0x00c3: "SecureDfuControl", 0x1802: "DeviceReset",
    0x1803: "GpioAccess", 0x1805: "OobState", 0x1806: "ConfigurableDeviceProperties",
    0x1811: "EquadPairingEnc", 0x1830: "PowerModes", 0x18a1: "LedControl",
    0x1861: "BatteryCalibration", 0x1890: "RfTest", 0x18b0: "DeviceInformation",
    0x1a20: "ALSCalibration", 0x1b05: "FullKeyCustomization", 0x1e22: "SpiDirectAccess",
    0x1eb0: "TdeAccessToNvm", 0x1f1f: "FirmwareProperties", 0x1f20: "AdcMeasurement",
    0x2110: "SmartShift", 0x2130: "Ratchet", 0x2202: "AnglesSnapping",
    0x2205: "PointerMotion", 0x2230: "SensorInfo", 0x2240: "SurfaceTuning",
    0x1a00: "PresenterControl", 0x1a01: "3DSensor", 0x1b00: "ReprogControls",
    0x1bc0: "ReportHidUsage", 0x1c00: "PersistentRemappableAction",
    0x1d4b: "WirelessDeviceStatus", 0x1df0: "RemainingPairings", 0x1e20: "SpiDirectAccess",
    0x1f03: "DeviceDebugInfo", 0x6100: "TouchpadRawXY", 0x6110: "TouchMouseRawPoints",
    0x6120: "Gestures", 0x6500: "Gesture", 0x8010: "GKey", 0x8020: "MKeys",
    0x8030: "MR", 0x8040: "BrightnessControl", 0x8070: "ColorLedEffects",
    0x8071: "RgbEffects", 0x8080: "PerKeyLighting", 0x8090: "ModeStatus",
    0x80a4: "LatencyMonitoring", 0x8110: "MouseButtonSpy", 0x8300: "SideTone",
    0x8310: "Equalizer", 0x8320: "HeadsetOut", 0x1b10: "ControlList",
    0x4100: "Encryption", 0x4220: "LockKeyState", 0x4522: "DisableKeysByUsage",
    0x4600: "Crown", 0x6010: "TouchpadFwItems", 0x6011: "TouchpadSwItems",
    0x6012: "TouchpadWin8FwItems", 0x1600: "Backlight", 0x1601: "Backlight1",
    0x1700: "PresenceDetection", 0x9001: "Pmw3816AndPmw3826", 0x9300: "Lis3Dh",
    0x1ed0: "I2cDirectAccess", 0x2111: "SmartShiftEnhanced", 0x2251: "MouseWheelAnalytics",
    0x19c0: "ForcePairing", 0x1500: "ForceSensingButton", 0x0008: "KeyboardLayout",
    0x1860: "BatteryUnifiedLevelStatus",
}


class Channel:
    def __init__(self, path):
        self.fd = os.open(path, os.O_RDWR | os.O_NONBLOCK)

    def write(self, report):
        # Drop stale notifications so a reply is matched to this request.
        while select.select([self.fd], [], [], 0.02)[0]:
            os.read(self.fd, 64)
        os.write(self.fd, bytes(report))

    def read(self, timeout=TIMEOUT):
        r, _, _ = select.select([self.fd], [], [], timeout)
        if not r:
            return None
        return os.read(self.fd, 64)

    def request(self, device, feature_index, function, params=b"", long=False):
        header = [0x11 if long else 0x10, device, feature_index, (function << 4) | SW_ID]
        size = 20 if long else 7
        report = (bytes(header) + bytes(params)).ljust(size, b"\0")
        self.write(report)
        deadline_tries = 20
        while deadline_tries:
            deadline_tries -= 1
            data = self.read()
            if data is None:
                return None
            if len(data) < 4 or data[1] != device:
                continue
            if data[2] == 0xFF and data[3] == feature_index and (data[4] & 0x0F) == SW_ID:
                raise ProtocolError(data[5])
            if data[2] == 0x8F and data[3] == feature_index:
                raise ProtocolError(data[5], legacy=True)
            if data[2] == feature_index and data[3] == header[3]:
                return data[4:]
        return None


class ProtocolError(Exception):
    def __init__(self, code, legacy=False):
        super().__init__(f"{'HID++1.0' if legacy else 'HID++2.0'} error 0x{code:02x}")
        self.code = code
        self.legacy = legacy


def feature_index(ch, device, feature_id):
    reply = ch.request(device, 0x00, 0, feature_id.to_bytes(2, "big"))
    if reply is None or reply[0] == 0:
        return None
    return reply[0]


def read_name(ch, device, idx):
    count = ch.request(device, idx, 0)[0]
    name = b""
    while len(name) < count:
        chunk = ch.request(device, idx, 1, bytes([len(name)]))
        name += chunk[: count - len(name)]
    kind = ch.request(device, idx, 2)[0]
    return name.decode("ascii", "replace"), kind


def read_firmware(ch, device, idx):
    count = ch.request(device, idx, 0)[0]
    out = []
    for n in range(count):
        r = ch.request(device, idx, 1, bytes([n]))
        kind = r[0] & 0x0F
        prefix = r[1:4].decode("ascii", "replace").strip("\0 ")
        number = f"{r[4]:02x}"
        revision = f"{r[5]:02x}"
        build = int.from_bytes(r[6:8], "big")
        label = {0: "main app", 1: "bootloader", 2: "hardware", 3: "touchpad", 4: "optical sensor", 5: "softdevice", 6: "rf companion"}.get(kind, f"type {kind}")
        out.append(f"{label}: {prefix}{number}.{revision}.B{build:04d}")
    return out


def read_unified_battery(ch, device, idx):
    caps = ch.request(device, idx, 0)
    status = ch.request(device, idx, 1)
    soc, level_bits, charging, external = status[0], status[1], status[2], status[3]
    levels = [n for b, n in ((1, "critical"), (2, "low"), (4, "good"), (8, "full")) if level_bits & b]
    charge = {0: "discharging", 1: "charging", 2: "charging (slow)", 3: "charge complete", 4: "charge error"}.get(charging, f"state {charging}")
    soc_text = f"{soc} %" if caps[1] & 0x02 else "(no percentage)"
    return f"{soc_text}, level {','.join(levels) or '-'}, {charge}, external power {bool(external & 1)}"


def read_battery_status(ch, device, idx):
    r = ch.request(device, idx, 0)
    status = {0: "discharging", 1: "recharging", 2: "charge nearly complete", 3: "charge complete", 4: "recharging below optimal speed", 5: "invalid battery type", 6: "thermal error", 7: "other charging error"}.get(r[2], f"state {r[2]}")
    return f"{r[0]} %, next level {r[1]} %, {status}"


def read_voltage(ch, device, idx):
    r = ch.request(device, idx, 0)
    return f"{int.from_bytes(r[0:2], 'big')} mV, flags 0x{r[2]:02x}"


def read_dpi(ch, device, idx):
    sensors = ch.request(device, idx, 0)[0]
    out = []
    for s in range(sensors):
        r = ch.request(device, idx, 2, bytes([s]))
        out.append(f"sensor {s}: {int.from_bytes(r[1:3], 'big')} dpi")
    return out


def read_report_rate(ch, device, idx):
    r = ch.request(device, idx, 1)
    return f"{r[0]} ms"


def probe_device(ch, device):
    try:
        root = ch.request(device, 0x00, 1, bytes([0, 0, 0x5A]))
    except ProtocolError as e:
        print(f"  device {device}: {e}")
        return
    if root is None:
        print(f"  device {device}: no answer")
        return
    print(f"  device {device}: HID++ {root[0]}.{root[1]}")
    fs = feature_index(ch, device, FEATURE_SET)
    count = ch.request(device, fs, 0)[0]
    features = {}
    for i in range(1, count + 1):
        r = ch.request(device, fs, 1, bytes([i]))
        fid = int.from_bytes(r[0:2], "big")
        features[fid] = (i, r[2])
    print(f"    features ({count}):")
    for fid, (i, flags) in features.items():
        tags = []
        if flags & 0x80: tags.append("obsolete")
        if flags & 0x40: tags.append("hidden")
        if flags & 0x20: tags.append("engineering")
        print(f"      0x{fid:04x} {KNOWN.get(fid, '?'):28s} idx {i:2d} {' '.join(tags)}")
    if DEVICE_NAME in features:
        name, kind = read_name(ch, device, features[DEVICE_NAME][0])
        kinds = {0: "keyboard", 1: "remote control", 2: "numpad", 3: "mouse", 4: "trackpad", 5: "trackball", 6: "presenter", 7: "receiver", 8: "headset", 9: "webcam", 10: "steering wheel", 11: "joystick", 12: "gamepad", 13: "dock", 14: "speaker", 15: "microphone", 16: "illumination light", 17: "programmable controller", 18: "car sim pedals", 19: "adapter"}
        print(f"    name: {name} ({kinds.get(kind, kind)})")
    if DEVICE_FW in features:
        for line in read_firmware(ch, device, features[DEVICE_FW][0]):
            print(f"    firmware {line}")
    for fid, fn, label in ((UNIFIED_BATTERY, read_unified_battery, "unified battery"), (BATTERY_STATUS, read_battery_status, "battery status"), (BATTERY_VOLTAGE, read_voltage, "battery voltage"), (REPORT_RATE, read_report_rate, "report rate")):
        if fid in features:
            try:
                print(f"    {label}: {fn(ch, device, features[fid][0])}")
            except ProtocolError as e:
                print(f"    {label}: {e}")
    if ADJUSTABLE_DPI in features:
        for line in read_dpi(ch, device, features[ADJUSTABLE_DPI][0]):
            print(f"    dpi {line}")


def main(path):
    ch = Channel(path)
    print(f"{path}")
    # Receiver, HID++ 1.0 (no software id): short register read 0x81 of
    # register 0x02, connection state; byte 1 of the payload is the number
    # of connected devices on this receiver (verified on the Bolt).
    ch.write(bytes([0x10, RECEIVER, 0x81, 0x02, 0, 0, 0]))
    data = ch.read()
    print(f"  receiver connection state: {data.hex() if data else None} -> {data[5] if data else '?'} devices connected")
    # Long register read 0x83 of 0xB5, sub register 0x50 + index: Bolt pairing information (layout unverified).
    for sub in (0x51, 0x52, 0x53, 0x54, 0x55, 0x56):
        ch.write(bytes([0x10, RECEIVER, 0x83, 0xB5, sub, 0, 0]))
        data = ch.read()
        if data and data[2] != 0x8F:
            print(f"  receiver pairing info 0x{sub:02x}: {data[5:].hex()}")
    for device in range(1, 7):
        try:
            probe_device(ch, device)
        except (ProtocolError, TypeError) as e:
            print(f"  device {device}: {e}")


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "/dev/hidraw3")
