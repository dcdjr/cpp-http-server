# C++ HTTP Server

A lightweight HTTP/1.1 server written in C++ using POSIX sockets. Supports basic request parsing, static file serving, and multithreaded client handling.

## Features
- HTTP/1.1 request parsing (method, path, headers, body)
- Static file serving from `./static` (defaults `/` → `/index.html`)
- MIME type detection (html, css, js, png, jpg/jpeg, ico, txt)
- Thread-per-connection handling with `std::thread`
- Directory traversal protection (`..` → `403`)
- Timeouts + size limits:
  - `408 Request Timeout` (receive timeout)
  - `413 Payload Too Large` (max request buffer)
- Errors / validation:
  - `400 Malformed Request`
  - `404 Not Found`
  - `405 Method Not Allowed`
  - `505 HTTP Version Not Supported`
- Console logging: timestamp, client IP, method, path, status, response size

## How It Works
1. Listen on port `8080`
2. Accept a connection and handle it in a detached thread
3. Read until `\r\n\r\n` (headers), then read body based on `Content-Length`
4. Parse into `HTTPRequest { method, path, version, headers, body }`
5. Route:
   - `GET` serves files from `./static`
   - Other methods return placeholder responses
6. Write full response and log the request

## Project layout

```text
include/   headers
src/       server, parser, router, and socket code
static/    files served by the HTTP server
bin/       compiled binary output
```

## Requirements

- Linux or WSL
- `g++`
- `make`

## Build & Run
```bash
g++ -std=c++17 -pthread *.cpp -o server
./server
```

Open: http://localhost:8080

## Quick checks

```bash
curl -i http://127.0.0.1:8080/
curl -i http://127.0.0.1:8080/style.css
curl -i http://127.0.0.1:8080/missing
```

## Notes

- Run the server from the repository root so relative paths like `static/index.html` resolve correctly.
- Maximum request size is `8192` bytes.
- The current router is intentionally simple and does not implement dynamic routes.
