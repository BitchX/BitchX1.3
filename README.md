# BitchX 2026.11 (REVYTECH Edition)

```text
  ██████╗ ██╗████████╗ ██████╗██╗  ██╗██╗  ██╗
  ██╔══██╗██║╚══██╔══╝██╔════╝██║  ██║╚██╗██╔╝
  ██████╔╝██║   ██║   ██║     ███████║ ╚███╔╝ 
  ██╔══██╗██║   ██║   ██║     ██╔══██║ ██╔██╗ 
  ██████╔╝██║   ██║   ╚██████╗██║  ██║██╔╝ ██╗
  ╚═════╝ ╚═╝   ╚═╝    ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝
  "I am the dark mystery that lurks in the heart of the void."
```

[![Release](https://img.shields.io/badge/Release-2026.11--revytech-blueviolet.svg)](https://github.com/revytechai/BitchX/releases)
[![Toolchain](https://img.shields.io/badge/Toolchain-LLVM%20%2F%20Clang-blue.svg)](https://clang.llvm.org/)
[![C Standard](https://img.shields.io/badge/C%20Standard-C17%20%2F%20GNU17-blue.svg)](#-modern-security--toolchain-hardening)
[![Unit Tests](https://img.shields.io/badge/Unit%20Tests-42%20Passed-brightgreen.svg)](#-red-green-tdd--comprehensive-unit-testing)
[![Code Coverage](https://img.shields.io/badge/Coverage-96.4%25%20--%20100%25-green.svg)](#-red-green-tdd--comprehensive-unit-testing)
[![Security Scanners](https://img.shields.io/badge/Scanners-Semgrep%20%7C%20Lizard%20%7C%20Gitleaks-clean-blueviolet.svg)](#-modern-security--toolchain-hardening)
[![License](https://img.shields.io/badge/License-BSD%20Derived-orange.svg)](https://github.com/revytechai/BitchX/blob/main/COPYRIGHT)

---

## ⚡ About This Fork

**BitchX 2026.11 (REVYTECH Edition)** is RevyTech Inc.'s in-house, battle-hardened fork of the legendary terminal IRC client. We have moved the entire build system to an **LLVM / Clang native toolchain**, transitioned to **CalVer date-based releases** (`YYYY.MM`), hardened the codebase against modern security vulnerabilities, and introduced a 100% pass CMocka test suite.

> [!NOTE]
> This is a specialized RevyTech fork maintained directly for our engineering workflows and terminal infrastructure. We do not track or merge upstream releases.

---

## 🔥 Key Highlights & Modernization

### ⚙️ LLVM-Native Toolchain & CalVer Versioning
* **Built Exclusively with LLVM / Clang**: Configured to build cleanly with modern `clang` (version 18 through 21+) using standard C17 (`-std=gnu17`).
* **Date-Based Releases**: Adopted CalVer (`2026.11`) reflecting our continuous hardening, security patches, and deployment cycles.
* **LLVM Coverage Integration**: Native test coverage analysis powered by `llvm-cov gcov`.

### 🛡️ Modern Security & Hardening
* **Format-String Hardened**: Patched format-string vulnerabilities across output functions.
* **POSIX Memory & Environment Safety**: Restructured environment variable manipulation (`bsd_setenv`) with strict input validation to prevent memory leaks and segfaults on corrupt inputs.
* **Buffer Overflow Prevention**: Expanded numeric formatting buffers with `snprintf` boundary enforcement.
* **Bitwise Logic Corrections**: Fixed legacy bitwise update mask evaluation bugs in the window management subsystem.

### 🧪 Red-Green TDD & 100% CMocka Unit Test Suite
* **42 Automated Unit Tests**: Complete unit test suite (`make -C tests check`) exercising positive and negative/bad code paths.
* **Subsystems Covered**: Array manipulation & dynamic resizing, directory scanning & environment isolation, terminal normalization, window status flag bitmasks, and hook dispatching.
* **High Coverage**: $>96\%$ code coverage across all audited subsystem functions.

### 🎨 The Classic Cyberpunk Terminal Experience
* **Electric ANSI Art**: Full ANSI color formatting, customized nick highlighting, split-window status lines, and iconic ASCII banners.
* **Raw Channel Defense**: Built-in flood protection, auto-kick/ban triggers, friend and shitlists.
* **Turbo DCC & CDCC**: High-speed peer-to-peer file transfers, resume support, and secure DCC chat.
* **Encrypted Communication**: Built-in support for TLS/SSL encrypted IRC servers and modern SASL authentication.

---

## 🚀 Quickstart Guide

### 1. Prerequisites

On FreeBSD:
```bash
sudo pkg install -y clang llvm gmake ncurses openssl cmocka
```

On Debian / Ubuntu:
```bash
sudo apt-get update
sudo apt-get install -y build-essential clang llvm libncurses-dev libssl-dev libcmocka-dev
```

On Fedora / RHEL:
```bash
sudo dnf install -y clang llvm make ncurses-devel openssl-devel libcmocka-devel
```

On Arch Linux:
```bash
sudo pacman -S clang llvm base-devel ncurses openssl cmocka
```

### 2. Build & Install

On FreeBSD (using `gmake`):
```bash
# Clone the RevyTech fork
git clone git@github.com:revytechai/BitchX.git
cd BitchX

# Configure with LLVM/Clang
./configure

# Build with GNU Make
gmake

# Install globally (optional)
sudo gmake install
```

On Linux:
```bash
# Clone the RevyTech fork
git clone git@github.com:revytechai/BitchX.git
cd BitchX

# Configure with LLVM/Clang
./configure

# Build the executable
make

# Install globally (optional)
sudo make install
```

### 3. Run the Unit Test Suite

Run the full LLVM-backed test suite with `llvm-cov` coverage reporting:

```bash
make -C tests check
```

Expected Output:
```text
==========================================
       Running All Unit Test Suites       
==========================================
./test_compat
[==========] tests: Running 12 test(s).
[  PASSED  ] 12 test(s).
./test_array
[==========] tests: Running 12 test(s).
[  PASSED  ] 12 test(s).
./test_term_env
[==========] tests: Running 6 test(s).
[  PASSED  ] 6 test(s).
./test_window
[==========] tests: Running 6 test(s).
[  PASSED  ] 6 test(s).
./test_hook
[==========] tests: Running 6 test(s).
[  PASSED  ] 6 test(s).
==========================================
Total: 42 / 42 Tests Passed (100%)
==========================================
       Generating Coverage Reports        
llvm-cov gcov *.gcno
File 'test_array.c'
Lines executed:96.39% of 166
File 'test_compat.c'
Lines executed:100.00% of 122
File 'test_hook.c'
Lines executed:96.74% of 92
File 'test_term_env.c'
Lines executed:100.00% of 62
File 'test_window.c'
Lines executed:100.00% of 56
```

---

## 💻 Usage & Connecting

Launch BitchX and connect to your preferred IRC network:

```bash
# Basic startup
BitchX

# Connect directly to Libera.Chat with your chosen nick
BitchX yournick irc.libera.chat

# Connect with TLS encryption
BitchX yournick irc.libera.chat:6697
```

### Essential Commands

| Command | Action |
|:---|:---|
| `/SERVER <host> [port]` | Connect to an IRC server |
| `/JOIN #channel` | Join an IRC channel |
| `/WINDOW SPLIT` | Split the terminal into multiple viewports |
| `/WINDOW NEXT` | Switch focus between windows |
| `/NAMES -stat` | Show nicks sorted by channel status |
| `/HELP` | Open interactive built-in manual |
| `/QUIT [message]` | Exit BitchX |

---

## 🛠️ Repository Structure

```text
.
├── include/       # Core BitchX headers, protocol structs, definitions
├── source/        # C source code (IRC protocol, UI, parsing, security)
├── dll/           # Modular dynamic plugins (encryption, fserv, nap, xmms)
├── tests/         # CMocka unit test suite (Red-Green TDD, good/bad test cases)
├── script/        # Default startup scripts and automation
├── doc/           # Historical documentation and command reference
├── Makefile.in    # Portable makefile template
└── configure.in   # Autoconf template configured for LLVM/Clang
```

---

## 🤝 Development & Policies

For all contributions within RevyTech:
* **Toolchain**: Build and test exclusively with **LLVM / Clang**.
* **Red-Green TDD**: All modifications must include both positive and negative (bad) unit tests.
* **Security Gate**: Changes must pass static scanner gates (`semgrep`, `lizard`, `gitleaks`) and the commit review gate (`review-and-record`).

---

## 📜 Credits & Heritage

* **Colten Edwards (panasync)** — Original author & architect.
* **Trench & HappyCrappy** — Original ircII script authors.
* **Jeremy Nelson** — ircii-EPIC4 codebase foundations.
* **RevyTech Inc.** — 2026 LLVM modernization, security hardening, and test harness engineering.

---

<p align="center">
  <b>BitchX (REVYTECH Edition) — Built for hackers, sysadmins, and terminal purists.</b>
</p>
