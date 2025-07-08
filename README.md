# PLeSATY - PoloLetargický SAT Yolver

Projekt do kurzu [IA085](https://is.muni.cz/auth/predmet/fi/jaro2024/IA085).

## Inštalácia

```
cmake -B build
make -C build
```

## Pustenie solveru

Rozhranie Solveru je pomocou stdin a stdout, napr.

```
> cat test.dimacs | build/src/sat
s UNSATISFIABLE
```
