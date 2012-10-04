# LLVM backend for ARCompact #

ARCompact is the ISA used by the
[EnCore](http://groups.inf.ed.ac.uk/pasta/hw_encore.html) microprocessor
family. This projects aims to create a working LLVM backend for the EnCore
processor family, and port Clang to support the architecture as well.

As is standard for LLVM and Clang, the Clang source is kept in a different
repository - Clang-ARCompact. Instructions for combining the two repositories
is given below.

Note that this repository is forked from the [LLVM Github
mirror](https://github.com/llvm-mirror/llvm). Upstream changes will be fetched
and merged into the arcompact branch roughly every two weeks, or when I
remember.

### Building ###

First, get the sources:

    git clone git://github.com/llvm-dcpu16/llvm-dcpu16.git # Checkout LLVM
    cd llvm-dcpu16/tools
    git clone git://github.com/llvm-dcpu16/clang.git # Checkout Clang

Then it is just a case of building LLVM:

    mkdir ../build
    cd ../build
    make -j6
    
### Using LLVM-ARCompact ###
    
Consider the following C file:

fib.c:

```c
int fib(int n) {
  int cur = 1;
  int prev = 1;
  for (int i = 0; i < n; i++) {
    int next = cur+prev;
    prev = cur;
    cur = next;
  }
  return cur;
}

int main(void) {
  return fib(5);
}
```

To compile this to ARCompact assembly, run:

    bin/clang -ccc-host-triple arcompact -S fib.c -o fib.s

#### Reliance on GCC ####

LLVM-ARCompact is currently only able to output assembly, and relies on the
[EnCore GCC Compiler](http://groups.inf.ed.ac.uk/pasta/tools_gcc.html) to
assemble and link the output assembly. This means that in order for
LLVM-ARCompact to be useful, you probably want to have access to the GCC port.
Currently the location that LLVM looks for GCC in is hard-coded in the Clang
front-end, but in the future the aim will be to let it be set as a build or
runtime flag.

Enjoy and, please, [report bugs](https://github.com/llvm-arcompact-2/llvm-arcompact-2/issues)!
