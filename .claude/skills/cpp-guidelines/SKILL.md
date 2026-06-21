---
name: cpp-guidelines
description: C++ coding standards based on the C++ Core Guidelines (isocpp.github.io). Use when writing, reviewing, or refactoring C++ code to enforce modern, safe, and idiomatic practices.
disable-model-invocation: true
---

# C++ Coding Standards (C++ Core Guidelines)

Comprehensive coding standards for modern C++ (C++17/20/23) derived from the [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines). Enforces type safety, resource safety, immutability, and clarity.

> **Note on code examples**: Comments inside code blocks (lines starting with `//`) explain the rationale behind each pattern. They are meant to help the AI understand the intent — the AI should **not** reproduce these explanatory comments verbatim in generated code, unless they genuinely serve as documentation for human readers.

## When to Use

- Writing new C++ code (classes, functions, templates)
- Reviewing or refactoring existing C++ code
- Making architectural decisions in C++ projects
- Enforcing consistent style across a C++ codebase
- Choosing between language features (e.g., `enum` vs `enum class`, raw pointer vs smart pointer)

### When NOT to Use

- Non-C++ projects
- Legacy C codebases that cannot adopt modern C++ features
- Embedded/bare-metal contexts where specific guidelines conflict with hardware constraints (adapt selectively)

## Cross-Cutting Principles

These themes recur across the entire guidelines and form the foundation:

1. **RAII everywhere**: Bind resource lifetime to object lifetime
2. **Immutability by default**: Start with `const`/`constexpr`; mutability is the exception
3. **Type safety**: Use the type system to prevent errors at compile time
4. **Express intent**: Names, types, and concepts should communicate purpose
5. **Minimize complexity**: Simple code is correct code
6. **Value semantics over pointer semantics**: Prefer returning by value and scoped objects

## Philosophy & Interfaces

- Express ideas directly in code
- Express intent
- Ideally, a program should be statically type safe
- Prefer compile-time checking to run-time checking
- Don't leak any resources
- Prefer immutable data to mutable data
- Make interfaces explicit
- Avoid non-const global variables
- Make interfaces precisely and strongly typed
- Never transfer ownership by a raw pointer or reference
- Keep the number of function arguments low

### DO

```cpp
// Immutable, strongly typed interface
struct Temperature {
    double kelvin;
};

Temperature boil(const Temperature& water);
```

### DON'T

```cpp
// Weak interface: unclear ownership, unclear units
double boil(double* temp);

// Non-const global variable — avoid
int g_counter = 0;
```

## Functions

- Package meaningful operations as carefully named functions
- A function should perform a single logical operation
- Keep functions short and simple
- If a function might be evaluated at compile time, declare it `constexpr`
- If your function must not throw, declare it `noexcept`
- Prefer pure functions
- For "in" parameters, pass cheaply-copied types by value and others by `const&`
- For "out" values, prefer return values to output parameters
- To return multiple "out" values, prefer returning a struct
- Never return a pointer or reference to a local object

### Parameter Passing

```cpp
// Cheap types by value, others by const&
void print(int x);                           // cheap: by value
void analyze(const std::string& data);       // expensive: by const&
void transform(std::string s);               // sink: by value (will move)

// Return values, not output parameters
struct ParseResult {
    std::string token;
    int position;
};

ParseResult parse(std::string_view input);   // GOOD: return struct

// BAD: output parameters
void parse(std::string_view input,
           std::string& token, int& pos);    // avoid this
```

### Value Semantics

- Pass small/trivial types by value
- Pass large types by `const&`
- Return by value (rely on RVO/NRVO)
- Use move semantics for sink parameters

### Pure Functions and constexpr

```cpp
// Pure, constexpr where possible
constexpr int factorial(int n) noexcept {
    return (n <= 1) ? 1 : n * factorial(n - 1);
}

static_assert(factorial(5) == 120);
```

### Anti-Patterns

- Returning `T&&` from functions
- Using `va_arg` / C-style variadics
- Capturing by reference in lambdas passed to other threads
- Returning `const T` which inhibits move semantics

## Classes & Class Hierarchies

- Use `class` if invariant exists; `struct` if data members vary independently
- Minimize exposure of members
- If you can avoid defining default operations, do (Rule of Zero)
- If you define or `=delete` any copy/move/destructor, handle them all (Rule of Five)
- Base class destructor: public virtual or protected non-virtual
- A constructor should create a fully initialized object
- Declare single-argument constructors `explicit`
- A polymorphic class should suppress public copy/move
- Virtual functions: specify exactly one of `virtual`, `override`, or `final`

### Rule of Zero

```cpp
// Let the compiler generate special members
struct Employee {
    std::string name;
    std::string department;
    int id;
    // No destructor, copy/move constructors, or assignment operators needed
};
```

### Rule of Five

```cpp
// If you must manage a resource, define all five
class Buffer {
public:
    explicit Buffer(std::size_t size)
        : data(std::make_unique<char[]>(size)), size(size) {}

    ~Buffer() = default;

    Buffer(const Buffer& other)
        : data(std::make_unique<char[]>(other.size)), size(other.size) {
        std::copy_n(other.data.get(), size, data.get());
    }

    Buffer& operator=(const Buffer& other) {
        if (this != &other) {
            auto newdata = std::make_unique<char[]>(other.size);
            std::copy_n(other.data.get(), other.size, newdata.get());
            data = std::move(newdata);
            size = other.size;
        }
        return *this;
    }

    Buffer(Buffer&&) noexcept = default;
    Buffer& operator=(Buffer&&) noexcept = default;

private:
    std::unique_ptr<char[]> data;
    std::size_t size;
};
```

### Class Hierarchy

```cpp
// Virtual destructor, use override
class Shape {
public:
    virtual ~Shape() = default;
    virtual double area() const = 0;  // pure interface
};

class Circle : public Shape {
public:
    explicit Circle(double r) : radius(r) {}
    double area() const override { return 3.14159 * radius * radius; }

private:
    double radius;
};
```

### Anti-Patterns

- Calling virtual functions in constructors/destructors
- Using `memset`/`memcpy` on non-trivial types
- Providing different default arguments for virtual function and overrider
- Making data members `const` or references, which suppresses move/copy

## Resource Management

- Manage resources automatically using RAII
- A raw pointer (`T*`) is non-owning
- Prefer scoped objects; don't heap-allocate unnecessarily
- Avoid `malloc()`/`free()`
- Avoid calling `new` and `delete` explicitly
- Use `unique_ptr` or `shared_ptr` to represent ownership
- Prefer `unique_ptr` over `shared_ptr` unless sharing ownership
- Use `make_shared()` to make `shared_ptr`s

### Smart Pointer Usage

```cpp
// RAII with smart pointers
auto widget = std::make_unique<Widget>("config");  // unique ownership
auto cache  = std::make_shared<Cache>(1024);        // shared ownership

// Raw pointer = non-owning observer
void render(const Widget* w) {  // does NOT own w
    if (w) w->draw();
}

render(widget.get());
```

### RAII Pattern

```cpp
// Resource acquisition is initialization
class FileHandle {
public:
    explicit FileHandle(const std::string& path)
        : handle(std::fopen(path.c_str(), "r")) {
        if (!handle) throw std::runtime_error("Failed to open: " + path);
    }

    ~FileHandle() {
        if (handle) std::fclose(handle);
    }

    FileHandle(const FileHandle&) = delete;
    FileHandle& operator=(const FileHandle&) = delete;
    FileHandle(FileHandle&& other) noexcept
        : handle(std::exchange(other.handle, nullptr)) {}
    FileHandle& operator=(FileHandle&& other) noexcept {
        if (this != &other) {
            if (handle) std::fclose(handle);
            handle = std::exchange(other.handle, nullptr);
        }
        return *this;
    }

private:
    std::FILE* handle;
};
```

### Anti-Patterns

- Naked `new`/`delete`
- `malloc()`/`free()` in C++ code
- Multiple resource allocations in a single expression (exception safety hazard)
- `shared_ptr` where `unique_ptr` suffices

## Expressions & Statements

- Keep scopes small
- Always initialize an object
- Prefer `{}` initializer syntax
- Declare objects `const` or `constexpr` unless modification is intended
- Use lambdas for complex initialization of `const` variables
- Avoid magic constants; use symbolic constants
- Avoid narrowing/lossy arithmetic conversions
- Use `nullptr` rather than `0` or `NULL`
- Avoid casts
- Don't cast away `const`

### Initialization

```cpp
// Always initialize, prefer {}, default to const
const int max_retries{3};
const std::string name{"widget"};
const std::vector<int> primes{2, 3, 5, 7, 11};

// Lambda for complex const initialization
const auto config = [&] {
    Config c;
    c.timeout = std::chrono::seconds{30};
    c.retries = maxRetries;
    c.verbose = debugMode;
    return c;
}();
```

### Anti-Patterns

- Uninitialized variables
- Using `0` or `NULL` as pointer (use `nullptr`)
- C-style casts (use `static_cast`, `const_cast`, etc.)
- Casting away `const`
- Magic numbers without named constants
- Mixing signed and unsigned arithmetic
- Reusing names in nested scopes

## Error Handling

- Develop an error-handling strategy early in a design
- Throw an exception to signal that a function can't perform its assigned task
- Use RAII to prevent leaks
- Use `noexcept` when throwing is impossible or unacceptable
- Use purpose-designed user-defined types as exceptions
- Throw by value, catch by reference
- Destructors, deallocation, and swap must never fail
- Don't try to catch every exception in every function

### Exception Hierarchy

```cpp
// Custom exception types, throw by value, catch by reference
class AppError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class NetworkError : public AppError {
public:
    NetworkError(const std::string& msg, int code)
        : AppError(msg), status_code(code) {}
    int status_code;
};

void fetch_data(const std::string& url) {
    // Throw to signal failure
    throw NetworkError("connection refused", 503);
}

void run() {
    try {
        fetch_data("https://api.example.com");
    } catch (const NetworkError& e) {
        log_error(e.what(), e.status_code);
    } catch (const AppError& e) {
        log_error(e.what());
    }
    // Don't catch everything here — let unexpected errors propagate
}
```

### Anti-Patterns

- Throwing built-in types like `int` or string literals
- Catching by value (slicing risk)
- Empty catch blocks that silently swallow errors
- Using exceptions for flow control
- Error handling based on global state like `errno`

## Constants & Immutability

- By default, make objects immutable
- By default, make member functions `const`
- By default, pass pointers and references to `const`
- Use `const` for values that don't change after construction
- Use `constexpr` for values computable at compile time

```cpp
// Immutability by default
class Sensor {
public:
    explicit Sensor(std::string id) : id(std::move(id)) {}

    // const member functions by default
    const std::string& id() const { return id; }
    double last_reading() const { return reading; }

    // Only non-const when mutation is required
    void record(double value) { reading = value; }

private:
    const std::string id;  // never changes after construction
    double reading{0.0};
};

// Pass by const reference
void display(const Sensor& s) {
    std::cout << s.id() << ": " << s.last_reading() << '\n';
}

// Compile-time constants
constexpr double PI = 3.14159265358979;
constexpr int MAX_SENSORS = 256;
```

## Concurrency & Parallelism

- Avoid data races
- Minimize explicit sharing of writable data
- Think in terms of tasks, rather than threads
- Don't use `volatile` for synchronization
- Use RAII, never plain `lock()`/`unlock()`
- Use `std::scoped_lock` to acquire multiple mutexes
- Never call unknown code while holding a lock
- Don't wait without a condition
- Remember to name your `lock_guard`s and `unique_lock`s
- Don't use lock-free programming unless you absolutely have to

### Safe Locking

```cpp
// RAII locks, always named
class ThreadSafeQueue {
public:
    void push(int value) {
        std::lock_guard<std::mutex> lock(mutex);  // named!
        queue.push(value);
        cv.notify_one();
    }

    int pop() {
        std::unique_lock<std::mutex> lock(mutex);
        // Always wait with a condition
        cv.wait(lock, [this] { return !queue.empty(); });
        const int value = queue.front();
        queue.pop();
        return value;
    }

private:
    std::mutex mutex;             // mutex with its data
    std::condition_variable cv;
    std::queue<int> queue;
};
```

### Multiple Mutexes

```cpp
// std::scoped_lock for multiple mutexes (deadlock-free)
void transfer(Account& from, Account& to, double amount) {
    std::scoped_lock lock(from.mutex_, to.mutex_);
    from.balance_ -= amount;
    to.balance_ += amount;
}
```

### Anti-Patterns

- `volatile` for synchronization (it's for hardware I/O only)
- Detaching threads (lifetime management becomes nearly impossible)
- Unnamed lock guards: `std::lock_guard<std::mutex>(m);` destroys immediately
- Holding locks while calling callbacks (deadlock risk)
- Lock-free programming without deep expertise

## Templates & Generic Programming

- Use templates to raise the level of abstraction
- Use templates to express algorithms for many argument types
- Specify concepts for all template arguments
- Use standard concepts whenever possible
- Prefer shorthand notation for simple concepts
- Prefer `using` over `typedef`
- Use template metaprogramming only when you really need to
- Don't specialize function templates (overload instead)

### Concepts (C++20)

```cpp
#include <concepts>

// Constrain templates with standard concepts
template<std::integral T>
T gcd(T a, T b) {
    while (b != 0) {
        a = std::exchange(b, a % b);
    }
    return a;
}

// Shorthand concept syntax
void sort(std::ranges::random_access_range auto& range) {
    std::ranges::sort(range);
}

// Custom concept for domain-specific constraints
template<typename T>
concept Serializable = requires(const T& t) {
    { t.serialize() } -> std::convertible_to<std::string>;
};

template<Serializable T>
void save(const T& obj, const std::string& path);
```

### Anti-Patterns

- Unconstrained templates in visible namespaces
- Specializing function templates instead of overloading
- Template metaprogramming where `constexpr` suffices
- `typedef` instead of `using`

## Standard Library

- Use libraries wherever possible
- Prefer the standard library to other libraries
- Prefer `std::array` or `std::vector` over C arrays
- Prefer `std::vector` by default
- Use `std::string` to own character sequences
- Use `std::string_view` to refer to character sequences
- Avoid `endl` (use `'\n'` — `endl` forces a flush)

```cpp
// Prefer vector/array over C arrays
const std::array<int, 4> fixed_data{1, 2, 3, 4};
std::vector<std::string> dynamic_data;

// string owns, string_view observes
std::string build_greeting(std::string_view name) {
    return "Hello, " + std::string(name) + "!";
}

// Use '\n' not endl
std::cout << "result: " << value << '\n';
```

## Enumerations

- Prefer enumerations over macros
- Prefer `enum class` over plain `enum`
- Don't use ALL_CAPS for enumerators
- Avoid unnamed enumerations

```cpp
// Scoped enum, no ALL_CAPS
enum class Color { red, green, blue };
enum class LogLevel { debug, info, warning, error };

// BAD: plain enum leaks names, ALL_CAPS clashes with macros
enum { RED, GREEN, BLUE };           // avoid plain enum
#define MAX_SIZE 100                  // use constexpr instead
```

## Source Files & Naming

- Use `.cpp` for code files and `.h` for interface files
- Don't write `using namespace` at global scope in a header
- Use `#include` guards for all `.h` files
- Header files should be self-contained
- Avoid encoding type information in names (no Hungarian notation)
- Use a consistent naming style
- Use ALL_CAPS for macro names only
- Prefer `underscore_style` names

### Header Guard

```cpp
// Include guard (or #pragma once)
#ifndef PROJECT_MODULE_WIDGET_H
#define PROJECT_MODULE_WIDGET_H

// Self-contained — include everything this header needs
#include <string>
#include <vector>

namespace project::module {

class Widget {
public:
    explicit Widget(std::string name);
    const std::string& name() const;

private:
    std::string name_;
};

}  // namespace project::module

#endif  // PROJECT_MODULE_WIDGET_H
```

### Naming Conventions

Follow one consistent convention. If the project does not define one via clang-format, choose from:

**Option A — Underscore style:**

```cpp
namespace my_project {

constexpr int max_buffer_size = 4096;  // not ALL_CAPS (it's not a macro)

class tcp_connection {                 // underscore_style class
public:
    void send_message(std::string_view msg);
    bool is_connected() const;

private:
    std::string host;                 // trailing underscore for members
    int port;
};

}  // namespace my_project
```

**Option B — PascalCase style:**

- Types/Classes: `PascalCase`
- Functions/Methods: `snake_case` or `camelCase` (follow project convention)
- Constants: `kPascalCase` or `UPPER_SNAKE_CASE`
- Namespaces: `lowercase`
- Member variables: `snake_case_` (trailing underscore) or `m_` prefix

### Anti-Patterns

- `using namespace std;` in a header at global scope
- Headers that depend on inclusion order
- Hungarian notation like `strName`, `iCount`
- ALL_CAPS for anything other than macros

## Performance

- Don't optimize without reason
- Don't optimize prematurely
- Don't make claims about performance without measurements
- Design to enable optimization
- Rely on the static type system
- Move computation from run time to compile time
- Access memory predictably

### Guidelines

```cpp
// Compile-time computation where possible
constexpr auto lookup_table = [] {
    std::array<int, 256> table{};
    for (int i = 0; i < 256; ++i) {
        table[i] = i * i;
    }
    return table;
}();

// Prefer contiguous data for cache-friendliness
std::vector<Point> points;           // GOOD: contiguous
std::vector<std::unique_ptr<Point>> indirect_points; // BAD: pointer chasing
```

### Anti-Patterns

- Optimizing without profiling data
- Choosing "clever" low-level code over clear abstractions
- Ignoring data layout and cache behavior

## Formatting

- Use **clang-format** — no style debates
- Run `clang-format -i <file>` before committing

## Quick Reference Checklist

Before marking C++ work complete:

- [ ] No raw `new`/`delete` — use smart pointers or RAII
- [ ] Objects initialized at declaration
- [ ] Variables are `const`/`constexpr` by default
- [ ] Member functions are `const` where possible
- [ ] `enum class` instead of plain `enum`
- [ ] `nullptr` instead of `0`/`NULL`
- [ ] No narrowing conversions
- [ ] No C-style casts
- [ ] Single-argument constructors are `explicit`
- [ ] Rule of Zero or Rule of Five applied
- [ ] Base class destructors are public virtual or protected non-virtual
- [ ] Templates are constrained with concepts
- [ ] No `using namespace` in headers at global scope
- [ ] Headers have include guards and are self-contained
- [ ] Locks use RAII (`scoped_lock`/`lock_guard`)
- [ ] Exceptions are custom types, thrown by value, caught by reference
- [ ] `'\n'` instead of `std::endl`
- [ ] No magic numbers
