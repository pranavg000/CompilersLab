%{
    #include <stdlib.h>
    #include <stdio.h>
    #include <string.h>
    // Declaring manifest constants
    #define ID_TT 22
    #define NUM_TT 23

    // Declaring sizes
    #define SIZE 20
    #define MAX_SYM_SIZE 40

    // Declaring Node struct for Hashmap
    struct Node {
        int tokenType;
        char symName[MAX_SYM_SIZE];
        struct Node* next;
    };

    // Decalring hashmap
    struct Node* hm[SIZE];

    //Declaring function prototypes
    struct Node* install_id();
    struct Node* install_num();

    //Declaring global variables
    int lineNo = 1;
    struct Node* pointerST;
%}
ws [ \t]+
id [_A-Za-z][_A-Za-z0-9]*
num (\+|-)?[0-9]+

%%
"\n" {lineNo++; }           // Counting line numbers
{ws} {}
"PROGRAM" {  return 1; }
"VAR" {  return 2; }
"BEGIN" {  return 3; }
"END" {  return 4; }
"END." {  return 5; }
"INTEGER" {  return 6; }
"FOR" {  return 7; }
"READ" {  return 8; }
"WRITE" {  return 9; }
"TO" {  return 10; }
"DO" {  return 11; }
";" {  return 12; }
":" {  return 13; }
"," {  return 14; }
":=" {  return 15; }
"+" {  return 16; }
"-" {  return 17; }
"*" {  return 18; }
"DIV" {  return 19; }
"(" {  return 20; }
")" {  return 21; }
{id} {  pointerST = install_id(); return ID_TT; }      // installing identifiers into Symboltable
{num} {  pointerST = install_num(); return NUM_TT; }   // installing integer constants into Symboltable

%%

// Helper fn to insert node at index:in of Symboltable
void insert_node_(int in, struct Node* node){
    node->next = hm[in];
    hm[in] = node;
}

// Searches for symbolname at index:in of Symboltable
struct Node* search_node(int in, char symn[]){
    struct Node* node = hm[in];
    while(node){                                // Iterates through list till node is found
        if(strcmp(node->symName, symn) == 0) return node;
        node = node->next;
    }
    return NULL;
}

// Calculates hash value of symbolname
int hash(char symn[]){
    int res=0;
    char c;
    while(c = *symn++){                 // Polynomial hash is calcuted
        res = res*256 + c;
        res%=SIZE;
    }
    return res;
}

// Creates new node and initializes it
struct Node* newNode(int tokenType, char symn[]){
    struct Node* node = (struct Node*)(malloc(sizeof(struct Node)));    // Creates newnode through malloc
    node->next = NULL;                              // Sets next ptr to null
    node->tokenType = tokenType;                    // Sets token type
    strcpy(node->symName, symn);                    // Copies symbol name into required symname buffer
    return node;                                    
}

// Inserts the node for symbolname at appropriate posn of Symboltable
struct Node* insert_node(int tokenType, char symn[]){
    int hsh = hash(symn);                           // Calculates hash of symbolname
    struct Node* temp = search_node(hsh, symn);
    if(temp) return temp;                           // Checks if symbolname is already present 
    struct Node* node = newNode(tokenType, symn);   // If not, creates new node
    insert_node_(hsh, node);                        // Insert node at index:hsh of Symboltable
    return node;
}
 
// install the identifier lexeme pointed to by yytext, into the symbol table and return its pointer
struct Node* install_id(){
    return (insert_node(ID_TT, yytext));    // calls insert_node fn
}

// install the Integer constant lexeme pointed to by yytext, into the symbol table and return its pointer
struct Node* install_num(){
    return (insert_node(NUM_TT, yytext));    // calls insert_node fn
}

// defining the yywrap function
int yywrap(){
    return 1;
}


int main(int argc, char* argv[]){
    if(argc > 1){                           // Gets file as argument from cmd line
		FILE *fileptr = fopen(argv[1], "r");
		if(fileptr)
			yyin = fileptr;                 // Makes yyin equal to file ptr
	}

    int tokenType;
    printf("Line          Token          Token Type          Token Specifier          Pointer\n");
    while(tokenType = yylex()){             // Iterating till EOF
        char prefix = 0;
        if(tokenType == ID_TT){             // If token is identifier
            prefix = '^';
        }
        else if(tokenType == NUM_TT){       // If token is Integer constant
            prefix = '#';
        }
        if(prefix)                          // Print appropriate outputs for diff types of tokens
            printf("%-13d %-14s %-19d %c%-23s %p\n", lineNo, yytext, tokenType, prefix, yytext, pointerST);
        else
            printf("%-13d %-14s %-19d \n", lineNo, yytext, tokenType);
    }
    printf("\n\n\nSYMBOL TABLE:\n\n");
    printf("Indetifiers - \n");
    printf("Token name              Pointer \n");
    for(int i=0;i<SIZE;i++) {               // Printing all identifiers in the symbol table
        struct Node* node = hm[i];
        while(node){
            if(node->tokenType == ID_TT)
                printf("%-23s %p\n", node->symName, node);
            node = node->next;
        }
    }
    printf("\nInteger constants - \n");
    printf("Integer value           Pointer \n");
    for(int i=0;i<SIZE;i++) {               // Printing all Integer constants in the symbol table
        struct Node* node = hm[i];
        while(node){
            if(node->tokenType == NUM_TT)
                printf("%-23s %p\n", node->symName, node);
            node = node->next;
        }
    }

    

    return 1;
}