// abstract class to describe how command will be executed

typedef struct ExecutionContext{
    char* input_file;
    char* output_file;

    int is_background;
} ExecutionContext;