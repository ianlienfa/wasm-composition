#include <stdio.h>
#include <cerrno>
#include <thread>
#include <vector>
#include "wasmer.h"

#define BUF_SIZE 128
#define own

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