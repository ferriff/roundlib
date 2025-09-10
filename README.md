# roundlib

A header‑only `C++` library to round and format for display the central value of a measurement and its associated uncertainties.

The library currently implements the [Particle Data Group (PDG)](https://pdg.lbl.gov/2025/web/viewer.html?file=../reviews/rpp2024-rev-rpp-intro.pdf#subsection.0.5.3) rounding algorithm and a fixed two-digits precision algorithm. 

Numbers can be provided in (hopefully!) any standard string-like and numeric-like `C++` type. Rounding is performed with integer arithmetic, ensuring no information loss occurs, apart from the initial conversion from numeric-like to string, if needed. The total uncertainty, when used, is computed minimizing numerical errors and assuming that all uncertainties are uncorrelated, i.e., it is the quadrature sum of the individual uncertainties, symmetrized with the average if asymmetric.

Supported display modes are `terminal`, [(La)TeX](https://www.latex-project.org/), [typst](https://typst.app/), [gnuplot](http://gnuplot.info/).

A simple executable is also provided, to format numbers from the command line.



## TL;DR

For the impatient user:

```fish
> ./round -t -l 27.432 2.134 0.125 -L "(stat),(syst)"
27.4 ± 2.1 (stat) ± 0.1 (syst)

> ./round -t -e -X 27.462 +0.3134 -0.292 0.0124 -L "(stat),(syst),(theo)"
27.46 \,^{+0.31} _{-0.29} \text{(stat)} \pm 0.01 \text{(syst)}  
```

For the impatient coder:

```cpp
#include "roundlib.hpp"
#include <fmt/format.h>

int main()
{
        std::string_view central = "27.462";
        double even_more_syst = 0.456;
        rounder::measurement m{central, // central value
            // the uncertainties, given as a std::vector of `number's
            // implicitly constructed from `double`s,
            // with indications if upper (+1) or lower (-1) uncertainty
            {.3234, {.2064, +1}, {.194, -1}, {0.023}, even_more_syst},
            // an (optional) std::vector<std::string_view> indicating the labels to display
            // for the systematic uncertainties
            {"(stat)", "(syst)", "(theo)", "(more)"}};

        // round to two significant digits (t),
        // uniformize the precision to the largest uncertainty (l)
        // display in typst mode
        fmt::println("{:tlT}", m);

        // round with the PDG algorithm (p),
        // uniformize the precision to the total uncertainty (l),
        // symmetrize upper/lower uncertainty if within +/-10% (s),
        // display in (La)TeX mode (X)
        auto s = fmt::format("{:pesX}", m);
        fmt::println("{}", s);
        return 0;
}
```
Output:
```
27.46  plus.minus  0.32 "(stat)" #h(0.0em)^(+0.21) _(-0.19) "(syst)"  plus.minus  0.02 "(theo)"  plus.minus  0.46 "(more)"
27.5 \pm 0.3 \text{(stat)} \pm 0.2 \text{(syst)} \pm 0.0 \text{(theo)} \pm 0.5 \text{(more)}
```



## Library structure

The library exposes few API functions for basic common use cases. The only dependency beside the standard `C++` library is from the [fmt](https://github.com/fmtlib/fmt) formatting library.



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

Standard `C++` types can be converted to `number` via an implicit constructor, e.g.,
```cpp
rounder::number n = 1.23;       // central value or symmetric uncertainty
rounder::number n = {0.13, -1}; // asymmetric uncertainty, lower part
rounder::number n = "+0.14";    // asymmetric uncertainty, higher part
```
or via three explicit helper functions:
```cpp
static number from_numeric(const T v, int sgn = 0)
{ /* ... */ }

static number from_string(std::string_view sv)
{ /* ... */ }

static number from_anything(const T& v, int sgn = 0)
{ /* ... */ }
```
In case of an uncertainty, the parameter `sgn` regulates if it is a symmetric uncertainty (`sgn = 0`), a higher (`sgn = +1`), or a lower (`sgn = -1`) uncertainty. For string-like types, `sgn` is deduced from the sign of the provided value (none, `+`, `-`).



### Format a `measurement` using the `fmt::formatter` specialization
The `rounder::measurement` structure holds the central value, its associated uncertainties, and the optional labels for the uncertainties:
```cpp
struct measurement {
        rounder::number central;
        std::vector<rounder::number> errors;
        std::vector<std::string_view> labels;
};
```
The provided specialization of `fmt::formatter` round a measurement and its uncertainties according to the parsing options provided.



### Options

The final formatting is regulated via different options, provided as members of the `format_options` structure for code, or as single-letter knobs for the specialization of `fmt::formatter`, or as command-line options for the `round` executable. The available options are listed below, where most of the namespaces are omitted for compactness. Capital letters are for display options, lowercase letters for algorithmic options.

| `rounder::format_options` | `fmt::formatter` | `round` (command-line) | Description                                                  |
| ---                       | ---              | ---                    | ---                                                          |
| `mode = terminal`         |  -               |   -                    | display for the terminal                                     |
| `mode = tex`              | `X`              | `-X`                   | display for `(La)TeX` syntax (math mode)                     |
| `mode = typst`            | `T`              | `-T`                   | display for `typst` syntax (math mode)                       |
| `mode = gnuplot`          | `G`              | `-G`                   | display for `gnuplot` syntax                                 |
| `algo = pdg`              | `p`              | `-p`                   | round with the PDG algorithm                                 |
| `algo = twodigits`        | `t`              | `-t`                   | round to two-digits precision                                |
| `symmetrize_errors`       | `s`              | `-s`                   | symmetrize asymmetric errors if they differ by less than 10% |
| `prec_to_total_err`       | `e`              | `-e`                   | uniformize the precision to the rounding of the total error  |
| `prec_to_larger_err`      | `l`              | `-l`                   | uniformize the precision to the largest supplied error       |
| `factorize_powers`        | `F`              | `-F`                   | display with factorized powers of 10                         |
| `no_utf8`                 | `U`              | `-U`                   | do not use `utf8` chars when displaying to the terminal      |
| `cdot`                    | `D`              | `-D`                   | use a cdot instead of times symbol for the powers of 10      |
| -                         |  -               | `-N`                   | do not include the trailing new line                         |



## Usage

### Library (`roundlib.hpp`)

Just include the header `roundlib.hpp` in your favourite `C++` program.

`roundlib` depends on the [fmt](https://github.com/fmtlib/fmt) library, either from the `.so` or from the header-only version. The latter case can be chosen by compiling your code defining the variable `FMT_HEADER_ONLY`, e.g. `clang++ -DFMT_HEADER_ONLY ...`.

The `TL;DR` example already show how to use the specialization of `fmt::formatter`.

Additional examples showing more features and the `format` function of `roundlib`:
```cpp
#include "roundlib.hpp"
#include <fmt/format.h>

int main()
{
        // labels for the uncertainties (optional)
        std::vector<std::string_view> labels = {"(stat)", "(syst)", "(theo)", "(more)"};
        rounder::format_options opts;
        opts.prec_to_larger_err = true;
        opts.algo = rounder::format_options::round_algo::twodigits;
        opts.mode = rounder::mode_type::terminal;
        opts.labels = &labels;

        // central value as a string_view
        std::string_view val = "27.462";
        // multiple uncertainties as string_view, some asymmetric
        std::vector<std::string_view> errors = {".324", "+.286", "-.124", "0.0234"};
        // format the numbers
        std::string out = rounder::format(val, errors, opts);

        // central value as a double
        double dval = 27.462;
        // uncertainties (symmetric only) as double
        std::vector<double> derrors = {.3234, .2864, .124, 0.0234};
        std::string d_out = rounder::format(dval, derrors, opts);

        // central value as `number`
        rounder::number nval = "0.27462";
        // uncertainties (symmetric) as `number`
        std::vector<rounder::number> nerrors = {.003234, {.002864, +1}, {.00124, -1}, 0.000234};
        opts.factorize_powers = true;
        std::string n_out = rounder::format_numbers(nval, nerrors, opts);

        // can also mix types, if need be
        std::string m_out = rounder::format(val, derrors, opts);

        fmt::println("{}", out);
        fmt::println("{}", d_out);
        fmt::println("{}", n_out);
        fmt::println("{}", m_out);

        // single error, direct printing
        std::string_view err = "0.321";
        fmt::println("{}", rounder::format(dval, err));
        return 0;
}
```
Output:
```
27.46 ± 0.32 (stat) +0.29 -0.12 (syst) ± 0.02 (theo)
27.46 ± 0.32 (stat) ± 0.29 (syst) ± 0.12 (theo) ± 0.02 (more)
(2746 ± 32 (stat) +29 -12 (syst) ± 2 (theo))×10^-4
(2746 ± 32 (stat) ± 29 (syst) ± 12 (theo) ± 2 (more))×10^-2
27.46 ± 0.32
```

### Executable (`round`)

Clone the repository, `cd` into it and `make`.

The following compilers are tried in order: `CXX=...` as provided from the command line, `clang++`, `g++`. The compiler links to `fmt`, the only dependence.

The [fmt](https://github.com/fmtlib/fmt) formatting library is available for many platforms as a standard library or as a header-only library. Compile with `make HEADER_ONLY=1` if you prefer to use the header-only version.
