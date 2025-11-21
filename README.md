<h1 align="center">Project Lucario</h1>
<p align="center">Virtual Architecture Implementation for Minikernel Support</p>

<p align="center">
<a href="docs/DEVELOPMENT.md"><img src="https://img.shields.io/badge/development%20guide-BD93F9?style=for-the-badge"></a>
<a href="#instalación"><img src="https://img.shields.io/badge/installation-FF79C6?style=for-the-badge"></a>
<a href="#dependencias"><img src="https://img.shields.io/badge/dependencies-BD93F9?style=for-the-badge"></a>

## Description

This is a university programming project for the **Operating Systems** course at **Universidad Central de Venezuela**. The goal of this project (Phase I) is to implement a virtual architecture running on native Linux. This software simulates the hardware layer—including an accumulator-based processor, a memory management system, and a DMA controller to support the future implementation of a minikernel.

## Key Features

- **Dual-Mode Processor:** Supports both Privileged (Kernel) and User execution modes.
- **Virtual Memory:** Simulation of 2000 memory positions with protection registers (RB/RL).
- **I/O System:** Full simulation of a shared bus, DMA controller, and a geometric disk structure (Tracks/Cylinders/Sectors).
- **Execution Modes:** Runs in **Normal** mode for standard execution and **Debugger** mode for step-by-step instruction analysis.

## Development Guide

This document provides guidelines for the project development. For more details, refer to the [DEVELOPMENT.md](docs/DEVELOPMENT.md) file.

## License

This project is licensed under the GPL-3.0 License. See the [LICENSE](LICENSE) file for more details.
