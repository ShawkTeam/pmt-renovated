@page building-from-source Building PMT From Source

This guide provides comprehensive instructions for building PMT (Partition Management Tool) from source code on Linux systems.

## Table of Contents
- [Prerequisites](#prerequisites)
- [Environment Setup](#environment-setup)
- [Repository Setup](#repository-setup)
- [Build System Overview](#build-system-overview)
- [Building PMT](#building-pmt)
- [Build Configuration](#build-configuration)
- [Troubleshooting](#troubleshooting)
- [Development Workflow](#development-workflow)

## Prerequisites

### System Requirements
- **Operating System**: Linux (any distribution)
- **Processor**: x86_64 architecture
- **Memory**: Minimum 250MB free RAM
- **Storage**: 500MB free disk space for build artifacts

### Required Software

#### Android NDK
- **Recommended Version**: r28c (or latest stable)
- **Download**: [Android NDK Downloads](https://developer.android.com/ndk/downloads)

#### Development Tools
Choose the appropriate package manager for your distribution:

**Arch Linux / Arch-based:**
```bash
sudo pacman -S git cmake ninja python
```

**Fedora / Fedora-based:**
```bash
sudo dnf install git cmake ninja-build python3
```

**Debian / Ubuntu / Debian-based:**
```bash
sudo apt install git cmake ninja-build python3
```

**openSUSE / SUSE-based:**
```bash
sudo zypper install git cmake ninja python3
```

## Environment Setup

### Installing Android NDK

1. **Download the NDK**:
   ```bash
   wget https://dl.google.com/android/repository/android-ndk-r28c-linux.zip
   ```

2. **Extract to a permanent location**:
   ```bash
   unzip android-ndk-r28c-linux.zip -d ~/Development/
   ```

3. **Set environment variable**:
   ```bash
   export ANDROID_NDK="$HOME/Development/android-ndk-r28c"
   ```

4. **Persist the environment variable** (optional):
   Add to your shell configuration file (`~/.bashrc` or `~/.zshrc`):
   ```bash
   echo 'export ANDROID_NDK="$HOME/Development/android-ndk-r28c"' >> ~/.bashrc
   source ~/.bashrc
   ```

> **Note**: Extract via command line rather than file managers. Some file managers (like Dolphin) don't create proper symbolic links, causing NDK clang to malfunction.

### Optional Environment Variables
```bash
# Set minimum Android API level (default: android-22)
export ANDROID_PLATFORM="android-22"
```

## Repository Setup

### Cloning the Repository

PMT uses a single development branch. For stable builds, use tagged releases:

```bash
# Clone main branch (development version)
git clone -b main https://github.com/ShawkTeam/pmt-renovated

# Initialize submodules
cd pmt-renovated
git submodule update --init --recursive
```

### Alternative Checkout Methods

**Clone and switch to specific tag:**
```bash
git clone https://github.com/ShawkTeam/pmt-renovated
cd pmt-renovated
git submodule update --init --recursive
git switch 20260503  # Example tag
```

**List available tags:**
```bash
git tag --sort=-version:refname
```

## Build System Overview

PMT uses a sophisticated build system with the following components:

### Modern C++20 Toolchain
- Uses **CMake 3.20+** with **Ninja** generator for fast builds
- **C++20** standard with modern features (concepts, modules, etc.)
- Supports both **Android NDK** cross-compilation and native builds

### Build Options
- `BUILTIN_PLUGINS=ON/OFF` - Enable/disable built-in plugin compilation
- `LINK_TIME_OPTIMIZATION_THIN=ON/OFF` - Enable/disable link time optimization (thin)
- `CMAKE_BUILD_TYPE=Debug/Release` - Build configuration
- `ANDROID_PLATFORM` - Minimum Android API level (default: android-22)

### ClassicPartitionData & Helper Scripts (`build/scripts/`)

#### `build.sh` - Main Build Script
- **Purpose**: Orchestrates the entire build process
- **Features**: Multi-architecture support, dependency checking, git hooks configuration
- **Default Architectures**: `arm64-v8a`, `armeabi-v7a`
- **Build Types**: Standard and builtin variants

#### `reformat.sh` - Code Formatting
- **Purpose**: Maintains consistent code style using clang-format
- **Features**: Directory exclusion support, file change detection

#### `android_genrule.sh` - Build Information Generation
- **Purpose**: Generates build lpMetadata headers
- **Information**: Version, build date, compiler info, commit ID

### CMake Configuration (`build/cmake/`)

#### `functions.cmake` - Custom CMake Functions
- **Shared library creation utilities**
- **Build configuration management**
- **Interface library handling**

#### `interface.cmake` - Build Interface
- **Cross-compilation setup**
- **Android platform configuration**

#### Linker Scripts (`build/ld/`)
- **Module linking configuration**
- **Builtin module support**

## Building PMT

### Basic Build

```bash
# Navigate to repository root
cd pmt-renovated

# Build with default settings
bash build/scripts/build.sh build
```

### Build Output

The build process creates directories for each architecture:
- `build_arm64-v8a/` - Standard ARM64 build
- `build_arm64-v8a-builtin/` - ARM64 build with builtin plugins
- `build_armeabi-v7a/` - Standard ARM32 build
- `build_armeabi-v7a-builtin/` - ARM32 build with builtin plugins

### Build Artifacts

Each build directory contains:
- **PMT Binary**: `pmt` or `pmt_static`
- **Shared Libraries**: `libhelper.so`, `libpartition_map.so`
- **Plugin Libraries**: Various `.so` files for functionality modules

## Build Configuration

### Architecture Selection

**Default architectures**: `arm64-v8a`, `armeabi-v7a`

**Override with command line**:
```bash
# Build single architecture
bash build/scripts/build.sh build --arch arm64-v8a

# Build multiple architectures
bash build/scripts/build.sh build --arch arm64-v8a --arch x86_64

# Supported ABIs: arm64-v8a, armeabi-v7a, x86_64, x86
```

### Build Commands

| Command                    | Description                    |
|----------------------------|--------------------------------|
| `build`                    | Standard build                 |
| `rebuild`                  | Clean then build               |
| `clean`                    | Remove build artifacts         |
| `cleanup-generated-docs`   | Remove generated documentation |

### Advanced Configuration

**Custom CMake options**:
```bash
bash build/scripts/build.sh build -DCMAKE_BUILD_TYPE=Debug -DENABLE_TESTING=ON
```

**Custom working directory**:
```bash
bash build/scripts/build.sh build --working-directory /path/to/source
```

**Thread count optimization**:
Build uses `$(nproc - 2)` threads by default. Modify in `build.sh`.

## Troubleshooting

### Common Issues

**NDK not found**:
```bash
export ANDROID_NDK="/path/to/android-ndk"
```

**Missing dependencies**:
```bash
# Verify installations
which cmake ninja python3 git
```

**Git hooks not configured**:
```bash
bash build/scripts/build.sh only-configure-git-hooks
```

**Build fails with permission errors**:
```bash
chmod +x build/scripts/*.sh
```

### Clean Build

To start fresh:
```bash
bash build/scripts/build.sh clean
```

## Development Workflow

### Code Formatting

Format code before committing:
```bash
bash build/scripts/reformat.sh
```

**Custom ignore file**:
```bash
bash build/scripts/reformat.sh --ignore-file /path/to/.clang-format-ignore
```

### Git Hooks

The build system automatically configures git hooks for:
- Pre-commit formatting checks
- Commit message validation
- Submodule synchronization

### Build Information

Build lpMetadata is automatically generated and includes:
- Version from CMakeLists.txt
- Build date and time
- Compiler information
- Git commit ID
- Build flags

### Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Format code with `reformat.sh`
5. Test your build
6. Submit a pull request

Your contributions are valuable to the PMT project!
