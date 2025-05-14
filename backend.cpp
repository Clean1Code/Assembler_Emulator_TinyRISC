#include <iostream>
#include<fstream>
#include<sstream>
#include<string>
#include<map>
#include<vector>
#include<bitset>
#include<stack>
#include<algorithm>
#include <cstdint>
using namespace std;

map<string, pair<string, int>> opcodes; // For eg: opcodes[add] = (00000, 3); 00000 is machine code and 3 is number of paramters required
map<string, bool> opcodeWithModifier; //map of opcodes that can have modifiers 'u', 'h'
map<string, int> label;
vector<string> Errors; //Stores all errors in the program
vector<string> MachineCode; //Store all machine codes of a program
map<int, string> accessedMemory; //Memory accessed during execution of code
int currLine = 0;
int currLineOfAssembler = 0;
vector<string> EmulatorErrors;
map<string, string> operation;
bool regIndOffset = false; //Check whether the program line contains register index offset type

void InitializeOpcodes()
{
    opcodes["add"] = { "00000", 3 };
    opcodes["sub"] = { "00001", 3 };
    opcodes["mul"] = { "00010", 3 };
    opcodes["div"] = { "00011", 3 };
    opcodes["mod"] = { "00100", 3 };
    opcodes["cmp"] = { "00101", 2 };
    opcodes["and"] = { "00110", 3 };
    opcodes["or"] = { "00111", 3 };
    opcodes["not"] = { "01000", 2 };
    opcodes["mov"] = { "01001", 2 };
    opcodes["lsl"] = { "01010", 3 };
    opcodes["lsr"] = { "01011", 3 };
    opcodes["asr"] = { "01100", 3 };
    opcodes["nop"] = { "01101", 0 };
    opcodes["ld"] = { "01110", 3 };
    opcodes["st"] = { "01111", 3 };
    opcodes["beq"] = { "10000", 1 };
    opcodes["bgt"] = { "10001", 1 };
    opcodes["b"] = { "10010", 1 };
    opcodes["call"] = { "10011", 1 };
    opcodes["ret"] = { "10100", 0 };
    opcodes["hlt"] = { "10101", 0 };

    for (auto x : opcodes)
    {
        operation[x.second.first] = x.first;
    }

    opcodeWithModifier["add"] = true;
    opcodeWithModifier["sub"] = true;
    opcodeWithModifier["mul"] = true;
    opcodeWithModifier["div"] = true;
    opcodeWithModifier["mod"] = true;
    opcodeWithModifier["and"] = true;
    opcodeWithModifier["or"] = true;
    opcodeWithModifier["lsl"] = true;
    opcodeWithModifier["lsr"] = true;
    opcodeWithModifier["asr"] = true;
}

string convertLabelValToBin(int val)
{
    string bin = "";

    int S = 0;
    if (val < 0)
    {
        val = -val + 1;
        S = 1;
    }

    for (int i = 0; i < 27; i++)
    {
        if (val % 2 == 1) bin += '1' - S;
        else bin += '0' + S;
        val = val / 2;
    }

    reverse(bin.begin(), bin.end());
    return bin;
}

string valToBin(int32_t val)
{
    string bin = "";

    int S = 0;
    if (val < 0)
    {
        val = -val - 1;
        S = 1;
    }

    for (int i = 0; i < 32; i++)
    {
        if (val % 2 == 1) bin += '1' - S;
        else bin += '0' + S;
        val = val / 2;
    }

    reverse(bin.begin(), bin.end());
    return bin;
}

int getNumberOfOperands(stringstream& words)
{
    int numberOfOperands = 0;
    string token;

    while (words >> token)
    {
        numberOfOperands++;
    }

    return numberOfOperands;
}

bool checkRegister(string num)
{
    if (num[0] == 'r' and num.size() > 1 and num.size() < 4)
    {
        num.erase(0, 1);

        if (num.size() == 1 and num < "1") return false;
        if (num.size() == 2 and num > "15") return false;

        for (int i = 0; i < num.size(); i++)
        {
            if (!isdigit(num[i]))
            {
                return false;
            }
        }
    }
    else
    {
        return false;
    }

    return true;
}

string convertRegisterToBin(string num)
{
    string bin, tem;
    bin = "";
    tem = "";

    int x = 0;

    for (int i = 1; i < num.size(); i++)
    {
        x *= 10;
        x += num[i] - '0';
    }

    for (int i = 0; i < 4; i++)
    {
        tem += to_string(x % 2);
        x = x / 2;
    }

    for (int i = 0; i < 4; i++)
    {
        bin += tem[3 - i];
    }

    return bin;
}

bool checkHex(string num)
{
    if (num.size() < 3 || num[0] != '0' || num[1] != 'x' || num.size() >= 7)
        return false;

    for (int i = 2; i < num.size(); i++)
    {
        if (!isxdigit(num[i]))
        {
            return false; // Ensure valid hex characters
        }
    }
    return true;
}

string convertHexToBin(string hex_string, int len)
{
    stringstream ss;

    ss << hex << hex_string;
    unsigned long long num;
    ss >> num;

    return bitset<32>(num).to_string().substr(32 - len);
}

bool checkHex2(string num)
{
    if (num.size() < 3 || num[0] != '0' || num[1] != 'x' || num.size() >= 10)
        return false;

    for (int i = 2; i < num.size(); i++)
    {
        if (!isxdigit(num[i]))
        {
            return false; // Ensure valid hex characters
        }
    }

    string s = convertHexToBin(num, 32);

    for (int i = 0; i < 5; i++)
    {
        if (s[i] == '1')
        {
            return false;
        }
    }

    return true;
}

string fetchOperandOpcode(stringstream& words, int numberOfOperands, string modifier, vector<string>& Errors, bool Switch)
{
    vector<string> operands; //list of operands present
    string token;
    string operandOpcode = "";  //opcode of operand
    string offset = ""; //offset incase if all operand consists of all registers
    int countNumberOfOperands = 0;

    for (int i = 0; i < 14; i++) offset += "0";

    while (words >> token)
    {
        operands.push_back(token);
        countNumberOfOperands++;
    }

    if (countNumberOfOperands > numberOfOperands) //check whether valid number of operands for the opcode is present
    {
        Errors.push_back("Too many operands found at line " + to_string(currLine + 1));
        return "-1";
    }
    else if (countNumberOfOperands < numberOfOperands)
    {
        Errors.push_back("Too few operands found at line " + to_string(currLine + 1));
        return "-1";
    }

    if (regIndOffset)
    {
        swap(operands[1], operands[2]);
    }

    if (numberOfOperands == 0) //handle opcodes which has 0 operand
    {
        for (int i = 0; i < 27; i++) operandOpcode += "0";
        return operandOpcode;
    }

    if (numberOfOperands == 1) //handle opcodes which has 1 operand
    {
        if (checkHex2(operands[0]))
        {
            operandOpcode = convertHexToBin(operands[0], 27);
            return operandOpcode;
        }
        else
        {
            if (label.find(operands[0]) == label.end())
            {
                Errors.push_back("Expected label or Hexadecimal number less than 0x7FFFFFF as an operand at line " + to_string(currLine + 1));
                return "-1";
            }
            else
            {
                operandOpcode = convertLabelValToBin(label[operands[0]]);
                return operandOpcode;
            }
        }
    }

    if (numberOfOperands == 2) //handle opcodes which has 2 operands
    {
        if (checkHex(operands[0]) or !checkRegister(operands[0]))
        {
            Errors.push_back("Expected Register as the first operand at line " + to_string(currLine + 1));
            return "-1";
        }
        else
        {
            if (checkHex(operands[1]))
            {
                operandOpcode = "1";
            }
            else
            {
                if (modifier != "00")
                {
                    Errors.push_back("Expected hexadecimal number found register instead at line " + to_string(currLine + 1));
                    return "-1";
                }

                if (!checkRegister(operands[1]))
                {
                    Errors.push_back("Invalid register or hexadecimal operand found at line " + to_string(currLine + 1));
                    return "-1";
                }

                operandOpcode = "0";
            }


            if (Switch)
            {
                if (operandOpcode == "0")
                {
                    operandOpcode += "0000" + convertRegisterToBin(operands[0]) + convertRegisterToBin(operands[1]) + offset;
                }
                else
                {
                    operandOpcode += "0000" + convertRegisterToBin(operands[0]) + modifier + convertHexToBin(operands[1], 16);
                }
            }
            else
            {
                if (operandOpcode == "0")
                {
                    operandOpcode += convertRegisterToBin(operands[0]) + "0000" + convertRegisterToBin(operands[1]) + offset;
                }
                else
                {
                    operandOpcode += convertRegisterToBin(operands[0]) + "0000" + modifier + convertHexToBin(operands[1], 16);
                }
            }
        }

        return operandOpcode;
    }

    if (numberOfOperands == 3) //handle opcode which has 3 operands
    {
        if (checkHex(operands[0]) or !checkRegister(operands[0]))
        {
            Errors.push_back("Expected Register as the first operand at line " + to_string(currLine + 1));
            return "-1";
        }
        else
        {
            if (checkHex(operands[1]) or !checkRegister(operands[1]))
            {
                Errors.push_back("Expected Register as the second operand at line " + to_string(currLine + 1));
                return "-1";
            }
            else
            {
                if (checkHex(operands[2]))
                {
                    operandOpcode = "1";
                }
                else
                {
                    if (modifier != "00")
                    {
                        Errors.push_back("Expected hexadecimal number found register instead at line " + to_string(currLine + 1));
                        return "-1";
                    }

                    if (!checkRegister(operands[2]))
                    {
                        Errors.push_back("Invalid hexadecimal number found at line " + to_string(currLine + 1));
                        return "-1";
                    }

                    operandOpcode = "0";
                }

                if (operandOpcode == "0")
                {
                    operandOpcode += convertRegisterToBin(operands[0]) + convertRegisterToBin(operands[1]) + convertRegisterToBin(operands[2]) + offset;
                }
                else
                {
                    operandOpcode += convertRegisterToBin(operands[0]) + convertRegisterToBin(operands[1]) + modifier + convertHexToBin(operands[2], 16);
                }
            }
        }

        return operandOpcode;
    }
}

string modifyBin(string immediate, string modifier)
{
    string offset0 = "";
    string offsetF = "";
    for (int i = 0; i < 16; i++) offset0 += '0';
    for (int i = 0; i < 16; i++) offsetF += '1';

    if (modifier == "00")
    {
        if (immediate[0] == '1') immediate = offsetF + immediate;
    }
    else if (modifier == "10")
    {
        immediate = immediate + offset0;
    }

    return immediate;
}

int32_t binToDecimal(string bin)
{
    int Size = bin.size();
    int32_t num = 1;
    int32_t decimal = 0;

    for (int i = Size - 1; i >= 0; i--)
    {
        if (bin[i] == '1') decimal += num;
        num *= 2;
    }

    return decimal;
}

void Assembler(ifstream& code)
{
    string line; //Line of a code
    string token; //word or token present in a line
    int len;
    currLine = 0;
    int numberOfOperands = 0;
    bool Switch = false; //Switch is set for opcodes which doesn't have destination address

    vector<string> Line;
    string machineCode = ""; //Machine Code of a line

    while (getline(code, line)) // Fetch a line in the code
    {
        //cout << line << endl;
        for (int i = 0; i < line.size(); i++) //Replace all commas with spaces
        {
            regIndOffset = false;
            if (line[i] == ',')
            {
                line[i] = ' ';
            }
            else if (line[i] == '[' or line[i] == ']')
            {
                line[i] = ' ';
                regIndOffset = true;
            }

            if (i != line.size() - 1)
            {
                if (line[i] == '/' and line[i + 1] == '/')
                {
                    line = line.substr(0, i);
                }
            }
        }

        if (line.size() == 0)
        {
            currLine++;
            continue;
        }
        Line.push_back(line);

        stringstream words(line); //To iterate through token in the line
        stringstream copy(line); //make a copy
        int numberOfWords = getNumberOfOperands(copy); //Number of tokens or words in a line
        machineCode = "";


        int currTokenNumber = 0; //Current token being processed
        string modifier = "00"; //Default modifier
        numberOfOperands = -1; //Number of operands in the opcode

        while (words >> token)
        {
            if (currTokenNumber == 0) //Check for label definition
            {
                if (token.back() == ':')
                {
                    token.pop_back();
                    label[token] = currLineOfAssembler; //Assign label string to the current line of the code
                    currTokenNumber++;
                    continue;
                }
            }
            else
            {
                if (token.back() == ':')
                {
                    Errors.push_back("Invalid syntax expected Label '" + token + "' before at line " + to_string(currLine + 1));
                    break;
                }
            }

            if (currTokenNumber <= 1) //Check for opcode
            {
                if (opcodes.find(token) == opcodes.end()) //Check for modifiers
                {
                    if (token.back() == 'u' or token.back() == 'h') //Check for modifiers
                    {
                        if (token.back() == 'u')
                        {
                            modifier = "01";
                        }
                        else
                        {
                            modifier = "10";
                        }

                        token.pop_back();
                    }
                }

                if (opcodes.find(token) == opcodes.end()) //Check if opcode is valid
                {
                    Errors.push_back("Invalid Opcode '" + token + "' found at line " + to_string(currLine + 1));
                    break;
                }
                else
                {
                    //Convert line to machine code

                    if (token == "cmp") Switch = true;
                    machineCode += opcodes[token].first;
                    numberOfOperands = opcodes[token].second;
                    machineCode += fetchOperandOpcode(words, numberOfOperands, modifier, Errors, Switch);
                    break;
                }
            }

            currTokenNumber++;
        }

        MachineCode.push_back(machineCode);
        currLine++;
        currLineOfAssembler++;
    }

    /*for (int i = 0; i < Errors.size(); i++)
    {
        std::cout << Errors[i] << endl;
    }*/

    cout << "Machine Code:" << endl;
    for (int i = 0; i < MachineCode.size(); i++)
    {
        //cout << MachineCode[i].size() << endl;
        std::cout << MachineCode[i] << " " << Line[i] << endl;
    }
}

void getOperands(string Operator, char operationType, vector<int32_t>& operands, int i)
{
    int32_t operand;
    if (opcodes[Operator].second == 1)
    {
        operand = binToDecimal(MachineCode[i].substr(6, 27));
        operands.push_back(operand);
    }
    else if (opcodes[Operator].second == 2)
    {
        if (Operator == "cmp")
        {
            operand = binToDecimal(MachineCode[i].substr(10, 4));
            operands.push_back(operand);
        }
        else
        {
            operand = binToDecimal(MachineCode[i].substr(6, 4));
            operands.push_back(operand);
        }

        if (operationType == '0') operand = binToDecimal(MachineCode[i].substr(14, 4));
        else
        {
            string immediate = MachineCode[i].substr(16, 16);
            string modifier = MachineCode[i].substr(14, 2);

            immediate = modifyBin(immediate, modifier);
            operand = binToDecimal(immediate);
        }
        operands.push_back(operand);
    }
    else if (opcodes[Operator].second == 3)
    {
        //cout << opcodes[Operator].first << " " << MachineCode[i].substr(6, 4) << endl;
        operand = binToDecimal(MachineCode[i].substr(6, 4));
        operands.push_back(operand);
        //cout << operand << endl;

        operand = binToDecimal(MachineCode[i].substr(10, 4));
        operands.push_back(operand);

        if (operationType == '0') operand = binToDecimal(MachineCode[i].substr(14, 4));
        else
        {
            string immediate = MachineCode[i].substr(16, 16);
            string modifier = MachineCode[i].substr(14, 2);

            //cout << immediate << " " << modifier << endl;

            immediate = modifyBin(immediate, modifier);
            //cout << immediate << endl;
            operand = binToDecimal(immediate);
        }
        operands.push_back(operand);
    }
}

int executeLine(vector<int32_t>& operands, string Operator, vector<int32_t>& reg, int& flagEQ, int& flagGT, char operationType, int& programCounter, vector<string>& memory, stack<int32_t>& sMemory)
{
    if (Operator == "add")
    {
        if (operationType == '1') reg[operands[0]] = reg[operands[1]] + operands[2];
        else reg[operands[0]] = reg[operands[1]] + reg[operands[2]];
    }
    else if (Operator == "sub")
    {
        if (operationType == '1') reg[operands[0]] = reg[operands[1]] - operands[2];
        else reg[operands[0]] = reg[operands[1]] - reg[operands[2]];
    }
    else if (Operator == "mul")
    {
        if (operationType == '1') reg[operands[0]] = reg[operands[1]] * operands[2];
        else reg[operands[0]] = reg[operands[1]] * reg[operands[2]];
    }
    else if (Operator == "div")
    {
        if (operationType == '1') reg[operands[0]] = reg[operands[1]] / operands[2];
        else reg[operands[0]] = reg[operands[1]] / reg[operands[2]];
    }
    else if (Operator == "mod")
    {
        if (operationType == '1') reg[operands[0]] = reg[operands[1]] / operands[2];
        else reg[operands[0]] = reg[operands[1]] / reg[operands[2]];
    }
    else  if (Operator == "cmp")
    {
        if (operationType == '1')
        {
            if (reg[operands[0]] == operands[1])
            {
                flagEQ = 1;
                flagGT = 0;
            }
            else if (reg[operands[0]] > operands[1])
            {
                flagGT = 1;
                flagEQ = 0;
            }
            else
            {
                flagGT = 0;
                flagEQ = 0;
            }
        }
        else
        {
            if (reg[operands[0]] == reg[operands[1]])
            {
                flagEQ = 1;
                flagGT = 0;
            }
            else if (reg[operands[0]] > reg[operands[1]])
            {
                flagGT = 1;
                flagEQ = 0;
            }
            else
            {
                flagGT = 0;
                flagEQ = 0;
            }
        }
    }
    else if (Operator == "and")
    {
        reg[operands[0]] = reg[operands[1]];

        if (operationType == '1') reg[operands[0]] = reg[operands[0]] & operands[2];
        else reg[operands[0]] = reg[operands[0]] & reg[operands[2]];
    }
    else if (Operator == "or")
    {
        reg[operands[0]] = reg[operands[1]];

        if (operationType == '1') reg[operands[0]] = reg[operands[0]] | operands[2];
        else reg[operands[0]] = reg[operands[0]] | reg[operands[2]];
    }
    else if (Operator == "not")
    {
        if (operationType == '1') reg[operands[0]] = ~operands[1];
        else reg[operands[0]] = ~reg[operands[1]];
    }
    else if (Operator == "mov")
    {
        if (operationType == '1') reg[operands[0]] = operands[1];
        else reg[operands[0]] = reg[operands[1]];
    }
    else if (Operator == "lsl")
    {
        reg[operands[0]] = reg[operands[1]];

        if (operationType == '1') reg[operands[0]] = reg[operands[0]] << operands[2];
        else reg[operands[0]] = reg[operands[0]] << reg[operands[2]];
    }
    else if (Operator == "lsr")
    {
        reg[operands[0]] = reg[operands[1]];

        if (operationType == '1') reg[operands[0]] = reg[operands[0]] << operands[2];
        else reg[operands[0]] = reg[operands[0]] << reg[operands[2]];
    }
    else if (Operator == "asr")
    {

    }
    else if (Operator == "nop")
    {
        return 0;
    }
    else if (Operator == "ld")
    {
        if (operationType == '1') reg[operands[0]] = binToDecimal(memory[reg[operands[1]] + operands[2]]);
        else reg[operands[0]] = binToDecimal(memory[reg[operands[1]] + reg[operands[2]]]);
    }
    else if (Operator == "st")
    {
        string st = valToBin(reg[operands[0]]);

        if (operationType == '1')
        {
            memory[reg[operands[1]] + operands[2]] = st;
            accessedMemory[reg[operands[1]] + operands[2]] = st;
        }
        else
        {
            memory[reg[operands[1]] + reg[operands[2]]] = st;
            accessedMemory[reg[operands[1]] + reg[operands[2]]] = st;
        }
    }
    else if (Operator == "beq")
    {
        if (flagEQ == 1) programCounter = operands[0] - 1;
    }
    else if (Operator == "bgt")
    {
        //cout << operands[0] << endl;
        if (flagGT == 1) programCounter = operands[0] - 1;
    }
    else if (Operator == "b")
    {
        sMemory.push(programCounter);
        programCounter = operands[0] - 1;
    }
    else if (Operator == "call")
    {
        programCounter = operands[0] - 1;
    }
    else if (Operator == "ret")
    {
        if (sMemory.size() != 0)
        {
            programCounter = sMemory.top();
            //programCounter--;
            sMemory.pop();
        }
    }
    else if (Operator == "hlt")
    {
        return 1;
    }

    return 0;
}

void Emulator()
{
    int programCounter = 0;
    int programSize = MachineCode.size();
    //cout << MachineCode[3] << endl;

    string empty = "";
    for (int i = 0; i < 32; i++) empty += '0';

    vector<string> memory(1e7 + 1, empty);
    stack<int32_t> sMemory;

    vector<int32_t> reg(16, 0);
    int flagEQ = 0;
    int flagGT = 0;
    int halt;
    int i;

    for (i = 0; i <= 1e4; i++)
    {
        vector<int32_t> operands;

        //cout << "Program Counter: " << programCounter << " " << endl;
        if (programCounter >= programSize)
        {
            EmulatorErrors.push_back("EOF error no halt operation found");
            break;
        }

        string opcode = MachineCode[programCounter].substr(0, 5);
        //cout << opcode << " " << operation[opcode] << " ";

        char operationType = MachineCode[programCounter][5];
        //cout << operationType << " " << MachineCode[programCounter] << endl;

        getOperands(operation[opcode], operationType, operands, programCounter);

        //for (int I = 0; I < operands.size(); I++) cout << operands[I] << " "; cout << endl;

        halt = executeLine(operands, operation[opcode], reg, flagEQ, flagGT, operationType, programCounter, memory, sMemory);


        if (halt == 1) break;
        else programCounter++;
    }

    if (i >= 1e4) EmulatorErrors.push_back("Infinite loop detected");

    if (EmulatorErrors.size() > 0)
    {
        cout << "Emulator Errors:" << endl;
        for (int i = 0; i < EmulatorErrors.size(); i++) cout << EmulatorErrors[i] << endl;
    }

    cout << "Registers:" << endl;
    for (int i = 1; i < 16; i++)
    {
        cout << reg[i] << endl;
    }

    cout << "Memory:" << endl;
    for (auto it : accessedMemory)
    {
        cout << it.first << " " << it.second << endl;
    }
}

int main()
{
    char ch;
    /*std::cout << "Press anything to run code or (Q) to quit the program: ";
    cin >> ch;
    std::cout << endl;*/

    //while (ch != 'Q')
    //{
    string fileName = "Program.txt";
    ifstream code(fileName);
    InitializeOpcodes();

    if (!code)
    {
        cerr << '/' << fileName << " not found" << endl;
        return 1;
    }

    Assembler(code);
    //std::cout << endl;

    if (Errors.size() != 0)
    {
        cout << "Errors:" << endl;
        for (int i = 0; i < Errors.size(); i++) cout << Errors[i] << endl;
    }


    if (Errors.size() == 0)
    {
        /*cout << "Machine Code:" << endl;
        for (int i = 0; i < MachineCode.size(); i++)
        {
            cout << MachineCode[i] << endl;
        }*/
        Emulator();
    }
    MachineCode.clear();
    opcodes.clear();
    map<string, pair<string, int>> opcodes; // For eg: opcodes[add] = (00000, 3); 00000 is machine code and 3 is number of paramters required
    opcodeWithModifier.clear(); //map of opcodes that can have modifiers 'u', 'h'
    label.clear();
    Errors.clear(); //Stores all errors in the program
    currLine = 0;
    currLineOfAssembler = 0;
    EmulatorErrors.clear();
    operation.clear();

    code.close();
    /*std::cout << "Press anything to run code or (Q) to quit the program: ";
    cin >> ch;
    cout << endl;*/
    //}
    return 0;
}
