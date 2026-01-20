<h1 align="center">Project Lucario</h1>
<p align="center">« Lógica Unitaria de Control y Arquitectura de Recursos e Instrucciones Organizadas »</p>

<p align="center">
<a href="docs/ARCHITECTURE.md"><img src="https://img.shields.io/badge/Architecture-BD93F9?style=for-the-badge"></a>
<a href="#build-and-usage"><img src="https://img.shields.io/badge/build%20and%20usage-FF79C6?style=for-the-badge"></a>
<a href="lib/"><img src="https://img.shields.io/badge/dependencies-BD93F9?style=for-the-badge"></a>

## Description

This is a university programming project for the **Operating Systems** course at **Universidad Central de Venezuela**. The goal of this project (Phase I) is to implement a virtual architecture running on native Linux. This software simulates the hardware layer including an accumulator-based processor, a memory management system, and a DMA controller to support the future implementation of a minikernel.

## Key Features

- **Dual-Mode Processor:** Supports both Privileged (Kernel) and User execution modes.
- **Virtual Memory:** Simulation of 2000 memory positions with protection registers (RB/RL).
- **I/O System:** Full simulation of a shared bus, DMA controller, and a geometric disk structure (Tracks/Cylinders/Sectors).
- **Execution Modes:** Runs in **Normal** mode for standard execution and **Debugger** mode for step-by-step instruction analysis.

## Build

The project includes a `Makefile` to automate the compilation, execution, and testing processes. Below are the available commands to manage the project lifecycle.

### 1. Compilation

You can compile the project in two different modes:

- **Normal Mode:** Compiles the project with standard flags. The executable will be generated at `bin/project_lucario`.

    ```bash
    make
    ```

- **Debug Mode:** Compiles the project with debug flags to display additional messages and information that would not be visible in a standard compilation.

    ```bash
    make debug
    ```

### 2. Execution

To run the project after compilation without typing the full path to the binary:

```bash
make run
```

> **Note:** If the executable is not found, this command will alert you to compile the project first.

### 3. Testing Modules

The project allows running isolated unit tests for specific modules (e.g., cpu, memory). The test files must be located in the `test/` directory and follow the naming convention `test_<module_name>.c`. To run a test, specify the module name using the `mod` variable:

```bash
# Example: If you have test/test_cpu.c
make test mod=cpu
```

To run all available tests sequentially, use:

```bash
make test mod=all
```

### 4. Cleaning

To remove all compiled object files (`.o`) and executables (useful for a clean rebuild):

```bash
make clean
```

**Note:** It is highly recommended to run `make clean` before switching between **Normal Mode** and **Debug Mode** to ensure all components are rebuilt correctly with the appropriate flags.

## Usage

Upon execution, the system launches the **Interactive Console**, indicated by the `LUCARIO >` prompt.

### Main Commands

| Command | Description |
| :--- | :--- |
| `RUN <file>` | Loads and executes the specified program in **Normal Mode** (Continuous execution). |
| `DEBUG <file>` | Loads and starts the program in **Debug Mode** (Step-by-Step). |
| `LIST` | Lists available files in the current directory. |
| `COMANDS` | Displays the help menu with available commands. |
| `EXIT` | Shuts down the virtual machine and cleans up resources. |

> **Note:** The file must exist in the project directory or provide a valid path.

### Debug Mode
When you run `DEBUG <filename>`, the system enters an interactive inspection session indicated by the `DEBUG >` prompt.

| Command | Description |
| :--- | :--- |
| `STEP` (or `ENTER`) | Executes the next instruction cycle (Fetch-Decode-Execute). |
| `REGS` | Prints the full state of the CPU (PC, AC, IR, SP, MAR, MDR, etc.). |
| `QUIT` | Stops the debugging session and returns to the main `LUCARIO >` console. |

**Output details in Debug Mode:**
- **Execution Log:** After every step, it displays the executed PC address, the raw instruction code, and the resulting value in the Accumulator (AC).
- **Interrupts:** Any hardware interrupt (Timer, I/O, Overflow) triggered during the step will be displayed immediately.

For a deep dive into the simulated hardware specifications (Instruction Set, Memory Layout, and Interrupts), please refer to the [Virtual Architecture Reference](docs/ARCHITECTURE.md).

You can find assembly program examples for this architecture in the `test/` directory, identified by the `asm_` prefix (e.g., `test/asm_myprogram`).

## Development Guide

This document provides guidelines for the project development. For more details, refer to the [DEVELOPMENT.md](docs/DEVELOPMENT.md) file.

## License

This project is licensed under the GPL-3.0 License. See the [LICENSE](LICENSE) file for more details.
