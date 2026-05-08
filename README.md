# C++ HTTP Server

A lightweight multithreaded HTTP/1.1 server written in C++17 using POSIX sockets.

## Overview

This project implements a small web server from scratch to explore networking, request parsing, concurrency, and defensive systems programming.

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
```

Then open the server in a browser or send a request with `curl`.

## Why This Project Matters

This project demonstrates practical understanding of:

- TCP socket programming
- HTTP request/response structure
- File serving and MIME types
- Basic concurrency
- Error handling in network services
- Defensive checks for unsafe paths and malformed requests
