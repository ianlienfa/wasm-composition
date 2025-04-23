#include <iostream>
#include <unistd.h>
using namespace std;

#ifdef __cplusplus
extern "C" {  // Prevent name mangling for C++ code
#endif

__attribute__((used, visibility("default"))) volatile int glob_int = 42;
void read_int();

#ifdef __cplusplus
}  // End extern "C" block
#endif

struct A {
    int s;
    int arr[100];
};

void read_int(){
    printf("glob_int: %d\n", glob_int);
}  

void read_a(A a){
    a.arr[0] = 1;
}

int main(){
    cout << "wasm test!" << endl;
    read_int();
    A a;
    read_a(a);
}

