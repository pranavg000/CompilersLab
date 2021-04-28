/**************************************************************************
 *************** Read Read_Me.txt for instructions to run ******************
 *************************************************************************/

#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <iomanip>
#include <vector>
#include <cassert>

#define HEX uppercase << hex


using namespace std;
// LDA, LDX, LDL,STA, STX, STL, LDCH, STCH, ADD, SUB, MUL, DIV, 
// COMP, J, JLT, JEQ, JGT, JSUB, RSUB, TIX, TD, RD, WD, RESW, RESB, WORD, BYTE.

unordered_map<string, string> SYMTAB, OPTAB = {
        {"DD", "18"},
        {"AND", "40"},
        {"COMP", "28"},
        {"DIV", "24"}, 
        {"J", "3C"}, 
        {"JEQ", "30"}, 
        {"JGT", "34"}, 
        {"JLT", "38"}, 
        {"JSUB", "48"}, 
        {"LDA", "00"}, 
        {"LDCH", "50"}, 
        {"LDL", "08"}, 
        {"LDX", "04"}, 
        {"MUL", "20"}, 
        {"OR", "44"}, 
        {"RD", "D8"}, 
        {"RSUB", "4C"}, 
        {"STA", "0C"}, 
        {"STCH", "54"}, 
        {"STL", "14"}, 
        {"STSW", "E8"}, 
        {"STX", "10"}, 
        {"SUB", "1C"}, 
        {"TD", "E0"}, 
        {"TIX", "2C"}, 
        {"WD", "DC"}};

unordered_set<string> asmdrctv = {{"START", "END", "BYTE", "WORD", "RESB", "RESW"}}; // Assembler directives


int hexToInt(string& s) {   // Helper function to convert HEX to INT
    int temp;
    stringstream ss;
    ss << HEX << s;
    ss >> temp;
    return temp;
}

string intToHex(int n) {   // Helper function to convert INT to HEX
    stringstream ss;
    ss << HEX << n;
    return ss.str();
}

void removeTrailingWhitespaces(string& s){
    int sz = s.size();
    // bool c=false;
    for(int i=sz-1;i>=0;i--){
        if(s[i]!=' ' && s[i]!='\t'){
            sz=i+1;
            break;
        } 
    }
    s = s.substr(0, sz);
}

// Initalizes the instruction data members from the string stream
void initializeInst(stringstream& ss, bool& empty, string& label, string& opcode, string& operand){
    string first, second;
    
    ss>>first;
    ss>>second;
    if(OPTAB.count(first) || asmdrctv.count(first)){    // Checks if label or not
        opcode = first;
        operand = second;
    }
    else{
        getline(ss >> ws, operand);                      // Removes white spaces from start
        removeTrailingWhitespaces(operand);              // Removes white spaces from end
        label = first;
        opcode = second;
    }
    empty = false;
}

struct Instruction {                // Struct to hold Input assembly instructions
    string label, opcode, operand;
    bool empty;
    Instruction(string& line) {
        if(line.size() == 0) {     // Checks if empty line
            empty = true;
            return;
        }
        stringstream ss(line);
        initializeInst(ss, empty, label, opcode, operand); // Initalizes from the string stream
    }
    bool isLabel(){                 // returns true if there is label in the instruction
        return (label.size()>0);
    }
    int getOperandInt(){            // Converts operand to int
        return stoi(operand);
    }
    int getLengthByteArg(){         // returns length of string in BYTE operand 
        int sz = operand.size();
        assert(sz>=3);
        if(operand[0] == 'C'){      // Checks if char or hex string
            return sz-3;
        }
        else{
            assert((sz-3)%2==0);
            return (sz-3)/2;
        }
    }
    
};


struct InstructionInter {                       // Struct to hold intermediate instructions
    string address, label, opcode, operand;
    bool empty;
    InstructionInter(string& line) {
        if(line.size() == 0) {                  // Checks if empty line
            empty = true;
            return;
        }
        stringstream ss(line);
        ss>>address;
        initializeInst(ss, empty, label, opcode, operand); // Initalizes from the string stream
    }

    string getObjectCodeByteOperand(){                // get Object code in hex from BYTE string operand 
        string s;
        int sz = operand.size();
        if(operand[0]=='C'){                          // Checks type of operand
            for(int i=2;i<sz-1;i++){
                string temp = intToHex(int(operand[i]));    
                if(temp.size()==1) s += '0';
                s += temp;                            // Loops and convert byte-by-byte
            }
        }
        else{
            s+=operand.substr(2, sz-3);               // Hex can be used as it is after removing Quotes
        }
        return s;
    }
};

bool errorFlag=false;

struct TextRecord {                     // Struct to hold Text Records
    vector<string> objectCodes;
    int limit;
    string startingAddress;

    TextRecord(): limit{10} {}

    bool isFull(){                      // Internal helper function: check record is full
        return ((int)objectCodes.size()) >= limit;
    }

    void addObjectCode(string objectCode, string address){      // Adds object code to the text record
        if(!objectCodes.size()) startingAddress = address;
        objectCodes.push_back(objectCode);
    }

    void writeRecordOnStream(ofstream& fout){                   // Writes record to output file stream
        if(objectCodes.size() == 0) return;
        int sz=0;
        for(string& x: objectCodes){                            // Computing size of record
            assert(x.size()%2==0);
            sz+=(x.size()/2);
        }
        fout<<"T"                                               // Writing the beginning of Text record
            <<right<<setfill('0')<<setw(6)<<startingAddress
            <<right<<setfill('0')<<setw(2)<<HEX<<sz;
        
        for(string& x: objectCodes){                            // Writing object codes
            fout<<x;
        }
        fout<<'\n';
        objectCodes.clear();                                    // Clearing for next record
        startingAddress = "";
    }
} textRecord;

string padZeroesToLeft(string s, int sz=6){     // Pads string with zeroes in the beginning
    string temp;
    for(int i=sz;i>((int)s.size());i--) temp += '0';
    return temp+s;
}

void addressSymbolToValue(string& operand){    // get address opcode from instruction operand
    int address;
    int ci = operand.size();
    if(ci == 0) {                              // If no operand, set operand as 0
        operand = "0000";
        return;
    }
    bool directAddressing = true;
    for(int i=0;i<((int)operand.size());i++){
        if(operand[i] == ',') {ci=i; directAddressing=false; break;}
    }
    operand.erase(ci);
    if(SYMTAB.count(operand)){                  // Checks if operand in SYMTAB
        operand = SYMTAB[operand];
    }
    else {                                      // else but operand as 0 and errorFlag=true
        operand = "0000";
        errorFlag = true;
        return;
    }
    address = hexToInt(operand);
    if(!directAddressing) {                     // Indexed addressing
        address |= (1<<15);
    }
    operand = padZeroesToLeft(intToHex(address), 4);
}

int startingAddress;                            
int locctr;                                // Initializing locctr to 
string line;
string programName;
string inputFile, interFile = "intermediate.txt", outputFile="out.txt";

void addToRecord(ofstream& fout, string s, string address){   // Add object code of instruction to textRecord
    if(textRecord.isFull()){                                  // If textRecord is full, write record in ouput file
        textRecord.writeRecordOnStream(fout);
    }
    textRecord.addObjectCode(s, address);
}



void writeHeaderRecord(ofstream& fout){             // Write HEADER record to output file stream
    fout<<"H"
        <<left<<setfill(' ')<<setw(6)<<programName
        <<right<<setfill('0')<<setw(6)<<HEX<<startingAddress
        <<right<<setfill('0')<<setw(6)<<HEX<<locctr-startingAddress<<'\n';
}

void writeEndRecord(ofstream& fout){                // Write END record to output file stream
    fout<<"E"
        <<right<<setfill('0')<<setw(6)<<HEX<<startingAddress;
}

void writeLineToStream(ofstream& fout){             // Write current line to output file stream
    fout<<HEX<<locctr;
    fout<<" "<<line<<'\n';
}

void handleStrings(ofstream& fout, InstructionInter& inst){     // Handles character and hex BYTE strings
    string objectCode = inst.getObjectCodeByteOperand();
    int sz = objectCode.size();
    int j=0;
    for(int i=0;i<sz-6;i+=6){                                  // Breaks BYTE string to 3 byte blocks 
        string curAdd = intToHex(hexToInt(inst.address) + i);  // and add to text record
        addToRecord(fout,objectCode.substr(i,6), curAdd);
        j=i+6;
    }
    if(j<sz) {
        addToRecord(fout, objectCode.substr(j,sz-j), inst.address);
    }
}

int main(int argc, char *argv[]){

    if(argc != 2){                              // Taking input filename as operand
        cout<<"\nInvalid number of arguments\nSee Read_Me.txt for Steps to run\nABORTING!!!\n\n";
        return -1;
    }

    inputFile = argv[1];
    ifstream finput (inputFile);                  // Creating i/o streams for input/output files
    ofstream fileout (interFile);
    


    /**************** PASS 1 STARTS ****************/
    while(!finput.eof()){
        getline(finput >> ws, line);    // Read line from input file
        if(line.size() == 0) break;
        if(line[0] == '.') {            // It is a comment, so just write in intermediate file
            fileout<<line<<'\n';
            continue;
        }
        Instruction inst(line);
        if(inst.empty) continue;
        if(inst.opcode == "START"){     // If START inst, initializations done
            startingAddress = hexToInt(inst.operand);
            locctr = startingAddress;
            programName = inst.label;
            writeLineToStream(fileout);     // Write line to intermediate file
            continue;
        }
        else {
            writeLineToStream(fileout);     // Write line to intermediate file
            if(inst.opcode != "END") {
                if(inst.isLabel()){
                    if(SYMTAB.count(inst.label)){    // Duplicate label
                        errorFlag = true;
                    }
                    else{
                        SYMTAB[inst.label] = intToHex(locctr);  // Inserting (Label, locctr) in SYMTAB
                    }
                }
                if(OPTAB.count(inst.opcode)){
                    locctr += 3;
                }
                else if(inst.opcode == "WORD"){
                    locctr += 3;                                // locctr jumps 1 word ie 3 bytes
                }
                else if(inst.opcode == "RESW"){
                    locctr += (3*inst.getOperandInt());        // locctr jumps 3*noOfWords bytes
                }
                else if(inst.opcode == "RESB"){
                    locctr += inst.getOperandInt();
                }
                else if(inst.opcode == "BYTE"){                // locctr jumps size of string
                    locctr += inst.getLengthByteArg();
                }
                else{
                    errorFlag = true;
                }
            }
            
            if(inst.opcode == "END") {              // If END, write size in intermediate file
                fileout<<locctr - startingAddress<<'\n';
                break;
            }
        }
    }
    finput.close();
    fileout.close();

    /**************** PASS 1 ENDS ****************/

    cout<<"PASS 1 completed.\n";

    /**************** PASS 2 STARTS ****************/
    ifstream ifinter(interFile);
    ofstream fout(outputFile);

    getline(ifinter >> ws, line);
    InstructionInter inst(line);
    
    writeHeaderRecord(fout);                    // Write HEADER record

    while(!ifinter.eof()){
        getline(ifinter >> ws, line);
        if(line.size() == 0) break;
        if(line[0] == '.') {                    // It is a comment, so we ignore it
            fileout<<line<<'\n';
            continue;
        }
        inst = InstructionInter(line);              // Converting line to InstructionInter struct
        if(inst.empty) continue;
        string objectCode;
        if(inst.opcode != "END"){
            if(OPTAB.count(inst.opcode)){           // Check if opcode in optab
                addressSymbolToValue(inst.operand);
                objectCode = OPTAB[inst.opcode] + inst.operand;
            }
            else if(inst.opcode == "WORD") {
                objectCode = padZeroesToLeft(intToHex(stoi(inst.operand))); // Convert Int to HEX
            }
            else if(inst.opcode == "BYTE"){
                handleStrings(fout, inst);          // Handles character and hex BYTE strings
                continue;
            }
            else if(inst.opcode == "RESW" || inst.opcode == "RESB"){
                textRecord.writeRecordOnStream(fout);   // Writes last record because there is gap
                continue;
            }
            addToRecord(fout, (objectCode), inst.address);  // Add object code to Text record
        }
        else{
            break;
        }
    }
    textRecord.writeRecordOnStream(fout);           // Write the last text record in output file

    
    writeEndRecord(fout);                           // Write END Record         

    /**************** PASS 2 ENDS ****************/
    cout<<"PASS 2 completed.\n";

    if(errorFlag) {
        cout<<"INVALID Input\n";
    }
    else{
        cout<<"See file "<<outputFile<<" for the machine code\n";
    }

    return 0;
}