I have used Linux g++

INPUT FILE FORMAT: Label field: 10chars long, Ppcode field: 10chars long, operand field: >= 30 chars long
                    input.txt file is already in this format
                    
Steps to run. Run the following commands -
1) g++ assembler.cpp -o assembler
2) ./assembler input.txt
3) g++ loader.cpp -o loader
4) ./loader


3 files will be created -
- intermediate.txt: Intemediary file made by Pass 1 of assembler
- assemblerOutput.txt: Output of the assembler
- loaderOutput.txt: Output of the loader