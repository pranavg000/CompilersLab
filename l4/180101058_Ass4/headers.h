// Declaring sizes
#define MAX_SYM_SIZE 100
#define SYMTAB_SIZE 100
#define BUFF_SIZE 400
#define MAX_VARS 500

// Changing YYSTYPE to ptr
#define YYSTYPE uintptr_t

// Declaring Node struct for Hashmap
struct Node {
    int tokenType;
    int datatype;
    char symName[MAX_SYM_SIZE];
    struct Node* next;
};
