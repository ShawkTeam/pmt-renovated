@mainpage PMT Renovated

## Introduction
PMT Renovated is a fast, reliable, and feature-rich CLI application for Android devices that enables advanced partition operations such as backup, flashing, erasing, information retrieval, and more.

This tool provides a comprehensive solution for managing Android device partitions with a focus on speed, reliability, and user experience.

## Features

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


## Project Structure
The project is organized into several key components:

- **Core Library**: Main partition management logic
- `Helper` **Library**: Utility functions and common operations
- `PartitionMap` **Library**: Partition table parsing and management
- **Plugin System**: Extensible plugin architecture
- **CLI Interface**: Command-line interface for user interaction

## Contributing
We welcome contributions! Please see our [Contributing Guide](https://github.com/ShawkTeam/pmt-renovated/blob/main/CONTRIBUTING.md) for details.

## License
This project is licensed under the MIT License - see the [LICENSE](https://github.com/ShawkTeam/pmt-renovated/blob/main/LICENSE) file for details.

## Support
For support and questions:
- GitHub Issues: [Report issues](https://github.com/ShawkTeam/pmt-renovated/issues)

---
