# 🔐 OnCrypto

<div align="center">

![Version](https://img.shields.io/badge/version-1.5.0-blue.svg)
![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus)
![CMake](https://img.shields.io/badge/CMake-3.20+-064F8C?logo=cmake)
![Ninja](https://img.shields.io/badge/Ninja-Build-black)
![License](https://img.shields.io/badge/License-MIT-yellow.svg)
![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20Windows%20%7C%20macOS%20%7C%20Android-success)

**Modern Cross-Platform Encryption Library & CLI built with C++20**

</div>

---

## 📚 Table of Contents

- [Introduction](#-introduction)
- [Features](#-features)
- [Supported Algorithms](#-supported-algorithms)
- [Automatic Algorithm Selection](#-automatic-algorithm-selection)
- [Architecture](#-architecture)
- [Project Structure](#-project-structure)
- [Requirements](#-requirements)
- [Installation](#-installation)
- [Build Options](#-build-options)
- [CLI Usage](#-cli-usage)
- [Library Usage](#-library-usage)
- [Streaming API](#-streaming-api)
- [Running Tests](#-running-tests)
- [Version History](#-version-history)
- [License](#-license)
- [Contact](#-contact)

---

## 📖 Introduction

**OnCrypto** is a modern, lightweight, and high-performance cross-platform encryption library written in **C++20**.

It provides both:

- 🖥 **Command Line Interface (CLI)**
- 📚 **Shared Library API** (`liboncrypto.so`)

The library features a decoupled, backend-agnostic design with high-security authenticated encryption algorithms and automatic algorithm selection.

### 🎯 Design Philosophy

- **Simple API** – Just `encrypt()` and `decrypt()`
- **Smart Defaults** – Auto-selects the optimal algorithm for your payload size
- **Modular Backend** – Crypto backend is completely isolated via an abstracted interface
- **Cross-Platform** – Native support across Linux, macOS, Windows, and Android
- **Production Ready** – Fully unit-tested with 1000+ internal assertions

---

## ✨ Features

| Feature | Status |
|----------|--------|
| 🔐 AES-256-GCM | ✅ |
| ⚡ ChaCha20-Poly1305 | ✅ |
| 🚀 XChaCha20-Poly1305 | ✅ |
| 🤖 Automatic Algorithm Selection | ✅ |
| 🔑 PBKDF2 Key Derivation | ✅ |
| 🧂 16-byte Random Salt | ✅ |
| 🔄 Shared Library (`liboncrypto.so`) | ✅ |
| 💻 CLI Tool | ✅ |
| 🌊 Streaming Support (Large Files) | ✅ |
| 📦 OnC Binary Format | ✅ |
| 🧪 Unit Tests | ✅ |
| 🌍 Cross Platform | ✅ |
| ⚙ CMake Build System | ✅ |

---

## 🔐 Supported Algorithms

| Algorithm | Security | Speed | Best For |
|-----------|----------|-------|----------|
| **XChaCha20-Poly1305** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | Small data (<1KB) |
| **ChaCha20-Poly1305** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐☆ | Medium files |
| **AES-256-GCM** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | Large files (>1MB) |

---

## 📝 Documentation

For detailed SDK and architecture information, see `docs/architecture.md`, `docs/api.md`, `docs/format.md`, `docs/security.md`, and `docs/building.md`.

## 🤖 Automatic Algorithm Selection
