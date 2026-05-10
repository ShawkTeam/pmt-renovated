@page using-with-termux Using with Termux

This guide shows you how to use PMT with Termux and ADB. After that, you might want to read [Usage](Usage)!

## Using PMT via Termux

This part of the guide explains how to download and install PMT with **Termux**.

---

### Requirements
- Termux app from [F-Droid](https://f-droid.org/packages/com.termux) or [GitHub](https://github.com/termux/termux-app/releases).
- DO NOT INSTALL TERMUX FROM PLAY STORE!

---

### Step-by-Step Usage

#### Prepare Termux

If you haven't already, it's highly recommended that you allow internal storage usage. After all, I'm sure you'll want to keep your backups/flashbacks, etc., there.
```bash
termux-setup-storage
```

#### Install PMT

You can easily install PMT with the ready-made script!
```bash
# Download PMT's installer script for termux
curl -LSs "https://raw.githubusercontent.com/ShawkTeam/pmt-renovated/refs/heads/main/build/scripts/manager.sh" > manager.sh

# Start script!
bash manager.sh install

# Now, PMT is ready for usage if no error occurred. Try running:
pmt --help
```

---

### 3. Verify Installation

Test that PMT is working correctly:

```bash
# Check PMT version and available commands
pmt --help

# List available partitions (requires root)
pmt list
```

## Uninstallation

To remove PMT from your Termux environment:

```bash
bash manager.sh uninstall
```

## Troubleshooting Termux Issues

### Common Problems and Solutions

| Issue                        | Solution                                                       |
|------------------------------|----------------------------------------------------------------|
| **Permission denied errors** | Run `termux-setup-storage` and ensure proper permissions       |
| **Command not found**        | Reinstall PMT using the installation script                    |
| **Storage access issues**    | Check if storage permission was granted during setup           |
| **Root access denied**       | Ensure your device is properly rooted and grant su permissions |

---

# Using PMT with ADB

This section explains how to use the static build of PMT directly on Android devices via ADB, perfect for quick operations without Termux.  
It’s written for beginners — no advanced knowledge needed.

---

## Why Use the Static Version?

The static build of PMT offers several advantages:
- **Self-contained**: All dependencies are bundled in a single executable
- **No installation required**: Run directly without additional packages
- **Cross-platform compatibility**: Works on most Android devices
- **Minimal storage footprint**: Ideal for temporary usage

---

## Prerequisites

### Required Software
- **Android SDK Platform Tools** - [Download from Android Developer](https://developer.android.com/studio/releases/platform-tools)
- **USB Drivers** for your device (if required)

### Device Requirements
- **USB Debugging enabled**: Settings → Developer options → USB debugging
- **USB connection**: Connect device to computer via USB cable
- **ADB authorization**: Allow USB debugging connection when prompted

### Checking Device Architecture

Determine your device's architecture to download the correct binary:

```bash
# Via ADB shell
adb shell getprop ro.product.cpu.abi

# Common architectures:
# arm64-v8a (64-bit ARM)
# armeabi-v7a (32-bit ARM)
# x86_64 (64-bit x86)
# x86 (32-bit x86)
```

---

## Installation and Usage

### 1. Download the Correct Binary
Download the appropriate release file that matches your device's architecture from [releases](https://github.com/ShawkTeam/pmt-renovated/releases):
**For General Users (Recommended):**
- `pmt-static-builtin-arm64-v8a.zip` → For 64-bit devices, most portable
- `pmt-static-builtin-armeabi-v7a.zip` → For 32-bit devices, most portable

**Alternative Options:**
- `pmt-static-arm64-v8a.zip` → For 64-bit devices, requires separate plugins
- `pmt-static-armeabi-v7a.zip` → For 32-bit devices, requires separate plugins

Unzip the downloaded `.zip` file — you should now have a `pmt` binary.

---

#### Push the Binary to Your Device
Use ADB to copy the `pmt` file to your device’s temporary folder:
```bash
# Rename for more easily usage
mv pmt_static pmt

adb push pmt /data/local/tmp/pmt
```

#### Open an ADB Shell
Access your device shell:
```bash
adb shell
```

#### Change to the Directory
Move into the temporary directory where pmt is stored:
```bash
cd /data/local/tmp
```

#### Give Execute Permission
Allow the binary to be executed:
```bash
chmod 755 pmt
```

#### Run PMT
You can now run PMT directly from this directory:

```bash
# Open root terminal
su

./pmt --help
```
Example — Back up the boot partition:

```bash
./pmt backup boot
```

#### Tips
Commands must be run from /data/local/tmp unless you move pmt elsewhere.\
Static builds are completely standalone — no missing library issues.
