#include <iostream>
#include <unistd.h>
using namespace std;

extern "C" __attribute__((used, visibility("default"))) volatile int glob_int = 42;

void print_glob(){
    printf("glob_int: %d\n", glob_int);
}

int main(){
    cout << "wasm test!" << endl;
    print_glob();
}

