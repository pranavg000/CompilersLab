%{
    #include <stdlib.h>
    #include <stdio.h>
    #include <string.h>
    #include "headers.h"
    #include "main.tab.h"

    // Decalring hashmap
    struct Node* hm[SYMTAB_SIZE];

    //Declaring function prototypes
    struct Node* install_id();
    struct Node* install_num();
    struct Node* install_real();

    //Declaring global variables
    int lineNo = 1;

%}
ws [ \t]+
id [_A-Za-z][_A-Za-z0-9]*
num (\+|-)?[0-9]+
real {num}(\.[0-9]+)?(E{num})?

%%
"\n" {lineNo++;}           // Counting current line number
{ws} {}
"PROGRAM" {  return PROGRAM; }
"VAR" {  return VAR; }
"BEGIN" {  return BEGIN_TOKEN; }
"END" {  return END; }
"END." {  return ENDDOT; }
"INTEGER" {  return INTEGER; }
"REAL" {  return REAL; }
"FOR" {  return FOR; }
"READ" {  return READ; }
"WRITE" {  return WRITE; }
"TO" {  return TO; }
"DO" {  return DO; }
";" {  return SEMICOLON; }
":" {  return COLON; }
"," {  return COMMA; }
":=" {  return ASSIGNMENT; }
"+" {  return PLUS; }
"-" {  return MINUS; }
"*" {  return MULT; }
"DIV" {  return DIV; }
"(" {  return OPENING_BRACKET; }
")" {  return CLOSING_BRACKET; }
{id} {  yylval = (YYSTYPE)install_id(); return IDENTIFIER; }      // installing identifiers into Symboltable
{num} {  yylval = (YYSTYPE)install_num(); return INTEGER_VALUE; }   // installing integer constants into Symboltable
{real} {  yylval = (YYSTYPE)install_real(); return REAL_VALUE; }   // installing real constants into Symboltable
. { return UNKNOWN_SYMBOL; }
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
        res%=SYMTAB_SIZE;
    }
    return res;
}

// Creates new node and initializes it
struct Node* newNode(int tokenType, char symn[]){
    struct Node* node = (struct Node*)(malloc(sizeof(struct Node)));    // Creates newnode through malloc
    node->next = NULL;                              // Sets next ptr to null
    node->tokenType = tokenType;                    // Sets token type
    node->datatype = 0;
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
    return (insert_node(IDENTIFIER, yytext));    // calls insert_node fn
}

// install the Integer constant lexeme pointed to by yytext, into the symbol table and return its pointer
struct Node* install_num(){
    return (insert_node(INTEGER_VALUE, yytext));    // calls insert_node fn
}

// install the Real constant lexeme pointed to by yytext, into the symbol table and return its pointer
struct Node* install_real(){
    return (insert_node(REAL_VALUE, yytext));    // calls insert_node fn
}

// defining the yywrap function
int yywrap(){
    return 1;
}