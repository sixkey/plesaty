import urllib.request
import tarfile
import os
import subprocess
import sys
import multiprocessing.pool
import functools


### Benchmark downloader

base_url = "https://www.cs.ubc.ca/~hoos/SATLIB/Benchmarks/SAT/RND3SAT/"
benchmarks =\
    [ "uf20-91.tar.gz"
    , "uf50-218.tar.gz"
    , "uuf50-218.tar.gz"
    , "uf75-325.tar.gz"
    , "uuf75-325.tar.gz"
    , "uf100-430.tar.gz"
    , "uuf100-430.tar.gz"
    , "uf125-538.tar.gz"
    , "uuf125-538.tar.gz"
    , "uf150-645.tar.gz"
    , "uuf150-645.tar.gz"
    , "uf175-753.tar.gz"
    , "uuf175-753.tar.gz"
    , "uf200-860.tar.gz"
    , "uuf200-860.tar.gz"
    , "uf225-960.tar.gz"
    , "uuf225-960.tar.gz"
    , "uf250-1065.tar.gz"
    , "uuf250-1065.tar.gz"
    ]
benchmarks_dir = "benchmarks"

def extract_formulas(tar_file):
    for member in tar_file:
        if member.isfile():
            _, benchmark_name = os.path.split(member.name)
            member.name = benchmark_name
            tar_file.extract(member, path=benchmarks_dir)

def download_benchmark(archive_name):
    url = base_url + archive_name
    filename, _ = urllib.request.urlretrieve(url)
    with tarfile.open(filename, "r:gz") as tar:
        extract_formulas(tar)


### Solver interface

SAT = "SAT"
UNSAT = "UNSAT"
UNKNOWN = "UNKNOWN"
TIMEOUT = "TIMEOUT"

def parse_solver_output(output):
    model = []
    status = None
    for line in output.splitlines():
        if line.startswith("s"):
            status = line[1:].strip()
        elif line.startswith("v"):
            # parse sequence of numbers and remove the trailing zero
            model = list(map(int, line[1:].strip().split()))[:-1]

    if status is None:
        raise RuntimeError("The format is not correct", output.splitlines() )

    if status == "SATISFIABLE":
        return SAT, model
    elif status == "UNSATISFIABLE":
        return UNSAT, None
    elif status == "UNKNOWN":
        return UNKNOWN, None
    else:
        assert(False)


def run_solver(formula_file, solver_path, timeout):
    with open(f"{benchmarks_dir}/{formula_file}", "r") as input:
        try:
            proc = subprocess.run([solver_path], stdin=input, timeout=timeout, capture_output=True)
            return parse_solver_output(proc.stdout.decode())
        except subprocess.TimeoutExpired:
            return TIMEOUT, None


### Verifier

def parse_formula(formula_file):
    clauses = []
    with open(f"{benchmarks_dir}/{formula_file}", "r") as file:
        for line in file:
            if line.startswith("c"):
                continue

            if line.startswith("p"):
                [_, _, var_count_s, clause_count_s] = line.split()
                clause_count = int(clause_count_s)
                continue

            clauses.append(list(map(int, line.strip().split()))[:-1])
            clause_count -= 1

            if clause_count == 0:
                return clauses

def verify_model(formula_file, model):
    clauses = parse_formula(formula_file)
    model_set = set(model)

    for clause in clauses:
        if len(set(clause) & model_set) == 0:
            return False
    return True

CORRECT = "CORRECT"
INCORRECT_SAT = "INCORRECT_SAT"
INCORRECT_UNSAT = "INCORRECT_UNSAT"
INCORRECT_MODEL = "INCORRECT_MODEL"

def test_formula(formula_file, solver_path, timeout):
    status, model = run_solver(formula_file, solver_path, timeout)

    if status == UNKNOWN or status == TIMEOUT:
        return status
    if formula_file.startswith("uf"):
        if status != SAT:
            return INCORRECT_UNSAT
        if not verify_model(formula_file, model):
            return INCORRECT_MODEL
        return CORRECT
    elif formula_file.startswith("uuf"):
        if status != UNSAT:
            return INCORRECT_SAT
        return CORRECT

    assert(False)


def run_benchmarks(solver_path, timeout, jobs):
    formulas = os.listdir(benchmarks_dir)
    with multiprocessing.Pool(jobs) as p:
        results = p.map(functools.partial(test_formula, solver_path=solver_path, timeout=timeout), formulas)

    incorrect = []
    unknown = []
    correct = []
    for formula, result in zip(formulas, results):
        if result == CORRECT:
            correct.append(formula)
        elif result == UNKNOWN or result == TIMEOUT:
            unknown.append(formula)
        else:
            incorrect.append(formula)

    print(f"Total formulas tested: {len(formulas)}")
    print(f"- Correct results: {len(correct)}")
    print(f"- Incorrect results: {len(incorrect)}")
    print(f"- Unknown results: {len(unknown)}")

    print("Writing incorrect and unknown formulas to files 'incorrect' and 'unknown'")
    with open("incorrect", "w") as incorrect_f:
        for f in incorrect:
            print(f, file=incorrect_f)
    with open("unknown", "w") as unknown_f:
        for f in unknown:
            print(f, file=unknown_f)


if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("Usage: python3 eval.py <path_to_solver> <solver_timeout_seconds> <n_jobs>")

    if not os.path.isdir(benchmarks_dir):
        print("Downloading benchmarks...")
        for benchmark in benchmarks:
            download_benchmark(benchmark)
        print("Done")

    run_benchmarks(sys.argv[1], float(sys.argv[2]), int(sys.argv[3]))
