// #include <unistd.h>
// using namespace std;

#ifdef __cplusplus
extern "C" {  // Prevent name mangling for C++ code
#endif

// int get_counter();
int add_to_counter(int i);
// __attribute__((used, visibility("default"))) volatile int glob_int = 42;
__attribute__((used, visibility("default"))) int b_get_worker();

#ifdef __cplusplus
}  // End extern "C" block
#endif

int b_get_worker(){
    int cur = 0;
    for(int i = 0; i < 50000; i++){        
        int cur = add_to_counter(1);
    }
    return cur;
}
