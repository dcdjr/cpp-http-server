# C++ HTTP Server

A lightweight multithreaded HTTP/1.1 server written in C++17 using POSIX sockets.

## Features

- Raw TCP socket handling
- HTTP/1.1 request parsing
- Static file serving from `static/`
- MIME type detection
- Thread-per-connection concurrency
- Directory traversal protection
- Request timeout and size-limit handling
- Structured HTTP error responses
- Console request logging

## Build and Run

```bash
make
make run
