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
  std::vector<wasm_func_t*> func_exposed;
  std::string mod_name;

  Wasm_Mod(std::string mod_name, wasm_engine_t* engine):
  mod_name(mod_name), engine(engine)
  {    
    wasm_extern_vec_new_empty(&imports);
    wasm_extern_vec_new_empty(&exports);
    store = wasm_store_new(engine);
    config = wasi_config_new(mod_name.c_str());
    wasi_config_inherit_stdout(config);
    wasi_config_inherit_stdin(config);
    wasi_config_inherit_stderr(config);
  }  

  ~Wasm_Mod(){
    if(wasi_env)wasi_env_delete(wasi_env);
    if(module)wasm_module_delete(module);
    if(instance)wasm_instance_delete(instance);
    if(store)wasm_store_delete(store);
    if(exports.size)wasm_extern_vec_delete(&exports);
    if(imports.size)wasm_extern_vec_delete(&imports);
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
  } 

  void wasmmod_load_module_from_bin(std::string filepath){
    // Compile
    wasm_byte_vec_t binary = load_wasm_binary(filepath);
    printf("Compiling module...\n");
    own module = wasm_module_new(store, &binary);
    if (!module) {
      throw std::runtime_error("> Error compiling module!\n");
    }
    wasm_byte_vec_delete(&binary);

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
    if (!wasi_get_imports(store, wasi_env, module, &imports)) {
      print_wasmer_error();
      throw std::runtime_error("> Error getting WASI imports!\n");
      }
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
};


void export_smth(){
      // Extract export.
      // printf("Extracting export...\n");
      // own wasm_extern_vec_t exports;
      // wasm_instance_exports(instance, &exports);
      // if (exports.size == 0) {
      // printf("> Error accessing exports!\n");
      // return 1;
      // }
      // fprintf(stderr, "found %zu exports\n", exports.size);
  
  
      // if (exports.size == 0) {
      //   printf("> Error accessing exports!\n");
  
      //   return 1;
      // }
  
      // wasm_global_t* glob_int = wasm_extern_as_global(exports.data[2]);
      // if (glob_int == NULL) {
      //   printf("> Failed to get the `glob_int` global!\n");
      //   return 1;
      // }
    
      // wasm_globaltype_t* glob_int_type = wasm_global_type(glob_int);
      // wasm_mutability_t glob_int_mutability = wasm_globaltype_mutability(glob_int_type);
      // const wasm_valtype_t* glob_int_content = wasm_globaltype_content(glob_int_type);
      // wasm_valkind_t glob_int_kind = wasm_valtype_kind(glob_int_content);
  
      // printf("`glob_int` type: %s %hhu\n", glob_int_mutability == WASM_CONST ? "const" : "", glob_int_kind);
  
      // wasm_val_t glob_int_set_value = WASM_I32_VAL(11);
      // wasm_global_set(glob_int, &glob_int_set_value);
  
      // wasm_val_t glob_int_val;
      // wasm_global_get(glob_int, &glob_int_val);
      // printf("`glob_int_val` value: %d\n", glob_int_val.of.i32);
  
}

// void read_int(){
//   printf("glob_int: %d\n", glob_int);
// }

// void write_int(int i){
//   glob_int = i;
// }

int main(int argc, const char* argv[]) {

    // start wasm
    {
      // Initialize.
      printf("Initializing...\n");
      wasm_engine_t* engine = wasm_engine_new();

      // Initialize Wasm_Mod instances
      Wasm_Mod a("a", engine);
      printf("a initialized.\n");
      Wasm_Mod b("b", engine);
      printf("b initialized.\n");
            
      a.wasmmod_load_module_from_bin("./a.wasm");
      // no import, export handling, setup instance directly
      // import function      

      // import function to a done
      a.wasmmod_build_instance(); 
      b.wasmmod_load_module_from_bin("./a.wasm");
      b.wasmmod_build_instance();

      a.wasmmod_run_start();
      b.wasmmod_run_start();

      // Shut down.
      if(engine)wasm_engine_delete(engine);
      printf("Shutting down...\n");
    }
    // All done.
    printf("Done.\n");

  return 0;
}


// wasm_trap_t* host_func_callback(const wasm_val_vec_t* args, wasm_val_vec_t* results) {
//   printf("Calling back...\n> ");

//   wasm_val_t val = WASM_I32_VAL(42);
//   wasm_val_copy(&results->data[0], &val);

//   wasm_val_delete(&val);

//   return NULL;
// }

// int main(int argc, const char* argv[]) {
//   const char *wat_string =
//       "(module\n"
//       "  (func $host_function (import \"\" \"host_function\") (result i32))\n"
//       "  (global $host_global (import \"env\" \"host_global\") i32)\n"
//       "  (func $function (export \"guest_function\") (result i32) (global.get $global))\n"
//       "  (global $global (export \"guest_global\") i32 (i32.const 42))\n"
//       "  (table $table (export \"guest_table\") 1 1 funcref)\n"
//       "  (memory $memory (export \"guest_memory\") 1))";

//   wasm_byte_vec_t wat;
//   wasm_byte_vec_new(&wat, strlen(wat_string), wat_string);
//   wasm_byte_vec_t wasm_bytes;
//   wat2wasm(&wat, &wasm_bytes);
//   wasm_byte_vec_delete(&wat);

//   printf("Creating the store...\n");
//   wasm_engine_t* engine = wasm_engine_new();
//   wasm_store_t* store = wasm_store_new(engine);

//   printf("Compiling module...\n");
//   wasm_module_t* module = wasm_module_new(store, &wasm_bytes);

//   if (!module) {
//       printf("> Error compiling module!\n");

//       return 1;
//   }

//   wasm_byte_vec_delete(&wasm_bytes);

//   printf("Creating the imported function...\n");
//   wasm_functype_t* host_func_type = wasm_functype_new_0_1(wasm_valtype_new_i32());
//   wasm_func_t* host_func = wasm_func_new(store, host_func_type, host_func_callback);
//   wasm_functype_delete(host_func_type);

//   printf("Creating the imported global...\n");
//   wasm_globaltype_t* host_global_type = wasm_globaltype_new(wasm_valtype_new(WASM_F32), WASM_CONST);
//   wasm_val_t host_global_val = WASM_I32_VAL(42);
//   wasm_global_t* host_global = wasm_global_new(store, host_global_type, &host_global_val);
//   wasm_globaltype_delete(host_global_type);

//   wasm_extern_t* externs[] = {
//       wasm_func_as_extern(host_func),
//       wasm_global_as_extern(host_global)
//   };

//   wasm_extern_vec_t import_object = WASM_ARRAY_VEC(externs);

//   printf("Instantiating module...\n");
//   wasm_instance_t* instance = wasm_instance_new(store, module, &import_object, NULL);
//   wasm_func_delete(host_func);
//   wasm_global_delete(host_global);

//   if (!instance) {
//     printf("> Error instantiating module!\n");

//     return 1;
//   }

//   printf("Retrieving exports...\n");
//   wasm_extern_vec_t exports;
//   wasm_instance_exports(instance, &exports);

//   if (exports.size == 0) {
//       printf("> Error accessing exports!\n");

//       return 1;
//   }

//   printf("Retrieving the exported function...\n");
//   wasm_func_t* func = wasm_extern_as_func(exports.data[0]);

//   if (func == NULL) {
//       printf("> Failed to get the exported function!\n");

//       return 1;
//   }

//   printf("Got the exported function: %p\n", func);

//   printf("Retrieving the exported global...\n");
//   wasm_global_t* global = wasm_extern_as_global(exports.data[1]);

//   if (global == NULL) {
//       printf("> Failed to get the exported global!\n");

//       return 1;
//   }

//   printf("Got the exported global: %p\n", global);

//   printf("Retrieving the exported table...\n");
//   wasm_table_t* table = wasm_extern_as_table(exports.data[2]);

//   if (table == NULL) {
//       printf("> Failed to get the exported table!\n");

//       return 1;
//   }

//   printf("Got the exported table: %p\n", table);

//   printf("Retrieving the exported memory...\n");
//   wasm_memory_t* memory = wasm_extern_as_memory(exports.data[3]);

//   if (memory == NULL) {
//       printf("> Failed to get the exported memory!\n");

//       return 1;
//   }

//   printf("Got the exported memory: %p\n", memory);

//   wasm_module_delete(module);
//   wasm_extern_vec_delete(&exports);
//   wasm_instance_delete(instance);
//   wasm_store_delete(store);
//   wasm_engine_delete(engine);
// }

