#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <fstream>
using namespace std;
ofstream  output;
/*{ Sample program
  in TINY language
  compute factorial
}

read x; {input an integer}
if 0<x then {compute only if x>=1}
  fact:=1;
  repeat
    fact := fact * x;
    x:=x-1
  until x=0;
  write fact {output factorial}
end
*/

// sequence of statements separated by ;
// no procedures - no declarations
// all variables are integers
// variables are declared simply by assigning values to them :=
// variable names can include only alphabetic charachters(a:z or A:Z) and underscores
// if-statement: if (boolean) then [else] end
// repeat-statement: repeat until (boolean)
// boolean only in if and repeat conditions < = and two mathematical expressions
// math expressions integers only, + - * / ^
// I/O read write
// Comments {}

// Strings /////////////////////////////////////////////////////////////////////////
bool Equals(const char* a, const char* b)
{
    return strcmp(a, b)==0;
}

bool StartsWith(const char* a, const char* b)
{
    int nb=strlen(b);
    return strncmp(a, b, nb)==0;
}

void Copy(char* a, const char* b, int n=0)
{
    if(n>0) {strncpy(a, b, n); a[n]=0;}
    else strcpy(a, b);
}

void AllocateAndCopy(char** a, const char* b)
{
    if(b==0) {*a=0; return;}
    int n=strlen(b);
    *a=new char[n+1];
    strcpy(*a, b);
}

////////////////////////////////////////////////////////////////////////////////////
// Input and Output ////////////////////////////////////////////////////////////////

#define MAX_LINE_LENGTH 10000

struct InFile
{
    FILE* file;
    int cur_line_num;
    char line_buf[MAX_LINE_LENGTH];
    int cur_ind, cur_line_size;

    InFile(const char* str) {
        file=0;
        if(str) file=fopen(str, "r");
        cur_line_size=0;
        cur_ind=0;
        cur_line_num=0;}

    ~InFile(){if(file) fclose(file);}

    void SkipSpaces()
    {
        while(cur_ind<cur_line_size)
        {
            char ch=line_buf[cur_ind];
            if(ch!=' ' && ch!='\t' && ch!='\r' && ch!='\n') break;
            cur_ind++;
        }
    }

    bool SkipUpto(const char* str)
    {
        while(true)
        {
            SkipSpaces();
            while(cur_ind>=cur_line_size) {if(!GetNewLine()) return false; SkipSpaces();}

            if(StartsWith(&line_buf[cur_ind], str))
            {
                cur_ind+=strlen(str);
                return true;
            }
            cur_ind++;
        }
        return false;
    }

    bool GetNewLine()
    {
        cur_ind=0; line_buf[0]=0;
        if(!fgets(line_buf, MAX_LINE_LENGTH, file)) return false;
        cur_line_size=strlen(line_buf);
        if(cur_line_size==0) return false; // End of file
        cur_line_num++;
        return true;
    }

    char* GetNextTokenStr()
    {
       // SkipSpaces();
        while(cur_ind>=cur_line_size) {if(!GetNewLine()) return 0; //SkipSpaces();
         }
        return &line_buf[cur_ind];
    }

    void Advance(int num)
    {
        cur_ind+=num;
    }
};

struct OutFile
{
    FILE* file;
    OutFile(const char* str) {file=0; if(str) file=fopen(str, "w");}
    ~OutFile(){if(file) fclose(file);}

    void Out(const char* s)
    {
        fprintf(file, "%s\n", s); fflush(file);
    }
    void Outint(int i)
    {
        fprintf(file, "%d\n", i); fflush(file);
    }

};

////////////////////////////////////////////////////////////////////////////////////
// Compiler Parameters /////////////////////////////////////////////////////////////
struct CompilerInfo
{
    InFile in_file;
    OutFile out_file;
    OutFile debug_file;

    CompilerInfo(const char* in_str, const char* out_str, const char* debug_str)
            : in_file(in_str), out_file(out_str), debug_file(debug_str)
    {
    }
};
////////////////////////////////////////////////////////////////////////////////////
// Scanner /////////////////////////////////////////////////////////////////////////
#define MAX_TOKEN_LEN 40
enum TokenType{
    IF, THEN, ELSE, END, REPEAT, UNTIL, READ, WRITE,
    ASSIGN, EQUAL, LESS_THAN,
    PLUS, MINUS, TIMES, DIVIDE, POWER,
    SEMI_COLON,
    LEFT_PAREN, RIGHT_PAREN,
    LEFT_BRACE, RIGHT_BRACE,
    ID, NUM,
    ENDFILE, ERROR
};
// Used for debugging only /////////////////////////////////////////////////////////

const char* TokenTypeStr[]=
        {
                "If", "Then", "Else", "End", "Repeat", "Until", "Read", "Write",
                "Assign", "Equal", "LessThan",
                "Plus", "Minus", "Times", "Divide", "Power",
                "SemiColon",
                "LeftParen", "RightParen",
                "LeftBrace", "RightBrace",
                "ID", "Num",
                "EndFile", "Error"
        };

struct Token
{
    TokenType type;
    char str[MAX_TOKEN_LEN+1];

    Token(){str[0]=0; type=ERROR;}
    Token(TokenType _type, const char* _str) {type=_type; Copy(str, _str);}
};

const Token reserved_words[]=
        {
                Token(IF, "if"),
                Token(THEN, "then"),
                Token(ELSE, "else"),
                Token(END, "end"),
                Token(REPEAT, "repeat"),
                Token(UNTIL, "until"),
                Token(READ, "read"),
                Token(WRITE, "write")
        };
const int num_reserved_words=sizeof(reserved_words)/sizeof(reserved_words[0]);

// if there is tokens like < <=, sort them such that sub-tokens come last: <= <
// the closing comment should come immediately after opening comment
const Token symbolic_tokens[]=
        {
                Token(ASSIGN, ":="),
                Token(EQUAL, "="),
                Token(LESS_THAN, "<"),
                Token(PLUS, "+"),
                Token(MINUS, "-"),
                Token(TIMES, "*"),
                Token(DIVIDE, "/"),
                Token(POWER, "^"),
                Token(SEMI_COLON, ";"),
                Token(LEFT_PAREN, "("),
                Token(RIGHT_PAREN, ")"),
                Token(LEFT_BRACE, "{"),
                Token(RIGHT_BRACE, "}")
        };
const int num_symbolic_tokens=sizeof(symbolic_tokens)/sizeof(symbolic_tokens[0]);

inline bool IsDigit(char ch){return (ch>='0' && ch<='9');}
inline bool IsLetter(char ch){return ((ch>='a' && ch<='z') || (ch>='A' && ch<='Z'));}
inline bool IsLetterOrUnderscore(char ch){return (IsLetter(ch) || ch=='_');}

struct item
{
    string lineNum;
    string actualString;
    string Type;
};

bool inSymboilcToken(string s){
    for (int i = 0; i < 13; ++i) {
        if(s == symbolic_tokens[i].str){
            return true;
        }
    }
    return false;
}
bool inReservedWords(string s){
    for (int i = 0; i < 8; ++i) {
        if(s == reserved_words[i].str){
            return true;
        }
    }
    return false;
}
int index_of_ReservedWord(string s) {
    for (int i = 0; i < 8; ++i) {
        if (s == reserved_words[i].str) {
            return i;
        }
    }
    return -1;
}
string returnTokenType(string s){
    for (int i = 0; i < 13; ++i) {
        if(s == symbolic_tokens[i].str){
            return TokenTypeStr[symbolic_tokens[i].type];
        }
    }
    return "Not Found";
}

bool isWhitespace(string s){
    for(int index = 0; index < s.length(); index++){
        if(!std::isspace(s[index]))
            return false;
    }
    return true;
}

int main() {
    InFile file("input.txt");
    output.open("output.txt");
    vector<item> res;
    int lineCnt = 0;
    string tmp ="";
    while (file.GetNewLine()== true){
         lineCnt++;
        string line = file.GetNextTokenStr();
        if(line.size()==0){lineCnt++;}
        for (int i = 0; i < line.size(); ++i) {
            tmp += line[i];
            item it;
             if (IsLetterOrUnderscore(line[i])) //
            {
                while(IsLetterOrUnderscore(line[i+1]))
                {   tmp+=line[i+1];
                    i++;
                }
                if(inReservedWords(tmp)){
                    it.lineNum = to_string(lineCnt);
                    it.actualString = tmp;
                    it.Type = TokenTypeStr[index_of_ReservedWord(tmp)];
                    res.push_back(it);
                    tmp="";
                }
                else {
                    it.lineNum = to_string(lineCnt);
                    it.actualString = tmp;
                    it.Type = "ID";
                    res.push_back(it);
                    tmp="";
                }
            }
             else if(isdigit(line[i])){
                 while(isdigit(line[i+1]))
                 {   tmp+=line[i+1];
                     i++;
                 }
                 it.lineNum = to_string(lineCnt);
                 it.actualString = tmp;
                 it.Type = "NUM";
                 res.push_back(it);
                 tmp="";
             }
            else if(inSymboilcToken(tmp)){
                it.lineNum = to_string(lineCnt);
                it.actualString = tmp;
                it.Type = returnTokenType(tmp);
                res.push_back(it);
                if(tmp=="{")
                {   string comment="";
                    i++;
                    int start=lineCnt;
                    while(line[i] !='}')
                    {   comment+=line[i];
                        i++;

                        if(i==line.size())
                        {
                            while (file.GetNewLine())
                            {line= file.GetNextTokenStr();
                             lineCnt++;
                             i=0;
                            break;
                            }

                        }
                    }
                    it.lineNum = to_string(start);
                    it.actualString =comment;
                    it.Type = "comment";
                    res.push_back(it);

                    it.lineNum = to_string(lineCnt);
                    it.actualString ='}';
                    it.Type = "RightBrace";
                    res.push_back(it);

                    tmp="";
                }
                 tmp="";
             }
               else if (isWhitespace(tmp)){
                tmp="";
            }

        }



     }

   
    for(int i=0 ; i<(int)res.size(); i++){
       output<<("[");
        output<<(res[i].lineNum);
        output<<("]");
        output<<(res[i].actualString);
        output<<(" (");
        output<<(res[i].Type);
        output<<(")");
        output<<("\n");


    }

    return 0;
}
