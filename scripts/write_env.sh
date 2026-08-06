{     
    uname -a;     
    printf '\nArchitecture:\n';     
    uname -m;     
    printf '\nglibc:\n';     
    ldd --version | head -n 1;     
    printf '\nGCC:\n';     
    gcc --version | head -n 1;     
    printf '\nClang:\n';     
    clang --version | head -n 1; } | tee environment.txt