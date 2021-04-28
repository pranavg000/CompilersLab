I have used Ubuntu Operating System. And GNU gcc compiler

Note: Kindly ensure that 'flex' and 'bison' are installed and updated to the latest versions

Steps to run - 
1) bison -d main.y 
2) flex main.l
3) gcc lex.yy.c main.tab.c -o main.o
4) ./main.o input_without_errors.txt           // ./main.o <INPUT_FILENAME>
5) ./main.o input_with_errors.txt              // ./main.o <INPUT_FILENAME>

Note: 
- The file input_without_errors.txt has valid pascal code according to the grammar given in the Assignment
- The file input_with_errors.txt has pascal code with some syntax and semantic errors. These errors will be found and outputted when Step 5 is run.

The output is printed on the console.