# Hidsight

<p align="center">
  <img src="data/icons/de.tordenskjold.hidsight.svg" width="128" height="128" alt="Hidsight icon: a keyboard and a mouse in a blue ring">
</p>

A native Plasma application that shows the state of Logitech keyboards
and mice: which devices are on the receiver or connected over Bluetooth,
their battery and charging state, firmware, DPI and report rate, with a
tray icon for the battery. It reads over Logitech's HID++ 2.0 protocol
as the logged-in user, without a daemon and without root.

Solaar does this job, but it looks foreign on a KDE desktop and is built
to change settings. Plasma's own battery widget cannot see devices on a
Logi Bolt receiver while the kernel driver does not know that receiver.
Hidsight is the small, read-only, Plasma-native view of these devices.

**Status:** Phase 3 of the [plan](PLAN.md). The window lists the
Logitech keyboards and mice on a receiver with their battery and
charging state; a device page shows firmware, connection and the
features of the device in plain words. Hidsight finds receivers as they
are plugged in and follows battery changes and devices that wake up or
are switched on. Tested with an MX Keys S and an MX Master 3S on a Logi
Bolt Receiver. The tray icon and notifications follow in Phase 4.

## What Hidsight does not do

- Change any device setting: DPI, SmartShift, buttons, backlight, Fn
  inversion. Version 1 sends only read requests.
- Pair or unpair devices, switch hosts or update firmware.
- Run as root, as a daemon or as a system service.
- Access the network in any way.

## Device access

Hidsight reads the hidraw nodes of Logitech receivers and devices as the
logged-in user. The udev rule [data/70-hidsight.rules](data/70-hidsight.rules)
grants that access through `uaccess`, scoped to vendor 046d. The package
installs it to `/usr/lib/udev/rules.d`. For a build from source:

```sh
sudo install -Dm644 data/70-hidsight.rules /etc/udev/rules.d/70-hidsight.rules
sudo udevadm control --reload
sudo udevadm trigger --subsystem-match=hidraw
```

## Building

Hidsight needs Qt 6.12 and KDE Frameworks 6.31 or newer. On Arch Linux:

```sh
sudo pacman -S --needed cmake extra-cmake-modules ninja qt6-base \
    qt6-declarative kirigami kirigami-addons ki18n kconfig kcoreaddons \
    kdbusaddons knotifications kstatusnotifieritem qqc2-desktop-style \
    systemd-libs appstream desktop-file-utils
```

Build, test and run:

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
./build/debug/bin/hidsight
```

`Messages.sh` extracts the translatable strings in the form KDE's
translation tools expect.

The `asan` preset builds with AddressSanitizer and UndefinedBehaviorSanitizer,
`release` builds optimized. `cmake --build --preset debug --target
clang-format` formats the sources in the KDE Frameworks style.

## Command line

`hidsight-cli` lists the devices on a receiver as text or JSON. It needs
the HID++ node of the receiver, the hidraw node whose report descriptor
uses usage page 0xFF00. The app finds that node by itself.

```sh
hidsight-cli /dev/hidraw3
hidsight-cli --json /dev/hidraw3
```

`--watch` keeps reading and prints every change: a battery event, a
device that wakes up or is switched on, one that stops answering.
`--record <file>` writes every report sent and received to a capture
file, and `--replay <file>` reads a capture instead of a node. The tests
replay captures recorded this way. A capture holds the raw reports of
your devices; the unit ids of the devices are replaced by zeros. Look
at it before you share it.

## Packaging

To build an Arch package from the checkout:

```sh
cd packaging
makepkg -si
```

## AI disclosure

- **AI-assisted development.** I write Hidsight's code together with
  Claude Code, an AI coding agent. I write much of it myself; Claude
  Code contributes code, suggested fixes, refactoring, tests and
  documentation, always under my direction.
- **Reviewed and owned by me.** I review every change before it is
  committed. I decide what ships and I am responsible for it.
- **Verified, not trusted.** Generated code gets no special trust.
  Device reports are treated as untrusted input; protocol code is tested
  against reports recorded from real devices and runs under
  AddressSanitizer and UndefinedBehaviorSanitizer. Protocol knowledge
  comes from Logitech's public HID++ documents, never from the code of
  GPL projects. Device results are stated only when they were measured
  on my desk.

## License

MIT, see [LICENSES/MIT.txt](LICENSES/MIT.txt). The metadata files are
CC0-1.0. Licensing follows the [REUSE](https://reuse.software)
specification.

Hidsight is an independent project and not affiliated with Logitech.
Logitech, Logi Bolt and Unifying are trademarks of Logitech.
