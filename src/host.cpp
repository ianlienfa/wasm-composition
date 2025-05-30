#include <stdio.h>
#include <cerrno>
#include <thread>
#include <vector>
#include "wasmer.h"

#define BUF_SIZE 128
#define own
int glob_int = 1;

// Use the last_error API to retrieve error messages
void print_wasmer_error()
{
    int error_len = wasmer_last_error_length();
    if (error_len > 0) {
      printf("Error len: `%d`\n", error_len);
      char *error_str = (char*)malloc(error_len);
      wasmer_last_error_message(error_str, error_len);
      printf("Error str: `%s`\n", error_str);
    }
}

struct Wasm_Mod {  
  wasm_engine_t* engine = nullptr;
  wasm_store_t* store = nullptr;
  wasm_module_t* module = nullptr;
  wasi_config_t* config = nullptr;
  wasi_env_t* wasi_env = nullptr;
  wasm_instance_t* instance = nullptr;
  wasm_extern_vec_t imports;
  wasm_extern_vec_t exports;
  wasm_exporttype_vec_t export_types;
  wasm_memory_t* memory;
  std::vector<wasm_func_t*> func_exposed;
  std::string mod_name;

  Wasm_Mod(std::string mod_name, wasm_engine_t* engine, bool std_inherit=true):
  mod_name(mod_name), engine(engine)
  {    
    wasm_extern_vec_new_empty(&imports);
    wasm_extern_vec_new_empty(&exports);
    wasm_exporttype_vec_new_empty(&export_types);
    store = wasm_store_new(engine);
    config = wasi_config_new(mod_name.c_str());
    if(std_inherit){
      wasi_config_inherit_stdout(config);
      wasi_config_inherit_stdin(config);
      wasi_config_inherit_stderr(config);
    }
  }  

  ~Wasm_Mod(){
    if(wasi_env)wasi_env_delete(wasi_env);
    if(module)wasm_module_delete(module);
    if(instance)wasm_instance_delete(instance);
    if(store)wasm_store_delete(store);
    if(exports.size)wasm_extern_vec_delete(&exports);
    if(export_types.size)wasm_exporttype_vec_delete(&export_types);    
    for(auto f: func_exposed){
      wasm_func_delete(f);
    }
  }

  wasm_byte_vec_t load_wasm_binary(std::string filepath){
    printf("Loading binary...\n");
    FILE* file = fopen(filepath.c_str(), "rb");
    if (!file) {
      throw std::runtime_error("> Error loading module!\n");
      return wasm_byte_vec_t();
    }
    fseek(file, 0L, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0L, SEEK_SET);
    wasm_byte_vec_t binary;
    wasm_byte_vec_new_uninitialized(&binary, file_size);
    if (fread(binary.data, file_size, 1, file) != 1) {
      throw std::runtime_error("> Error initializing module!\n");
      return wasm_byte_vec_t();
    }
    fclose(file);
    return binary;
  }
  
  void wasmmod_build_instance(){
    if (!wasi_get_imports(store, wasi_env, module, &imports)) {
      print_wasmer_error();
      throw std::runtime_error("> Error getting WASI imports!\n");
      }
    own instance =
      wasm_instance_new(store, module, &imports, NULL);
    if (!instance) {
      throw std::runtime_error("> Error instantiating module!\n");
      print_wasmer_error();
      }
    if (!wasi_env_initialize_instance(wasi_env, store, instance)) {
      print_wasmer_error();
      throw std::runtime_error("> Error initializing wasi env memory!\n");
      }
    
    // process export
    wasm_instance_exports(instance, &exports);
      if (exports.size == 0) {
        printf("> Error accessing exports!\n");
      }
    printf("export size: %ld\n", exports.size);
    if (!wasm_extern_as_memory(exports.data[0])) {
      printf("> Error accessing memory export %u!\n", 0);
    }
    memory = wasm_extern_as_memory(exports.data[0]); // get memory
  } 

  void nowasi_wasmmod_build_instance(){
    own instance =
      wasm_instance_new(store, module, &imports, NULL);
    if (!instance) {
      throw std::runtime_error("> Error instantiating module!\n");
      print_wasmer_error();
      }
    // process export
    wasm_instance_exports(instance, &exports);
      if (exports.size == 0) {
        printf("> Error accessing exports!\n");
      }
    printf("export size: %ld\n", exports.size);
    if (!wasm_extern_as_memory(exports.data[0])) {
      printf("> Error accessing memory export %u!\n", 0);
    }
    memory = wasm_extern_as_memory(exports.data[0]); // get memory
  } 

  // this deletes the binary vec 
  void wasmmod_load_module_from_bytes(wasm_byte_vec_t *binary){
    printf("Compiling module...\n");
    own module = wasm_module_new(store, binary);
    if (!module) {
      throw std::runtime_error("> Error compiling module!\n");
    }
    wasm_byte_vec_delete(binary);

    // create wasi env
    wasi_env = wasi_env_new(store, config);
    if (!wasi_env) {
      print_wasmer_error();
      throw std::runtime_error("> Error building WASI env!\n");
      return ;
    }

    // populate import too
    if(!(module && wasi_env && store)){      
      if(!module) throw std::runtime_error("> module is NULL!\n");
      if(!wasi_env) throw std::runtime_error("> wasi_env is NULL!\n");
      if(!store) throw std::runtime_error("> store is NULL!\n");
    }
  }

  void nowasi_wasmmod_load_module_from_bytes(wasm_byte_vec_t *binary){
    printf("Compiling module...\n");
    own module = wasm_module_new(store, binary);
    if (!module) {
      throw std::runtime_error("> Error compiling module!\n");
    }
    wasm_byte_vec_delete(binary);

    // populate import too
    if(!(module && store)){      
      if(!module) throw std::runtime_error("> module is NULL!\n");
      if(!store) throw std::runtime_error("> store is NULL!\n");
    }

    // populate export types
    wasm_module_exports(module, &export_types);

    
  }

  void wasmmod_load_module_from_str(const char* wat_string){
    own wasm_byte_vec_t wat;
    wasm_byte_vec_new(&wat, strlen(wat_string), wat_string);
    own wasm_byte_vec_t wasm_bytes;
    wat2wasm(&wat, &wasm_bytes);
    wasm_byte_vec_delete(&wat); // own wasm_byte_vec_t wat
    nowasi_wasmmod_load_module_from_bytes(&wasm_bytes); // ownership trans
  }

  void wasmmod_load_module_from_file(std::string filepath){
    wasm_byte_vec_t binary = load_wasm_binary(filepath);
    nowasi_wasmmod_load_module_from_bytes(&binary); // ownership trans
  }
  
  int wasmmod_run_start(){
    if(!instance){
      printf("> Instance not initialized!\n");
      return 1;
    }
    wasm_func_t* run_func = wasi_get_start_function(instance);
    func_exposed.push_back(run_func);
    if (run_func == NULL) {
      printf("> Error accessing export!\n");
      print_wasmer_error();
      return 1;
    }
    wasm_val_vec_t args = WASM_EMPTY_VEC;
    wasm_val_vec_t res = WASM_EMPTY_VEC;
    printf("=============== %s: Start module function call=============\n", mod_name.c_str());
    if (wasm_func_call(run_func, &args, &res)) {
      printf("> Error calling function!\n");    
      return 1;
    }
    printf("===================Call completed===========================\n");
    return 0;
  }

  // func/arg mem will automatically be cleaned up after call
  wasm_func_t* wasmmod_get_export_func(int func_index, std::string func_name = ""){
    printf("Retrieving function %s at index %d.\n", func_name.c_str(), func_index);

    wasm_func_t* func = wasm_extern_as_func(exports.data[func_index]);
    if (func == NULL) {
        printf("> Failed to get the `%s` function!\n", func_name.c_str());
        return NULL;
    }
    return func;
  }

  const wasm_name_t* wasmmod_get_export_name(int func_index){
    if (export_types.size == 0) {
      printf("> Error accessing exports!\n, call nowasi_wasmmod_load_module_from_bytes before processing name!");
      return NULL;
    }
    printf("export_types size: %ld\n", exports.size);

    wasm_exporttype_t* export_type = export_types.data[func_index];
    if (export_type == NULL) {
      printf("> Failed to get the export_type at index %d!\n", func_index);
      return NULL;
    }
    
    const wasm_name_t* export_name = wasm_exporttype_name(export_type);
    if (export_name == NULL) {
        printf("> Failed to get the name at %d!\n", func_index);
        return NULL;
    }
    return export_name;
  }

  const wasm_externtype_t* wasmmod_get_export_type(int func_index){
    if (export_types.size == 0) {
      printf("> Error accessing exports!\n, call nowasi_wasmmod_load_module_from_bytes before processing name!");
      return NULL;
    }
    printf("export_types size: %ld\n", exports.size);

    wasm_exporttype_t* export_type = export_types.data[func_index];
    if (export_type == NULL) {
      printf("> Failed to get the export_type at index %d!\n", func_index);
      return NULL;
    }
    const wasm_externtype_t* extern_type = wasm_exporttype_type(export_type);
    if (extern_type == NULL) {
        printf("> Failed to get the type at %d!\n", func_index);
        return NULL;
    }
    return extern_type;
  }

  // get current memory offset
  byte_t * wassmod_get_mem(){
    return wasm_memory_data(memory);
  }
};


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
wasm_val_t val = WASM_I32_VAL(env->counter);
wasm_val_copy(&results->data[0], &val);
results->data[0] = val;
wasm_val_delete(&val);
return NULL;
}

wasm_trap_t* add_to_counter(void* env_arg, const wasm_val_vec_t* args, wasm_val_vec_t* results) {
Env* env = (Env*)env_arg;
pthread_mutex_lock(&env->lock); // Lock before accessing counter
int32_t add = args->data[0].of.i32;
env->counter += add;
wasm_val_t val = WASM_I32_VAL(env->counter);
wasm_val_copy(&results->data[0], &val);
wasm_val_delete(&val);
pthread_mutex_unlock(&env->lock); // Unlock afterward
return NULL;
}


int main(int argc, const char* argv[]) {
    // start wasm
    {
      // Initialize.
      printf("Initializing...\n");
      wasm_engine_t* engine = wasm_engine_new();

      // Initialize Wasm_Mod instances
      Wasm_Mod a("a", engine, false /*std_inherit*/);
      printf("a initialized.\n");
      Wasm_Mod b("b", engine, false /*std_inherit*/);
      printf("b initialized.\n");
            
      // load module
      a.wasmmod_load_module_from_file("./a.wasm");
      b.wasmmod_load_module_from_file("./b.wasm");

      // build env
      own Env env_data;

      // build function instance prep
      // own wasm_functype_t* get_counter_type = wasm_functype_new_0_1(wasm_valtype_new_i32());
      own wasm_functype_t* add_to_counter_type = wasm_functype_new_1_1(wasm_valtype_new_i32(), wasm_valtype_new_i32());

      // build import function instance for a      
      // own wasm_func_t* get_counter_func_a = wasm_func_new_with_env(a.store, get_counter_type, get_counter, &env_data, NULL);
      own wasm_func_t* add_to_counter_func_a = wasm_func_new_with_env(a.store, add_to_counter_type, add_to_counter, &env_data, NULL);
      wasm_extern_t* externs_a[] = {
        wasm_func_as_extern(add_to_counter_func_a),
        // wasm_func_as_extern(get_counter_func_a)
      };
      a.imports = WASM_ARRAY_VEC(externs_a);
    
      // build import function instance for b
      // own wasm_func_t* get_counter_func_b = wasm_func_new_with_env(b.store, get_counter_type, get_counter, &env_data, NULL);
      own wasm_func_t* add_to_counter_func_b = wasm_func_new_with_env(b.store, add_to_counter_type, add_to_counter, &env_data, NULL);
      wasm_extern_t* externs_b[] = {
        // wasm_func_as_extern(get_counter_func_b),
        wasm_func_as_extern(add_to_counter_func_b)
      };
      b.imports = WASM_ARRAY_VEC(externs_b);

      // post function instance build cleanup
      // wasm_functype_delete(get_counter_type); // own wasm_func_t* get_counter_func
      wasm_functype_delete(add_to_counter_type); // own wasm_func_t* add_to_counter_func

      // build instance with the newly populated import
      a.nowasi_wasmmod_build_instance(); 
      b.nowasi_wasmmod_build_instance();

      // prep a_get_worker      
      // wasm_func_t* a_get_counter = a.wasmmod_get_export_func(2);
      wasm_func_t* a_get_worker = a.wasmmod_get_export_func(1);
      const wasm_name_t* a_export_name_1 = a.wasmmod_get_export_name(1);
      printf("function name: %s\n", a_export_name_1->data);
      if(!a_get_worker){
        printf("> failed retreiving `a_get_worker` function!\n");
        return 1;
      }

      // prep a_append_hello
      wasm_func_t* a_append_hello = a.wasmmod_get_export_func(2);
      const wasm_name_t* a_export_name_2 = a.wasmmod_get_export_name(2);
      printf("function name: %s\n", a_export_name_2->data);
      if(!a_append_hello){
        printf("> failed retreiving `a_append_hello` function!\n");
        return 1;
      }
      
      {
        // a_append_hello call
        wasm_byte_t* memory_data_offset = a.wassmod_get_mem();
        int offset = 1024;
        const char* host_string = "hey";
        memset(memory_data_offset + offset, 0, 100); // make sure the memory is clean
        printf("%s\n", host_string);
        strcpy((char*)(memory_data_offset + offset), host_string);
        printf("[%p]: %s\n", memory_data_offset + offset, memory_data_offset + offset);
        
        wasm_val_t results_val[1] = { WASM_INIT_VAL };
        wasm_val_t args_val[1] = { WASM_I32_VAL((int32_t)offset) };        
        wasm_val_vec_t args = WASM_ARRAY_VEC(args_val);
        wasm_val_vec_t results = WASM_ARRAY_VEC(results_val);
        if (wasm_func_call(a_append_hello, &args, &results)) {
          print_wasmer_error();
          printf("> Error calling the `a_append_hello` function!\n");            
        }
        printf("Results of `a_append_hello`: %s\n", (memory_data_offset + results_val[0].of.i32));                
        printf("Results at offset: %d\n", (results_val[0].of.i32));                

      }


      // if(!a_get_counter){
      //   printf("> failed retreiving `a_get_counter` function!\n");
      //   return 1;
      // }

      // prep b_get_worker
      wasm_func_t* b_get_worker = b.wasmmod_get_export_func(1);
      if(!b_get_worker){
        printf("> failed retreiving `b_get_worker` function!\n");
        return 1;
      }

      std::thread a_worker([&]()
      {
          // a_get_worker call
          wasm_val_t results_val[1] = { WASM_INIT_VAL };
          wasm_val_vec_t args = WASM_EMPTY_VEC;
          wasm_val_vec_t results = WASM_ARRAY_VEC(results_val);
          if (wasm_func_call(a_get_worker, &args, &results)) {
            print_wasmer_error();
            printf("> Error calling the `a_get_worker` function!\n");            
          }
          printf("Results of `a_get_worker`: %d\n", results_val[0].of.i32);
          printf("Result of actual val: %d\n", env_data.counter);
      });

      std::thread b_worker([&]()
      {
        // b_get_worker call
        wasm_val_t results_val[1] = { WASM_INIT_VAL };
        wasm_val_vec_t args = WASM_EMPTY_VEC;
        wasm_val_vec_t results = WASM_ARRAY_VEC(results_val);
        if (wasm_func_call(b_get_worker, &args, &results)) {
          print_wasmer_error();
          printf("> Error calling the `b_get_worker` function!\n");
        }
        printf("Results of `b_get_worker`: %d\n", results_val[0].of.i32);
        printf("Result of actual val: %d\n", env_data.counter);
      });

      a_worker.join();
      b_worker.join();

      // Shut down.
      if(engine)wasm_engine_delete(engine);
      printf("Shutting down...\n");
    }
    // All done.
    printf("Done.\n");

  return 0;
}
