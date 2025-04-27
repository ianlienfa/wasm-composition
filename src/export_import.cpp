#include <stdio.h>
#include <cerrno>
#include <thread>
#include <vector>
#include "wasmer.h"

#define BUF_SIZE 128
#define own

typedef struct Env {
    int32_t counter;
    pthread_mutex_t lock;
    
    Env(){
      counter = 0;
      pthread_mutex_init(&lock, NULL);
    }

    ~Env(){
      pthread_mutex_destroy(&lock);
    }
} Env;
  
wasm_trap_t* get_counter(void* env_arg, const wasm_val_vec_t* args, wasm_val_vec_t* results) {
  Env* env = (Env*)env_arg;
  results->data[0].of.i32 = env->counter;
  return NULL;
}
  
wasm_trap_t* add_to_counter(void* env_arg, const wasm_val_vec_t* args, wasm_val_vec_t* results) {
  Env* env = (Env*)env_arg;
  pthread_mutex_lock(&env->lock); // Lock before accessing counter
  int32_t add = args->data[0].of.i32;
  env->counter += add;
  results->data[0].of.i32 = env->counter;
  pthread_mutex_unlock(&env->lock); // Unlock afterward
  return NULL;
}

int main() {
  own wasm_engine_t* engine = wasm_engine_new();
  own wasm_store_t* store = wasm_store_new(engine);

  // WAT (WebAssembly text format) source
  const char* wat_string =
      "(module\n"
      "  (func $get_counter (import \"env\" \"get_counter\") (result i32))\n"
      "  (func $add_to_counter (import \"env\" \"add_to_counter\") (param i32) (result i32))\n"
      "  (type $increment_t (func (param i32) (result i32)))\n"
      "  (func $increment_f (type $increment_t) (param $x i32) (result i32)\n"
      "    (block\n"
      "      (loop\n"
      "        (call $add_to_counter (i32.const 1))\n"
      "        (local.set $x (i32.sub (local.get $x) (i32.const 1)))\n"
      "        (br_if 1 (i32.eq (local.get $x) (i32.const 0)))\n"
      "        (br 0)))\n"
      "    call $get_counter)\n"
      "  (export \"increment_counter_loop\" (func $increment_f)))";

  own wasm_byte_vec_t wat;
  wasm_byte_vec_new(&wat, strlen(wat_string), wat_string);
  own wasm_byte_vec_t wasm_bytes;
  wat2wasm(&wat, &wasm_bytes);
  wasm_byte_vec_delete(&wat); // own wasm_byte_vec_t wat
  
  own wasm_module_t* module = wasm_module_new(store, &wasm_bytes);
  if (!module) {
      fprintf(stderr, "Failed to compile module.\n");
      return 1;
  }
  wasm_byte_vec_delete(&wasm_bytes); // own wasm_byte_vec_t wasm_bytes

  // Set up the environment
  own Env env_data;

  // Creating import function 
  own wasm_functype_t* get_counter_type = wasm_functype_new_0_1(wasm_valtype_new_i32());
  own wasm_functype_t* add_to_counter_type = wasm_functype_new_1_1(wasm_valtype_new_i32(), wasm_valtype_new_i32());

  own wasm_func_t* get_counter_func = wasm_func_new_with_env(store, get_counter_type, get_counter, &env_data, NULL);
  own wasm_func_t* add_to_counter_func = wasm_func_new_with_env(store, add_to_counter_type, add_to_counter, &env_data, NULL);

  wasm_functype_delete(get_counter_type); // own wasm_func_t* get_counter_func
  wasm_functype_delete(add_to_counter_type); // own wasm_func_t* add_to_counter_func

  wasm_extern_t* externs[] = {
      wasm_func_as_extern(get_counter_func),
      wasm_func_as_extern(add_to_counter_func)
  };

  wasm_extern_vec_t imports = WASM_ARRAY_VEC(externs);

  own wasm_instance_t* instance = wasm_instance_new(store, module, &imports, NULL);
  if (!instance) {
      fprintf(stderr, "Failed to instantiate module.\n");
      return 1;
  }

  own wasm_extern_vec_t exports;
  wasm_instance_exports(instance, &exports);

  wasm_func_t* increment_func = wasm_extern_as_func(exports.data[0]);

  wasm_val_t args_val[1] = { WASM_I32_VAL(5) };
  own wasm_val_vec_t args = WASM_ARRAY_VEC(args_val);
  wasm_val_t results_val[1];
  own wasm_val_vec_t results = WASM_ARRAY_VEC(results_val);

  // run in multiple threads
  std::thread wasm_thread([&](){
      for(int i = 0; i < 50000; i++){
        wasm_func_call(increment_func, &args, &results);
      }
  });

  std::thread main_thread([&](){
    for(int i = 0; i < 50000; i++){
      pthread_mutex_lock(&env_data.lock); // Lock before accessing counter
      env_data.counter++;
      pthread_mutex_unlock(&env_data.lock); // Unlock afterward
    }
  });

  wasm_thread.join();
  main_thread.join();

  // wasm_func_call(increment_func, &args, &results);
  // printf("Result from WASM: %d\n", results.data[0].of.i32);
  // printf("Counter from host: %d\n", env_data->counter);

  // wasm_func_call(increment_func, &args, &results);
  // printf("Result from WASM: %d\n", results.data[0].of.i32);
  // printf("Counter from host: %d\n", env_data->counter);

  printf("Actual env: %d\n", env_data.counter);  

  wasm_module_delete(module); // own wasm_module_t* module
  wasm_func_delete(get_counter_func); // own wasm_func_t* get_counter_func
  wasm_func_delete(add_to_counter_func); // own wasm_func_t* add_to_counter_func
  wasm_instance_delete(instance); // own wasm_instance_t* instance
  wasm_store_delete(store); // own wasm_store_t* store
  wasm_engine_delete(engine); //own wasm_engine_t* engine
  wasm_extern_vec_delete(&exports); // own wasm_extern_vec_t exports    
  
  return 0;
}
