#include <iostream>
using namespace std;

// In C++ (avoid name mangling)
// extern "C" __attribute__((used, visibility("default"))) int glob_int = 42;

// void print_glob(){
//     cout << "glob_int: " << glob_int << endl;
// }

int main(){
    cout << "wasm test!" << endl;
    // print_glob();
}

