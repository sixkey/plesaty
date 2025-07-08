# PLeSATY - PoloLetargický SAT Yolver

Projekt do kurzu [IA085](https://is.muni.cz/auth/predmet/fi/jaro2024/IA085).

## Installation

```
cmake -B build
make -C build
```

## Running the solver

Interface using stdin and stdout, eg.

```
> cat test.dimacs | build/src/sat
s UNSATISFIABLE
```
