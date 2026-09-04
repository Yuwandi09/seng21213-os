# SENG21213-OS — Complete 32-bit x86 Operating System

> **Course**: SENG 21213 – Computer Architecture & Operating Systems  
> **Student Name**: Yuwandi Sandanayake  \n> **Student Number**: SE/2023/047  
> **Department**: Department of Software Engineering, Faculty of Engineering  
> **Repository**: [https://github.com/Yuwandi09/seng21213-os](https://github.com/Yuwandi09/seng21213-os)

---

## Overview

**SENG21213-OS** is a complete, monolithic 32-bit x86 Protected Mode operating system developed across all 5 course milestones. The operating system features a custom MBR bootloader, VGA text-mode driver, PS/2 keyboard driver, Interrupt Descriptor Table (IDT) with PIC remapping, i8253 PIT 100 Hz timer, preemptive Round-Robin process scheduler, kernel-level threads, synchronization primitives (Spinlock Mutex & Counting Semaphore), a bitmap-based Physical Memory Manager (PMM), and an in-memory RAM-disk file system with full interactive shell commands.

---

## Milestone Releases

All 5 required release tags are committed and pushed:

| Milestone Tag | Stage | Key Features Implemented |
|:---|:---|:---|
| [`v0.1-stage0`](https://github.com/Yuwandi09/seng21213-os/releases/tag/v0.1-stage0) | **Stage 0** | MBR Bootloader (16-bit to 32-bit PM transition), GDT, VGA text driver, PS/2 keyboard driver, interactive shell with `help`, `version`, `colour`, `clear`, `halt`, `about`, `mem`. |
| [`v0.2-stage1`](https://github.com/Yuwandi09/seng21213-os/releases/tag/v0.2-stage1) | **Stage 1** | 256-entry IDT, 8259 PIC remapping (IRQs 32–47), i8253 PIT timer at 100 Hz (IRQ0), assembly context switch (`switch.asm`), Process Control Block (16 slots, 4 KB stacks), Round-Robin preemptive scheduler (20 ms quantum), `ps`, `kill`, and concurrent process `demo`. |
| [`v0.3-stage2`](https://github.com/Yuwandi09/seng21213-os/releases/tag/v0.3-stage2) | **Stage 2** | Kernel-level threads (`thread_create`, `thread_exit`, `thread_kill`), spinlock `mutex_t` using atomic `XCHG` instruction, counting `semaphore_t` (Dijkstra P/V), shell commands `threads`, `mutex-demo`, and `prodcon` (producer-consumer). |
| [`v0.4-stage3`](https://github.com/Yuwandi09/seng21213-os/releases/tag/v0.4-stage3) | **Stage 3** | Physical Memory Manager (PMM) with bitmap frame allocator (8,192 frames @ 4 KB = 32 MB RAM), memory reservation for low 1 MB & kernel sections, `meminfo` command with live frame alloc/free test. |
| [`v0.5-stage4`](https://github.com/Yuwandi09/seng21213-os/releases/tag/v0.5-stage4) | **Stage 4** | In-memory RAM-disk file system (16 file slots, 1 KB max per file), pre-populated `readme.txt` and `info.txt`, shell commands `ls`, `touch`, `write`, `cat`, and `rm`. |

---

## Project Structure

```
seng21213-os/
├── boot/
│   └── boot.asm          ← MBR Bootloader (16-bit real mode → GDT → 32-bit protected mode)
├── kernel/
│   ├── kernel_entry.asm  ← Protected-mode entry point, calls kernel_main()
│   ├── idt_asm.asm       ← Low-level ISR/IRQ assembly stubs and IDT flush
│   ├── switch.asm        ← Assembly context switch stub (switch_to)
│   ├── kernel.c          ← Kernel entry, shell loop, command dispatcher
│   ├── vga.c / vga.h     ← VGA text-mode driver (80x25, colors, cursor, printf)
│   ├── keyboard.c / .h   ← PS/2 keyboard driver (scancode mapping, buffer)
│   ├── idt.c / idt.h     ← 256-entry IDT, 8259 PIC remapping, IRQ dispatch
│   ├── pit.c / pit.h     ← i8253 PIT at 100 Hz (10 ms ticks, pit_sleep)
│   ├── process.c / .h    ← Process table (16 PCBs), create, yield, exit, kill
│   ├── scheduler.c / .h  ← Round-Robin preemptive scheduler (20 ms quantum)
│   ├── thread.c / .h     ← Kernel threads (16 threads, 4 KB stacks each)
│   ├── mutex.c / .h      ← Mutex lock using atomic XCHG instruction
│   ├── semaphore.c / .h  ← Counting semaphore with sem_wait and sem_signal
│   ├── pmm.c / pmm.h     ← Physical Memory Manager (bitmap frame allocator)
│   └── fs.c / fs.h       ← In-memory RAM-disk file system
├── include/
│   └── types.h           ← C23-compatible primitive types (uint8_t, bool, size_t, etc.)
├── linker.ld             ← Linker script loading kernel at 0x10000
├── Makefile              ← Build system with gcc, nasm, ld, qemu
├── .gitignore            ← Excludes build artifacts and disk images
└── README.md             ← Project documentation
```

---

## Building and Running

### Prerequisites (Ubuntu / Debian / WSL2)

```bash
sudo apt update
sudo apt install nasm gcc gcc-multilib binutils qemu-system-x86 make
```

### Build the OS

```bash
make clean && make
```

This compiles all assembly (`.asm`) and C (`.c`) sources into `build/kernel.bin` and packages it with `boot/boot.bin` into a 1.44 MB bootable floppy image `seng21213-os.img`.

### Run in QEMU

```bash
make run
```

Or manually:

```bash
qemu-system-i386 -drive format=raw,file=seng21213-os.img -m 32M
```

---

## Available Shell Commands

| Command | Usage | Description |
|:---|:---|:---|
| `help` | `help` | Display list of all shell commands |
| `clear` | `clear` | Clear the VGA screen |
| `version` | `version` | Display kernel version information |
| `colour` | `colour <fg> <bg>` | Change text foreground (0–15) and background (0–15) |
| `echo` | `echo <text>` | Echo text to standard output |
| `about` | `about` | Display system architecture and components |
| `mem` | `mem` | Display physical memory address layout map |
| `ps` | `ps` | List all processes in the PCB table (PID, state, ticks, name) |
| `kill` | `kill <pid>` | Terminate a running process |
| `demo` | `demo` | Spawn 2 concurrent processes to demonstrate multitasking |
| `threads` | `threads` | List all active kernel threads (TID, state, ticks, name) |
| `mutex-demo`| `mutex-demo` | Run 2 threads safely incrementing a shared counter with mutex |
| `prodcon` | `prodcon` | Run Producer-Consumer demonstration using counting semaphores |
| `meminfo` | `meminfo` | Display PMM frame statistics, memory bar, and alloc/free test |
| `ls` | `ls` | List all files in the in-memory RAM disk with file sizes |
| `touch` | `touch <file>` | Create a new empty file in the file system |
| `write` | `write <file> <text>`| Write text contents into a file |
| `cat` | `cat <file>` | Display contents of a file |
| `rm` | `rm <file>` | Delete a file from the file system |
| `halt` | `halt` | Halt the CPU |

---

## Architectural Details

### 1. Bootloader & Kernel Entry
- `boot/boot.asm`: 512-byte MBR loaded at `0x7C00` by BIOS. Switches from 16-bit Real Mode to 32-bit Protected Mode after configuring the Global Descriptor Table (GDT), enables the A20 line, loads 64 disk sectors into `0x10000`, and jumps to `kernel/kernel_entry.asm`.
- `kernel/kernel_entry.asm`: Sets up data segment registers (`DS`, `ES`, `FS`, `GS`, `SS` = `0x10`), sets `ESP` to `0x90000`, and invokes `kernel_main()`.

### 2. Interrupts & Timer
- **IDT**: 256 interrupt gates configured in `kernel/idt.c`. PIC master (ports `0x20`/`0x21`) and slave (ports `0xA0`/`0xA1`) remapped to interrupt vectors 32–47.
- **PIT**: i8253 timer configured via channel 0 (ports `0x43`/`0x40`) with divisor `11932` (100 Hz / 10 ms period), generating IRQ0 (vector 32).

### 3. Process & Thread Management
- **PCB Table**: 16 slots with 4 KB dedicated stacks. Fake stack frames pre-populated with initial register values (`EFLAGS` with IF=1, entry address, exit function).
- **Context Switch**: Handled in assembly (`switch_to` in `switch.asm`), atomically pushing/popping caller-saved registers and swapping `ESP`.
- **Synchronization**: `mutex_t` implemented via atomic `xchgl` instruction; `semaphore_t` implemented with interrupt-safe atomic counters and cooperative CPU yielding.

### 4. Memory Management
- **PMM**: 8,192 frames of 4 KB each (32 MB total RAM). Bitmap tracking (256 `uint32_t` words). Low 1 MB (VGA, IVT, BIOS), kernel binary, and stack space are marked permanently reserved. Extended memory (1 MB–32 MB) is allocated and freed on demand.

### 5. In-Memory File System
- Flat RAM disk supporting up to 16 files, each up to 1,024 bytes. Full support for creation, reading, writing, deletion, and directory listing.

---

## Author

- **Name**: Yuwandi Sandanayake\n- **Student Number**: SE/2023/047
- **Repository**: [github.com/Yuwandi09/seng21213-os](https://github.com/Yuwandi09/seng21213-os)
