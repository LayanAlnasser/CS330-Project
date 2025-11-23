# Library Access Simulation

> A concurrent access simulation for a shared library catalog system using POSIX threads

## Overview

This project simulates concurrent access to a shared library catalog by students and librarians, implementing fine-grained synchronization to prevent race conditions while maintaining data consistency.

### Key Features
- **Fine-grained readers-writers synchronization** using POSIX threads
- **Multiple students** can read books concurrently  
- **Multiple librarians** can update different books concurrently
- **Book catalog management** with add/remove operations
- **Comprehensive logging** with timestamps for all access patterns
- **Starvation prevention** and resource contention management

## System Architecture

### Entities
- **Students (S threads)**: Read book summaries and data concurrently
- **Librarians (L threads)**: Update book metadata, add/remove books
- **Book Catalog**: Shared resource with individual book locks

### Synchronization Strategy
- **Per-book read-write locks** for fine-grained access control
- **Global catalog lock** for structural changes (add/remove operations)
- **Tracked metrics**: `read_count` and `update_count` for each book


### Prerequisites
```bash
sudo apt-get update
sudo apt-get install gcc make
