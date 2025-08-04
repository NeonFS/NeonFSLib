# NeonFS

![License: AGPL v3](https://img.shields.io/badge/license-AGPLv3-blue.svg)
![Status: Early Development](https://img.shields.io/badge/status-early--dev-yellow)

**NeonFS** is a lightweight, modular virtual filesystem library written in modern C++20. It's designed to serve as the secure, high-performance backend for ElectronJS-based desktop applications that require custom file handling, encryption, and metadata control.

---

## Status

> NeonFS is currently **in early development**.  
> APIs and architecture are subject to change as we evolve the core.

---

## Project Goals

- Provide a secure and fast virtual filesystem layer for desktop apps
- Handle files and directories via custom metadata logic
- Support encrypted block-level file storage
- Enable smooth integration with ElectronJS via native bindings
- Build with modularity and performance in mind

---

## Planned Features

- Virtual file and directory abstraction
- Encrypted block storage with AES-GCM
- Secure memory handling for sensitive data
- SQLite-backed metadata layer
- Key derivation and session management
- API layer exposed to Node.js/Electron

---

## Usage

The library is designed to be embedded into a Node.js environment and accessed from Electron apps. A JS wrapper will expose its functionality in a clean and async-friendly interface.

_Example usage and API documentation will come as development progresses._

---

## Project Structure (Work in Progress)

```yaml
NeonFS/
├── include/
│ └── NeonFS/ # Public headers
├── src/ # Core implementation
├── bindings/ # Node.js/Electron bindings (planned)
├── tests/ # Unit and integration tests
├── docs/ # Documentation
├── LICENSE.txt
└── README.md
```

## Development Notes

We're currently finalizing the low-level design and core architecture.  
The initial milestones focus on:

- Block-based file storage system
- Secure key management
- Minimal working FS node structure

Expect significant updates over time.

## License

This project is licensed under the GNU Affero General Public License v3.0.  
See the [LICENSE.txt](LICENSE.txt) file for details.

## Contributions

We're not accepting contributions at this stage.  
Stay tuned for updates as the foundation stabilizes!

## Contact:

Mohammed Ifkirne
Email: mohammedifkirne569@gmail.com
GitHub: [https://github.com/MohaIfk/](https://github.com/MohaIfk/)
