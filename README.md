# Scoom Text Editor

Scoom Text Editor is a simple text editor written in C, inspired by the Kilo tutorial. This project was created to deepen understanding of terminal I/O, dynamic memory allocation, and raw mode handling in Unix systems.

<img width="1842" height="306" alt="image" src="https://github.com/user-attachments/assets/3ad385bb-49d2-4fac-bd6d-e2ff0fe61e01" />

## Features

- Core editing functionality based on the Kilo text editor guide
- Custom enhancements, such as shift cursor movements
- Lightweight and fast
- Runs in the terminal
- No external dependencies (works out of the box on Unix-like systems)

## Why Scoom?

This project was built by closely following the Kilo tutorial, with additional features and improvements. It served as a learning experience for low-level programming and working with terminal interfaces.

## Getting Started

### Prerequisites

- GCC or Clang (any C compiler)
- Unix-like operating system (Linux, macOS, WSL)

### Build

```bash
gcc -o scoom scoom.c
```

Or if using CMake (if provided):

```bash
cmake .
make
```

### Run

```bash
./SCOOM <filename>
```

## Project Structure

- `main.c` - Main source code for the editor
- `README.md` - This documentation

## Inspiration

- [Kilo Text Editor Tutorial](https://viewsourcecode.org/snaptoken/kilo/)

---

Made by [Paran-oid](https://github.com/Paran-oid)
