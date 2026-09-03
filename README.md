# High-Performance-CPP-HTTP-Server

## BUILD COMMANDS

### Executable build and run

```bash
cmake -S . -B build
cmake --build build

./build/http_server
```

### Test build and run

```bash
cmake -S . -B build
cmake --build build
cd build
ctest --output-on-failure
```

## Formatting

```bash
find . \( -name "*.cpp" -o -name "*.hpp" \) -exec clang-format -i {} +
```
