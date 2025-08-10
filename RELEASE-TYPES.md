# Release Types

This project provides four different release packages, each tailored for specific Android device architectures and usage preferences.

---

## The Four Release Files

| File Name                 | Architecture    | Bitness | Library Type   | Description                                              |
|---------------------------|-----------------|---------|----------------|----------------------------------------------------------|
| `pmt-arm64-v8a.zip`       | ARM64 (ARMv8-A) | 64-bit  | Dynamic (.so)  | For 64-bit devices, uses dynamic libraries. Requires accompanying `.so` files (`libhelper` and `libpartition_map`). |
| `pmt-static-arm64-v8a.zip`| ARM64 (ARMv8-A) | 64-bit  | Static (.a)    | Fully static build for 64-bit devices. No external dependencies. Great for general use and ADB environments. |
| `pmt-armeabi-v7a.zip`     | ARM (ARMv7)     | 32-bit  | Dynamic (.so)  | For 32-bit devices, uses dynamic libraries. Requires `.so` files (`libhelper` and `libpartition_map`). |
| `pmt-static-armeabi-v7a.zip`| ARM (ARMv7)   | 32-bit  | Static (.a)    | Fully static build for 32-bit devices. No external dependencies. Great for general use and ADB environments. |

---

## Architecture & Bitness Explained

- **ARM64 (arm64-v8a)**:  
  This is a 64-bit architecture used by newer Android devices. It can handle larger amounts of memory and generally runs faster for heavy tasks.

- **ARM (armeabi-v7a)**:  
  This is a 32-bit architecture common on older or less powerful Android devices. It has some limitations compared to 64-bit but is still widely supported.

---

## Dynamic vs Static Libraries

The project relies on two helper libraries:
- **libhelper**
- **libpartition_map**

### Dynamic Versions (`.so` files)

- In the non-static (`pmt-arm64-v8a.zip` and `pmt-armeabi-v7a.zip`) packages, these libraries are **compiled as shared objects (`.so` files)**.
- This means that the main program (`pmt`) **depends on these libraries being present** on the device or alongside the executable to run correctly.
- If these libraries are missing, the program will fail to start.
- These builds are mostly for developers or users who want to customize or work closely with the libraries.

### Static Versions (`.a` files)

- The static packages (`pmt-static-arm64-v8a.zip` and `pmt-static-armeabi-v7a.zip`) **include these libraries inside the main executable** by linking them statically.
- This means the `pmt` binary is **completely self-contained** and **does not require any external `.so` files**.
- These versions are ideal for general users and especially convenient for ADB usage, where installing separate `.so` files might be cumbersome.

---

## Which Should You Use?

- If you want a hassle-free experience and don’t want to worry about missing libraries, **choose the static version** matching your device’s architecture.
- If you are a developer or want to experiment with the libraries separately, or save space by sharing `.so` files between multiple programs, the **dynamic version** is the way to go.

---

## Summary

| Release Type | Architecture | Dependencies          | Best For                  |
|--------------|--------------|-----------------------|---------------------------|
| Static       | 32-bit / 64-bit | None (fully standalone) | General users, ADB usage  |
| Dynamic      | 32-bit / 64-bit | Requires `.so` libs   | Developers, advanced users |

---

If you’re unsure which one to pick, try the **static version** first — it works out of the box on all supported devices.
