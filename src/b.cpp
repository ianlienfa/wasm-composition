#include <iostream>
#include <unistd.h>
using namespace std;

#ifdef __cplusplus
extern "C" {  // Prevent name mangling for C++ code
#endif

__attribute__((used, visibility("default"))) volatile int glob_int = 42;
int get_counter();
int add_to_counter(int i);


#ifdef __cplusplus
}  // End extern "C" block
#endif

int main(){
    for(int i = 0; i < 10; i++){
        sleep(1);
        printf("[b]: try to add counter!\n");
        int cur = add_to_counter(1);
        printf("[b]: added! cur val: %d\n", cur);
    }
}

