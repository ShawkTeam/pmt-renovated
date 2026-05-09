@page example-plugins Example Plugins (Writing plugins)

# Example Plugins (Writing plugins)

This page contains documentation and notes about the example plugins provided with PMT Renovated. These plugins serve as reference implementations and learning resources for developers who want to create their own plugins for the Partition Manager Tool.

## Table of Contents
- [Overview](#overview)
- [Getting Started](#getting-started)
- [ExamplePlugin - Basic Plugin](#exampleplugin---basic-plugin)
- [AdvancedExamplePlugin - Advanced Features](#advancedexampleplugin---advanced-features)
- [AsyncExamplePlugin - Asynchronous Operations](#asyncexampleplugin---asynchronous-operations)
- [Best Practices](#best-practices)
- [Plugin Development Workflow](#plugin-development-workflow)
- [Common Patterns](#common-patterns)
- [Troubleshooting](#troubleshooting)
- [Next Steps](#next-steps)

## Overview

The example plugins are located in the `docs/plugin-instructions/` directory and demonstrate various aspects of plugin development:

- **ExamplePlugin.cpp** - Basic plugin functionality
- **SimplePlugin.cpp** - Super simple plugin functionally
- **AdvancedExamplePlugin.cpp** - Complex plugin with advanced features
- **AsyncExamplePlugin.cpp** - Asynchronous operations and parallel processing

Each plugin is fully functional and can be compiled and used with PMT Renovated. They are designed to be educational examples that showcase best practices and common patterns in plugin development.

## Getting Started

To use these example plugins as a starting point for your own plugin development:

1. Copy one of the example files to your plugin directory
2. Rename the class and plugin constants to match your plugin name
3. Modify the CLI command and options to fit your needs
4. Implement your plugin's core functionality in the `run()` method
5. Update the plugin registration at the bottom of the file

> **Note**: All example plugins include comprehensive documentation comments that explain each method and its purpose. Use these as a reference for understanding the plugin API.

## ExamplePlugin - Basic Plugin

The `ExamplePlugin` demonstrates fundamental plugin concepts:

### Key Features
- **CLI Integration**: Shows how to create subcommands and add options
- **Partition Access**: Demonstrates accessing partition information
- **Output Formats**: Supports both text and JSON output
- **Error Handling**: Basic error handling and validation
- **Logging**: Shows how to use the logging system

### Usage Examples

```bash
# Show all partitions
pmt example

# Show specific partitions
pmt example system,userdata,cache

# Show partition count only
pmt example --count

# Show detailed information in JSON format
pmt example --detailed --json
```

### Code Structure

The plugin follows this basic structure:

- `onLoad()` - Initialize CLI options and store references
- `onUnload()` - Clean up resources
- `used()` - Check if the plugin was invoked
- `run()` - Main plugin execution logic
- `getName()`/`getVersion()` - Plugin metadata

### Learning Points

- How to inherit from `BasicPlugin`
- CLI option creation and validation
- Partition enumeration and filtering
- Output formatting in multiple formats
- Basic error handling patterns

## AdvancedExamplePlugin - Advanced Features

The `AdvancedExamplePlugin` showcases more complex plugin development patterns:

### Key Features
- **Option Groups**: Organized CLI options with validation
- **Advanced Filtering**: Size-based and type-based partition filtering
- **Sorting**: Multiple sorting criteria with reverse order
- **File Output**: Write results to files instead of stdout
- **Size Units**: Configurable size display units
- **Complex Validation**: Cross-option validation and error handling

### Usage Examples

```bash
# Filter partitions by size
pmt advanced --min-size 1GB --max-size 10GB

# Sort partitions by size in descending order
pmt advanced --sort-by size --reverse

# Analyze only logical partitions with JSON output
pmt advanced --logical --json

# Include table information and output to file
pmt advanced --tables --output analysis.txt

# Use different size units
pmt advanced --unit GiB --sort-by size
```

### Advanced Code Structure

The plugin demonstrates advanced patterns:

- **Option Groups**: Logical grouping of related CLI options
- **Input Validation**: Cross-option dependency checking
- **Data Processing Pipeline**: Filter → Sort → Output workflow
- **Flexible Output**: File or stdout with format selection
- **Enum Parsing**: Converting string inputs to enum values

### Learning Points

- Complex CLI option organization with groups
- Input validation and error handling
- Data processing pipelines
- File I/O operations
- String-to-enum conversion patterns

## AsyncExamplePlugin - Asynchronous Operations

The `AsyncExamplePlugin` demonstrates how to implement asynchronous operations:

### Key Features
- **Async Processing**: Parallel partition processing
- **Progress Reporting**: Real-time progress updates
- **Error Handling**: Async error collection and reporting
- **Work Simulation**: Configurable delays for testing
- **Result Aggregation**: Collect and summarize async results

### Usage Examples

```bash
# Process all partitions asynchronously
pmt async-example

# Process specific partitions with simulated work
pmt async-example system,userdata --simulate --delay 2000

# Fast processing without simulation
pmt async-example cache,boot --simulate=false
```

### Async Code Structure

Key async patterns demonstrated:

- **AsyncManager**: Using the Helper::AsyncManager for parallel processing
- **AsyncResult_t**: Handling async success and error results
- **Task Creation**: Adding async tasks to the manager
- **Result Processing**: Collecting and processing async results

### Learning Points

- How to use AsyncManager for parallel operations
- Creating async worker functions
- Handling async results and errors
- Progress reporting in async operations
- Performance considerations for async processing

## Best Practices

Based on the example plugins, here are recommended best practices:

### Logging
- Use `LOGNF(PLUGIN, logPath, INFO)` for informational messages
- Log plugin lifecycle events (load/unload)
- Include relevant context in log messages

### Error Handling
- Use `PluginError` for plugin-specific errors
- Validate inputs early in the execution flow
- Provide clear, actionable error messages
- Use CLI validation for command-line options

### CLI Design
- Group related options logically
- Provide help text for all options
- Use sensible defaults where appropriate
- Validate option combinations

### Output Design
- Support multiple output formats when useful
- Use consistent formatting
- Provide both human-readable and machine-readable options
- Consider file output for large results

### Performance
- Use async operations for I/O-bound tasks
- Avoid unnecessary work in the `run()` method
- Cache expensive operations when possible
- Consider memory usage for large datasets

## Plugin Development Workflow

Follow these steps when developing a new plugin:

1. **Planning**: Define the plugin's purpose and requirements
2. **Template Selection**: Choose the most appropriate example as a starting point
3. **CLI Design**: Plan the command structure and options
4. **Implementation**: Write the core functionality
5. **Testing**: Test with various inputs and edge cases
6. **Documentation**: Add comprehensive comments and help text
7. **Integration**: Update the build system if needed

## Common Patterns

### Partition Access Patterns

```cpp
// Check if partition exists
if (Tables.hasPartition(name)) {
    const auto& partition = Tables.partitionWithDupCheck(name)->get();
    // Use partition
}

// Iterate over all partitions
Tables.forEach([](const PartitionMap::Partition_t& partition) -> bool {
    // Process partition
    return true; // Continue iteration
});

// Get partition lists
auto allParts = Tables.allPartitions();
auto logicalParts = Tables.logicalPartitions();
auto physicalParts = Tables.partitions();
```

### Output Format Patterns

```cpp
// JSON output
nlohmann::json j;
j["key"] = "value";
j["array"] = nlohmann::json::array();
Out::println("{}", j.dump(2));

// Formatted text output
Out::println("Name: {} | Size: {}", name, size);

// File output
std::ofstream file(outputFile);
if (file) {
    file << content;
    Out::println("Output written to: {}", outputFile);
}
```

### Error Handling Patterns

```cpp
// Validation errors
if (invalidCondition) {
    throw CLI::ValidationError("Invalid option combination");
}

// Plugin errors
if (partitionNotFound) {
    throw PluginError("Partition not found: {}", name);
}

// File I/O errors
if (!file) {
    throw PluginError("Failed to write to file: {}", filename);
}
```

## Troubleshooting

### Compilation Issues
- Ensure all required headers are included
- Check that the plugin registration macro is correct
- Verify namespace usage
- Check for missing dependencies (CLI11, nlohmann/json)

### Runtime Issues
- Check plugin loading in logs
- Verify CLI option parsing
- Ensure partition names are correct
- Check file permissions for file output

### Async Issues
- Ensure async functions are properly marked
- Check for race conditions in shared data
- Verify result processing logic
- Monitor async operation completion

## Next Steps

After reviewing these examples:

1. **Study the Code**: Read through each example plugin completely
2. **Experiment**: Modify the examples to understand how they work
3. **Start Simple**: Begin with a basic plugin based on ExamplePlugin
4. **Graduate**: Move to more complex features as needed
5. **Contribute**: Consider contributing your plugins back to the project

> **Note**: These examples are maintained as part of the PMT Renovated project and are updated to reflect current best practices and API changes. Always refer to the latest version when developing new plugins.
