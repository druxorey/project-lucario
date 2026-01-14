# Virtual Architecture Specification

This document provides a comprehensive technical specification of the **Project Lucario** virtual machine. It details the processor design, memory organization, instruction set architecture (ISA), and input/output subsystems implemented in the project.

## 1. Core Processor Design

The architecture simulates a **Decimal Machine** behavior running on a binary host. It does not use general-purpose registers; instead, it relies on a single Accumulator (AC) for arithmetic and logical operations.

### 1.1 Data Representation

The fundamental unit of data is the **Word**.
* **Size:** 8 Decimal Digits.
* **Internal Type:** `int32_t` (Simulated via software definitions).
* **Format:** Sign-Magnitude.
    * **Sign Bit:** The most significant digit represents the sign.
        * `0xxxxxxx`: Positive (+)
        * `1xxxxxxx`: Negative (-)
    * **Magnitude:** The remaining 7 digits represent the absolute value.
    * **Range:** `[-9,999,999, +9,999,999]`.

### 1.2 Registers

The CPU contains specific registers defined in the `CPU_t` structure.

| Register | Name | Width | Function |
| :--- | :--- | :--- | :--- |
| **AC** | Accumulator | 8 digits | Primary register for arithmetic and logic results. |
| **PC** | Program Counter | 5 digits | Holds the address of the next instruction to fetch. |
| **IR** | Instruction Register | 8 digits | Stores the current instruction being executed. |
| **MAR** | Memory Address Register | 5 digits | Buffer for the address bus during memory access. |
| **MDR** | Memory Data Register | 8 digits | Buffer for the data bus during memory access. |
| **RB** | Base Register | 5 digits | Defines the *start* of the current process memory partition. |
| **RL** | Limit Register | 5 digits | Defines the *end* of the current process memory partition. |
| **RX** | Index Register | 8 digits | Auxiliary register used for Indexed Addressing mode. |
| **SP** | Stack Pointer | 5 digits | Points to the top of the system stack. |

### 1.3 Program Status Word (PSW)

The PSW controls the CPU state and flags. It is structured as follows:

1.  **Condition Code (CC):** Indicates the result of the last arithmetic/logic operation.
    * `0` (CC_ZERO): Result was Zero.
    * `1` (CC_NEG): Result was Negative.
    * `2` (CC_POS): Result was Positive.
    * `3` (CC_OVERFLOW): Operation exceeded the 7-digit magnitude limit.
2.  **Operation Mode:**
    * `0` (MODE_USER): Restricted access. Memory protection active.
    * `1` (MODE_KERNEL): Full access. Memory protection disabled.
3.  **Interrupt Enable:**
    * `0` (ITR_DISABLED): CPU ignores maskable interrupts.
    * `1` (ITR_ENABLED): CPU processes interrupts.

## 2. Memory System

The system simulates a linear array of **2000 Words** (`RAM[2000]`). Access is thread-safe, arbitrated by a system bus mutex.

### 2.1 Memory Map

* **0000 - 0299:** **OS Reserved Area.** Stores interrupt vectors and kernel data. Accessible only in Kernel Mode.
* **0300 - 1999:** **User Space.** Dynamic storage for user programs and stack.

### 2.2 Memory Management Unit (MMU) & Protection

The system implements software-based memory protection using Base and Limit registers.

* **Logical Address:** The address used by the program code (relative to 0).
* **Physical Address:** The actual index in the RAM array.

**Translation Logic:**

* **Kernel Mode:** `Physical = Logical` (Absolute Addressing).
* **User Mode:** `Physical = Logical + RB`.

**Protection Check:**

In User Mode, if `Physical < RB` or `Physical > RL`, the MMU blocks the access and raises an `IC_INVALID_ADDR` (Segmentation Fault) interrupt.

## 3. Instruction Set Architecture (ISA)

Instructions are 8-digit words encoded in the format: **`OO D VVVVV`**.

### 3.1 Instruction Decoding

* **OO (OpCode):** The first 2 digits specify the operation (00-33).
* **D (Direction/Mode):** The 3rd digit specifies the addressing mode.
* **VVVVV (Value):** The last 5 digits represent the operand.

### 3.2 Addressing Modes

| Mode | Code | Description |
| :--- | :--- | :--- |
| **Direct** | `0` | The `Value` is a memory address. Operand = `RAM[Value]`. |
| **Immediate** | `1` | The `Value` is the operand itself. |
| **Indexed** | `2` | The `Value` is an offset. Effective Address = `Value + AC`. |

### 3.3 Operations Reference

#### Arithmetic (ALU)

| OpCode | Mnemonic | Description |
| :--- | :--- | :--- |
| `00` | `SUM` | AC = AC + Operand |
| `01` | `RES` | AC = AC - Operand |
| `02` | `MULT` | AC = AC * Operand |
| `03` | `DIVI` | AC = AC / Operand (Integer division) |
| `08` | `COMP` | Performs (AC - Operand) to update PSW flags only. |

#### Data Movement

| OpCode | Mnemonic | Description |
| :--- | :--- | :--- |
| `04` | `LOAD` | Load Operand into AC. |
| `05` | `STR` | Store AC into Memory Address (Direct/Indexed). |
| `06` | `LOADRX` | AC = RX. |
| `07` | `STRRX` | RX = AC. |
| `19` | `LOADRB` | AC = RB. |
| `20` | `STRRB` | RB = AC. |
| `21` | `LOADRL` | AC = RL. |
| `22` | `STRRL` | RL = AC. |
| `23` | `LOADSP` | AC = SP. |
| `24` | `STRSP` | SP = AC. |

#### Flow Control

| OpCode | Mnemonic | Description |
| :--- | :--- | :--- |
| `27` | `J` | Unconditional Jump to address. |
| `09` | `JMPE` | Jump if `AC == Memory[SP]` (Top of Stack). |
| `10` | `JMPNE` | Jump if `AC != Memory[SP]`. |
| `11` | `JMPLT` | Jump if `AC < Memory[SP]`. |
| `12` | `JMPLGT` | Jump if `AC > Memory[SP]`. |
| `14` | `RETRN` | Return from subroutine (Pop PC). |

#### Stack Operations

| OpCode | Mnemonic | Description |
| :--- | :--- | :--- |
| `25` | `PSH` | Push (Decrement SP, Store AC). |
| `26` | `POP` | Pop (Read SP into AC, Increment SP). |

#### System & Control

| OpCode | Mnemonic | Description |
| :--- | :--- | :--- |
| `13` | `SVC` | System Call (Software Interrupt). |
| `15` | `HAB` | Enable Interrupts. |
| `16` | `DHAB` | Disable Interrupts. |
| `17` | `TTI` | Set Timer Interval. |
| `18` | `CHMOD` | Switch Mode (0=User, 1=Kernel). |

### 3.4 System Calls (SVC)

The `SVC` instruction (OpCode 13) serves as the interface between user programs and the kernel.

> **Note:** The System Call table is currently **Under Development**.

Current implementation supports the following service codes passed via the **Accumulator (AC)**:

| AC Value | Service | Description |
| :--- | :--- | :--- |
| `0` | **EXIT** | Terminates the current program execution immediately. |
| `> 0` | *Reserved* | Reserved for future kernel services (e.g., I/O requests, Process creation). Currently treated as No-Operation. |

## 4. Input/Output (DMA & Disk)

The system features a **Direct Memory Access (DMA)** controller to handle I/O without blocking the CPU completely, running on a separate thread.

### 4.1 Disk Geometry

The disk is simulated as a 3D array: `DISK[10][10][100]` (Tracks, Cylinders, Sectors). Each sector holds one `word`.

### 4.2 DMA Controller Instructions

To perform I/O, the CPU must configure the DMA registers sequentially using instructions `28` to `33`.

| OpCode | Mnemonic | Description |
| :--- | :--- | :--- |
| `28` | `SDMAP` | Set Disk **P**latter (Track). |
| `29` | `SDMAC` | Set Disk **C**ylinder. |
| `30` | `SDMAS` | Set Disk **S**ector. |
| `31` | `SDMAIO` | Set Direction: `0` = Read (Disk->RAM), `1` = Write (RAM->Disk). |
| `32` | `SDMAM` | Set Target Memory Address. |
| `33` | `SDMAON` | Activate DMA Engine (Start Transfer). |

**Note:** The `SDMAM` instruction validates memory protection immediately based on the current process `RB/RL`.

## 5. Interrupt System

The CPU polls for interrupts at the end of every instruction cycle.

### 5.1 Interrupt Priority

If multiple interrupts occur simultaneously, they are handled in this order:
1.  **Faults:** Invalid Instruction, Invalid Address, Arithmetic Overflow/Underflow.
2.  **System Calls:** SVC instruction.
3.  **Hardware:** Timer, I/O Completion (DMA).

### 5.2 Interrupt Codes

| Code | Name | Description |
| :--- | :--- | :--- |
| `0` | `IC_INVALID_SYSCALL` | Unknown SVC code. |
| `1` | `IC_INVALID_INT_CODE` | Unknown interrupt vector. |
| `2` | `IC_SYSCALL` | Triggered by `SVC` instruction. |
| `3` | `IC_TIMER` | Triggered when CPU cycle counter meets the limit. |
| `4` | `IC_IO_DONE` | Triggered by DMA when transfer finishes. |
| `5` | `IC_INVALID_INSTR` | OpCode not recognized. |
| `6` | `IC_INVALID_ADDR` | Memory access violation (SegFault) or Out of Bounds. |
| `7` | `IC_UNDERFLOW` | Arithmetic result too small (not currently generated). |
| `8` | `IC_OVERFLOW` | Arithmetic magnitude > 7 digits. |

