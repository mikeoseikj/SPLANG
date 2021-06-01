#!/bin/sh
echo "[+] Compiling the code [binary name => ./build/spl-compiler]"
echo "=============================================================="
gcc -o ./build/splang scan.c symtab.c codegen.c eval.c expr.c utils.c func.c error.c warning.c stmts.c optimize.c main.c -g

echo "[+] Compiling user library used by the compiler"
echo "================================================"
cd lib
gcc -c lib.c -m32 -w
ar -rc libmystdlib.a lib.o
mv libmystdlib.a ../build/mylibs
rm lib.o
cd ..
