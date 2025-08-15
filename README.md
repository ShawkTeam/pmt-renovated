# Partition Manager Tool (PMT)

**Partition Manager Tool** is a fast, reliable, and feature-rich CLI application for **Android** devices that enables advanced partition operations such as backup, flashing, erasing, information retrieval, and more.  
This **renovated edition**, written in modern **C++**, is faster, more stable, and more powerful than its previous versions, thanks to optimized multithreading and improved error handling.

PMT is designed for developers, technicians, and Android enthusiasts who need fine-grained control over device partitions via a clean, flexible, and scriptable interface.

---

## Key Features

- **Backup** partitions to files (with permission adjustments for non-root access).
- **Flash** image files directly to partitions.
- **Erase** partitions with zero-byte filling.
- **Retrieve** partition sizes in multiple units.
- **Display** partition info (name, size, logical status) in text or JSON.
- **Resolve** real block device paths and symbolic links.
- **Identify** partition or image file types via magic number checks.
- **Reboot** the device into multiple modes (normal, recovery, etc.).
- **Asynchronous processing** for speed — each partition runs in its own thread.
- **Error isolation** so one failing operation doesn’t cancel the rest. For back upping, flashing and erasing.
- **Test** sequential read/write speed of your memory.

---

## Documentation

For all information about PMT, see the [wiki](https://github.com/ShawkTeam/pmt-renovated/wiki)\
Read [Wiki - Using PMT via Termux or ADB](https://github.com/ShawkTeam/pmt-renovated/wiki/Using-PMT-via-Termux-or-ADB) for learn how to use PMT via Termux or ADB.\
Detailed usage instructions and option references can be found in the [Wiki - Usage](https://github.com/ShawkTeam/pmt-renovated/wiki/Usage).

---

## Credits
 - [CLI11: Command line parser for C++11](https://github.com/CLIUtils/CLI11)
 - [PicoSHA2: A header-file-only, SHA256 hash generator in C++](https://github.com/okdshin/PicoSHA2)
 - [nlohmann/json: JSON for Modern C++](https://github.com/nlohmann/json)
