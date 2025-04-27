// #include <unistd.h>
// using namespace std;

#ifdef __cplusplus
extern "C" {  // Prevent name mangling for C++ code
#endif

int get_counter();
int add_to_counter(int i);
__attribute__((used, visibility("default"))) volatile int glob_int = 42;
__attribute__((used, visibility("default"))) int a_worker();
__attribute__((used, visibility("default"))) int a_get_counter();

#ifdef __cplusplus
}  // End extern "C" block
#endif

int a_worker(){
    int cur = 0;
    for(int i = 0; i < 10; i++){        
        cur = add_to_counter(1);
    }
    return cur;
}

int a_get_counter(){
    return get_counter();
}