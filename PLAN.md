# Hidsight - Project Plan

Status: 2026-10-24 - name decided (Hidsight), stack decided (section
12), devices on the maintainer's desk read by the Python spike
(section 6). Phases 0 to 2 done: discovery, sessions with events and
the model run on the desk, tests pass under ASan and UBSan, CI is
green. Phase 3 is next.

Hidsight is the maintainer's first desktop project after three Sailfish
OS apps (SailVault, SailToken, SailTune) and the PineTime firmware
PineForge. It shares no code with them; their planning, release and
review experience carries over (section 11).

## 1. Goal

A native Plasma application, open source (MIT), that shows the state of
Logitech keyboards and mice: which devices are on the receiver or
connected over Bluetooth, their battery and charging state, firmware,
DPI and report rate, with a tray item for the battery. It reads over
Logitech's HID++ 2.0 protocol on the hidraw nodes, as the logged-in
user, without a daemon and without root. It exists because Solaar does
the job but looks foreign on a KDE desktop, and because Plasma's own
battery widget cannot see devices on a Bolt receiver while the kernel
driver does not know that receiver.

- Binary and package: `hidsight`; desktop id `de.tordenskjold.hidsight`
- Primary target: the maintainer's desk: CachyOS, Plasma 6.8 on
  Wayland, Qt 6.12, KDE Frameworks 6.31, Logi Bolt Receiver with an
  MX Keys S and an MX Master 3S
- Secondary targets: other Arch-based systems first (PKGBUILD), then
  distributions whose Qt and Frameworks are new enough (section 12)
- v1 reads; it changes no setting on any device (section 4)

## 2. Success criteria

| # | Criterion | Measured by |
|---|-----------|-------------|
| 1 | Native Plasma | Kirigami and KDE Frameworks only; follows the KDE Human Interface Guidelines; Breeze icons; light and dark color schemes; every action reachable by keyboard; looks at home next to the System Settings |
| 2 | Reads, never changes | v1 sends only request reports (HID++ reads, pings and feature queries); no function that writes a device setting exists in the binary |
| 3 | No root, no daemon | A udev rule gives the logged-in user the hidraw nodes; the app runs as the user; the tray item lives in the app process; nothing runs as root or as a system service |
| 4 | Untrusted device input | Every report is parsed with fixed sizes and bounded lengths; a malformed or hostile device cannot crash the app or make it read out of bounds (ASan and UBSan clean under the test suite and a fuzz-like test with random reports) |
| 5 | Responsive | The window shows within 1 s of start; two awake devices are listed with battery within 2 s of plugging in the receiver; the GUI thread never blocks on a device |
| 6 | Coexists | Works while Solaar or logiops hold the same hidraw node; replies are matched by device index, feature index and software id, so another tool's traffic is ignored; the node is never opened exclusively |
| 7 | Packaged | `cmake --install` puts everything under the standard KDE install dirs; the PKGBUILD builds in a clean Arch container; `appstreamcli validate` passes; the desktop file validates |
| 8 | Tested | The protocol library is tested against recorded captures of real devices; the tests run in CI before every tag |

## 3. Positioning (as of 2026-10)

Tools for Logitech devices on Linux (desk research 2026-10-24, Solaar
and OpenLogi not run first-hand):

| Tool | What it is | Gap for a Plasma user |
|------|------------|------------------------|
| Solaar | Python, GPL-2, the reference: pairing, reading and writing settings for receivers and Bluetooth devices, GTK GUI and tray | GTK look on Plasma; does everything, so it looks like a control panel |
| logiops | C++, GPL-3, daemon with a config file for gestures, DPI and buttons | No GUI, no battery, root daemon |
| OpenLogi | Rust, MIT/Apache, since 2026-08 a Logi Options+ replacement for Linux, macOS and Windows, "not yet stable" | Control, not state; per-app profiles need X11; own look |
| libratbag and Piper | MIT, GTK, gaming mice | Not for the MX line |
| Plasma battery widget | Shows batteries UPower knows | Bolt devices are invisible while the kernel's Logitech driver does not know the Bolt receiver (a patch was under review in 2026-07); Bluetooth-connected devices show through UPower on some systems |
| hidpp crate (Rust) | Protocol library, 0.2.0 | No declared license; unusable |

Hidsight competes on one thing: it is the Plasma-native view of these
devices, small, read-only and good-looking. It does not compete on
features.

## 4. Non-goals

- Changing any device setting in v1: DPI, SmartShift, button mapping,
  backlight, Fn inversion. An opt-in writing mode is a candidate for a
  later release (Phase 6), never the default.
- Pairing and unpairing, host switching, firmware updates (DFU)
- Gaming devices: RGB lighting, onboard profiles, G-keys
- Other vendors. The protocol library is Logitech HID++ only
- A daemon, a system service, root helpers, polkit
- A replacement for the kernel driver or for UPower integration;
  Hidsight shows what it reads itself
- Windows, macOS, GNOME, Flatpak in v1 (Flatpak needs full device
  access and is a later packaging decision)
- Telemetry, update checks, network access of any kind

## 5. Architecture

```
src/hidpp/     protocol library, QtCore only, no QML, no KDE:
               Report (7 and 20 bytes), Channel interface,
               HidrawChannel (QSocketNotifier), Receiver (HID++ 1.0
               registers), Device (HID++ 2.0 feature table), one small
               type per feature (DeviceName, DeviceFwVersion,
               UnifiedBattery, BatteryStatus, AdjustableDpi, ReportRate)
src/app/       QObject layer: DeviceMonitor (libudev hotplug and
               enumeration of HID++ hidraw nodes), DeviceModel
               (QAbstractListModel), BatteryTray (KStatusNotifierItem),
               Notifier (KNotifications), Settings (KConfig), main
src/cli/       hidsight-cli: lists devices as text or JSON; the first
               consumer of the library and the tool for captures
qml/           Kirigami pages: device list, device page, about, empty
               states (no device, no permission)
data/          desktop file, AppStream metainfo, udev rule, icons,
               notifyrc
tests/         Qt Test against captures in tests/captures/
packaging/     PKGBUILD
```

- Strictly layered: QML never sees bytes or feature indices; the app
  layer never parses a report; the protocol library never knows about
  QML, KDE or udev.
- The transport is an interface. Production uses `HidrawChannel`; tests
  use a channel that replays a capture and checks the requests.
- One request in flight per channel, matched by device index, feature
  index and software id; a timeout and one retry per request; stale
  reports are drained before a request. Unsolicited reports (battery
  events, connection notifications) go to the device's event handler.
- Discovery: a hidraw node belongs to Hidsight when its report
  descriptor declares vendor usage page 0xFF00 with report ids 0x10 and
  0x11 and the device is vendor 046d. Receivers are HID++ 1.0 at device
  index 0xFF with up to six devices at indices 1 to 6; a device
  connected over Bluetooth or cable speaks HID++ 2.0 directly at index
  0xFF (unverified: no such device on the desk; section 12).
- Battery: Unified Battery (0x1004) first, Battery Status (0x1000) and
  Battery Voltage (0x1001) as fallbacks; events update the model, a
  timer re-reads every device at a long interval as a safety net.
- Errors are values; nothing throws across a signal. A device that
  answers with an error or not at all is shown as "not reachable", not
  dropped.
- Logging through `ecm_qt_declare_logging_category`: feature ids and
  lengths at debug level, raw bytes only in a dedicated `hidsight.raw`
  category that is off by default, never serial numbers or unique ids
  above debug.

Stack (decided 2026-10-24, section 12): C++ 20, Qt 6, Kirigami 6 with
Kirigami Addons, qqc2-desktop-style, Breeze icons, KI18n, KConfig,
KCoreAddons, KStatusNotifierItem, KNotifications, KDBusAddons; CMake with
extra-cmake-modules and Ninja; libudev; hidraw directly.

## 6. HID++ facts verified on the desk (2026-10-24, `spikes/enumerate.py`, `spikes/listen.py`, `hidsight-cli`)

Receiver: Logi Bolt Receiver 046d:c548 on `hid-generic`; the kernel's
`hid_logitech_dj` does not know the id, so there is no power_supply
entry. The receiver exposes four hidraw nodes; the HID++ one declares
usage page 0xFF00 with report ids 0x10 (7 bytes) and 0x11 (20 bytes)
only, no 0x12.

| Index | Device | Firmware | Battery read | Features worth showing |
|-------|--------|----------|--------------|------------------------|
| 1 | MX Keys S (keyboard) | RBK81.02.B0023, bootloader BL188.02.B0023 | Unified Battery: 100 %, full, charge complete, external power | Backlight2 0x1982, FnInversion 0x40a3, LockKeyState 0x4220, MultiPlatform 0x4531, KeyboardLayout2 0x4540, HostsInfo 0x1815 |
| 2 | MX Master 3S (mouse) | RBM22.02.B0009, bootloader BL169.02.B0009 | Unified Battery: 85 %, discharging | AdjustableDpi 0x2201 (1000 dpi), ExtendedAdjustableDpi 0x2250, SmartShift 0x2110, HiResWheel 0x2121, Thumbwheel 0x2150, HostsInfo 0x1815 |

Both devices speak HID++ 4.5. Indices 3 to 6 answer HID++ 1.0 error
0x09 (resource error): nothing paired. The full feature tables are in
`spikes/capture-bolt-mxkeys-s-mxmaster-3s-2026-10-24.txt`.

- HID++ 1.0 to the receiver (index 0xFF) carries no software id. Short
  register read is sub id 0x81, long register read 0x83. Register 0x02
  returns the number of paired devices in payload byte 1 (still 2 with
  the mouse switched off). Register
  0xB5 read long with sub register 0x50 + index returns Bolt pairing
  information; its layout is not decoded. Sub registers 0x60 + index
  return zeros, so names come from feature 0x0005. Error replies carry
  sub id 0x8F; codes seen: 0x02 invalid address, 0x04 connection
  failed, 0x09 resource error, 0x0B invalid parameter value.
- A request to a paired device that is switched off or asleep gets
  HID++ 1.0 error 0x04 from the receiver at once, without waking the
  device; an empty slot gets 0x09. The receiver also answers 0x04 when
  two programs send to the same device at the same moment (seen with
  the app and the CLI on one node), so the session tries a device once
  more after 1 s before it shows it as not reachable.
- A resting MX Keys S still answers: from 19:39 to 19:50 it answered a
  re-read every minute without a key press, the first request of each
  taking 0.6 to 1.25 s instead of 15 ms. The receiver answered 0x04 for
  it only once (17:42, about 3 minutes after the last key press and
  shortly after its cable was unplugged); that deeper sleep could not
  be brought about on purpose. A switched-off device gets 0x04 every
  time.
- HID++ 2.0: `[report id, device index, feature index, function << 4 |
  software id, params]`; errors carry feature index 0xFF. Devices
  answer in a long report even to a short request.
- Right after the receiver is plugged in, and when a device sleeps, the
  first request can take well over 0.5 s or get no answer. A drain
  before each request, a 2 s timeout and one retry made the enumeration
  reliable; the first spike run without them reported "no answer" for
  both devices.
- Feature flags: 0x80 obsolete, 0x40 hidden, 0x20 engineering. Hidden
  and engineering features never reach the UI.
- A full read of both devices by `hidsight-cli` (ping, feature table,
  name, firmware, battery, DPI; 100 requests) took 3.3 s with both
  devices awake and no retry. The first request to a device after a
  pause takes about 0.5 s, every further one 15 to 22 ms. Reading the
  feature table first does not fit criterion 5, so the session first
  reads ping, name and battery of every device (about 0.7 s each) and
  only then the rest.
- Events arrive without Hidsight enabling anything: receiver register
  0x00 (notification flags) reads `00 00 00`, and still the devices
  send HID++ 2.0 events with software id 0. On every change of the
  charging cable of the MX Keys S a Unified Battery event with the
  layout of getStatus (the contact bounces: three events within
  350 ms). On switching the MX Master 3S on and on waking the MX Keys S
  with a key press a WirelessDeviceStatus (0x1d4b) event `01 01 01`,
  then a battery event. Switching off and falling asleep send nothing;
  key presses send nothing on the HID++ node.
- Replugging the receiver: the devices answer 0x04 until they
  reconnect. The MX Keys S was listed with its battery 1.63 s after the
  receiver was plugged in; the MX Master 3S once it was moved.
- The DeviceFwVersion entity count carries the unit id of the device
  (the same bytes as in the Bolt pairing information), and each
  firmware entry four bytes of unknown meaning that differ between the
  devices. Captures zero both; Hidsight parses neither.
- Unplugging the charging cable of the MX Keys S between two reads by
  `hidsight-cli`: the second read showed 100 %, discharging, no external
  power (Unified Battery status bytes 2 and 3).
- The raw capture of that read is
  `tests/captures/bolt-mxkeys-s-mxmaster-3s-2026-10-24.capture`; the
  spike's capture is decoded text and stays a reference.

## 7. Device access

- `data/70-hidsight.rules`: `TAG+="uaccess"` for hidraw nodes of USB
  vendor 046d and of Bluetooth HID devices `0005:046D:*`. Installed to
  the udev rules dir by CMake; during development the maintainer
  installed a copy by hand as `/etc/udev/rules.d/70-logitech-hidpp.rules`
  (2026-10-24; remove it once the package installs its own).
- Without the rule the app starts and shows an empty state that names
  the rule and the path, never a crash and never a request for root.
- The node is opened read-write and non-blocking, never exclusive.

## 8. Tray and notifications

- KStatusNotifierItem with the battery level of the device with the
  lowest charge as icon and all devices in the tooltip; left click opens
  the window, the menu lists the devices and Quit.
- KNotifications: battery low (device-reported level "low" or
  "critical"), charge complete. Both can be turned off in the settings.
- Start minimized to the tray and XDG autostart are settings, off by
  default (mechanism: section 12).

## 9. Phases

Version per finished phase: 0.N.0 after Phase N, as in the Sailfish apps;
0.5.0 is the first release announced on the website.

### Phase 0 - Scaffold

Repository, `.gitignore` with the agent files, MIT license, README with
the AI disclosure, `CMakeLists.txt` with ECM (KDECompilerSettings,
KDEInstallDirs, KDECMakeSettings), `CMakePresets.json` (debug, asan,
release), an empty Kirigami window that starts, desktop file, metainfo,
icon placeholder, clang-format, PKGBUILD that builds, GitHub Actions on
an Arch container running the build and tests. Verified: builds and
starts on the maintainer's desk.

### Phase 1 - Protocol library and CLI

`src/hidpp/` as in section 5 with the features of section 6; the
request matching, timeouts, drain and retry of section 6; the capture
channel and tests against the 2026-10-24 capture; `hidsight-cli` that
lists devices as text and JSON. Verified: the CLI reproduces the spike's
output for both devices; the tests pass under ASan and UBSan.

### Phase 2 - Discovery and models

Node discovery by report descriptor, libudev hotplug, receiver and
device objects with lifetime tied to the node, DeviceModel, battery
events and the periodic re-read, wake-up handling. Verified: replugging
the receiver, switching a device off and on, and letting it sleep, each
end in a correct list within the time of criterion 5.

### Phase 3 - Kirigami UI

Device list with name, type icon, battery and charging state; device
page with firmware, DPI, report rate, connection and the feature list
in plain words; empty states for no device and no permission; about
page; light and dark; keyboard navigation; strings through KI18n.
Verified against the KDE HIG checklist and with both color schemes.

### Phase 4 - Tray and notifications

Section 8. Verified: a low battery notification by letting the mouse
run down, or by a capture channel that replays a low level in a debug
build.

### Phase 5 - Release

Metainfo screenshots, PKGBUILD in the AUR, website project page and a
devlog entry, 0.5.0 from a clean tag.

### Phase 6 - After the first release

Candidates, each its own decision: Bluetooth-connected devices if not
covered by then; an opt-in writing mode for a few settings (DPI,
SmartShift, Fn inversion); Flatpak; testing the kernel's Bolt patch and
reporting upstream; German translation.

## 10. Security principles

- No network access, no telemetry, no update check; the binary links
  no network library.
- Device reports are untrusted input: fixed-size buffers, every length
  bounded, every index checked; the parser is tested with random and
  truncated reports.
- The app never asks for or uses root; access comes from the udev rule
  and nothing else. The rule is scoped to hidraw nodes of one vendor.
- v1 writes nothing to a device but request reports; there is no code
  path that changes a setting.
- Logs carry no raw reports by default, and never serial numbers or
  unique identifiers above debug level.

## 11. Lessons from the Sailfish apps and PineForge, applied from the start

- Spike the platform risk before the plan: done; the enumeration ran
  against both devices before a line of C++ exists.
- Check the name before anything carries it: done 2026-10-24 (no
  repository or package named Hidsight on that day).
- Plan, hard requirements and non-goals before code; one minor version
  per finished phase; releases from a clean tag; README with the AI
  disclosure in Phase 0, not in the last week.
- Claim nothing that was not measured on the desk.
- Layer strictly and keep the untrusted-input parser in one place.

## 12. Open decisions

- **Bluetooth-connected devices in v1.** A device paired directly over
  Bluetooth speaks HID++ on its own hidraw node at index 0xFF. The desk
  has no such setup; the MX Keys S and MX Master 3S could be paired to
  the computer's Bluetooth to test it. Decide after Phase 2.
- **Minimum Qt and Frameworks versions.** The desk has Qt 6.12 and KF
  6.31. Set the minimum to what is tested; lower it only when a
  distribution asks and a build proves it.
- **Battery re-read interval.** Events should carry the day; the timer
  is a safety net. Start with 15 minutes. Verified on the desk: a read
  of a sleeping device gets 0x04 at once and does not wake it. A device
  that falls asleep stays "online" in the list until the next re-read
  finds it.
- **Autostart mechanism.** XDG autostart desktop file written by the
  app, or a KConfig-managed entry. Decide in Phase 4.
- **Icon.** A symbolic tray icon set for the battery levels and an app
  icon; design in Phase 3.

Decided:

- 2026-10-24: a device that does not answer is shown neutrally as "not
  connected, asleep or switched off", greyed, with its last known
  battery and how long ago it was read. The receiver answers the same
  for both, and neither sends an event (section 6), so Hidsight does not
  claim which one it is.
- 2026-10-24: the name and kind of each device are remembered per
  receiver model and slot in the KConfig state file, so a device that is
  asleep when Hidsight starts is listed by name. Nothing else is stored;
  no unit ids or serial numbers.
- 2026-10-24: C++ only, no Rust core. The KDE pieces (tray,
  notifications, config) have complete bindings only in C++; the
  protocol layer is a few hundred lines; a second build system buys
  nothing. Python only for throwaway spikes.
- 2026-10-24: hidraw directly over open, read, write and
  QSocketNotifier; no hidapi (mixed GPL3/BSD/custom license, little
  gain).
- 2026-10-24: Kirigami 6 with CMake and extra-cmake-modules, no qmake.
- 2026-10-24: name Hidsight (HID + insight); no "Logi" or "Logitech" in
  the name because of the trademark. Binary and package `hidsight`,
  desktop id `de.tordenskjold.hidsight`.
- 2026-10-24: v1 reads only (section 4).
- 2026-10-24: MIT; SPDX headers following REUSE, as KDE does it.
  Logitech's public HID++ documents are the protocol source; Solaar and
  the kernel driver (both GPL-2) are behavior references only, never
  code.

## 13. References

- Logitech HID++ documents (public Google Drive folder):
  https://drive.google.com/drive/folders/0BxbRzx7vEV7eWmgwazJ3NUFfQ28
- KDE Human Interface Guidelines: https://develop.kde.org/hig/
- Kirigami: https://develop.kde.org/docs/getting-started/kirigami/
- extra-cmake-modules: https://api.kde.org/ecm/
- KStatusNotifierItem: https://api.kde.org/frameworks/kstatusnotifieritem/html/
- AppStream metainfo: https://www.freedesktop.org/software/appstream/docs/
- Solaar (behavior reference): https://github.com/pwr-Solaar/Solaar
- Kernel patch, Bolt receiver support (2026-07):
  https://patchew.org/linux/20260713201201.391538-1-erikhakan@gmail.com/
- Spike: `spikes/enumerate.py`, capture
  `spikes/capture-bolt-mxkeys-s-mxmaster-3s-2026-10-24.txt`
