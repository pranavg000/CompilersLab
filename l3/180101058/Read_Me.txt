Steps to run - 
1) flex 180101058_main.l
2) gcc lex.yy.c
3) ./a.out 180101058_input.txt > out.txt        // ./a.out <input_file_name> > <output_file_name>

Output will be found in file out.txt.
    First all the tokens found in the program are displayed. 
    Then the indentifiers and integer constants in the symbol table are again displayed.