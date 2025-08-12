# Terminus

**Terminus** is a high-performance, minimalist command-line interpreter for Unix-like systems, engineered in pure C. It was built on the philosophy that a shell should be transparent, lightweight, and fast—a sharp contrast to the complex, feature-heavy shells common today.

While modern terminals provide extensive scripting capabilities, they often sacrifice performance and simplicity. Terminus is an experiment in returning to first principles, offering a superior experience in three key areas:

*   **⚡ Blazing Fast & Dependency-Free:** Written in C with direct system calls and no external library dependencies, Terminus has a near-instant startup time and a minimal memory footprint. It proves that powerful functionality doesn't require bloat.

*   **🔬 Transparent by Design:** Unlike shells with layers of abstraction, every function in Terminus is purposeful and auditable. You have a direct line to the operating system's process and I/O management, offering unparalleled clarity into how your commands are actually executed.

*   **🔧 Infinitely Hackable:** With a small, clean, and well-defined codebase, Terminus serves as the perfect foundation for experimentation. Adding new features or modifying process-handling logic is straightforward, making it an ideal environment for systems programming innovation.

![Language](https://img.shields.io/badge/Language-C-blue.svg)![Platform](https://img.shields.io/badge/Platform-Linux-yellow.svg)![License](https://img.shields.io/badge/License-MIT-green.svg)

---

---

### 🏛️ System Architecture

The architecture of Terminus is designed around the fundamental principles of Unix process management, making it both efficient and reliable.

| Component                | Architectural Role & Implementation                               | Key System Calls Used                |
| ------------------------ | ----------------------------------------------------------------- | ------------------------------------ |
| **Process Engine**       | Manages the process lifecycle using the `fork-exec` model.        | `fork()`, `execvp()`, `waitpid()`    |
| **IPC Pipeline**         | Creates kernel-managed data pipes for inter-process communication.| `pipe()`, `dup2()`                   |
| **I/O Subsystem**        | Rewires file descriptors to redirect process I/O streams.         | `open()`, `close()`, `dup2()`        |
| **Signal Handling Layer**| Asynchronously responds to OS signals (`SIGCHLD`) for job cleanup.| `signal()`, `waitpid(WNOHANG)`       |

When a command is entered, the input is tokenized and parsed into an execution tree. For simple commands, a child process is forked and executes the command. For complex pipelines, Terminus forks multiple child processes and uses its IPC Pipeline component to connect their standard I/O streams, allowing for powerful, script-like command chains. Background tasks are managed by the parent process, which uses the Signal Handling Layer to listen for termination events without blocking, ensuring the shell remains responsive at all times.

---

### 🚀 Getting Started

1.  **Clone the repository:**
    ```bash
    git clone https://github.com/[YourGitHubUsername]/Terminus.git
    cd Terminus
    ```
2.  **Compile the binary:**
    ```bash
    make
    ```
3.  **Run Terminus:**
    ```bash
    ./Terminus
    ```

---

### Future Development Roadmap

Terminus is actively being developed. Future enhancements include:

*   Implementation of full job control with `fg`, `bg`, and `Ctrl+Z`.
*   Support for `&&` and `||` logical operators.
*   A tab-completion engine for commands and file paths.
*   A command history module with arrow-key navigation.

---

*This project was developed by Rutuja*
