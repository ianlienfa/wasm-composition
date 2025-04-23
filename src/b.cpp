#include <iostream>
#include <unistd.h>
using namespace std;

extern "C" __attribute__((used, visibility("default"))) volatile int glob_int = 42;

void modify_glob(){
    for(int i = 10; i >= 0; i--){
        sleep(1);
        glob_int--;
    }
}

int main(){
    cout << "wasm test!" << endl;
    modify_glob();
}

