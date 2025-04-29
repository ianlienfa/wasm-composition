// #include <unistd.h>
// using namespace std;
#include <stdlib.h>
#include <string.h>
#ifdef __cplusplus
extern "C" {  // Prevent name mangling for C++ code
#endif

// int get_counter();
int add_to_counter(int i);
// __attribute__((used, visibility("default"))) volatile int glob_int = 42;
__attribute__((used, visibility("default"))) int a_get_worker();
__attribute__((used, visibility("default"))) char* a_append_hello(char* s);
char hello_world[] = "hello_world";

#ifdef __cplusplus
}  // End extern "C" block
#endif

int a_get_worker(){
    int* cur = (int*)(malloc(sizeof(int)));
    for(int i = 0; i < 50000; i++){        
        *cur = add_to_counter(1);
    }
    int c = *cur;
    free(cur);
    free(cur); // double free
    (*cur)++; // accessing that pointer
    return c;
}

char* a_append_hello(char* s){
    return hello_world;
}