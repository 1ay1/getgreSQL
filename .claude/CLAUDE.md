# getgreSQL

PostgreSQL web manager/viewer/monitor built in C++23 with type-theoretic compile-time design.

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -Wno-dev -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build build -j$(nproc)
```

## Run

```bash
./build/getgresql --pg-url "postgresql://user:pass@localhost/mydb" --port 5433
```

## Test

```bash
./build/tests/tests
```

## Dependencies

- GCC 15+ (C++23)
- Boost 1.83+ (Beast/Asio, system-installed)
- libpq (postgresql-libs package)
- htmx (vendored in assets/js/)

## Architecture

- **Type-state connections**: `PgConnection<State>` with phantom types (Disconnected, Connected, InTransaction). Invalid transitions are compile errors.
- **Compile-time route table**: Routes are types in `api/routes.hpp`. Path parameters extracted via `StaticString` pattern matching.
- **`std::expected` everywhere**: No exceptions. Monadic error handling via `Result<T>`.
- **htmx frontend**: Server-rendered HTML with htmx for partial updates. All assets embedded in binary.
