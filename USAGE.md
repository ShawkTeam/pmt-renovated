# Partition Manager Tool (PMT)

**Partition Manager Tool** is a powerful command-line utility for **Android** devices, designed to perform various operations on partitions quickly, reliably, and efficiently.  
This is the **renovated version** of PMT, rewritten in C++ for improved performance, stability, and usability compared to its older variant.

It supports **asynchronous operations**, allowing multiple partitions to be processed in parallel, and includes safety measures to prevent a single error from breaking the entire operation batch.

---

## Features

- **Backup** partitions to files (with optional permissions fix for non-root access).
- **Flash** image files directly to partitions.
- **Erase** partitions by filling them with zero bytes.
- **Get** partition sizes in various units.
- **Display** partition information in plain text or JSON format.
- **Retrieve** real paths and symbolic link paths of partitions.
- **Identify** file system or image types by checking magic numbers.
- **Reboot** the device into different modes.

---

## Usage

```bash
pmt [OPTIONS] [SUBCOMMAND]
```

### Global Options

| Option | Long Option            | Description |
|--------|------------------------|-------------|
| `-h`   | `--help`               | Print basic help message and exit. |
|        | `--help-all`           | Print full help message and exit. |
| `-S`   | `--search-path TEXT`   | Set the partition search path. |
| `-L`   | `--log-file TEXT`      | Set log file path. |
| `-f`   | `--force`              | Force the process to be executed even if checks fail. |
| `-l`   | `--logical`            | Specify that the target partition is **dynamic**. |
| `-q`   | `--quiet`              | Suppress output. |
| `-V`   | `--verbose`            | Enable detailed logs during execution. |
| `-v`   | `--version`            | Print version and exit. |

**Example usages for global options:**\
`pmt [SUBCOMMAND ...] --quiet`\
`pmt [SUBCOMMAND ...] -S /dev/block/platform/bootdevice/by-name`\
`pmt [SUBCOMMAND ...] [GLOBAL OPTIONS ...]`

---

## Subcommands

### 1. `backup`
Backup partitions to files. General syntax:
```bash
pmt backup partition(s) [output(s)] [OPTIONS]
```

**Options:**
- `-b`, `--buffer-size SIZE` → Set buffer size (in bytes) for read/write operations.
- `-O`, `--output-directory DIR` → Specify an output directory for backups.

**Notes:**
- Partition names are separated by commas.
- If custom output names are provided, they must match the number of partitions.
- Automatically adjusts permissions so backup files can be read/written without root.

**Example usages:**\
`pmt backup boot`\
`pmt backup boot boot_backup.img`\
`pmt backup boot,recovery,vendor`\
`pmt backup boot`\
`pmt backup boot,recovery -O /sdcard`\
`pmt backup system,vendor --buffer-size=8192 # '=' is not mandatory`

---

### 2. `flash`
Flash an image or multiple images to partitions. general syntax:
```bash
pmt flash partition(s) image(s) [OPTIONS]
```

**Options:**
- `-b`, `--buffer-size SIZE` → Set buffer size (in bytes).
- `-I`, `--image-directory DIR` → Directory containing image files.

**Notes:**
- Multiple partitions and images are separated by commas.

- **Example usages:**\
  `pmt flash boot boot_backup.img`\
  `pmt flash boot,recovery /sdcard/backups/boot_backup.img,/sdcard/backups/recovery_backup.img`\
  `pmt flash boot boot_backup.img,recovery_backup.img -I /sdcard/backups`\
  `pmt flash system,vendor system_backup.img,vendor_backup.img -I /sdcard/backups --buffer-size=8192`

---

### 3. `erase`
Fill partition(s) with zero bytes (like `dd if=/dev/zero of=/dev/block/by-name/<partition>`). General syntax:
```bash
pmt erase partition(s) [OPTIONS]
```
**Options:**
- `-b`, `--buffer-size SIZE` → Set buffer size.

**Example usages (DO NOT USE FOR TRYING!!!):**\
`pmt erase boot`\
`pmt erase nvdata,nvram`\
`pmt erase system,vendor --buffer-size=8192`

---

### 4. `sizeof`
Show the size of partition(s). General syntax:
```bash
pmt sizeof partition(s) [OPTIONS]
```

**Options:**
- `--as-byte` → Show size in bytes.
- `--as-kilobyte` → Show size in KB.
- `--as-megabyte` → Show size in MB (default).
- `--as-gigabyte` → Show size in GB.
- `--only-size` → Output only the numeric value (no partition name).

**Example usages:**\
`pmt sizeof boot` - Example output: `boot: 64MB`\
`pmt sizeof boot --as-byte` - Example output: `64`\
`pmt sizeof boot --as-<write type here> --only-size` - Example output (for `--as-kilobyte`): `98304`

---

### 5. `info`
Show partition name, size, and dynamic status. General syntax:
```bash
pmt info partition(s) [OPTIONS]
```

**Options:**
- `-J`, `--json` → Output in JSON format.
- `--json-partition-name NAME` → Custom JSON key for partition name.
- `--json-size-name NAME` → Custom JSON key for size.
- `--json-logical-name NAME` → Custom JSON key for dynamic status.

**Example usages:**\
`pmt info boot` - Example output: `partition=boot size=100663296 isLogical=false`\
`pmt info boot -J` - Example output: `{"name": "boot", "size": 100663296, "isLogical": false}`\
`pmt info boot -J --json-partition-name=partitionName` - Example output: `{"partitionName": "boot", "size": 100663296, "isLogical": false}`

---

### 6. `real-path`
Show the **absolute block device path** for each partition. General syntax:
```bash
pmt real-path partition(s) [OPTIONS]
```

**Example usages:**\
`pmt real-path boot` - Example output: `/dev/block/sda25`

---

### 7. `real-linkpath`
Show the **symbolic link path** for each partition (e.g., `/dev/block/by-name/boot`). General syntax:
```bash
pmt real-link-path partition(s) [OPTIONS]
```

---

### 8. `type`
Check magic numbers to determine file system or other types of partition(s) or image(s). General syntax:
```bash
pmt type partition(s) [OPTIONS]
```

**Options:**
- `-b`, `--buffer-size SIZE` → Set buffer size.
- `--only-check-android-magics` → Check only Android-related magic numbers.
- `--only-check-filesystem-magic` → Check only file system magic numbers.

**Example usages:**\
`pmt type boot` - Example output: `boot contains Android Boot Image magic (0x2144494F52444241)`\
`pmt type vendor_boot.img` - Example output: `vendor_boot.img contains Android Vendor Boot Image magic (0x544F4F4252444E56)`

---

### 9. `reboot`
Reboot the device. Default reboot target is normal. General syntax:
```bash
pmt reboot [rebootTarget] [OPTIONS]
```

**Example usages:**\
`pmt reboot`
`pmt reboot recovery`
`pmt reboot download`

## Additional Notes

- **Comma-separated inputs**: All commands (except `reboot`) require multiple inputs to be separated by commas.
- **Asynchronous execution**: For `backup`, `flash`, and `erase`, each partition is processed in a separate thread for maximum speed.
- **Error isolation**: A failure in processing one partition will not cancel the others. Only for `backup`, `flash` and `erase` functions.
- **Automatic diagnostics**: By default, whether a partition is dynamic or regular is determined automatically. With global options, you only specify precision.
- **Root access**: Root access is required if operations are to be performed on partitions.

## Extra Note: Comma Usage

In **Partition Manager Tool**, whenever you provide **multiple partitions**, **multiple image files**, or **multiple output file names**, they **must** be separated by commas (`,`), without spaces.

✅ **Correct:**\
`pmt backup boot,recovery`\
`pmt flash boot,recovery boot.img,recovery.img`


❌ **Incorrect:**\
`pmt backup boot recovery`\
`pmt flash boot recovery boot.img recovery.img`

The **number of items must match** when providing both input and output lists.  
For example, if you specify 3 partitions, you must also provide 3 output file names.

This rule applies to **all commands except `reboot`**, since `reboot` only takes one optional argument.

---

## License
Partition Manager Tool is licensed under the **Apache 2.0 License**.  
Copyright © YZBruh.

---

## Bug Reports
Please submit issues at:  
[https://github.com/ShawkTeam/pmt-renovated/issues](https://github.com/ShawkTeam/pmt-renovated/issues)
