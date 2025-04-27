#include <iostream>
#include <unistd.h>
using namespace std;

#ifdef WASI_ON
#ifdef __cplusplus
extern "C" {  // Prevent name mangling for C++ code
#endif

__attribute__((used, visibility("default"))) volatile int glob_int = 42;
int get_counter();
int add_to_counter(int i);


#ifdef __cplusplus
}  // End extern "C" block
#endif
#else // WASI_ON not defined
int add_to_counter(int i){return 0;}
#endif


int main(){
    for(int i = 0; i < 10; i++){        
        int cur = add_to_counter(1);
    }
}

