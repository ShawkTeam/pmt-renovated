@page usage-guide Usage Guide

**Partition Manager Tool** is a powerful command-line utility for **Android** devices, designed to perform various operations on partitions quickly, reliably, and efficiently.  
This is the **renovated version** of PMT, rewritten in C++ for improved performance, stability, and usability compared to its older variant.

It supports **asynchronous operations**, allowing multiple partitions to be processed in parallel, and includes safety measures to prevent a single error from breaking the entire operation batch.

**This guide is for version 1.9.0+.**

---

## Features

- **Backup** partitions to files (with optional permissions fix for non-root access).
- **Flash** image files directly to partitions.
- **Erase** partitions by filling them with zero bytes.
- **Get** partition sizes in various units.
- **Read metadata** of logical partitions.
- **Read metadata** of dynamic partition groups.
- **Display** partition information in plain text or JSON format.
- **Retrieve** real paths and symbolic link paths of partitions.
- **Identify** file system or image types by checking magic numbers.
- **Reboot** the device into different modes.
- **Test** sequential read/write speed of your memory.

---

## Start Using
Don't forget to check out how to use it with **ADB**! See [Using PMT via ADB](@ref using-with-termux-and-adb)

```bash
pmt [OPTIONS] [SUBCOMMAND]
```

### Global Options

| Option | Long Option              | Description                                                        |
|--------|--------------------------|--------------------------------------------------------------------|
| `-h`   | `--help`                 | Print basic help message and exit.                                 |
|        | `--help-all`             | Print full help message and exit.                                  |
| `-L`   | `--log-file TEXT`        | Set log file path. Default: `<current directory>/last_logs.log`    |
| `-f`   | `--force`                | Force the process to be executed even if checks fail.              |
| `-l`   | `--logical`              | Specify that the target partition is **dynamic** (logical).        |
| `-q`   | `--quiet`                | Suppress output.                                                   |
| `-V`   | `--verbose`              | Enable detailed logs during execution.                             |
| `-v`   | `--version`              | Print version and exit.                                            |
|        | `--license`              | Print license information and exit.                                |
| `-s`   | `--select-on-duplicate`  | Select partition for work if has input named duplicate partitions. |
| `-p`   | `--plugins TEXT`         | Load input plugin files (comma-separated).                         |
| `-d`   | `--plugin-directory DIR` | Load plugins from specified directory.                             |

**Example usages for global options:**
```bash
pmt [SUBCOMMAND ...] --quiet
pmt [SUBCOMMAND ...] -L /custom/log/path.log
pmt [SUBCOMMAND ...] --select-on-duplicate
pmt [SUBCOMMAND ...] [GLOBAL OPTIONS ...]
```

---

## Subcommands

### Backing up partition(s)
Backup partitions to files with asynchronous processing for better performance. General syntax:
```bash
pmt backup partition(s) [output(s)] [OPTIONS]
```

**Options:**
- `-b`, `--buffer-size SIZE` → Set buffer size for read/write operations. Default: 1MB.
- `-O`, `--output-directory DIR` → Specify an output directory for backups (must exist).
- `-n`, `--no-set-perms` → Don't automatically adjust file permissions for non-root access.
- `-S`, `--verify` → Verify SHA-256 hash of backup after completion for integrity.

**Technical Details:**
- Uses multithreaded asynchronous processing for parallel backups
- Each partition processed in separate thread with progress tracking
- Automatic file permission adjustment (0664) and owner change (AID_EVERYBODY)
- SHA-256 verification compares source partition with backup file
- Default buffer size: 1MB (adjustable per partition size)
- Supports size expressions: KB, MB, GB (case-insensitive)
- Error isolation: failed backups don't affect other partitions

**Notes:**
- Partition names are separated by commas without spaces.
- Custom output names must match the number of partitions provided.
- Automatically adjusts permissions so backup files can be accessed without root by default.

**Example usages:**
```bash
pmt backup boot  # Creates boot.img in current directory
pmt backup boot boot_backup.img  # Custom filename
pmt backup boot,recovery,vendor  # Multiple partitions
pmt backup boot,recovery -O /sdcard  # Output to directory
pmt backup system,vendor --buffer-size=8KB  # Custom buffer size
pmt backup userdata --no-set-perms  # Keep default permissions
pmt backup boot --verify  # Verify backup integrity
pmt backup system,vendor -O /backups --buffer-size=2MB --verify
```

---

### Flashing image(s) to partition(s)
Flash image files to partitions with asynchronous processing for optimal speed. General syntax:
```bash
pmt flash partition(s) image(s) [OPTIONS]
```

**Options:**
- `-b`, `--buffer-size SIZE` → Set buffer size for reading/writing. Default: 1MB.
- `-d`, `--delete` → Delete image file(s) after successful flashing.
- `-I`, `--image-directory DIR` → Directory containing image files to flash.

**Technical Details:**
- Multithreaded asynchronous processing for parallel operations
- Validates image file existence before processing
- Size validation: image must not exceed partition size
- Buffer size automatically optimized per partition
- Progress tracking with real-time updates
- Automatic cleanup option with `--delete` flag
- Error isolation prevents cascade failures
- Supports all size expressions (KB, MB, GB)

**Notes:**
- Multiple partitions and images are separated by commas without spaces.

**Example usages:**
```bash
pmt flash boot boot_backup.img
pmt flash boot,recovery /sdcard/backups/boot.img,/sdcard/backups/recovery.img
pmt flash boot boot_backup.img,recovery_backup.img -I /sdcard/backups --delete
pmt flash system,vendor system.img,vendor.img -I /backups --buffer-size=8192
pmt flash userdata userdata.img --buffer-size=4MB
```

---

### Erasing partition(s) content(s)
Securely erase partition(s) by filling them with zero bytes (equivalent to `dd if=/dev/zero of=/dev/block/by-name/<partition>`). General syntax:
```bash
pmt erase partition(s) [OPTIONS]
```
**Options:**
- `-b`, `--buffer-size SIZE` → Set buffer size for zero-fill operations. Default: 4KB.

**Technical Details:**
- **Destructive Operation**: This is equivalent to `dd if=/dev/zero of=/dev/block/by-name/<partition>`
- Multithreaded processing for multiple partitions
- Interactive confirmation prompt (bypass with `--force`)
- Warning: "This could render your device unusable!"
- Optimized buffer sizing based on partition characteristics
- Sequential zero-byte writing with progress tracking
- Requires explicit confirmation unless `--force` flag used

**Example usages (⚠️ **WARNING**: These operations are destructive and will erase all data on the specified partitions):**
```bash
pmt erase boot  # Requires confirmation
pmt erase nvdata,nvram --force  # Skip confirmation
pmt erase system,vendor --buffer-size 8KB  # Custom buffer size
pmt erase userdata --force --buffer-size 1MB
```

---

### Getting partition size(s)
Display the size of partition(s) in various units. General syntax:
```bash
pmt sizeof partition(s) [OPTIONS]
```

**Options:**
- `--as-byte` → Show size in bytes.
- `--as-kilobyte` → Show size in KB.
- `--as-megabyte` → Show size in MB (default).
- `--as-gigabyte` → Show size in GB.
- `--only-size` → Output only numeric value (no partition name).

**Special Partition Names:**
- `get-all` or `getvar-all` → Show sizes for all partitions
- `get-logicals` → Show sizes only for logical partitions
- `get-physicals` → Show sizes only for physical partitions

**Technical Details:**
- Supports multiple unit conversions with proper formatting
- Batch processing of multiple partitions
- Special partition selectors for system-wide queries
- Optimized for quick size retrieval operations

**Example usages:**
```bash
pmt sizeof boot  # Output: `boot: 64MB`
pmt sizeof boot --as-byte  # Output: `67108864`
pmt sizeof boot --as-kilobyte --only-size  # Output: `65536`
pmt sizeof system,vendor --as-megabyte  # Multiple partitions
pmt sizeof get-all --as-gigabyte  # All partitions in GB
pmt sizeof get-logicals --only-size  # All logical partitions, size only
```

---

### Getting partition information
Display detailed partition information including name, size, and logical status. General syntax:
```bash
pmt info partition(s) [OPTIONS]
```

**Options:**
- `-J`, `--json` → Output in JSON format.
- `--json-indent-size NUM` → JSON indentation size. Default: 2.
- `--json-partition-name NAME` → Custom JSON key for partition name. Default: "name".
- `--json-table-name NAME` → Custom JSON key for table name. Default: "table".
- `--json-size-name NAME` → Custom JSON key for size. Default: "size".
- `--json-logical-name NAME` → Custom JSON key for logical status. Default: "isLogical".
- `--as-byte` → View sizes in bytes.
- `--as-kilobyte` → View sizes in KB.
- `--as-megabyte` → View sizes in MB.
- `--as-gigabyte` → View sizes in GB.

**Special Partition Names:**
- `get-all` or `getvar-all` → Information for all partitions
- `get-logicals` → Information only for logical partitions
- `get-physicals` → Information only for physical partitions

**Technical Details:**
- Comprehensive partition lpMetadata display
- JSON output with customizable field names and formatting
- Multiple size unit support
- Batch processing capabilities
- Special selectors for partition categories
- Table information included for physical partitions

**Example usages:**
```bash
pmt info boot  # Output: `partition=boot table=main size=100663296 isLogical=false`
pmt info boot -J  # JSON output
pmt info boot -J --json-partition-name=partitionName  # Custom field names
pmt info get-all -J  # All partitions in JSON
pmt info system,vendor --json-indent-size=4  # Custom JSON formatting
pmt info get-logicals --as-megabyte  # Logical partitions with MB units
pmt info get-physicals -J --json-table-name=sourceTable
```

---

### Getting partition path(s)
Display the absolute block device path and symbolic link information for partitions. General syntax:
```bash
pmt real-path partition(s) [OPTIONS]
```

**Options:**
- `--by-name` → Show symbolic link path instead of absolute block device path.

**Technical Details:**
- Resolves both absolute block device paths and symbolic links
- Absolute path example: `/dev/block/sda25`
- Symbolic link example: `/dev/block/by-name/boot`
- Supports batch processing of multiple partitions
- Useful for scripting and automation
- Validates partition existence before path resolution

**Example usages:**
```bash
pmt real-path boot  # Output: `/dev/block/sda25`
pmt real-path boot --by-name  # Output: `/dev/block/by-name/boot`
pmt real-path system,vendor  # Multiple partitions
pmt real-path userdata --by-name
```

---

### Detecting partition/image type(s)
Analyze magic numbers to identify file system or image types of partitions and files. General syntax:
```bash
pmt type partition(s) [OPTIONS]
```

**Options:**
- `-b`, `--buffer-size SIZE` → Buffer size for magic detection. Default: 4KB.
- `--only-check-android-magics` → Check only Android-specific magic numbers.
- `--only-check-filesystem-magics` → Check only filesystem magic numbers.

**Technical Details:**
- Magic number detection using file header analysis
- Supports both partition and image file analysis
- Three detection modes: all magics, Android-only, filesystem-only
- Comprehensive magic database including:
  - Android Boot Image, Vendor Boot Image
  - Various filesystem types (EXT4, F2FS, etc.)
  - Android-specific formats
- Configurable search depth via buffer size
- Special handling for encrypted filesystems

**Example usages:**
```bash
pmt type boot  # Android Boot Image detection
pmt type vendor_boot.img  # Image file analysis
pmt type system.img --only-check-filesystem-magic  # Filesystem only
pmt type userdata --buffer-size 8KB  # Custom search depth
pmt type boot,recovery --only-check-android-magics  # Android formats only
```

---

### Rebooting device
Reboot the device into different modes. Default reboot target is normal mode. When using via ADB terminal, root access is **not** required for this feature. General syntax:
```bash
pmt reboot [rebootTarget] [OPTIONS]
```

**Options:**
- `[rebootTarget]` → Target reboot mode (optional, default: normal reboot).

**Supported Reboot Targets:**
- No argument / empty → Normal reboot
- `recovery` → Reboot to recovery mode
- `download` → Reboot to download mode
- `bootloader` → Reboot to bootloader/fastboot mode

**Technical Details:**
- Uses Android's native reboot system calls
- Works without root privileges when used via ADB
- Direct system integration for reliable reboot operations
- No confirmation required (use with caution)
- Supports all standard Android reboot modes

**Example usages:**
```bash
pmt reboot  # Normal reboot
pmt reboot recovery  # Recovery mode
pmt reboot download  # Download mode
pmt reboot bootloader  # Bootloader/fastboot mode
```

---

### Memory Performance Test
Test sequential read/write performance of your storage device. Random tests coming soon. General syntax:
```bash
pmt memtest [testPath] [OPTIONS]
```

**Options:**
- `[testDirectory]` → Test directory path. Default: `/data/local/tmp`.
- `-s`, `--file-size SIZE` → Size of test file. Default: 1GB.
- `--no-read-test` → Skip read performance test.

**Technical Details:**
- **Sequential Performance Testing**: Measures real-world storage performance
- **Write Test**: Random data generation with synchronized writes (O_SYNC)
- **Read Test**: Direct I/O with page-aligned buffers (O_DIRECT)
- **Default Test Path**: `/data/local/tmp` (excludes FUSE-mounted paths)
- **Size Limitation**: Warns for files >2GB (use `--force` to override)
- **Buffer Management**: 4MB default buffer with random data generation
- **Path Validation**: Prevents testing on FUSE-mounted storage for accuracy
- **Automatic Cleanup**: Test files removed automatically
- **Results**: Performance reported in MB/s with precision formatting

**Example Usages:**
```bash
pmt memtest  # Default 1GB test in /data/local/tmp
pmt memtest /data  # Custom test directory
pmt memtest -s 2GB  # 2GB test file
pmt memtest /data/local/tmp --file-size 512MB --no-read-test  # Write-only test
pmt memtest --file-size 3GB --force  # Override 2GB warning
```

### Cleaning PMT Logs
Remove PMT log files and reset logging system. General syntax:
```bash
pmt clean-logs [OPTIONS]
```

**Technical Details:**
- Removes the current log file specified by `-L/--log-file` or default location
- Resets logging system after cleanup
- Useful for log rotation and privacy
- No additional options required
- Automatic logging reinitialization

**Example Usages:**
```bash
pmt clean-logs  # Remove default log file
pmt clean-logs -L /custom/log/path.log  # Remove custom log file
```

---

### Reading logical partition metadata
Read LP metadata.
```bash
pmt lp-metadata SUBCOMMAND ... [OPTIONS]
```

**Subcommands:**
- `read-groups` → Display metadata for logical partition groups including name, maximum size, and flags.
- `read-partition` → Read detailed metadata for logical partitions including group, size, and attributes.

**Options:**
- `-v`, `--version` → View version of plugin.

#### read-groups
**Example Usages:**
```bash
pmt lp-metadataread-groups  # Display all logical partition groups
```

**Technical Details:**
- Requires device support for dynamic partitions
- Displays all logical partition groups in the system
- Shows group name, maximum size, and flags (e.g., slot_suffixed)
- No arguments required - reads all available groups
- Useful for understanding dynamic partition layout
- Direct access to liblp metadata format

#### read-partition
**Example Usages:**
```bash
pmt lp-metadata read-partition partition(s)
pmt lp-metadata read-partition system  # Read metadata for system partition
pmt lp-metadata read-partition system,vendor  # Read metadata for multiple partitions
pmt lp-metadata read-partition get-all  # Read metadata for all logical partitions
pmt lp-metadata read-partition getvar-all  # Same as get-all
```

**Special Partition Names:**
- `get-all` or `getvar-all` → Read metadata for all logical partitions.

**Technical Details:**
- Requires device support for dynamic partitions
- Displays partition name, group name, size, and attributes
- Attribute flags include: readonly, slot_suffixed, updated, disabled
- Uses ioctl(BLKGETSIZE64) for accurate size detection
- Resolves actual block device paths via symbolic links
- Batch processing support for multiple partitions
- Validates partition existence before reading metadata

---

### GPT Operations
Perform GPT (GUID Partition Table) partition table operations including reading, backing up, and restoring partition tables. General syntax:
```bash
pmt gpt SUBCOMMAND [OPTIONS]
```

**Subcommands:**
- `read-table` → Send a command to the Linux kernel to read the specified partition table(s) (using ioctl()).
- `backup-table` → Backup specified GPT partition table(s) including protective MBR, main and second GPT header and partition entries.
- `restore-table` → Restore specified GPT partition table(s) from backup files.

**Global Options:**
- `-v`, `--version` → View version of plugin.

#### read-table
Send a command to the Linux kernel to read the specified partition table(s) using ioctl(). General syntax:
```bash
pmt gpt read-table table(s) [OPTIONS]
```

**Options:**
- `table(s)` → Names of the partition tables to read (required).

**Special Table Names:**
- `read-all` → Read all partition tables.

**Technical Details:**
- Uses ioctl() system call to re-read partition tables from the kernel
- Closes opened file descriptors by libpartition_map before reading
- Validates table existence before attempting re-read operation
- Resolves actual block device paths (e.g., `/dev/block/sda`)
- Warning: High probability of errors in normal mode (on normal system)
- Recommended to use in recovery mode for safer operation
- Could cause the device to restart suddenly
- Skips failed tables if `--force` flag is used

**Example Usages:**
```bash
pmt gpt read-table sda  # Re-read sda partition table
pmt gpt read-table sda,sdb  # Re-read multiple partition tables
pmt gpt read-table read-all  # Re-read all partition tables
pmt gpt read-table sda --force  # Skip errors and continue
```

#### backup-table
Backup specified GPT partition table(s) including protective MBR, main and second GPT header and partition entries. General syntax:
```bash
pmt gpt backup-table table(s) [OPTIONS]
```

**Options:**
- `table(s)` → Names of the partition tables to backup (required).
- `output(s)` → Names of the backup files to backup (optional, default: `<table_name>.gpt`).
- `-o`, `--output-directory DIR` → Directory to save GPT backup(s).

**Special Table Names:**
- `read-all` → Backup all partition tables.

**Technical Details:**
- Creates complete backup of GPT structure including:
  - Protective MBR (Master Boot Record)
  - Main GPT header
  - Secondary GPT header
  - Partition entries
- Default output naming: `<table_name>.gpt`
- Validates output file existence (overwrites only with `--force`)
- Custom output names must match the number of tables provided
- Directory support for organized backup storage
- Error isolation: failed backups don't affect other tables

**Example Usages:**
```bash
pmt gpt backup-table sda  # Backup sda to sda.gpt
pmt gpt backup-table sda,sdb  # Backup multiple tables
pmt gpt backup-table sda custom_backup.gpt  # Custom filename
pmt gpt backup-table sda,sdb backup1.gpt,backup2.gpt  # Custom names for multiple
pmt gpt backup-table sda -o /backups  # Save to directory
pmt gpt backup-table read-all -o /backups  # Backup all tables to directory
```

#### restore-table
Restore specified GPT partition table(s) from backup files. General syntax:
```bash
pmt gpt restore-table table(s) file(s) [OPTIONS]
```

**Options:**
- `table(s)` → Names of the partition tables to restore (required).
- `file(s)` → Names of the backup files to restore (required).
- `-F`, `--file-directory DIR` → Directory to restore GPT backup(s) from.

**Special Table Names:**
- `read-all` → Restore all partition tables.

**Technical Details:**
- Loads GPT backup files and writes to specified partition tables
- Automatically syncs changes to disk after loading
- Validates table existence before restore operation
- File names must match the number of tables provided
- Directory support for organized backup storage
- Error isolation: failed restores don't affect other tables
- Skips failed tables if `--force` flag is used
- Writes and syncs changes immediately after loading backup

**Example Usages:**
```bash
pmt gpt restore-table sda sda.gpt  # Restore sda from backup
pmt gpt restore-table sda,sdb sda.gpt,sdb.gpt  # Restore multiple tables
pmt gpt restore-table sda backup.gpt -F /backups  # Restore from directory
pmt gpt restore-table sda,sdb backup1.gpt,backup2.gpt -F /backups  # Multiple from directory
pmt gpt restore-table sda backup.gpt --force  # Skip errors and continue
```

---

---

## Additional Notes

- **Comma-separated inputs**: All commands (except `reboot`) require multiple inputs to be separated by commas **without spaces**.
- **Asynchronous execution**: For `backup`, `flash`, and `erase`, each partition is processed in a separate thread for maximum performance.
- **Error isolation**: A failure in processing one partition will not cancel the others (applies to back up, flash, and erase operations).
- **Automatic partition detection**: The tool automatically determines whether a partition is logical or regular. Use `-l/--logical` flag to specify logical partitions explicitly.
- **Root access requirement**: Root access is required for partition operations. Reboot command works without root when used via ADB.
- **Plugin system**: Supports loading external plugins via `-p/--plugins` or `-d/--plugin-directory` options.
- **Logging**: Detailed logging available with `-V/--verbose` and custom log file paths via `-L/--log-file`.
- **Signal handling**: Gracefully handles SIGINT (Ctrl+C) and SIGABRT signals.

## Architecture Overview

**Partition Manager Tool (PMT)** is built with a modular plugin architecture:

- **Core System**: C++20-based main application with CLI11 command-line interface
- **Plugin Framework**: Dynamic loading of functionality via plugin system
- **Built-in Plugins**: 12 core plugins providing all major operations
- **Asynchronous Processing**: Multi-threaded operations for backup, flash, and erase
- **Error Handling**: Comprehensive error isolation and recovery
- **Logging System**: Detailed logging with configurable output destinations

### Built-in Plugins
- **BackupPlugin**: Partition backup with SHA-256 verification, permission management, and async processing
- **FlashPlugin**: Image flashing with size validation, automatic cleanup, and parallel operations
- **ErasePlugin**: Secure partition erasure with zero-fill, confirmation prompts, and safety warnings
- **InfoPlugin**: Comprehensive partition information with JSON output, custom field names, and batch queries
- **PartitionSizePlugin**: Size queries in multiple units with special partition selectors
- **RealPathPlugin**: Block device path resolution with symbolic link support
- **TypePlugin**: Magic number-based type detection with Android and filesystem-specific modes
- **RebootPlugin**: Device reboot management supporting all Android reboot modes
- **MemoryTestPlugin**: Sequential storage performance testing with direct I/O and synchronized writes
- **CleanLogPlugin**: Log management utilities for cleanup and rotation
- **MetadataReaderPlugin**: Logical partition metadata reader with group, size, and attribute information
- **GroupMetadataReaderPlugin**: Logical partition groups metadata reader with name, maximum size, and flags
- **ReReadTablePlugin**: Re-read partition tables

---

## Comma Usage Guidelines

In **Partition Manager Tool**, whenever you provide **multiple partitions**, **multiple image files**, or **multiple output file names**, they **must** be separated by commas (`,`), without spaces.

✅ **Correct:**
```bash
pmt backup boot,recovery
pmt flash boot,recovery boot.img,recovery.img
```

❌ **Incorrect:**
```bash
pmt backup boot-recovery
pmt flash boot-recovery boot.img-recovery.img
```

The **number of items must match** when providing both input and output lists.  
For example, if you specify 3 partitions, you must also provide 3 output file names.

This rule applies to **all commands except `reboot`**, since `reboot` only takes one optional argument.

