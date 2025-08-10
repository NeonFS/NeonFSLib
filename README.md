# NeonFS

![License: AGPL v3](https://img.shields.io/badge/license-AGPLv3-blue.svg)
![Status: Early Development](https://img.shields.io/badge/status-early--dev-yellow)

**NeonFS** is a modular, high-security virtual filesystem library implemented in C++20.
It delivers encrypted, metadata-driven storage with consistent security guarantees, suitable for desktop, server, or embedded applications requiring strict control over file handling, storage policies, and security boundaries.

---

## Status

> NeonFS is currently **in early development**.  
> APIs and architecture are subject to change as we evolve the core.

---

## Global Philosophy

NeonFS is founded on one immutable principle:

> Security first. Performance is important. Convenience is optional. Leaks are unacceptable.

All architectural and implementation decisions are subjected to rigorous security evaluation. Components are designed as potential attack surfaces and hardened accordingly.

### Key Tenets

1. **Security is the Goal, Not a Feature**
   Every design choice is rigorously evaluated for confidentiality and integrity impacts.

2. **Leak Intolerance**
   No data leakage—partial or indirect—is tolerated.

3. **Controlled Performance Trade-offs**
   Performance enhancements must never degrade security guarantees.

4. **Minimal Attack Surface**
   Only essential metadata, APIs, and interfaces are exposed.

5. **Isolation and Encapsulation**
   Subsystems are compartmentalized to prevent cascading compromises.

6. **Security Through Obscurity Is Not Security**
   Cryptography and strict access enforcement are the foundation.

7. **No Blind Trust in Dependencies**
   All third-party components undergo verification and sandboxing where feasible.

8. **Proactive Threat Modeling**
   Threats are anticipated and addressed in design, not post-incident.

9. **End-to-End Protection**
   Data remains protected throughout its lifecycle.

10. **Audit-Friendly Design**
    Architected to facilitate external verification without weakening security.

---

## Purpose and Scope

NeonFS is designed as a robust foundation for any system demanding:

* Verified, encrypted storage with granular metadata control
* Predictable, auditable security behavior overriding raw throughput priorities
* Flexible integration across multiple runtime environments without sacrificing security

---

## Differentiators

* AES-GCM encrypted block storage with low overhead
* Encrypted metadata storage that eliminates plaintext leakage
* Cryptographically hashed indexes for secure, efficient lookups
* Modular, extensible architecture adaptable to diverse deployment scenarios
* Implemented in C++20 for consistent, measurable performance characteristics

These features enable NeonFS to support sensitive workloads ranging from compliance-bound data storage to embedded systems requiring strong confidentiality guarantees.

---

## Project Goals

* Deliver a secure, high-performance virtual filesystem core
* Provide encrypted, block-level storage with verifiable integrity
* Implement a metadata and permissions system resilient against leakage
* Maintain modularity to support multiple runtime environments
* Ensure behavior consistent with stringent security policies

---

## Planned Features

* Virtual file and directory abstraction with strict access controls
* AES-GCM block encryption and authentication
* Secure memory management, including zeroization of sensitive buffers
* SQLite-backed encrypted metadata layer with pluggable backends
* Robust key derivation and session lifecycle management
* Language bindings targeting multiple runtime environments

---

## Metadata Obfuscation

NeonFS treats all filesystem metadata as sensitive, preventing leakage through:

* **Full Encryption:** Filenames, sizes, timestamps, directory structures, and block mappings are consolidated into AES-256-GCM encrypted blobs.
* **Cryptographic Indexing:** Lookups rely on HMAC-derived hashes to avoid storing plaintext names.
* **Encrypted Relationships:** Parent-child links and block mappings are cryptographically protected using AES-SIV and opaque handles.
* **Anti-Forensic Measures:** Fixed-size storage units mask size patterns; timestamps include randomized offsets; unused fields are populated with noise data.

This architecture guarantees that no unauthorized party, including backend administrators, can extract meaningful metadata without the appropriate cryptographic keys, ensuring confidentiality even in adversarial conditions.

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

## License

This project is licensed under the GNU Affero General Public License v3.0.  
See the [LICENSE.txt](LICENSE.txt) file for details.
  
This project uses OpenSSL, which is licensed under the Apache License 2.0.  
Please see the `LICENSE-DEPENDENCIES.txt` file for details on third-party licenses.

## Contributions

We're not accepting contributions at this stage.  
Stay tuned for updates as the foundation stabilizes!

## Contact:

Mohammed Ifkirne
Email: mohammedifkirne569@gmail.com
GitHub: [https://github.com/MohaIfk/](https://github.com/MohaIfk/)
