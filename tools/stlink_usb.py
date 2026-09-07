"""Read Windows USB enumeration to explain ST-Link discovery failures.
This module never installs drivers, opens SWD, or changes a device.
"""
import json
import os
from pathlib import Path
import re
import subprocess


def windows_devices():
    """Return present USB devices; a query failure is not an empty device list."""
    powershell = Path(os.environ.get("SystemRoot", "C:/Windows")) / "System32/WindowsPowerShell/v1.0/powershell.exe"
    script = r"""$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = [System.Text.UTF8Encoding]::new($false)
$usbDevices = @(Get-CimInstance Win32_PnPEntity -Filter 'Present = True' |
    Where-Object { $_.PNPDeviceID -like 'USB\*' } |
    Select-Object Name, PNPDeviceID, Service, ConfigManagerErrorCode)
ConvertTo-Json -InputObject $usbDevices -Compress
"""
    result = subprocess.run([str(powershell), "-NoProfile", "-NonInteractive", "-Command", script],
                            capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=20)
    if result.returncode:
        raise RuntimeError(result.stderr.strip() or "Windows USB query failed")
    devices = json.loads(result.stdout.lstrip("\ufeff"))
    if not isinstance(devices, list) or any(not isinstance(d, dict) for d in devices):
        raise ValueError("Unexpected Windows USB query format")
    return devices


def diagnose(devices):
    """Classify observed USB state, without claiming that a target is connected."""
    # IDs are from STSW-LINK009 v3; unknown future IDs remain visible in the report.
    known = re.compile(r"VID_0483&PID_(?:3748|374A|374B|374E|374F|3752|3753|3754|3757)", re.I)
    loader = re.compile(r"VID_0483&PID_(?:374D|3755)", re.I)
    probes = [d for d in devices if known.search(d.get("PNPDeviceID", ""))
              or re.search(r"ST[- ]?LINK", d.get("Name") or "", re.I)]
    if any(loader.search(d.get("PNPDeviceID", "")) for d in devices):
        return "STLINK_LOADER", "ST-Link USB loader detected. Check probe mode in ST tools; no automatic firmware update is performed."
    if probes:
        if any(d.get("ConfigManagerErrorCode") not in (0, None) for d in probes):
            return "USB_DRIVER_ERROR", "ST-Link is visible to Windows with a device error. Check Device Manager error code and the official STSW-LINK009 driver."
        return "USB_PRESENT_CLI_UNAVAILABLE", "Windows sees ST-Link, but CubeProgrammer did not enumerate a serial. Close other probe sessions and inspect the raw CLI output and active USB driver."
    if any(d.get("ConfigManagerErrorCode") not in (0, None) for d in devices):
        return "USB_DEVICE_ERROR", "A USB device has an error but cannot be identified as ST-Link. Check the listed error and reconnect with a known data cable."
    return "NO_STLINK_USB", "Windows has no recognized ST-Link USB device. Check the probe USB port, data cable and direct PC connection; changing the MCU/SWD target config cannot create a missing USB device."


def report():
    if os.name != "nt":
        print("[USB] Check macOS System Information > USB for the ST-Link probe.")
        return
    try:
        devices = windows_devices()
        code, message = diagnose(devices)
        print(f"[USB:{code}] {message}")
        print("[USB present devices] " + json.dumps(devices, ensure_ascii=False, indent=2))
    except (OSError, RuntimeError, ValueError, subprocess.TimeoutExpired) as exc:
        print(f"[USB:QUERY_FAILED] Unable to determine USB state: {exc}")
