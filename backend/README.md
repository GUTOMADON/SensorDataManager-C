# SensorDataManager-C Backend

A C HTTP API that persists industrial sensor readings in PostgreSQL. See the
[root README](../README.md) for the overall architecture, database schema,
and full API reference with examples. This document only covers building and
running the backend itself.

## Technology

- **Language:** C (C11)
- **Database access:** [libpq](https://www.postgresql.org/docs/current/libpq.html), the official PostgreSQL C client library, using parameterized queries throughout
- **HTTP server:** [libmicrohttpd](https://www.gnu.org/software/libmicrohttpd/), a lightweight embeddable HTTP server library
- **JSON:** [json-c](https://github.com/json-c/json-c) for request parsing and response serialization
- **Build system:** GNU Make

## Source layout

```
backend/
├── include/            Public headers, one per module
│   ├── config.h         environment based configuration
│   ├── db.h              PostgreSQL data access layer
│   ├── handlers.h        request handlers (business logic)
│   ├── http_server.h     libmicrohttpd wiring
│   ├── json_util.h        struct -> JSON conversion helpers
│   └── models.h           plain domain structs
├── src/                Implementation files matching the headers above
│   ├── config.c
│   ├── db.c
│   ├── handlers.c
│   ├── http_server.c
│   ├── json_util.c
│   └── main.c            process entry point and lifecycle
└── Makefile
```

Each module has a single responsibility: `db.c` never touches HTTP, and
`http_server.c` never issues SQL. `handlers.c` is the only place that talks
to both.

## Prerequisites

Tested on Ubuntu/Debian. Install the development headers and libraries for
libpq, libmicrohttpd, and json-c, plus a compiler and pkg-config:

```bash
sudo apt-get update
```

```bash
sudo apt-get install -y build-essential pkg-config libpq-dev libmicrohttpd-dev libjson-c-dev postgresql-client
```

On Fedora/RHEL, the equivalent packages are `gcc make pkgconfig postgresql-devel libmicrohttpd-devel json-c-devel`.
On macOS with Homebrew: `brew install libpq libmicrohttpd json-c pkg-config`.
On Windows, use WSL (Windows Subsystem for Linux) with an Ubuntu distribution and follow the Linux steps above; libmicrohttpd and libpq are not commonly packaged for native MSVC builds.

## Configuration

The backend reads its configuration from environment variables at startup.
All of them are optional and have local-development defaults.

| Variable          | Default                 | Description                                             |
|-------------------|--------------------------|-----------------------------------------------------------|
| `DATABASE_URL`     | (unset)                 | Full libpq connection string or URI. Overrides the individual `SDM_DB_*` variables when set. |
| `SDM_DB_HOST`      | `localhost`              | PostgreSQL host                                            |
| `SDM_DB_PORT`      | `5432`                   | PostgreSQL port                                             |
| `SDM_DB_NAME`      | `sensor_data_manager`    | Database name                                                |
| `SDM_DB_USER`      | `postgres`               | Database user                                                 |
| `SDM_DB_PASSWORD`  | `postgres`               | Database password                                              |
| `SDM_HTTP_PORT`    | `8080`                   | Port the HTTP API listens on                                    |

Example using individual variables:

```bash
export SDM_DB_HOST=localhost SDM_DB_PORT=5432 SDM_DB_NAME=sensor_data_manager SDM_DB_USER=postgres SDM_DB_PASSWORD=postgres SDM_HTTP_PORT=8080
```

Or with a single connection URL:

```bash
export DATABASE_URL="postgresql://postgres:postgres@localhost:5432/sensor_data_manager"
```

## Build

From the `backend/` directory:

```bash
make
```

This produces `backend/bin/sensor_data_manager`. Object files go in
`backend/build/` (both directories are git-ignored).

## Run

```bash
./bin/sensor_data_manager
```

The server prints its configuration, probes the database connection, and
then listens for HTTP requests:

```
SensorDataManager-C backend starting...
HTTP port: 8080
Connected to PostgreSQL successfully.
Listening on http://0.0.0.0:8080
Try: curl http://localhost:8080/api/health
Press Ctrl+C to stop.
```

Stop the server with `Ctrl+C` (SIGINT) or `SIGTERM`; both trigger a clean
shutdown of the HTTP daemon.

## Concurrency and connection model

Each incoming HTTP request opens a short-lived PostgreSQL connection, runs
its query, and closes the connection before the response is sent. This
keeps every request self-contained and avoids sharing a single `PGconn`
across the worker threads that libmicrohttpd spawns per connection (libpq
connections are not safe to use concurrently from multiple threads). It is
simple and correct; the [root README's Limitations section](../README.md#limitations-and-future-work)
notes connection pooling as a natural next step for higher throughput.

## Cleaning build artifacts

```bash
make clean
```
