/*************************************************************************
 *************** Read Read_Me.txt for instructions to run ****************
 *************************************************************************/

#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <iomanip>
#include <vector>
#include <climits>
#include <utility>

#define HEX uppercase << hex

using namespace std;

// Prototype for structures and functions (Use is defined later comments)
struct MRecord;
struct Instruction;
struct InstructionInter;
struct TextRecord;
int findType(string& operand);
void handleAddressType2(string& operand);
void handleAddressType3(string& operand);
void handleAddressType4(string& operand, bool isImmediate, string address);
void getAbsoluteAddress(string& operand);
void initializeInst(string& line, bool& empty, string& label, string& opcode, string& operand, int& type);
string getLocctrS();
bool isLiteral(string& s);
string getObjectCodeOfString(string& operand);
void addressSymbolToValue(string& operand);
void writeHeaderRecord(ofstream& fout);
void writeEndRecord(ofstream& fout);
void writeLineToStream(ofstream& fout);
void handleStrings(ofstream& fout, InstructionInter& inst);
void makeStringEvenLength(string& s);

struct LitInfo {                // struct to handle litterals
    string value, address;
    int length;
    bool isString;
    LitInfo(string litName){    
        isString = true;
        if(litName == "*") value = getLocctrS();        // if =*, set value to locctr
        else value = getObjectCodeOfString(litName);    // otherwise get obejct code
        makeStringEvenLength(value);
        length = value.size()/2;
    }
    LitInfo(){}
};


unordered_map<string, string> optab = {
        {"ADD" , "18"},
        {"ADDF" , "58"},
        {"ADDR" , "90"},
        {"AND" , "40"},
        {"CLEAR" , "B4"},
        {"COMP" , "28"},
        {"COMPF" , "88"},
        {"COMPR" , "A0"},
        {"DIV" , "24"},
        {"DIVF" , "64"},
        {"DIVR" , "9C"},
        {"FIX" , "C4"},
        {"FLOAT" , "C0"},
        {"HIO" , "F4"},
        {"J" , "3C"},
        {"JEQ" , "30"},
        {"JGT" , "34"},
        {"JLT" , "38"},
        {"JSUB" , "48"},
        {"LDA" , "00"},
        {"LDB" , "68"},
        {"LDCH" , "50"},
        {"LDF" , "70"},
        {"LDL" , "08"},
        {"LDS" , "6C"},
        {"LDT" , "74"},
        {"LDX" , "04"},
        {"LPS" , "D0"},
        {"MUL" , "20"},
        {"MULF" , "60"},
        {"MULR" , "98"},
        {"NORM" , "C8"},
        {"OR" , "44"},
        {"RD" , "D8"},
        {"RMO" , "AC"},
        {"RSUB" , "4C"},
        {"SHIFTL" , "A4"},
        {"SHIFTR" , "A8"},
        {"SIO" , "F0"},
        {"SSK" , "EC"},
        {"STA" , "0C"},
        {"STB" , "78"},
        {"STCH" , "54"},
        {"STF" , "80"},
        {"STI" , "D4"},
        {"STL" , "14"},
        {"STS" , "7C"},
        {"STSW" , "E8"},
        {"STT" , "84"},
        {"STX" , "10"},
        {"SUB" , "1C"},
        {"SUBF" , "5C"},
        {"SUBR" , "94"},
        {"SVC" , "B0"},
        {"TD" , "E0"},
        {"TIO" , "F8"},
        {"TIX" , "2C"},
        {"TIXR" , "B8"},
        {"WD" , "DC"}};

unordered_set<string> globalSymtab;                         // globalsymtab for extdefs
unordered_map<string,unordered_map<string,string>> symtabs; // Map for symtabs of sections
unordered_map<string,LitInfo> littab;
unordered_map<string,int> length, startingAddresses;        // storing lengtha and starting addresses of sections 

unordered_map<string, string> regs = {                      // Maps reg name to number
    {"A", "0"},
    {"X", "1"},
    {"L", "2"},
    {"B", "3"},
    {"S", "4"},
    {"T", "5"},
    {"F", "6"},
    {"PC", "8"},
    {"SW", "9"}
};

unordered_set<string> extRefs;                              // set for managing external references

unordered_set<string> asmdrctv = {{"START", "END", "BYTE", "WORD", "RESB", "RESW", "BASE", "EQU", "LTORG", "EXTDEF", "EXTREF"}}; // Assembler directives

// int startingAddress;                            
int locctr;                                // Initializing locctr to 
int base;
string line;
string subRoutineName;
string inputFile, interFile = "intermediate.txt", outputFile="assemblerOutput.txt";
int pass;
string firstSubroutine, lastSubroutine;

long hexToInt(string s) {   // convert HEX to INT
    long temp;
    stringstream ss;
    ss << HEX << s;
    ss >> temp;
    return temp;
}

string intToHex(long n) {   // convert INT to HEX
    stringstream ss;
    ss << HEX << n;
    return ss.str();
}

void setBit(string& hexString, int pos){   // set bit in hex string at pos
    hexString = intToHex(hexToInt(hexString)|(1<<pos));
}

void removeTrailingWhitespaces(string& s){   // remove trailing whitepaces
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


string padZeroesToLeft(string s, int sz=6){     // Pads string with zeroes in the beginning
    string temp;
    for(int i=0;i<(sz-(int)s.size());i++) temp += '0';
    return temp+s;
}


void trimWhitespaces(string& s){        // trim whitespaces from begin and end
    while(s.size() && (s.back()==' '|| s.back()=='\t')) s.pop_back();
    if(s.size()==0) return;
    int mi=s.size();
    for(int i=0;i<s.size();i++) if(s[i]!=' ' && s[i]!='\t') {mi=i;break;}
    s = s.substr(mi);
}


void makeStringEvenLength(string& s){   // pad string to make it even length
    if(s.size()%2) s = "0"+s;
}


struct MRecord {                        // struct to handle Modification Records
    vector<string> records;             // List of mrecords
    void addRecord(string& address, int length, char sign, string& name){   // Adding record to list
        stringstream ss;
        ss  <<'M' 
            <<right<<setfill('0')<<setw(6)<<address
            <<right<<setfill('0')<<setw(2)<<HEX<<length
            <<sign
            <<left<<setfill(' ')<<setw(6)<<name;
        records.push_back(ss.str());
    }
    void writeToStream(ofstream& fout){             // Writing records to out stream
        for(string& record: records) fout<<record<<'\n';
        records.clear();
    }  
};

unordered_map<string,MRecord> mRecords;             // Map to keep track of mrecords for diff sections

int findType(string& operand){                      // Find type(1/2/3/4) of inst
    int n = operand.size();
    if(n==0) return 1;
    int mi=operand.size();
    for(int i=0;i<n;i++){
        if(operand[i]==',') {
            mi=i;break;
        }
    }
    string lef = operand.substr(0, mi);
    if(regs.count(lef)) return 2;
    return 3;
}


bool errorFlag=false;                       // Error flag
void setErrorFlag(){                        // Sets error flag
    cout<<"ERROR"<<'\n';
    errorFlag = true;
}



void handleAddressType2(string& operand){       // Helper function to get object codes of 
    int n = operand.size();                     // instruction of type 2
    int mi=n;
    for(int i=0;i<n;i++){
        if(operand[i]==',') {
            mi=i;break;
        }
    }
    string leftReg = operand.substr(0, mi);     // Get register name from operand
    if(regs.count(leftReg)==0) {                // If register not valid, set errorflag
        setErrorFlag();
        return;
    }
    if(mi==n){
        operand = regs[leftReg] + "0";
    }
    else{
        string rightReg = operand.substr(mi+1);     // Get register name from operand
        if(regs.count(rightReg)==0) {
            setErrorFlag();
            return;
        }
        operand = regs[leftReg] + regs[rightReg];
    }
}



void getAbsoluteAddress(string& operand){           // Find value of operand
    if(symtabs[subRoutineName].count(operand)){
        operand = symtabs[subRoutineName][operand]; // If found int symtab, return value
    }
    else if(operand[0]=='='){
        string litName = operand.substr(1);         // If literal, get value from littab
        if(litName == "*") litName = getLocctrS();
        operand = littab[litName].address;
    }
    else{
        try {
            operand = intToHex(stoi(operand));      // If immediate no, get hex value
        }
        catch(const exception& e) {
            setErrorFlag();
        }
    }
}

string getHex2Comp(int num){                // get num in 2s complement form
    if(num>=0){
        return intToHex(num);
    }
    else {
        int comp = (1<<12)+num;             // Calculating 2s complement if num<0
        return intToHex(comp);
    }
}

void handleAddressType3(string& operand){       // Helper function to get object codes of 
    getAbsoluteAddress(operand);                // instruction of type 3
    
    int address = hexToInt(operand);
    int pc = locctr+3;
    if(address - pc >= -2048 && address - pc < 2048) {  // Checks if PC relative addressing works
        operand = getHex2Comp(address - pc);
        setBit(operand, 13);
    }
    else if(address - base >= -2048 && address - base < 2048) {  // Checks if base relative addressing works
        operand = getHex2Comp(address - base);
        setBit(operand, 14);
    }
    else {                                              // Otherwise set error flag
        setErrorFlag();
    }
}


void handleAddressType4(string& operand, bool isImmediate, string address){       // Helper function to get object codes of 
    if(!isImmediate) {                                                            // instruction of type 4
        if(extRefs.count(operand)){
            mRecords[subRoutineName].addRecord(address, 5, '+', operand);           // Add mrecord if Address type is 4
        }
        else{
            mRecords[subRoutineName].addRecord(address, 5, '+', subRoutineName);
        }
    }
    getAbsoluteAddress(operand);
    setBit(operand, 20);                                                        // Set bit 20 
}

int adl=6, ll=10, ocl=10, opl=30;           // Length of diff parts of input files
                                            // Used in parsing
// Initalizes the instruction data members from the string stream
void initializeInst(string& line, bool& empty, string& label, string& opcode, string& operand, int& type){
    string first, second;
    label = line.substr(0,ll);
    trimWhitespaces(label);                         // Parse input line to initialize various variables 
    opcode = line.substr(ll,ocl);
    trimWhitespaces(opcode);
    operand = line.substr(ll+ocl,opl);
    trimWhitespaces(operand);
    
    if(opcode[0] == '+') {                          // Check extended format instruction
        type = 4;
        opcode = opcode.substr(1, opcode.size()-1);
    }
    else if(opcode == "RSUB") type=3;
    else {
        type = findType(operand);                   // Call findType to get type of instruction from operand
    }
    empty = false;
}

string getLocctrS(){                // returns locctr value in hex
    string s = intToHex(locctr);
    makeStringEvenLength(s);
    return s;
}


bool isLiteral(string& s){          // checks if string is literal
    if(s.size()>0 && s[0]=='=') return true;
    return false;
}

struct Instruction {                // Struct to hold Input assembly instructions
    string label, opcode, operand;
    int type, length;
    bool empty;
    Instruction(string& line) {
        if(line.size() == 0) {     // Checks if empty line
            empty = true;
            return;
        }
        initializeInst(line, empty, label, opcode, operand, type); // Initalizes from the string stream
        setInstLength();
    }
    bool isLabel(){                 // returns true if there is label in the instruction
        return (label.size()>0);
    }
    int getOperandInt(){            // Converts operand to int
        return stoi(operand);
    }
    int getLengthByteArg(){         // returns length of string in BYTE operand 
        int sz = operand.size();
        if(operand[0] == 'C'){      // Checks if char or hex string
            return sz-3;
        }
        else{
            return (sz-3)/2;
        }
    }
    bool operandIsLiteral(){        // Checks if the operand is a literal
        return isLiteral(operand);
    }
    void setInstLength(){           // set the length of instruction
        length = type;
    }
    
};

string getObjectCodeOfString(string& operand){    // returns object code of operand
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

struct InstructionInter {                       // Struct to hold intermediate instructions
    string address, label, opcode, operand;
    int type;
    bool empty;
    InstructionInter(string& line) {
        if(line.size() == 0) {                  // Checks if empty line
            empty = true;
            return;
        }
        address = line.substr(0,adl);
        trimWhitespaces(address);
        string restString = line.substr(adl);
        initializeInst(restString, empty, label, opcode, operand, type);    // Initalizes from the string stream
    }

    string getObjectCodeByteOperand(){                // get Object code in hex from BYTE string operand 
        return getObjectCodeOfString(operand);
    }
    bool operandIsLiteral(){                        // Checks if operand is a literal
        return isLiteral(operand);
    }
    void calculateAddressOC(){                      // Calculates Object code of the operand
        if(type == 1){
            return;
        }
        else if(type == 2) {
            handleAddressType2(operand);            // Calling appropriate helper fn for diff types
        }
        else {
            if(opcode == "RSUB") {                  // Handle RSUB seperately
                operand = "0"; setBit(operand, 16); 
                setBit(operand, 17); 
                return;
            } 
            bool isIndirect = false, isImmediate = false, idx = false;
            if(operand[0]=='#') isImmediate = true;     // Checking for immediate and indirect addressing
            if(operand[0]=='@') isIndirect = true;
            int ci = operand.size();
            for(int i=0;i<((int)operand.size());i++){           // Checks for indexed addressing
                if(operand[i] == ',') {ci=i; idx=true; break;}
            }
            operand.erase(ci);
            if(isImmediate || isIndirect) operand = operand.substr(1);
            int c;
            if(type == 3) {
                c = 12;
                if(!isImmediate)
                    handleAddressType3(operand);            // Handles type 3 addresses
            }
            else {
                c = 20;
                bool isExtRef = extRefs.count(operand);
                handleAddressType4(operand, isImmediate, intToHex(hexToInt(address)+1));  // Handles type 4 addresses
            }
            if(idx) setBit(operand, c+3);                           // Set bits in object code accordingly
            if(isImmediate || !isIndirect) setBit(operand, c+4);
            if(isIndirect || !isImmediate) setBit(operand, c+5);
            // if(!isImmediate && !isIndirect) {setBit(operand, c+4); setBit(operand, c+5);}
        }
    }
    string getObjectCode(){                 // Gives object code for entire instruction
        calculateAddressOC();
        string opCodeV = optab[opcode];
        string objectCode;
        if(type == 1){
            objectCode = opcode;
        }
        else if(type == 2){
            objectCode = opCodeV+operand;
        }
        else if(type == 3){
            long operandL = hexToInt(operand);
            objectCode = intToHex((hexToInt(opCodeV)<<16) + operandL);  // Shift the opcode to appropriate position in instruction OC
        }
        else if(type == 4){
            long operandL = hexToInt(operand);
            objectCode = intToHex((hexToInt(opCodeV)<<24) + operandL);  // Shift the opcode to appropriate position in instruction OC
        }
        objectCode = padZeroesToLeft(objectCode, type*2);
        return objectCode;
    }
};


struct TextRecord {                     // Struct to hold Text Records
    vector<string> objectCodes;
    int limit;
    int size;
    string startingAddress;

    TextRecord(): limit(30),size(0) {}      // Inializing limit and size

    void addObjectCode(ofstream& fout, string objectCode, string address){      // Adds object code to the text record
        if(!objectCodes.size()) startingAddress = address;
        if(size + objectCode.size()/2 > limit) {        // If size exceeds limit write to file
            writeRecordOnStream(fout);
            addObjectCode(fout, objectCode, address);
        }
        else {
            objectCodes.push_back(objectCode);
            size+=(objectCode.size()/2);
        }
    }

    void writeRecordOnStream(ofstream& fout){                   // Writes record to output file stream
        if(objectCodes.size() == 0) return;
        
        fout<<"T"                                               // Writing the beginning of Text record
            <<right<<setfill('0')<<setw(6)<<startingAddress
            <<right<<setfill('0')<<setw(2)<<HEX<<size;
        
        for(string& x: objectCodes){                            // Writing object codes
            fout<<x;
        }
        fout<<'\n';
        objectCodes.clear(); 
        size=0;                                   // Clearing for next record
        startingAddress = "";
    }
} textRecord;



void addressSymbolToValue(string& operand){    // get address opcode from instruction operand
    int ci = operand.size();
    if(ci == 0) {                              // If no operand, TYPE 1
        return;
    }
    bool directAddressing = true;
    for(int i=0;i<((int)operand.size());i++){
        if(operand[i] == ',') {ci=i; directAddressing=false; break;}
    }
    operand.erase(ci);
    if(symtabs[subRoutineName].count(operand)){                  // Checks if operand in symtabs[programName]
        operand = symtabs[subRoutineName][operand];
    }
    else {                                      // else but operand as 0 and errorFlag=true
        operand = "0000";
        setErrorFlag();
        return;
    }
    if(!directAddressing) {                     // Indexed addressing
        setBit(operand, 15);
    }
    operand = padZeroesToLeft(operand, 4);
}

void writeHeaderRecord(ofstream& fout){             // Write HEADER record to output file stream
    fout<<"H"
        <<left<<setfill(' ')<<setw(6)<<subRoutineName
        <<right<<setfill('0')<<setw(6)<<HEX<<startingAddresses[subRoutineName]
        <<right<<setfill('0')<<setw(6)<<HEX<<length[subRoutineName]<<'\n';
}

void writeEndRecord(ofstream& fout){                // Write END record to output file stream
    fout<<"E";
    if(subRoutineName == firstSubroutine)
        fout<<right<<setfill('0')<<setw(6)<<HEX<<startingAddresses[subRoutineName];
    if(subRoutineName != lastSubroutine) 
        fout<<"\n\n";
}

void writeLineToStream(ofstream& fout){             // Write current line to output file stream
    fout<<right<<setfill('0')<<setw(adl-1)<<HEX<<locctr;
    fout<<" "<<line<<'\n';
}

void writeStringToStream(ofstream& fout, string& objectCode, InstructionInter& inst){
    int sz = objectCode.size();
    int j=0;
    for(int i=0;i<sz-6;i+=6){                                  // Breaks BYTE string to 3 byte blocks 
        string curAdd = intToHex(hexToInt(inst.address) + i);  // and add to text record
        textRecord.addObjectCode(fout,objectCode.substr(i,6), curAdd);
        // addToRecord(fout,objectCode.substr(i,6), curAdd);
        j=i+6;
    }
    if(j<sz) {
        textRecord.addObjectCode(fout, objectCode.substr(j,sz-j), inst.address);
    }
}

void handleStrings(ofstream& fout, InstructionInter& inst){     // Handles character and hex BYTE strings
    string objectCode = inst.getObjectCodeByteOperand();
    writeStringToStream(fout, objectCode, inst);
}

string findValue(string& expression, string address=""){      // Find value of expression
    string temp;
    long val=0;
    char lastSym='+';                                       // Doing initializations
    expression += '+';
    int n = expression.size();
    for(int i=0;i<n;i++) {                                  // Iterating through the expression
        if(expression[i]=='-' || expression[i]=='+' || expression[i]=='/' || expression[i]=='*'){
            long num;
            if(symtabs[subRoutineName].count(temp)) num=hexToInt(symtabs[subRoutineName][temp]);
            else num = stoi(temp);
            if(lastSym=='-') val-=num;                      // Handling the signs
            else if(lastSym=='+') val+=num;
            else if(lastSym=='/') val/=num;
            else if(lastSym=='*') val*=num;
            if(extRefs.count(temp)>0 && pass == 2)          // If its a ext ref, add mrecord
                mRecords[subRoutineName].addRecord(address, 6, lastSym, temp);
            lastSym = expression[i];
            temp="";
        }
        else temp+=expression[i];
    }
    return intToHex(val);                                   // return the final value as hex
}

struct RRecord {                                            // Struct to store reference record
    vector<string> names;
    void writeRecordOnStream(ofstream& fout){                   // Writes record to output file stream
        if(names.size() == 0) return;
        int sz=names.size();
        for(int i=0;i<sz;i+=12){
            fout<<"R";
            for(int j=i;j<min(sz,i+12);j++){
                fout<<left<<setfill(' ')<<setw(6)<<names[j];
            }
            fout<<'\n';
        }
        names.clear();
    }

    void checkExtref(){                                     // Checks if extrefs are actually present in the globalsymtab
        for(string name: names){
            if(globalSymtab.count(name) == 0) setErrorFlag();
        }
    }
};

unordered_map<string,RRecord> rRecords;                     // Map to hold RRecord for each section

struct DRecord {                                            // Struct to store Define record
    vector<string> names;
    vector<string> addresses;
    void writeRecordOnStream(ofstream& fout){               // Writes record to output file stream
        if(names.size() == 0) return;
        int sz=names.size();
        for(int i=0;i<sz;i+=6){
            fout<<"D";
            for(int j=i;j<min(sz,i+6);j++){
                fout<<left<<setfill(' ')<<setw(6)<<names[j]
                    <<right<<setfill('0')<<setw(6)<<addresses[j];
            }
            fout<<'\n';
        }
        names.clear();                                      // Clearing names and addresses
        addresses.clear();
    }

    void addRecord(string& name){                           // Add record to name
        names.push_back(name);
    }

};

unordered_map<string,DRecord> dRecords;                     // Map to hold DRecord for each section

void writeLitToStream(ofstream& fout, LitInfo& litInfo, string name){   // Writes literal to intermediary file
    fout<<right<<setfill('0')<<setw(adl-1)<<HEX<<locctr<<' ';
    fout<<left<<setfill(' ')<<setw(ll)<<"*";
    fout<<left<<setfill(' ')<<setw(ocl)<<"";
    fout<<left<<setfill(' ')<<setw(opl)<<litInfo.value;
    fout<<"\n";
}

void handleExtdef(string& operands){                    // Handles external defines
    string temp;
    operands+=',';
    DRecord drecord;
    for(int i=0;i<operands.size();i++){
        if(operands[i]!=',') temp+=operands[i];
        else {
            globalSymtab.insert(temp);                  // Inserts in global symtab
            drecord.addRecord(temp);                    // Add to drecord
            temp = "";
        }
    }
    dRecords[subRoutineName] = drecord;
}

void handleExtref(string& operands){                    // Handles external defines
    string temp;
    operands+=',';
    RRecord rRecord;
    for(int i=0;i<operands.size();i++){
        if(operands[i]!=',') temp+=operands[i];
        else if(temp.size()) {
            extRefs.insert(temp);                       // Inserts in appropriate data structures
            rRecord.names.push_back(temp);
            symtabs[subRoutineName][temp] = "0";
            temp = "";
        }
    }
    rRecords[subRoutineName] = rRecord;                 // Inserting RRecord in the map
}

void addExtref(string& operands){                       // Adds external references to extrefs DS
    string temp;
    operands+=',';
    for(int i=0;i<operands.size();i++){
        if(operands[i]!=',') temp+=operands[i];
        else if(temp.size()) {
            extRefs.insert(temp);
            temp = "";
        }
    }
}

void writeRecordsOnStream(ofstream& fout){
    textRecord.writeRecordOnStream(fout);           // Write the last text record in output file
    mRecords[subRoutineName].writeToStream(fout);                   // Write M records
    writeEndRecord(fout); 
}

void writeLitsToStream(ofstream& fout){             // Write lits the output file in the proper locations
    for(auto& it: littab){
        LitInfo& litInfo = it.second;
        if(litInfo.address.empty()){
            litInfo.address = intToHex(locctr);     // Calculates address for literals
            writeLitToStream(fout, litInfo, it.first);
            locctr += litInfo.length;               // Updates locctr
        }
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
    pass = 1;
    while(!finput.eof()){
        getline(finput, line);    // Read line from input file
        if(line.size() == 0) break;
        if(line[0] == '.') {            // It is a comment, so just write in intermediate file
            fileout<<line<<'\n';
            continue;
        }
        Instruction inst(line);
        if(inst.empty) continue;
        if(inst.opcode == "CSECT" || inst.opcode == "END"){     // After finishing section, we come here
            writeLitsToStream(fileout);
            length[subRoutineName] = locctr - startingAddresses[subRoutineName];
            for(auto &u: dRecords[subRoutineName].names){       // Loop through dRecords
                if(symtabs[subRoutineName].count(u) > 0){       // Set addresses for the Names in drecords
                    dRecords[subRoutineName].addresses.push_back(symtabs[subRoutineName][u]);
                }
                else{
                    setErrorFlag();
                }
            }
            
        }
        if(inst.opcode == "START" || inst.opcode == "CSECT"){     // If START/CSECT inst, initializations done
            subRoutineName = inst.label;
            lastSubroutine = subRoutineName;
            if(inst.opcode == "START"){
                startingAddresses[subRoutineName] = hexToInt(inst.operand);  // Setting startAddress
                firstSubroutine = subRoutineName;                           // Setting name of 1st subroutine
            }
            else startingAddresses[subRoutineName] = 0;
            locctr = startingAddresses[subRoutineName];
            globalSymtab.insert(subRoutineName);                            // Insert name in globalsymtab
            mRecords[subRoutineName] = MRecord();
            writeLineToStream(fileout);     // Write line to intermediate file
            continue;
        }
        else {
            writeLineToStream(fileout);     // Write line to intermediate file
            if(inst.opcode != "END") {
                if(inst.isLabel()){
                    if(symtabs[subRoutineName].count(inst.label)){    // Duplicate label
                        setErrorFlag();
                    }
                    else{
                        symtabs[subRoutineName][inst.label] = intToHex(locctr);  // Inserting (Label, locctr) in symtab
                    }
                }
                if(inst.operandIsLiteral()){                    // Handle literals
                    string litName = inst.operand.substr(1);    
                    LitInfo litInfo(litName);
                    if(litName == "*") {                        // Handle =* literal specially
                        litName = litInfo.value;
                    }
                    if(!littab.count(litName)) littab[litName] = litInfo;   // Insert symbol in littab
                }
                if(optab.count(inst.opcode)){
                    locctr += inst.length;
                }
                else if(inst.opcode == "LTORG"){
                    writeLitsToStream(fileout);                 // If LTORG, calc address andwrite lits to stream 
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
                else if(inst.opcode == "EQU"){
                    if(inst.operand == "*"){
                        symtabs[subRoutineName][inst.label] = intToHex(locctr); // Insert locctr in symtab
                    }
                    else{
                        symtabs[subRoutineName][inst.label] = findValue(inst.operand);  // Insert value of expression in symtab
                    }
                }
                else if(inst.opcode == "EXTDEF"){
                    handleExtdef(inst.operand);             // Parses and handles extdef
                }
                else if(inst.opcode == "EXTREF"){
                    handleExtref(inst.operand);             // Parses and handles extref
                }
                else{
                    setErrorFlag();
                }
            }
            
            if(inst.opcode == "END") {              // If END, write size in intermediate file
                break;
                fileout<<locctr - startingAddresses[subRoutineName]<<'\n';
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
    pass = 2;

    while(!ifinter.eof()){
        getline(ifinter, line);
        if(line.size() == 0) break;
        if(line[0] == '.') {                    // It is a comment, so we ignore it
            fileout<<line<<'\n';
            continue;
        }
        InstructionInter inst(line);              // Converting line to InstructionInter struct
        locctr = hexToInt(inst.address);
        if(inst.empty) continue;
        string objectCode;
        
        if(inst.opcode != "END"){
            if(optab.count(inst.opcode)){           // Check if opcode in optab

                objectCode = inst.getObjectCode();
            }
            else if(inst.opcode == "WORD") {
                objectCode = padZeroesToLeft(findValue(inst.operand, inst.address)); // Convert Int to HEX
            }
            else {
                if(inst.opcode == "START" || inst.opcode == "CSECT"){
                    if(inst.opcode == "CSECT"){
                        writeRecordsOnStream(fout);
                    }
                    extRefs.clear();
                    subRoutineName = inst.label;
                    writeHeaderRecord(fout);                    // Write HEADER record
                }
                else if(inst.opcode == "BYTE"){
                    handleStrings(fout, inst);          // Handles character and hex BYTE strings
                }
                else if(inst.opcode == "BASE"){
                    base = hexToInt(symtabs[subRoutineName][inst.operand]);
                }
                else if(inst.opcode == "RESW" || inst.opcode == "RESB"){
                    textRecord.writeRecordOnStream(fout);   // Writes last record because there is gap
                }
                else if(inst.opcode == "EXTDEF"){
                    dRecords[subRoutineName].writeRecordOnStream(fout); // write DRecords on stream
                }
                else if(inst.opcode == "EXTREF"){

                    rRecords[subRoutineName].checkExtref();             // Check if valid references
                    rRecords[subRoutineName].writeRecordOnStream(fout); // write RRecords on stream
                    addExtref(inst.operand);
                }
                else if(inst.label == "*"){                 // Handle Literals 
                    writeStringToStream(fout, inst.operand, inst);
                }
                continue;
            }
            textRecord.addObjectCode(fout, objectCode, inst.address);
        }
        else{
            writeRecordsOnStream(fout);
            break;
        }
    }

    fout.close();
    ifinter.close();
             

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