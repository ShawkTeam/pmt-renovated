# Installing And Using PMT via Termux or ADB

This guide shows you how to use PMT with Termux and ADB. After that, you might want to read [USAGE.md](./USAGE.md)!

## Using PMT via Termux

This part of the guide explains how to download and install PMT with **Termux**.

---

### 📋 Requirements
- Termux app from [F-Droid](https://f-droid.org/packages/com.termux) or [GitHub](https://github.com/termux/termux-app/releases).
- DO NOT INSTALL TERMUX FROM PLAY STORE!

---

### 🚀 Step-by-Step Usage

#### 1️⃣ Prepare Termux

If you haven't already, it's highly recommended that you allow internal storage usage. After all, I'm sure you'll want to keep your backups/flashbacks, etc., there.
```bash
termux-setup-storage
```

#### 2️⃣ Install PMT

You can easily install PMT with the ready-made script!
```bash
# Download PMT's installer script for termux
curl -LSs "https://raw.githubusercontent.com/ShawkTeam/pmt-renovated/refs/heads/main/manager.sh" > manager.sh

# Start script!
bash manager.sh install

# Now, PMT is ready for usage if no error occurred. Try running:
pmt --help
```

---

### Do you want to uninstall...?

Only run script via this command:
```bash
bash manager.sh uninstall
```

---

## Using PMT via ADB

This part of the guide will show you how to use the **static** version of Partition Manager Tool (`pmt-static`) on your Android device through **ADB**.  
It’s written for beginners — no advanced knowledge needed.

---

### 📦 Why Static Version?
The **static** build of PMT contains everything it needs inside one single file.  
This means you can run it directly on your Android device **without** installing extra libraries.  
Perfect for quick tasks via ADB.

---

### 📋 Requirements
- **ADB installed** on your computer  
  (Part of the Android SDK Platform Tools — [Download here](https://developer.android.com/studio/releases/platform-tools))
- **USB Debugging enabled** on your phone  
  (Settings → Developer options → Enable USB debugging)
- Your **phone connected via USB** and recognized by ADB

---

### 🚀 Step-by-Step Usage

#### 1️⃣ Get the Correct Binary
Download the **`pmt-static`** file that matches your device’s architecture:
- **`pmt-static-arm64-v8a`** → For 64-bit devices
- **`pmt-static-armeabi-v7a`** → For 32-bit devices

Unzip the downloaded `.zip` file — you should now have a `pmt` binary.

---

#### 2️⃣ Push the Binary to Your Device
Use ADB to copy the `pmt` file to your phone’s temporary folder:
```bash
# Rename for more easily usage
mv pmt_static pmt

adb push pmt /data/local/tmp/pmt
```

#### 3️⃣ Open an ADB Shell
Access your device shell:
```bash
adb shell
```

#### 4️⃣ Change to the Directory
Move into the temporary directory where pmt is stored:
```bash
cd /data/local/tmp
```

#### 5️⃣ Give Execute Permission
Allow the binary to be executed:
```bash
chmod 755 pmt
```

#### 6️⃣ Run PMT
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

#### 💡 Tips
Commands must be run from /data/local/tmp unless you move pmt elsewhere.\
The /data/local/tmp folder is cleared when you reboot your device.\
Static builds are completely standalone — no missing library issues.
