# Contributing to Partition Manager Tool (PMT)

Thank you for your interest in contributing to PMT! This document provides guidelines and information for contributors.

## Getting Started

### Prerequisites

- **Android NDK**
- **CMake 3.20 or higher**
- **Ninja**
- **Git**
- **Doxygen** (for documentation generation)

### Clone the Repository

```bash
git clone https://github.com/ShawkTeam/pmt-renovated.git
cd pmt-renovated
git submodule update --init --recursive
```

## Development Setup

### Build Dependencies

The project uses several external libraries managed as Git submodules:

- **CLI11** - Command line interface
- **PicoSHA2** - SHA256 hash generation
- **nlohmann/json** - JSON parsing and generation
- **libbase, etc.** - Android system libraries

### Build Configuration

```bash
# Using build scripts
bash build/scripts/build.sh build

### OR ###

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release -G Ninja \
         -DCMAKE_TOOLCHAIN_FILE="<ANDROID_NDK_PATH>/build/cmake/android.toolchain.cmake" \
         -DANDROID_ABI="arm64-v8a" \ # Example ABI
         -DANDROID_PLATFORM="android-30" \ # Example platform
         -DANDROID_STL=c++_static

# Or for development with debug symbols
cmake .. -DCMAKE_BUILD_TYPE=Debug -G Ninja \
         -DCMAKE_TOOLCHAIN_FILE="<ANDROID_NDK_PATH>/build/cmake/android.toolchain.cmake" \
         -DANDROID_ABI="arm64-v8a" \ # Example ABI
         -DANDROID_PLATFORM="$ANDROID_PLATFORM" \ # Example platform
         -DANDROID_STL=c++_static
```

## Code Style

### Formatting

This project uses **clang-format** for consistent code formatting. The configuration is defined in `.clang-format`.

To format your code:

```bash
# Format all source files
bash build/scripts/reformat.sh
```

### Coding Standards

- **Language**: Use **C++20** features where appropriate
- **Naming Conventions**:
  - Classes: `PascalCase`
  - Functions: `camelCase`
  - Variables: `snake_case`
  - Constants: `UPPER_SNAKE_CASE`
- **Comments**: Document public APIs with Doxygen-compatible comments

### Static Analysis

The project uses **clang-tidy** for static analysis. Configuration is in `.clang-tidy`.

```bash
# Run clang-tidy
clang-tidy src/**/*.cpp -- -I./include
```

## Building the Project

### Standard Build

```bash
# Or use ninja if available
ninja
```

### Build Options

- `BUILTIN_PLUGINS=ON` - Build plugins as built-in instead of dynamic
- `CMAKE_BUILD_TYPE=Debug` - Enable debug symbols and disable optimizations

## Testing

### Running Tests

It is recommended that you push the generated binary (and any other necessary components, if any) to the device using adb and test it.

```bash
adb push <path_to_binary> /data/local/tmp
adb shell /data/local/tmp/<binary_name>
```

### Writing Tests

- Add new tests in the `tests/` directories
- Use the project's testing framework
- Ensure tests cover both success and failure cases

## Submitting Changes

### Commit Messages

Follow the [Conventional Commits](https://www.conventionalcommits.org/) specification:

```
type(scope): description

[optional body]

[optional footer(s)]
```

Examples:
- `feat(BackupPlugin): add async partition backup support`
- `fix(cli): resolve crash when invalid device path provided`
- `docs(using-with-termux): update installation instructions`

### Pull Request Process

1. **Fork** the repository
2. **Create** a feature branch
3. **Make** your changes following the code style guidelines
4. **Test** your changes thoroughly
5. **Commit** your changes with clear messages
6. **Push** to your fork
7. **Create** a Pull Request

### Pull Request Requirements

- **Code must compile** without warnings on all supported platforms
- **Tests must pass** 
- **Documentation must be updated** if applicable
- **Code must be formatted** with clang-format
- **Changes must be logical** and focused on a single feature/fix

## Bug Reports

When reporting bugs, please include:

- **Operating system** and version
- **Compiler** and version
- **PMT version** or commit hash
- **Steps to reproduce** the issue
- **Expected vs actual behavior**
- **Relevant logs** or error messages
- **Device information** (if hardware-related)

Use the [GitHub Issues](https://github.com/ShawkTeam/pmt-renovated/issues) page with the provided bug report template.

## Feature Requests

For new features:

1. **Check existing issues** to avoid duplicates
2. **Provide a clear description** of the feature
3. **Explain the use case** and why it's valuable
4. **Consider implementation complexity**
5. **Be willing to contribute** the feature if possible

## Documentation

### Code Documentation

- Use **Doxygen** comments for public APIs
- Document function parameters and return values
- Include usage examples for complex functions

### Building Documentation

```bash
# Generate documentation
doxygen Doxyfile

# View documentation
firefox docs/html/index.html
```

### Wiki Contributions

The project wiki is hosted at [documentations.yzbruh.com.tr/pmt-renovated](https://documentations.yzbruh.com.tr/pmt-renovated). 

Documentation updates are automatically deployed when changes are pushed to the main branch.

## Development Workflow

### Before Starting

1. **Read existing code** to understand patterns
2. **Check the project roadmap** if available
3. **Discuss major changes** in an issue first

### During Development

1. **Commit frequently** with meaningful messages
2. **Test incrementally** as you develop
3. **Run static analysis** regularly
4. **Keep changes small** and focused

### Before Submitting

1. **Rebase** your branch if needed
2. **Resolve all conflicts**
3. **Run full test suite**
4. **Check code formatting**
5. **Update documentation**

## Getting Help

- **GitHub Issues**: For bug reports and feature requests
- **Discussions**: For general questions and ideas
- **Wiki**: For detailed documentation and guides

## License

By contributing to this project, you agree that your contributions will be licensed under the **GNU GPLv3**, the same license as the project.

---

Thank you for contributing to PMT! Your contributions help make this tool better for everyone.
