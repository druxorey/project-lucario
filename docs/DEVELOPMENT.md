
# Development Guide

---

This document serves as a guide for the syntax and conventions used in our projects.

## Table of Contents

1. [Variable Naming](#variable-naming)
2. [File Naming](#file-naming)
3. [Code Formatting](#code-formatting)
4. [Commits](#commits)
5. [Branching](#branching)
6. [Type Definitions](#type-definitions)
7. [Constants and Macros](#constants-and-macros)
8. [Logging Conventions](#logging-conventions)
9. [Concurrency & Synchronization](#concurrency--synchronization)
10. [Documentation Comments](#documentation-comments)

## Repository Maintenance

### Commits

For commits, we follow the [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/) philosophy. Here are some examples:

```
feat(engine): add collision detection
fix(player): correct health decrement bug
chore(tests): add unit tests for new game feature
docs(readme): update setup instructions
```

### Branching

Never push directly to `main`. Always create a new branch to implement features or make changes. Branches should follow the `type/description` style. Here are some examples:

```
feature/add-new-level
fix/correct-score-calculation
refactor/update-variable-names
chore/update-build-script
```

Once you've finished working on your branch, you can merge it into `main`.

## Code Formatting

To maintain consistency in the code, follow these formatting rules:

- Use 4-space tabs.
- Braces `{}` should be on the same line as the function or control block declaration.

### Variable Naming

Variables should follow the `lowerCamelCase` style. Here are some examples:

```c
int playerScore;
char* gameTitle;
bool isGameOver;
```

### File Naming

Files should follow the `snake_case` style. Here are some examples:

```bash
player_score.c;
is_game_over.h;
game_title.txt;
```

### Type Definitions

Since we are emulating a specific hardware architecture, avoid using raw `int` or `char` where specific hardware units are meant. Use `<stdint.h>` and `typedefs`.

```c
// Architecture specific types
typedef int32_t word;      // Represents the 8-digit word
typedef int32_t address;   // Represents memory addresses
typedef int8_t  flag;      // For PSW flags
```

### Constants and Macros

Do not use «magic numbers». Use `#define` or `enum` for all architecture constants defined in the documentation.

```c
#define OP_SUM 0
#define MEM_SIZE 2000
#define INT_OVERFLOW 8

if (ir == OP_SUM) { ... }
```

### Logging Conventions

Per requirement, the system must log every action. Do not use bare `printf`. Use the project's logging macros (to be defined in `logger.h`).

- **Output:** All logs must be written to the log file.
- **Interrupts:** Must be printed to BOTH standard output (console) and the log file .

```c
LOG_INFO("CPU", "Fetching instruction at %d", pc_register);
LOG_ERROR("MMU", "Invalid addressing interrupt: Code 6"); // Automatically writes to stdout & log
```

### Concurrency & Synchronization

We use `pthread` for threading. To avoid deadlocks and race conditions on the shared bus :

- **Naming:** Mutex variables should end with `_mutex`.
- **Locking Order:** Always acquire locks in the hierarchy order defined in the documentation to prevent deadlocks.

```c
pthread_mutex_t bus_mutex;
pthread_mutex_t memory_mutex;
```

## Documentation Comments

At the end of the project, every function in `.c` files must be documented describing inputs, outputs, and logic.

```c
/**
 * @brief Fetches the next instruction from memory to the IR.
 * @param cpu Pointer to the CPU struct.
 * @return 0 on success, error code otherwise.
 */
int fetch_instruction(cpu_t *cpu) { ... }
