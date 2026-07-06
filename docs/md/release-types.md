@page release-types Release Types, Etc.

This project provides multiple release configurations tailored for specific Android device architectures and usage preferences. The build system supports both dynamic and static linking, with optional built-in plugin support.

---

## Release Configurations

### Build Types

| Configuration          | Architecture      | Bitness | Library Type  | Plugin Support   | Description                                                               |
|------------------------|-------------------|---------|---------------|------------------|---------------------------------------------------------------------------|
| **Dynamic**            | ARM64 (arm64-v8a) | 64-bit  | Dynamic (.so) | External plugins | Uses shared libraries. Requires `libhelper.so` and `libpartition_map.so`. |
| **Static**             | ARM64 (arm64-v8a) | 64-bit  | Static (.a)   | External plugins | Fully self-contained executable with no external dependencies.            |
| **Dynamic + Built-in** | ARM64 (arm64-v8a) | 64-bit  | Dynamic (.so) | Built-in plugins | Dynamic linking with plugins compiled directly into the binary.           |
| **Dynamic**            | ARM (armeabi-v7a) | 32-bit  | Dynamic (.so) | External plugins | Uses shared libraries. Requires `libhelper.so` and `libpartition_map.so`. |
| **Dynamic + Built-in** | ARM (armeabi-v7a) | 32-bit  | Dynamic (.so) | Built-in plugins | Dynamic linking with plugins compiled directly into the binary.           |
| **Static + Built-in**  | ARM (armeabi-v7a) | 32-bit  | Static (.a)   | Built-in plugins | Fully static with built-in plugins - the most portable option.            |

### Release Files

Typical release files follow this naming pattern:
- `pmt-{architecture}.zip` - Dynamic linking, external plugins
- `pmt-static-{architecture}.zip` - Static linking, external plugins  
- `pmt-builtin-{architecture}.zip` - Dynamic linking, built-in plugins
- `pmt-static-builtin-{architecture}.zip` - Static linking, built-in plugins

---

## Architecture & Bitness Explained

- **ARM64 (arm64-v8a)**:  
  This is a 64-bit architecture used by newer Android devices. It can handle larger amounts of memory and generally runs faster for heavy tasks.

- **ARM (armeabi-v7a)**:  
  This is a 32-bit architecture common on older or less powerful Android devices. It has some limitations compared to 64-bit but is still widely supported.

---

## Core Libraries

The project relies on two core libraries:
- **libhelper** - File management, logging, and utility functions
- **libpartition_map** - Partition table parsing and management

### Dynamic Linking (`.so` files)

- In dynamic builds, these libraries are **compiled as shared objects (`.so` files)**.
- The main program (`pmt`) **depends on these libraries being present** on the device or alongside the executable.
- Missing libraries will cause the program to fail to start.
- Recommended for development or when multiple applications can share the same libraries.

### Static Linking (`.a` files)

- Static builds **include these libraries inside the main executable**.
- The `pmt` binary is **completely self-contained** with **no external dependencies**.
- Ideal for general users and ADB usage where installing separate libraries is inconvenient.

---

## Plugin System

PMT supports a modular plugin architecture for extending functionality.

### External Plugins

- **Dynamic builds**: Plugins are loaded as separate `.so` files at runtime.
- **Static builds**: Plugins can be built as separate static libraries and linked.
- Requires plugin files to be distributed alongside the main executable.
- Allows for runtime plugin discovery and loading.

### Built-in Plugins

- Plugins are **compiled directly into the main executable** using the `BUILTIN_PLUGINS` option.
- No external plugin files needed - everything is self-contained.
- Uses custom linker scripts for proper plugin initialization.
- Provides the most portable and hassle-free experience.
- Available plugins include:
  - **BackupPlugin** - Partition backup functionality
  - **CleanLogPlugin** - Log cleaning utilities
  - **InfoPlugin** - Partition information display
  - And more...

### Plugin Loading

- Built-in plugins are automatically initialized during program startup.
- External plugins are discovered and loaded dynamically from the plugin directory.
- The plugin system supports asynchronous operations and custom result types.

---

## Which Should You Use?

### For General Users
- **Static + Built-in**: Most portable option, no external dependencies, everything included.
- **Static**: Self-contained executable but requires separate plugin files.

### For Developers & Advanced Users
- **Dynamic**: Allows for easier debugging and library sharing between applications.
- **Dynamic + Built-in**: Easier plugin development while maintaining dynamic linking for core libraries.

### Recommendations by Use Case

| Use Case                  | Recommended Configuration         |
|---------------------------|-----------------------------------|
| **ADB Usage**             | Static + Built-in (most portable) |
| **Development**           | Dynamic (easier debugging)        |
| **Multiple Applications** | Dynamic (shared libraries)        |
| **Minimal Dependencies**  | Static + Built-in                 |
| **Plugin Development**    | Dynamic + Built-in                |

---

## Build System

### Modern C++20 Toolchain
- Uses **CMake 3.20+** with **Ninja** generator for fast builds
- **C++20** standard with modern features (concepts, modules, etc.)
- Supports both **Android NDK** cross-compilation and native builds

### Build Options
- `BUILTIN_PLUGINS=ON/OFF` - Enable/disable built-in plugin compilation
- `CMAKE_BUILD_TYPE=Debug/Release` - Build configuration
- `ANDROID_PLATFORM` - Minimum Android API level (default: android-30)

### Automated Build Script
The `build/scripts/build.sh` script provides:
- Multi-architecture compilation (arm64-v8a, armeabi-v7a)
- Parallel builds using available CPU cores
- Clean and rebuild operations
- Git hook configuration

---

## Recent Changes (v1.7.0)

### Core Improvements
- **Enhanced Plugin System**: Better plugin discovery and loading mechanisms
- **Modern C++ Features**: Utilizes C++20 concepts and modern library features
- **Improved Build System**: Better CMake integration and Android build support
- **File Management**: New `UniqueFD` and file pointer management classes
- **Logging System**: Enhanced logging with content transfer capabilities

### Library Enhancements
- **libhelper**: Improved file descriptor management, better logging infrastructure
- **libpartition_map**: Cleaner builder patterns, header-only optimizations
- **Plugin Architecture**: Custom result holders, async operation support

### Build & Deployment
- **Android Integration**: Updated Android.bp files for better system integration
- **CMake Improvements**: Better header generation and plugin management
- **Documentation**: Enhanced build and usage documentation

---

## Summary

| Configuration      | Dependencies        | Portability | Best For         |
|--------------------|---------------------|-------------|------------------|
| Static + Built-in  | None                | ★★★★★       | General use, ADB |
| Static             | Plugins only        | ★★★★☆       | General use      |
| Dynamic + Built-in | Core libs           | ★★★☆☆       | Development      |
| Dynamic            | Core libs + plugins | ★★☆☆☆       | Development      |

---

If you are unsure which one to pick, try the **Static + Built-in** version first — it works out of the box on all supported devices with the most complete feature set.
