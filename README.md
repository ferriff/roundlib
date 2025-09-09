# roundlib

A header‑only `C++` library to round and format for display a measured value and its associated uncertainties.

The library currently implements the Particle Data Group rounding algorithm and and a fixed two-digit precision algorithm. 

Numbers can be provided in (hopefully!) any standard string-like and numeric-like `C++` type. Rounding is performed with integer arithmetic, ensuring no information loss occurs, apart from the initial conversion from numeric-like to string, if needed. The total error, when used, is computed minimizing numerical errors and assuming that all errors are uncorrelated, i.e., it is the quadrature sum of the individual errors, symmetrized with the average.

Supported display modes are `terminal`, `(La)TeX`, `typst`, `gnuplot`.

A simple executable is also provided, to format numbers from the command line.

## Installation

## Usage
