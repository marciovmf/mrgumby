#include "minima.h"
#include "common.h"
#include "minima_array.h"
#include "minima_common.h"
#include "minima_eval.h"
#include "minima_parser.h"

//TODO: Arrays and strings must be deleted somehow. Garbage collection ?
//TODO: improve variable scope
//TODO: For loops
//TODO: Break statement

static void s_print_type(MiValue* value)
{
  if (value->type == MI_TYPE_BOOL)
  {
    printf("%s",((int) value->as.number_value) ? "true" : "false");
  }
  else if (value->type == MI_TYPE_INT)
  {
    printf("%d", (int) value->as.number_value);
  }
  else if (value->type == MI_TYPE_FLOAT)
  {
    printf("%f", value->as.number_value);
  }
  else if (value->type == MI_TYPE_STRING)
  {
    printf("%s", value->as.string_value);
  }
  else
  {
    log_info("Runtime value %llxn", value);
  }
}

static MiValue s_function_print(int param_count, MiValue* args)
{
  if (param_count == 1)
  {
    MiValue* value = &args[0];
    if (value->type == MI_TYPE_ARRAY)
    {
      MiArray* array = value->as.array_value;
      mi_array_print(array);
    }
    else
    {
      s_print_type(value);
    }
  }

  return mi_runtime_value_create_void();
}

static MiValue s_function_array_size(int param_count, MiValue* args)
{
  ASSERT(param_count == 1);
  ASSERT(args != NULL);
  ASSERT(args->type == MI_TYPE_ARRAY);
  return mi_runtime_value_create_int((int) args[0].as.array_value->size);
}

static MiValue s_function_array_append(int param_count, MiValue* parameters)
{
  ASSERT(param_count == 2);
  ASSERT(parameters != NULL);
  ASSERT(parameters->type == MI_TYPE_ARRAY);

  MiArray* array = parameters[0].as.array_value;
  MiValue* element = &parameters[1];

  switch(element->type)
  {
    case MI_TYPE_BOOL:
      mi_array_add_bool(array, (int) element->as.number_value);
      break;
    case MI_TYPE_INT:
      mi_array_add_int(array, (int) element->as.number_value);
      break;
    case MI_TYPE_FLOAT:
      mi_array_add_float(array, (float) element->as.number_value);
      break;
    case MI_TYPE_STRING:
      mi_array_add_string(array, element->as.string_value);
      break;
    case MI_TYPE_ARRAY:
      mi_array_add_array(array, element->as.array_value);
      break;
    default:
      ASSERT_BREAK(); // Unknown type;
      break;
  }

  return mi_runtime_value_create_void();
}


// Minima

MiProgram* mi_program_create(const char* source)
{
  MiProgram* program = (MiProgram*) malloc(sizeof(MiProgram));
  mi_symbol_table_init(&program->symbols);
  program->ast = mi_parse_program(source);


  // Add a function into the symbol table
  MiSymbol* function = NULL;

  // Print function
  function = mi_symbol_table_create_function(&program->symbols, s_function_print, "print", 1);
  mi_symbol_table_function_set_param(function, 0, "arg0", MI_TYPE_ANY);

  // Array functions
  function = mi_symbol_table_create_function(&program->symbols, s_function_array_size, "array_size", 1);
  mi_symbol_table_function_set_param(function, 0, "arg0", MI_TYPE_ARRAY);

  function = mi_symbol_table_create_function(&program->symbols, s_function_array_append, "array_append", 2);
  mi_symbol_table_function_set_param(function, 0, "array", MI_TYPE_ARRAY);
  mi_symbol_table_function_set_param(function, 0, "element", MI_TYPE_ANY);

  return program;
}

int mi_program_run(MiProgram* program)
{
  return mi_eval_program(&program->symbols, program->ast);
}

void mi_program_destroy(MiProgram* program)
{
  mi_symbol_table_destroy(&program->symbols);
  free(program);
}
