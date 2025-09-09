# roundlib

A header‑only `C++` library to round and format for display a measured value and its associated uncertainties.

The library currently implements the Particle Data Group rounding algorithm and a fixed two-digit precision algorithm. 

Numbers can be provided in (hopefully!) any standard string-like and numeric-like `C++` type. Rounding is performed with integer arithmetic, ensuring no information loss occurs, apart from the initial conversion from numeric-like to string, if needed. The total error, when used, is computed minimizing numerical errors and assuming that all errors are uncorrelated, i.e., it is the quadrature sum of the individual errors, symmetrized with the average.

Supported display modes are `terminal`, `(La)TeX`, `typst`, `gnuplot`.

A simple executable is also provided, to format numbers from the command line.


## Usage

#### Library (`roundlib.hpp`)

Just include the header `roundlib.hpp` in your favourite `C++` program.

There is a dependency either on the `fmt` library or on its header-only version. The latter case can be activated by uncommenting a line in both `round.cc` and `roundlib.hpp`.

Generic example using a measurement and the specialization for the `fmt` library.
```cpp
rounder::format_options opts;
std::string_view val = "27.462";
std::vector<std::string_view> errors = {".3234", "+.2864", "-.124", "0.023"};
std::vector<std::string_view> labels = {"(stat)", "(syst)", "(theo)", "(more)"};
opts.labels = &labels;
fmt::print("{}", rounder::format(val, errors, opts));
```

Generic example using a measurement and the specialization for the `fmt` library.
```cpp
double more_syst = 0.456;
rounder::measurement m{27.462, {.3234, {.2864, +1}, {.124, -1}, {0.023}, more_syst}, {"(stat)", "(syst)", "(theo)", "(more)"}};
fmt::println("{:tlT}", m);
```

#### Executable (`round`)

Clone the repository, `cd` into it and `make`. The `Makefile` is for `clang++`, change to `g++` or other compilers you may be using.
The executable depends on `fmt`, which is available for many platforms as a normal or header-only library. The `Makefile` currently links to `fmt`, but this can be trivially changed.
