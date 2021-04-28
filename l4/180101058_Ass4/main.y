%{ 
    // IMPORTS
    #include <ctype.h> 
    #include <stdio.h> 
    #include <stdlib.h>
    #include <stdint.h>
    #include "headers.h"

    // struct to store variable declaration lists
    struct DecVars {
       int size;
       YYSTYPE varlist[MAX_VARS];
    } tempDecVars = {0};                // DecVars object to store variable declaration lists


    int semanticErrorCount;                        // 1 if error is encountered, 0 otherwise
    int syntaxErrorCount;                           // 1 if error is encountered, 0 otherwise
    int isSemantic;

    // Common identifiers for int and real tokens
    int defaultIdentifier;
    int intIdentifier;
    int realIdentifier;

    // Inserts variable's symbol table pointer into decvars list
    void insertDecVar(struct DecVars* decVar, YYSTYPE varptr){
        if(decVar->size == MAX_VARS) {              // Checks if full
            printf("DECVAR FULL\n");
            return;
        }
        decVar->varlist[decVar->size]=varptr;       // Inserts into decvar
        decVar->size++;                             // Increments size
    }

    // Clears decvar after variable declaration stmt finishes
    void clearDecVar(struct DecVars* decVar){
        decVar->size=0;                         // Sets size to 0
        return;
    }

    // Checks if variable is declared
    int isDeclared(YYSTYPE var){
        struct Node* node = (struct Node*)var;      // Gets symbol table node entry from pointer
        return (node->datatype != 0);               // returns 1 if variable is declared 
    }

    // returns type of the variable pointed by var
    YYSTYPE getType(YYSTYPE var){
        struct Node* node = (struct Node*)var;                              // Gets symbol table node entry from pointer
        if(node->datatype == 1) return (YYSTYPE)(&intIdentifier);           // If datatyupe is int, returns int identifier
        else if(node->datatype == 2) return (YYSTYPE)(&realIdentifier);     // Else, return real identifier      
        return (YYSTYPE)(&defaultIdentifier);                                   
    }

    // Flag to check if inside body or in declaration
    int inBody;


    // Extending visibility as these are declared in .l file
    extern int yylex();
    extern int lineNo;          // Stores current line no
    extern FILE *yyin;          // File ptr for input file


    // Declaration of yyerror, takes error string as input
    void yyerror(const char* s){
        if(isSemantic) semanticErrorCount++;
        else syntaxErrorCount++;        
        isSemantic = 0;
        printf("%s | at line %d\n", s, lineNo);     // Prints error with line number
    }


    // returns name of variable from symbol table
    char* getName(YYSTYPE var){
        struct Node* node = (struct Node*)var;      // Gets symbol table node entry from pointer 
        return node->symName;                       // Returns symbol's name
    }

    // Gives error if variable not declared
    int checkDeclared(YYSTYPE var){
        if(!isDeclared(var)) {              // Checks if variable is declared
            char buff[BUFF_SIZE];                 // Buffer to store error message
            sprintf(buff, "semantic error, variable \'%s\' not declared", getName(var));    // Putting error message into buff
            isSemantic = 1;
            yyerror(buff); return 0;       // Showing the error message
        }
        return 1;
    }

    // void checkZero(YYSTYPE var) {
    //     // struct Node* temp = (struct Node*)var;
    //     // printf("HELLO %d\n", temp->tokenType);
    //     // if(temp->tokenType==24 && atoi(temp->symName)==0){yyerror("SEMANTIC ERROR: Divide by zero"); exit(0);}
    // }


    // Gives error if types var1, var2 don't match
    int compareTypes(YYSTYPE var1, YYSTYPE var2){
        if(var1 == (YYSTYPE)(&defaultIdentifier) || var2 == (YYSTYPE)(&defaultIdentifier) || var1==var2) return 1;
        isSemantic = 1;
        yyerror("semantic error, type mismatch");       // Showing Type Mismatch semantic error
        return 0;
    }

    // Gives error if iterator's type is not int
    int checkTypeIterator(YYSTYPE iteratorType, YYSTYPE iid){
        if(iteratorType != iid){
            isSemantic = 1;
            yyerror("semantic error, iterator in FOR loop should be an INTEGER variable"); 
            return 0;
        }
        return 1;           // Returns 1 if no error
    }

%} 

// %union
// {
//     int intNum;
//     double realNum;
//     char* strname;
// }

// Macro to display verbose syntax errors
%define parse.error verbose


// Declaring tokens and assigning integer codes
%token PROGRAM 1                 
%token VAR 2
%token BEGIN_TOKEN 3 
%token END 4 
%token ENDDOT 5 
%token INTEGER 6 
%token REAL 7 
%token FOR 8
%token READ 9 
%token WRITE 10 
%token TO 11 
%token DO 12 
%token SEMICOLON 13 
%token COLON 14
%token COMMA 15 
%token ASSIGNMENT 16 
%token PLUS 17 
%token MINUS 18 
%token MULT 19 
%token DIV 20 
%token OPENING_BRACKET 21
%token CLOSING_BRACKET 22 
%token IDENTIFIER 23
%token INTEGER_VALUE 24 
%token REAL_VALUE 25
%token UNKNOWN_SYMBOL


// Setting the mathematical operators as left associative
%left PLUS MINUS
%left MULT DIV          // Setting higher preference of MULT and DIV operators


%start prog             // Setting start symbol ie prog

%%

// Pascal grammar production rules as mentioned in Assignment
prog : PROGRAM progname VAR declist beginbody stmtlist ENDDOT 
        ;

beginbody : BEGIN_TOKEN { inBody = 1;}
        ;

progname : IDENTIFIER
            ;
declist : dec | declist SEMICOLON dec
            ;
dec : idlist COLON type { // If declaration is found, set the datatype of variables
            int* typeIdentifier = (int*)$3;     // Get type from the value $3
            if(typeIdentifier == (&intIdentifier) || typeIdentifier == (&realIdentifier)){
                for(int i=0;i<tempDecVars.size;i++) {               // Iterating through the variable declaration list
                    struct Node* node = (struct Node*)(tempDecVars.varlist[i]); // Getting symbol table node pointer
                    if(node->datatype != 0) {                                   // Giving error is variable already declared
                        char buff[BUFF_SIZE];
                        sprintf(buff, "semantic error, redeclaration of variable \'%s\'", node->symName);
                        isSemantic = 1;
                        yyerror(buff);
                    }
                    // Setting appropriate datatypes inside symbol table
                    if(typeIdentifier==(&intIdentifier)) node->datatype = 1;
                    else if(typeIdentifier==(&realIdentifier)) node->datatype = 2;
                }
            } 
            clearDecVar(&tempDecVars);          // Clearing the temp declaration list
        }
        | error {
            clearDecVar(&tempDecVars);          // Clearing the temp declaration list if error occured in dec
        }
        ;
type : INTEGER  { $$ = (YYSTYPE)(&intIdentifier); }         // Passing type identifiers forward
        | REAL  {  $$ = (YYSTYPE)(&realIdentifier); }
        ;
idlist : IDENTIFIER     {
                if(inBody) checkDeclared($1);               // If inside body, check if variable is declared
                else insertDecVar(&tempDecVars, $1);        // Otherwise, insert in declist
            }
        | idlist COMMA IDENTIFIER {
                if(inBody) checkDeclared($3);               // If inside body, check if variable is declared
                else insertDecVar(&tempDecVars, $3);        // Otherwise, insert in declist
            }
        ;
stmtlist : stmt | stmtlist SEMICOLON stmt  | stmtlist error stmt 
            ;
stmt : assign | read | write | for | error
        ;
assign : IDENTIFIER ASSIGNMENT exp { if(checkDeclared($1))          // Checking if variable is declared and comparing types
                                        compareTypes(getType($1), $3); } 
        ;
exp :  term {$$ = $1;}                                                  // Passing type identifiers ahead
        | exp PLUS term { if(compareTypes($1, $3)) $$ = $1;             // Passing type if no error occured
                            else $$=(YYSTYPE)(&defaultIdentifier); }    // Otherwise use the defaultidentifier
        | exp MINUS term { if(compareTypes($1, $3)) $$ = $1;            // Passing type if no error occured
                            else $$=(YYSTYPE)(&defaultIdentifier); }    // Otherwise use the defaultidentifier
        ;

term : factor {$$ = $1;}
        | term MULT factor { if(compareTypes($1, $3)) $$ = $1;          // Passing type if no error occured
                            else $$=(YYSTYPE)(&defaultIdentifier); }    // Otherwise use the defaultidentifier
        | term DIV factor { if(compareTypes($1, $3)) $$ = $1;           // Passing type if no error occured
                            else $$=(YYSTYPE)(&defaultIdentifier); }    // Otherwise use the defaultidentifier
        ;
factor : IDENTIFIER { if(checkDeclared($1)) $$ = getType($1); else $$=(YYSTYPE)(&defaultIdentifier); }        // Checks if variable is declared and forards type into $$
        | INTEGER_VALUE {$$ = (YYSTYPE)(&intIdentifier);}   // If factor is an integer value, set type to INT
        | REAL_VALUE {$$ = (YYSTYPE)(&realIdentifier);}     // If factor is an real value, set type to real
        | OPENING_BRACKET exp CLOSING_BRACKET {$$ = $2;}
        ;
read : READ OPENING_BRACKET idlist CLOSING_BRACKET
        ;
write : WRITE OPENING_BRACKET idlist CLOSING_BRACKET
        ;
for : FOR indexexp DO body | FOR indexexp error body 
        ;
indexexp : IDENTIFIER ASSIGNMENT exp TO exp { if(checkDeclared($1) &&        // Check if iterator variable is declared
                                        checkTypeIterator((YYSTYPE)&intIdentifier, getType($1)) &&   // Checks if iterator is int
                                        compareTypes($3, $5)) 
                                            compareTypes(getType($1), $3);    // Checks if types match
                                    }
            ;
body : stmt | BEGIN_TOKEN stmtlist END
        ;


%% 
  

   
int main(int argc, char* argv[]) 
{       
    if(argc > 1){                               // Gets file as argument from cmd line
    	FILE *fileptr = fopen(argv[1], "r");
    	if(fileptr)
    		yyin = fileptr;                     // Makes yyin equal to file ptr
    }
    printf("\nParsing started...\n\n");
    yyparse();                                  // Starts parsing

    // Printing appropriate messages on completion of parser
    if(semanticErrorCount + syntaxErrorCount == 0) printf("Parsing completed.\nSuccessfully parsed input file \'%s\'.\nNo syntax or semantic errors found\n", argv[1]);    // If no error encountered, prints "Successful"
    else {
        printf("\nParsing completed.\nSuccessfully parsed input file \'%s\'. \nTotal %d syntax errors, and %d semantic errors found, which are displayed above\n", argv[1], syntaxErrorCount, semanticErrorCount);
    }
    return 0;
}  