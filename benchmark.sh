#!/usr/bin/env bash

GCC="gcc"
CLANG="clang"
TINYOPT="./tinyopt"

EXAMPLES_DIR="./examples"
OUT_DIR="./bin"
RESULTS_FILE="results.csv"

PROGRAMAS=("fibonacci" "factorial" "prime" "gcd" "power")


mkdir -p "$OUT_DIR"
echo "File,Compiler,Opt level,Instructions" > "$RESULTS_FILE"

contar_instrucoes() {
    local bin="$1"
    objdump -d "$bin" | grep '^[[:space:]]*[0-9a-f]\+:' | wc -l
}

for prog in "${PROGRAMAS[@]}"; do
    src="$EXAMPLES_DIR/$prog.c"

    if [[ ! -f "$src" ]]; then
        echo "File not found: $src"
        continue
    fi

    echo "Compiling $prog..."

    # GCC -O0
    $GCC -O0 "$src" -o "$OUT_DIR/${prog}_gcc_O0"
    instr=$(contar_instrucoes "$OUT_DIR/${prog}_gcc_O0")
    echo "$prog,gcc,O0,$instr" >> "$RESULTS_FILE"

    # GCC -O2
    $GCC -O2 "$src" -o "$OUT_DIR/${prog}_gcc_O2"
    instr=$(contar_instrucoes "$OUT_DIR/${prog}_gcc_O2")
    echo "$prog,gcc,O2,$instr" >> "$RESULTS_FILE"

    # GCC -O3
    $GCC -O3 "$src" -o "$OUT_DIR/${prog}_gcc_O3"
    instr=$(contar_instrucoes "$OUT_DIR/${prog}_gcc_O3")
    echo "$prog,gcc,O3,$instr" >> "$RESULTS_FILE"

    # CLANG -O3
    $CLANG -O3 "$src" -o "$OUT_DIR/${prog}_clang_O3"
    instr=$(contar_instrucoes "$OUT_DIR/${prog}_clang_O3")
    echo "$prog,clang,O3,$instr" >> "$RESULTS_FILE"

    # TinyOpt + GCC -O0
    tiny_src="$OUT_DIR/${prog}_tinyopt.c"
    "$TINYOPT" < "$src" > "$tiny_src"
    $GCC -O0 "$tiny_src" -o "$OUT_DIR/${prog}_tinyopt_gcc_O0"
    instr=$(contar_instrucoes "$OUT_DIR/${prog}_tinyopt_gcc_O0")
    echo "$prog,tinyopt+gcc,O0,$instr" >> "$RESULTS_FILE"

done

echo "Results in $RESULTS_FILE"
