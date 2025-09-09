# roundlib

A header‑only `C++` library to round and format for display a measured value and its associated uncertainties.

The library currently implements the Particle Data Group rounding algorithm and a fixed two-digit precision algorithm. 

Numbers can be provided in (hopefully!) any standard string-like and numeric-like `C++` type. Rounding is performed with integer arithmetic, ensuring no information loss occurs, apart from the initial conversion from numeric-like to string, if needed. The total uncertainty, when used, is computed minimizing numerical errors and assuming that all uncertainties are uncorrelated, i.e., it is the quadrature sum of the individual uncertainties, symmetrized with the average if asymmetric.

Supported display modes are `terminal`, `(La)TeX`, `typst`, `gnuplot`.

A simple executable is also provided, to format numbers from the command line.


## Structure

The library exposes few API functions for basic common use cases.

### Format numbers provided as standard `C++` types
```cpp
template<typename V, typename E>
inline std::string format(const V& val, const E& err,
                          const format_options &opt = {})
{ /* ... */ }
```
where the inputs are any scalar or container whose elements are string‑like (`std::string`, `const char*`, `std::string_view`) or numeric‑like (`int`, `float`, `double`, etc.). The central value (`val`) and the error term(s) (`err`) may be of different types and may be supplied either as a single value or as a standard container (e.g. std::vector<double>, std::list<std::string>).

### Format numbers provided via the `number` type
```cpp
inline std::string format_numbers(number value, std::vector<number>& errors,
                          const format_options &opt = {})
{ /* ... */ }
```
where `rounder::number` holds the central value, `std::vector<rounder::number>` contains the error terms, and optionally `format_options` specify the options different from the default.

Standard `C++` types can be converted to `number` via an implicit constructor or three helper functions:
```cpp
static number from_numeric(const T v, int sgn = 0)
{ /* ... */ }

static number from_string(std::string_view sv)
{ /* ... */ }

static number from_anything(const T& v, int sgn = 0)
{ /* ... */ }
```
In case of an uncertainty, the parameter `sgn` regulates if it is a symmetric uncertainty (`sgn = 0`), a higher (`sgn = +1`), or a lower (`sgn = -1`) uncertainty. This parameter is only necessary for numeric-like types, as for string-like types it is deduced from the sign of the provided value.

### Format a `measurement` using the `fmt::formatter` specialization



### Options

The final formatting is regulated via different options, provided either as members of the `format_options` structure or as single-letter knobs for the specialization of `fmt`.


## Examples of usage

### Library (`roundlib.hpp`)

Just include the header `roundlib.hpp` in your favourite `C++` program.

There is a dependency either on the `fmt` library or on its header-only version. The latter case can be activated by uncommenting a line in both `round.cc` and `roundlib.hpp`.

Generic example using `number` and the `format` function of `roundlib` (more options are used in `round.cc`):
```cpp
#include "roundlib.hpp"

// ... more code

std::string_view val = "27.462";

// multiple errors, some asymmetric
std::vector<std::string_view> errors = {".3234", "+.2864", "-.124", "0.023"};
std::vector<std::string_view> labels = {"(stat)", "(syst)", "(theo)", "(more)"};
rounder::format_options opts;
opts.labels = &labels;
fmt::print("{}", rounder::format(val, errors, opts));

// single error, simple printing
std::string_view err = "0.321";
fmt::print("{}", rounder::format(val, error));
```

Generic example using a measurement and the specialization for the `fmt` library:
```cpp
#include "roundlib.hpp"

// ... more code

double more_syst = 0.456;
rounder::measurement m{27.462, {.3234, {.2864, +1}, {.124, -1}, {0.023}, more_syst}, {"(stat)", "(syst)", "(theo)", "(more)"}};
fmt::println("{:tlT}", m);
```

### Executable (`round`)

Clone the repository, `cd` into it and `make`. The `Makefile` is for `clang++`, change to `g++` or other compilers you may be using.
The executable depends on `fmt`, which is available for many platforms as a normal or header-only library. The `Makefile` currently links to `fmt`, but this can be trivially changed.
