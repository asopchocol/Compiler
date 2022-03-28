#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define MaxSymbols 200
#define MaxRules 100
#define MaxNumberOfStates 200

#define URT 48     // 인식할 수 없는 토큰을 나타내는 토큰 번호. 최종 토큰 번호로 ?? 을 고쳐주어야 함.
#define EOF_TOK 48 // token index of EOF token

typedef struct tkt {
	int kind;
	char sub_kind[3];
	int sub_data;
	double rnum;
	char str[20];
} tokentype;
typedef struct ssym {
	int kind;
	int no;
	char str[10];
} sym;

typedef struct orule {
	sym leftside;
	sym rightside[10];
	int rleng;
} onerule;

typedef struct twoints {
	int cr;
	int elm;
} element_stack;

typedef char oneword[50];

// keyword 테이블
//token index of the first keyword "if" is 31. token index of other keywords are computed using this info.
char keywords[17][50] =
{ "if", "else", "while", "do", "for", "include", "define", "typedef", "struct", "int", "char", "float", "double", "void", "return", "case", "then" };
int total_keywords = 17;
int Idx_First_keyword = 31;
// symbol 테이블
typedef struct symtbl { oneword idstr; int attribute; } sym_ent; //심볼테이블의 한 엔트리 구조.
sym_ent symtbl[500];
int total_ids = 0; // 심볼테이블에서 다음에 넣을 엔트리 번호

typedef struct typeitemnode* ty_ptr_item_node;
typedef struct typeitemnode {
	int RuleNum;
	int DotNum;
	ty_ptr_item_node link;
} type_item;

// state node : Goto graph에서 사용할 state node 구조체
typedef struct statenode* state_node_ptr;
typedef struct statenode {
	int id;								//자신의 ID(번호)
	int item_cnt;						//보유하고 있는 Item 들의 수
	ty_ptr_item_node item_start;		//보유하고 있는 Item_list 의 시작주소
	state_node_ptr next;				//다음 state로의 arc
} state_node;

// Arc_node : Item list에서 사용할 구조체
typedef struct itemnode* Arc_node_ptr;
typedef struct itemnode {
	int from_state;						// 룰번호
	int to_state;						// 점위치
	sym gram_sym;						// 사용된 심볼
	Arc_node_ptr link;
} Arc_node;


// goto_set
typedef struct gotoset* goto_set_ptr;
typedef struct gotoset {
	Arc_node_ptr Arc_list;
	state_node_ptr State_Node_List;
} goto_set;

typedef struct cell_action_tbl {
	char Kind;		// s, r, a, e 중 한 글자
	int num;		// s 이면 스테이트 번호, r 이면 룰 번호를 나타냄.
} ty_Cell_Action_Table;

//파스트리 노드 정의. 노드는 문법심볼을 가짐. 그런데, 이 노드는 LR stack 에도 넣음.
//그런데 LR stack 에는 state 번호도 들어가야 함. 그래서 node 가 문법심볼과 state 를 넣는 2 가지 용도로 사용함.

typedef struct nodetype* Ty_Node_Ptr;
typedef struct nodetype {
	int state;			// -1이면 grammar symbol을 가지는 것이며, 0 이상이면 state(번호)를 가지는 것임(이 경우 이하 필드들은 사용 안함.
	sym nodesym;		// 단말 기호나 비단말 기호를 저장.
	int child_cnt;		// number of children
	Ty_Node_Ptr children[10];	// pointers to children
	int rule_no_used;			// 이 노드를 만드는데 사용된 룰번호.
} nodeT;

int MaxTerminal;
int MaxNonterminal;
sym Nonterminals_list[MaxSymbols];
sym Terminals_list[MaxSymbols];

int totalNumberOfRules; 
onerule rules[MaxRules];

int FirstTable[MaxSymbols][MaxSymbols];
int FollowTable[MaxSymbols][MaxSymbols];
goto_set_ptr States_And_Arcs = NULL;
int Number_Of_States = 0;
int NumberOfArcs = 0;
ty_Cell_Action_Table Action_Table[MaxNumberOfStates][MaxSymbols];
int Goto_Table[MaxNumberOfStates][MaxSymbols];
Ty_Node_Ptr Stack[1000];
int Top = -1;

Ty_Node_Ptr Root = NULL;
int bar[100];

FILE* fps;
FILE* ofp;

state_node_ptr Add_A_State_Node(state_node_ptr State_Node_List_Header, ty_ptr_item_node To_list);
void Add_An_Arc(Arc_node_ptr* Arc_List_Header, int from_num, int to_num, sym Symbol);
int CheckExistItem(ty_ptr_item_node cs_list, ty_ptr_item_node s_val);
void Clear_Action_Table(void);
void Clear_Goto_Table(void);
ty_ptr_item_node closure(ty_ptr_item_node IS);
int Compute_first_of_one_nonterminal(sym X);
int Compute_first_of_any_string(sym alpha[], int first_result[]);
int deleteTyPtrItemNode(ty_ptr_item_node item_list);
int findToStateNodeId(Arc_node_ptr arc_list, int from_id, sym symbal);
void fitemListPrint(ty_ptr_item_node IS, FILE* fpw);
ty_ptr_item_node getLastItem(ty_ptr_item_node cs_list);
sym get_next_token(FILE* fps);
ty_ptr_item_node GoTo(ty_ptr_item_node IS, sym sym_val);
void printGotoGraph(goto_set_ptr gsp);
void itemListPrint(ty_ptr_item_node IS);
tokentype lexan(FILE* fps);
int lookUp_nonterminal(char* str);
int lookUp_terminal(char* str);
void Make_Action_Table();
void Make_Goto_Table();
Arc_node_ptr makeArcNode(void);
void makeGotoGraph(ty_ptr_item_node IS_0);
state_node_ptr makeStateNode(void);
Ty_Node_Ptr Parsing(FILE* fps);
void print_Action_Table(void);
void print_Goto_Table(void);
void print_parse_tree(FILE* fout, Ty_Node_Ptr curr, int standard, int first, int child);
void push_state(Ty_Node_Ptr Stack[], int State);
void push_symbol(Ty_Node_Ptr Stack[], Ty_Node_Ptr NodeToPush);
Ty_Node_Ptr pop();
void read_grammar(char* fileName);
int is_same_two_itemlists(ty_ptr_item_node list1, ty_ptr_item_node list2);
int itemListCounter(ty_ptr_item_node IS);


/////////////////////lexan ///////////////////////////////////////////

int iswhitespace(char c) {
	if (c == ' ' || c == '\n' || c == '\t')
		return 1;
	else
		return 0;
}

int lookup_keyword_tbl(char* str) {
	int i;
	for (i = 0; i < total_keywords; i++)
		if (strcmp(keywords[i], str) == 0)
			return i;
	return -1;

}

int lookup_symtbl(char* str) {
	int i;
	for (i = 0; i < total_ids; i++)
		if (strcmp(symtbl[i].idstr, str) == 0)
			return i;
	return -1;
}
//*
//-------------------------------------------------------------
tokentype lexan(FILE *fps) {
	int state = 0;
	char c;
	char buf[50];
	int bp = 0; // bp is buffer pointer(다음 넣을 위치).
	int upper_n; // number 토큰에서 소숫점 위 부분 즉 정수 부분을 저장함.
	double fraction; // number 토큰에서 소숫점 아래 부분을 저장함.
	tokentype token;
	int idx, FCNT, sign, Enum;

	while (1) {
		switch (state) {
		case 0: // 초기 상태. 각 토큰의 첫 글자에 따라서 작업 수행 및 다음 상태 결정함.
			c = fgetc(fps);  // fgetc can be called even if fp is after the end of file.
							// calling it again still returns EOF(-1) w/o invoking error.
			if (iswhitespace(c)) state = 0;  // this is white space.
			else if (isalpha(c)) { buf[bp] = c; bp++; buf[bp] = '\0'; state = 28; }
			else if (isdigit(c)) { buf[bp] = c; bp++; buf[bp] = '\0'; upper_n = c - '0'; state = 1; }
			else if (c == '<') state = 2;
			else if (c == '>') state = 32;
			else if (c == '=') state = 35;
			else if (c == '!') state = 38;
			else if (c == '+') state = 3;
			else if (c == '-') state = 4;
			else if (c == '*') state = 52;
			else if (c == '/') state = 8;
			else if (c == '\\') state = 53;
			else if (c == '%') state = 54;
			else if (c == '.') state = 55;
			else if (c == ',') state = 56;
			else if (c == '(') state = 57;
			else if (c == ')') state = 58;
			else if (c == '{') state = 59;
			else if (c == '}') state = 60;
			else if (c == '[') state = 61;
			else if (c == ']') state = 62;
			else if (c == ':') state = 63;
			else if (c == ';') state = 64;
			else if (c == '"') state = 65;
			else if (c == '\'') state = 66;
			else if (c == '#') state = 67;
			else if (c == '|') state = 68;
			else if (c == '&') state = 5;
			else if (c == EOF) state = 71;
			else {
				token.kind = URT; // 인식할 수 없는 토큰임을 나타냄.
				return token;
			}
			break;
		case 1: // NUM 토큰의 소수점 위 숫자열을 받아 들이는 상태.
			c = fgetc(fps);
			if (isdigit(c)) { buf[bp] = c; bp++; buf[bp] = '\0'; upper_n = 10 * upper_n + c - '0'; state = 1; }
			else if (c == '.') { buf[bp] = c; bp++; buf[bp] = '\0'; fraction = 0; FCNT = 0; state = 9; } // 소수점을 나왔으므로 실수를 처리하는 상태로 감.
			else if (c == 'E') { buf[bp] = c; bp++; buf[bp] = '\0'; fraction = 0; state = 16; }  // E 가 있는 exponent 처리부로 감.
			else state = 14;
			break;
		case 2: // 글자 < 가 나온 후의 처리를 담당하는 상태.
			c = fgetc(fps);
			if (c == '=') state = 30;
			else state = 31;
			break;
		case 3: // 글자 + 가 나온 후의 처리를 담당하는 상태.
			c = fgetc(fps);
			if (c == '+') state = 47;
			else if (c == '=') state = 45;
			else state = 46;
			break;
		case 4: // 글자 - 가 나온 후의 처리를 담당하는 상태.
			c = fgetc(fps);
			if (c == '-') state = 48;
			else if (c == '=') state = 49;
			else if (c == '>') state = 51;
			else state = 50;
			break;
		case 5: // 글자 + 가 나온 후의 처리를 담당하는 상태.
			c = fgetc(fps);
			if (c == '&') state = 6;
			else state = 7;
			break;
		case 6:
			token.kind = 2; strcpy(token.sub_kind, "");		 strcpy(token.str, "&&");
			return token;
		case 7: // 토큰 & 를 리턴해주는 상태.
			ungetc(c, fps);
			token.kind = 13;
			return token;
		case 8:
			c = fgetc(fps);
			if (c == '/') state = 74;
			else if (c == '*') state = 75;
			else if (c == EOF) state = 73;
			else state = 72;
			break;
		case 9: // 실수의 소수점 이하를 받아 들이는 상태
			c = fgetc(fps);
			if (isdigit(c)) {
				buf[bp] = c; bp++; buf[bp] = '\0';
				FCNT++; fraction = fraction + (c - '0') / pow(10.0, FCNT); state = 23;
			}
			else if (c == 'E') { buf[bp] = c; bp++; buf[bp] = '\0'; state = 16; }
			else if (c == EOF)  state = 26;
			else if (iswhitespace(c)) state = 24;
			else state = 25;
			break;
		case 14: // 양의 정수 토큰을 리턴하는 상태.
			ungetc(c, fps);
			token.kind = 1; strcpy(token.sub_kind, "in"); // 양의 정수를 나타냄.
			token.sub_data = upper_n;
			strcpy(token.str, buf);
			return token;
		case 16:
			c = fgetc(fps);
			if (c == '+') { buf[bp] = c; bp++; buf[bp] = '\0'; sign = 1; state = 17; }
			else if (c == '-') { buf[bp] = c; bp++; buf[bp] = '\0'; sign = -1; state = 17; }
			else if (isdigit(c)) { buf[bp] = c; bp++; buf[bp] = '\0'; sign = 1; Enum = c - '0'; state = 18; }
			else  state = 25; // error! 		 
			break;
		case 17:
			c = fgetc(fps);
			if (isdigit(c)) { buf[bp] = c; bp++; buf[bp] = '\0'; Enum = c - '0'; state = 18; }
			else state = 25; // error!
			break;
		case 18:
			c = fgetc(fps);
			if (isdigit(c)) { buf[bp] = c; bp++;  buf[bp] = '\0'; Enum = Enum * 10 + c - '0'; state = 18; }
			else state = 19;
			break;
		case 19:
			ungetc(c, fps);
			token.kind = 1; strcpy(token.sub_kind, "do"); // 실수를 나타냄.
			token.rnum = (upper_n + fraction) * pow(10.0, sign * Enum);
			strcpy(token.str, buf);
			return token;
		case 20:
			strcpy(token.str, buf);
			idx = lookup_keyword_tbl(buf); // -1 if not exist.
			if (idx >= 0) { token.kind = Idx_First_keyword + idx; return token; }  // Note: first keyword has token index 30.
																	// reaches here if it is not a keyword.
			idx = lookup_symtbl(buf); // -1 if not exist.
			if (idx >= 0) { token.kind = 0; token.sub_data = idx; return token; }
			// reaches here if it is not in symbol table.
			strcpy(symtbl[total_ids].idstr, buf); total_ids++;
			token.kind = 0; // ID 토큰임을 나타냄.
			token.sub_data = total_ids - 1; // 이 ID 가 들어 있는 심볼테이블 엔트리 번호.
			return token;
		case 23:
			c = fgetc(fps);
			if (isdigit(c)) {
				buf[bp] = c; bp++; buf[bp] = '\0';
				FCNT++; fraction = fraction + (c - '0') / pow(10.0, FCNT); state = 23;
			}
			else if (c == 'E') { buf[bp] = c; bp++; buf[bp] = '\0'; state = 16; }
			else state = 24;
			break;
		case 24:
			ungetc(c, fps);
			token.kind = 1; strcpy(token.sub_kind, "do"); // 실수를 나타냄.
			token.rnum = upper_n + fraction;
			strcpy(token.str, buf);
			return token;
		case 25:
			token.kind = -1;
			return token;
		case 26:  // do not call ungetc.
			token.kind = 1; strcpy(token.sub_kind, "do"); // 실수를 나타냄.
			token.rnum = upper_n + fraction;
			strcpy(token.str, buf);
			return token;
		case 28:
			c = fgetc(fps);
			if (isalpha(c) || isdigit(c) || c == '_') { buf[bp] = c; bp++; buf[bp] = '\0'; state = 28; }
			else if (c == EOF)  state = 20;
			else	 state = 29;
			break;
		case 29: // id 나 keyword 
			ungetc(c, fps);
			strcpy(token.str, buf);
			idx = lookup_keyword_tbl(buf); // -1 if not exist.
			if (idx >= 0) { token.kind = Idx_First_keyword + idx; return token; }  // Note: first keyword has token index 30.
																	// reaches here if it is not a keyword.
			idx = lookup_symtbl(buf); // -1 if not exist.
			if (idx >= 0) { token.kind = 0; token.sub_data = idx; return token; }
			// reaches here if it is not in symbol table.
			strcpy(symtbl[total_ids].idstr, buf); total_ids++;
			token.kind = 0; // ID 토큰임을 나타냄.
			token.sub_data = total_ids - 1; // 이 ID 가 들어 있는 심볼테이블 엔트리 번호.
			return token;
		case 30:
			token.kind = 2; strcpy(token.sub_kind, "LE"); strcpy(token.str, "<=");
			return token;
		case 31:
			ungetc(c, fps);
			token.kind = 2; strcpy(token.sub_kind, "LT"); strcpy(token.str, "<");
			return token;
		case 32:
			c = fgetc(fps);
			if (c == '=') state = 33;
			else state = 34;
			break;
		case 33:
			token.kind = 2; strcpy(token.sub_kind, "GE");	strcpy(token.str, ">=");
			return token;
		case 34:
			ungetc(c, fps);
			token.kind = 2; strcpy(token.sub_kind, "GT"); strcpy(token.str, ">");
			return token;
		case 35: // 글자 = 가 나온 후의 처리를 담당하는 상태.
			c = fgetc(fps);
			if (c == '=') state = 36;
			else state = 37;
			break;
		case 36: // 토큰 == 에 대한 처리를 수행하는 상태.
			token.kind = 2; strcpy(token.sub_kind, "EQ");	strcpy(token.str, "==");
			return token;
		case 37: // 토큰 > 에 대한 처리를 수행하는 상태.
			ungetc(c, fps);
			token.kind = 8; strcpy(token.str, "=");
			return token;
		case 38:
			c = fgetc(fps);
			if (c == '=') state = 39;
			else state = 40;
			break;
		case 39:
			token.kind = 2; strcpy(token.sub_kind, "NE");	 strcpy(token.str, "!=");
			return token;
		case 40:
			ungetc(c, fps);
			token.kind = 10;  strcpy(token.str, "!"); // NOT		
			return token;
		case 45:
			token.kind = 16; 		 strcpy(token.str, "+=");
			return token;
		case 46:
			ungetc(c, fps);
			token.kind = 3;  strcpy(token.str, "+");
			return token;
		case 47:
			token.kind = 14;		 strcpy(token.str, "++");
			return token;
		case 48:
			token.kind = 15;		 strcpy(token.str, "--");
			return token;
		case 49:
			ungetc(c, fps);
			token.kind = 17;  strcpy(token.str, "-=");
			return token;
		case 50:
			ungetc(c, fps);
			token.kind = 4;		 strcpy(token.str, "-");
			return token;
		case 51:
			token.kind = 9;		 strcpy(token.str, "->");
			return token;
		case 52:
			token.kind = 5;		 strcpy(token.str, "*");
			return token;
		case 53:
			token.kind = 30;		 strcpy(token.str, "\\");
			return token;
		case 54:
			token.kind = 7;		 strcpy(token.str, "%");
			return token;
		case 55:
			token.kind = 11;		 strcpy(token.str, ".");
			return token;
		case 56:
			token.kind = 12;		 strcpy(token.str, ",");
			return token;
		case 57:
			token.kind = 18;		 strcpy(token.str, "(");
			return token;
		case 58:
			token.kind = 19;		 strcpy(token.str, ")");
			return token;
		case 59:
			token.kind = 20;		 strcpy(token.str, "{");
			return token;
		case 60:
			token.kind = 21;		 strcpy(token.str, "}");
			return token;
		case 61:
			token.kind = 22;		 strcpy(token.str, "[");
			return token;
		case 62:
			token.kind = 23;		 strcpy(token.str, "]");
			return token;
		case 63:
			token.kind = 24;		 strcpy(token.str, ":");
			return token;
		case 64:
			token.kind = 25;		 strcpy(token.str, ";");
			return token;
		case 65:
			token.kind = 26;		 strcpy(token.str, "\"");
			return token;
		case 66:
			token.kind = 27;		 strcpy(token.str, "'");
			return token;
		case 67:
			token.kind = 28;		 strcpy(token.str, "#");
			return token;
		case 68:
			c = fgetc(fps);
			if (c == '|') 	state = 69;
			else 	 state = 70;
			break;
		case 69:
			token.kind = 2;	strcpy(token.sub_kind, "OR");	 strcpy(token.str, "||");
			return token;
		case 70:
			ungetc(c, fps);
			token.kind = 29;	 strcpy(token.str, "|");
			return token;
		case 71:
			token.kind = EOF_TOK; strcpy(token.str, "EOF");
			return token;
		case 72:
			ungetc(c, fps);
			token.kind = 6; strcpy(token.str, "/");
			return token;
		case 73:
			token.kind = 6; strcpy(token.str, "/");
			return token;
		case 74:
			c = fgetc(fps);
			if (c == '\n') state = 0;
			else if (c == EOF) state = 71;
			else state = 74;
			break;
		case 75:
			c = fgetc(fps);
			if (c == '*') state = 76;
			else if (c == EOF) state = 71;
			else state = 75;
			break;
		case 76:
			c = fgetc(fps);
			if (c == '/') state = 0;
			else if (c == EOF) state = 71;
			else state = 75;
			break;
		default: printf("Unrecognizable token! Stop generating tokens.\n");
			token.kind = URT; strcpy(token.str, "URT");
			return token;
		} // switch
	} //while
}

//--------------------------------------
void print_token(tokentype a_tok, FILE * ofp) {
	fprintf(ofp, "%s           Token_idx: %d ", a_tok.str, a_tok.kind); // 토큰 종류 출력 (종류는 스트링으로 대체함)
	if (a_tok.kind == 1) { // this is number token.
		if (strcmp(a_tok.sub_kind, "in") == 0) fprintf(ofp, "  Val= %d", a_tok.sub_data); // 정수토큰임.
		else if (strcmp(a_tok.sub_kind, "do") == 0) fprintf(ofp, "  Val= %10.7f", a_tok.rnum); // 실수 토큰임.
	}
	else if (a_tok.kind == 0) fprintf(ofp, "  Symtbl_idx = %5d", a_tok.sub_data); // id 의 심볼테이블 엔트리.
	else;
	fprintf(ofp, "\n");
	fflush(ofp);
}

// get the next token from the source file.

sym get_next_token(FILE* fps) {
	sym token;
	tokentype a_tok;			// the token produced by the lexical analyer we developed before.
	a_tok = lexan(fps);
	token.kind = 0;				// this is a terminal symbol
	if (a_tok.kind == EOF_TOK) {
		token.no = MaxTerminal - 1;		// this is a number of terminal $.
		strcpy(token.str, Terminals_list[MaxTerminal - 1].str);
	}
	else {
		token.no = a_tok.kind;
		strcpy(token.str, a_tok.str);
	}
	return token;
} //get_next_token

ty_ptr_item_node closure(ty_ptr_item_node IS) {
	ty_ptr_item_node new_cs = NULL;
	ty_ptr_item_node curr = NULL;
	ty_ptr_item_node cursor = NULL;

	sym SymAfterDot;

	int r_num, d_num;
	int i_0 = 0;

	// 초기화
	r_num = d_num = 0;

	// 포인터 curr 가 IS 의 첫 노드를 가리키게 한다.
	curr = IS;

	// Core Routine

	while (curr) {
		// curr  노드의 rule 번호
		r_num = curr->RuleNum;
		d_num = curr->DotNum;

		// SymAfterDot : type.sym { int kind / int no }
		if (d_num >= rules[r_num].rleng) {
			curr = curr->link;
			continue;
		}
		else
			SymAfterDot = rules[r_num].rightside[d_num];

		// 단말기호이면 무시하고 넘어간다.
		if (!SymAfterDot.kind) {
			curr = curr->link;
			continue;
		}

		for (i_0 = 0; i_0 < totalNumberOfRules; i_0++) {
			// 룰 i_0 의 좌측 심볼이 SymAfterDot 이 아니면 continue
			if (rules[i_0].leftside.no != SymAfterDot.no)
				continue;

			// item node 하나를 만든다.
			new_cs = (ty_ptr_item_node)malloc(sizeof(type_item));

			// rule 번호 r, dot number = 0 을 여기에 넣는다.
			new_cs->RuleNum = i_0;
			new_cs->DotNum = 0;
			new_cs->link = NULL;

			// new_cs 가 이미 존재하면 continue
			if (CheckExistItem(IS, new_cs)) {
				free(new_cs);
				continue;
			} // if : CheckExistItem ()

			// NewItemNodePtr 를 IS 의 맨 뒤에 붙인다.
			cursor = getLastItem(IS);
			cursor->link = new_cs;

		} // for : i_0
		curr = curr->link;
	} // while : curr

	return IS;
} // closure ()

int CheckExistItem(ty_ptr_item_node cs_list, ty_ptr_item_node s_val) {
	ty_ptr_item_node cursor = cs_list;

	while (cursor) {
		if (cursor->RuleNum == s_val->RuleNum && cursor->DotNum == s_val->DotNum)
			return 1;
		cursor = cursor->link;
	} // while : cursor

	return 0;
} // CheckExistItem()

ty_ptr_item_node getLastItem(ty_ptr_item_node cs_list) {
	ty_ptr_item_node cursor = cs_list;

	while (cursor) {
		if (cursor->link == NULL)
			return cursor;
		cursor = cursor->link;
	}

	return cursor;
}

ty_ptr_item_node GoTo(ty_ptr_item_node IS, sym sym_val) {
	int r_num, d_num;
	sym SymAfterDot;
	ty_ptr_item_node cursor = NULL;
	ty_ptr_item_node New_Item = NULL;
	ty_ptr_item_node Go_To_Result_List = NULL;
	ty_ptr_item_node i_cursor = NULL;
	ty_ptr_item_node temp_item = NULL;

	cursor = IS;
	while (cursor) {
		r_num = cursor->RuleNum;
		d_num = cursor->DotNum;

		// SymAfterDot : type.sym { int kind / int no }
		if (d_num >= rules[r_num].rleng) {
			cursor = cursor->link;
			continue; // ignore this item since dot is at the end.
		}
		SymAfterDot = rules[r_num].rightside[d_num];

		// 점 다음의 심볼과 goto할 심볼이 다르면 다음 아이템으로 넘어 감.
		if (!(SymAfterDot.kind == sym_val.kind && SymAfterDot.no == sym_val.no)) {
			cursor = cursor->link;
			continue;
		}

		New_Item = (ty_ptr_item_node)malloc(sizeof(type_item));
		New_Item->RuleNum = r_num;
		New_Item->DotNum = d_num + 1;
		New_Item->link = NULL;

		// New_Item 가 이미 존재하면 이 아이템은 무시한다.
		if (CheckExistItem(Go_To_Result_List, New_Item)) {
			free(New_Item);
			cursor = cursor->link;
			continue;
		} // if : CheckExistItem()

		// NewItemNodePtr 를 Go_To_Result_List 의 맨 뒤에 붙인다.
		if (Go_To_Result_List == NULL) {
			Go_To_Result_List = New_Item;
		}
		else {
			temp_item = getLastItem(Go_To_Result_List); temp_item->link = New_Item;
		} // if

		cursor = cursor->link;
	} // while : cursor

	if (Go_To_Result_List)
		return closure(Go_To_Result_List);
	else
		return NULL;
} // GoTo ( )

// Make a goto graph. The processing result is set to the global variable having headers to state list and arc list
void makeGotoGraph(ty_ptr_item_node IS_0) {
	// 지역변수 선언
	int i_0, i_1, i_max;
	sym sym_temp;
	goto_set_ptr result = NULL;
	state_node_ptr State_Node_List_Header = NULL;		// 스테이트 리스트의 헤더
	state_node_ptr state_cursor = NULL;					// 현재 작업중인 스테이트 노드에 대한 포인터.
	state_node_ptr To_state_node = NULL;				// 스테이트 리스트의 맨 마지막 스테이트 노드에 대한 포인터.
	Arc_node_ptr Arc_List_Header = NULL;
	ty_ptr_item_node To_item_list = NULL;

	// 스테이트 0 를 빈 스테이트 리스트에 넣도록 함.
	State_Node_List_Header = makeStateNode();
	State_Node_List_Header->id = 0; State_Node_List_Header->item_cnt = itemListCounter(IS_0);
	State_Node_List_Header->item_start = IS_0; State_Node_List_Header->next = NULL;
	Number_Of_States = 1;
	state_cursor = State_Node_List_Header;

	while (state_cursor) {
		// 단말기호, 비단말기호
		for (i_0 = 0; i_0 < 2; i_0++) { // i_0 = 0 이면 단말기호에 대한 처리이고, 1이면 비단말기호에 대한 처리임.
			// i_max 는 각 경우 (단말 / 비단말) 가 가진 총 기호의 수를 가지게 함.
			i_max = i_0 ? MaxNonterminal : MaxTerminal - 1; // -1 을 한 이유는 $ 단말기호를 무시하기 위함.
			for (i_1 = 0; i_1 < i_max; i_1++) {	// i_1 은 단말 / 비단말 각 경우에서 기호의 번호를 나타냄.
				// 문법 심볼 하나를 만듬.
				sym_temp.kind = i_0;
				sym_temp.no = i_1;
				// 현재 스테이트에서 문법심볼 sym_temp 에 의해 goto 할 아이템 리스트를 알아 옴.
				To_item_list = GoTo(state_cursor->item_start, sym_temp);

				if (To_item_list) { // goto result is not empty
					// goto할 아이템 리스트가 아직 state 로 등록되어 있지 않으면 새 state 로 등록함.
					To_state_node = Add_A_State_Node(State_Node_List_Header, To_item_list);

					// 알아낸 아크에 대하여 이것이 이미 존재하는 아크가 아니면 새로운 아크로 추가함.
					Add_An_Arc(&Arc_List_Header, state_cursor->id, To_state_node->id, sym_temp);
				} // if : To_item_list
			} // for : i_1 : 모든 기호에 대하여 goto 를 수행
		} // for : i_0 : 단말기호 -> 비단말기호
		state_cursor = state_cursor->next;
	} // while : state_cursor
	// Set the global structure to record the result of this function
	States_And_Arcs = (goto_set_ptr)malloc(sizeof(goto_set));
	States_And_Arcs->State_Node_List = State_Node_List_Header;
	States_And_Arcs->Arc_list = Arc_List_Header;
} // : makeGotoGraph ( )

state_node_ptr Add_A_State_Node(state_node_ptr State_Node_List_Header, ty_ptr_item_node To_list) {
	int Number_Of_Items = 0; // 새로운 item list의 item 개수를 저장하는 변수
	state_node_ptr cursor = State_Node_List_Header;
	state_node_ptr last_cursor = NULL;
	state_node_ptr new_state_node = NULL;

	Number_Of_Items = itemListCounter(To_list);

	// state list 내의 값들과 비교하여 list 추가 여부를 결정합니다.
	while (cursor) {
		// item 개수가 서로 같은지 확인함.
		if (cursor->item_cnt != Number_Of_Items) {
			last_cursor = cursor;
			cursor = cursor->next;
			continue;
		} // if : state node의 item list에 보유하고 있는 item의 개수가 일치 하지 않으면 확인하지 않아도 괜찮습니다.

		int is_same = is_same_two_itemlists(cursor->item_start, To_list); // 1 if they are same.
		if (is_same) { // cursor 의 스테이트의 아이템리스트와 to_list 가 동일한 상황임.
			deleteTyPtrItemNode(To_list); //free all Items In To_list.
			return cursor; // 동일한 것으로 판명된 현 스테이트 노드의 포인터를 반환해 줌.
		}

		// 두 아이템 리스트가 다른 것으로 판명되었으므로 다음 스테이트 노드로 커서를 옮김.
		last_cursor = cursor;
		cursor = cursor->next;
	} // while

	new_state_node = makeStateNode();

	Number_Of_States++;

	new_state_node->item_cnt = Number_Of_Items;
	new_state_node->item_start = To_list;
	new_state_node->id = last_cursor->id + 1;
	new_state_node->next = NULL;
	last_cursor->next = new_state_node;

	return new_state_node;
} // Add_A_State_Node ( )

void Add_An_Arc(Arc_node_ptr* Arc_List_Header, int from_num, int to_num, sym Symbol) {
	Arc_node_ptr Arc_cursor, Arc_last = NULL;
	Arc_node_ptr Arc_new = NULL;
	int same_arc_exists = 0;

	Arc_new = makeArcNode();

	Arc_new->from_state = from_num;
	Arc_new->to_state = to_num;
	Arc_new->gram_sym = Symbol;
	Arc_new->link = NULL;

	if (*Arc_List_Header == NULL) {
		*Arc_List_Header = Arc_new;
		NumberOfArcs++;
		return;
	}

	Arc_cursor = *Arc_List_Header;
	while (Arc_cursor) {
		if (Arc_cursor->from_state == from_num && Arc_cursor->to_state == to_num
			&& Arc_cursor->gram_sym.kind == Symbol.kind && Arc_cursor->gram_sym.no == Symbol.no) {
			same_arc_exists = 1;
			break;
		}
		else {
			Arc_last = Arc_cursor;
			Arc_cursor = Arc_cursor->link;
		}
	} // while

	if (!same_arc_exists) {
		Arc_last->link = Arc_new;
		NumberOfArcs++;
	}
	else
		free(Arc_new);
} // Add_An_Arc ( )

int is_same_two_itemlists(ty_ptr_item_node list1, ty_ptr_item_node list2) {
	int l1, l2;
	l1 = itemListCounter(list1); l2 = itemListCounter(list2);
	ty_ptr_item_node p1 = list1, p2;
	if (l1 != l2)
		return 0;
	while (p1) {
		p2 = list2;
		int p1_exists_in_list2 = 0;

		while (p2) {
			if (p1->RuleNum == p2->RuleNum && p1->DotNum == p2->DotNum) {
				p1_exists_in_list2 = 1;
				break;
			}

			p2 = p2->link;
		} //while.

		if (!p1_exists_in_list2)
			return 0;
		p1 = p1->link;
	} // while.
	return 1;
} // is_same_two_itemlists

state_node_ptr makeStateNode(void) {
	state_node_ptr cursor;

	cursor = (state_node_ptr)malloc(sizeof(state_node));

	cursor->id = -1;
	cursor->item_cnt = 0;
	cursor->item_start = NULL;
	cursor->next = NULL;

	return cursor;
} // makeStateNode( )

Arc_node_ptr makeArcNode(void) {
	Arc_node_ptr cursor;

	cursor = (Arc_node_ptr)malloc(sizeof(Arc_node));

	cursor->from_state = -1;
	cursor->to_state = -1;
	cursor->gram_sym.kind = -1;
	cursor->gram_sym.no = -1;
	cursor->link = NULL;

	return cursor;
} // makeArcNode ( )

int itemListCounter(ty_ptr_item_node IS) {
	int cnt = 0;
	ty_ptr_item_node cursor = IS;

	while (cursor) {
		cnt++;
		cursor = cursor->link;
	} // while : cursor

	return cnt;
} // itemListCounter ( )

int deleteTyPtrItemNode(ty_ptr_item_node item_list) {
	ty_ptr_item_node item_next = NULL;
	ty_ptr_item_node item_cursor = item_list->link;

	while (item_cursor) {
		item_next = item_cursor->link;
		free(item_cursor);
		item_cursor = item_next;
	} // while : item_cursor
	free(item_list);
	return 0;
} // deleteTyPtrItemNode ( )

void printGotoGraph(goto_set_ptr gsp) {
	state_node_ptr State_Node_List_Header = gsp->State_Node_List;
	Arc_node_ptr item_list = gsp->Arc_list;
	char str[10];
	FILE* fpw;
	fpw = fopen("goto_graph.txt", "w");

	while (State_Node_List_Header) {
		fprintf(fpw, "ID[%2d] (%3d) : ", State_Node_List_Header->id, State_Node_List_Header->item_cnt);
		fitemListPrint(State_Node_List_Header->item_start, fpw);
		State_Node_List_Header = State_Node_List_Header->next;
	} // while : State_Node_List_Header
	printf("\nTotal number of states = %d\n", Number_Of_States);
	fprintf(fpw, "Total number of states = %d\n", Number_Of_States);

	fprintf(fpw, "\nGoto arcs:\nFrom To Symbol\n");
	while (item_list) {
		fprintf(fpw, "%4d %4d     ", item_list->from_state, item_list->to_state);
		if (item_list->gram_sym.kind == 0)
			strcpy(str, Terminals_list[item_list->gram_sym.no].str);
		else
			strcpy(str, Nonterminals_list[item_list->gram_sym.no].str);
		fprintf(fpw, "%s \n", str);

		item_list = item_list->link;
	}  // while : item_list
	printf("Total number of arcs = %d\n", NumberOfArcs);
	fprintf(fpw, "Total number of arcs = %d\n", NumberOfArcs);
	fclose(fpw);
} // printGoToGraph

int findToStateNodeId(Arc_node_ptr arc_list, int from_id, sym symbal) {
	Arc_node_ptr cursor = arc_list;

	while (cursor) {
		if (cursor->from_state == from_id && cursor->gram_sym.kind == symbal.kind && cursor->gram_sym.no == symbal.no)
			return cursor->to_state;
		cursor = cursor->link;
	} // while : cursor

	return -1;
} // findToStateNodeId ( )

void fitemListPrint(ty_ptr_item_node IS, FILE* fpw) {
	int i_0;
	int r_num, d_num;

	ty_ptr_item_node cursor = IS;

	while (cursor) {
		r_num = cursor->RuleNum;
		d_num = cursor->DotNum;

		fprintf(fpw, "%s -> ", Nonterminals_list[rules[r_num].leftside.no].str);
		for (i_0 = 0; i_0 < rules[r_num].rleng; i_0++) {
			if (i_0 == d_num)
				fprintf(fpw, ". ");
			fprintf(fpw, "%s ", rules[r_num].rightside[i_0].kind ? Nonterminals_list[rules[r_num].rightside[i_0].no].str :
				Terminals_list[rules[r_num].rightside[i_0].no].str);
		} // for : i_0
		if (i_0 == d_num)
			fprintf(fpw, ".");
		if (!rules[r_num].rleng)
			fprintf(fpw, "e");
		fprintf(fpw, "     ");
		cursor = cursor->link;
	} // while : cursor
	fprintf(fpw, "\n");
} // itemListPrint( )

void Make_Action_Table() {
	int to_state_id = -1;
	int i_0 = 0;
	state_node_ptr state_cursor = States_And_Arcs->State_Node_List;
	ty_ptr_item_node item_cursor = NULL;
	sym symbol;

	//table 초기화
	Clear_Action_Table();
	while (state_cursor) {
		item_cursor = state_cursor->item_start;

		while (item_cursor) {
			symbol = rules[item_cursor->RuleNum].rightside[item_cursor->DotNum];
			if (rules[item_cursor->RuleNum].rleng > item_cursor->DotNum) {
				if (!symbol.kind) {
					to_state_id = findToStateNodeId(States_And_Arcs->Arc_list, state_cursor->id, symbol);

					if (to_state_id == -1) {	// to 스테이트가 없다. (이 경우는 실제론 일어날 수 없다.)
						printf("Logic error at Make_Action ( 1 ) \n");
						getchar();
					}

					if (Action_Table[state_cursor->id][symbol.no].Kind == 'e') {
						Action_Table[state_cursor->id][symbol.no].Kind = 's';
						Action_Table[state_cursor->id][symbol.no].num = to_state_id;
					}
					else {	// 무언가 이미 들어가 있음.
						if (Action_Table[state_cursor->id][symbol.no].Kind == 's'
							&& Action_Table[state_cursor->id][symbol.no].num == to_state_id) {
							// 이 경우는 다시 넣지 않고 그냥 넘어 감. 에러가 아님
						}
						else {	// 넣을 셀이 다른 내용이 이미 있음. "LR0 이 아니다"라는 에러임.
							printf("Make_Action_Table의 다중값 상황발생1\n");
							getchar();
						}
					}
				} // if (!symbol.kind) . . .
			}
			else { // dot 가 가장 마지막에 위치
				if (item_cursor->RuleNum == 0) {	// Augmented rule (S' -> S) 인 경우
					if (Action_Table[state_cursor->id][MaxTerminal - 1].Kind == 'e') {	// 비어 있으면 accept 액션 넣어줌
						Action_Table[state_cursor->id][MaxTerminal - 1].Kind = 'a'; // Action[state][$] 에 'a' (accept)를 넣음.
					}
					else {
						printf("Make_Action_Table의 다중값 상황발생2\n");
						getchar();
					}
				}
				else {
					for (i_0 = 0; i_0 < MaxTerminal; i_0++) {
						if (FollowTable[rules[item_cursor->RuleNum].leftside.no][i_0]) {
							if (Action_Table[state_cursor->id][i_0].Kind == 'e') {
								Action_Table[state_cursor->id][i_0].Kind = 'r';
								Action_Table[state_cursor->id][i_0].num = item_cursor->RuleNum;
							}
							else {
								if (Action_Table[state_cursor->id][i_0].Kind == 'r'
									&& Action_Table[state_cursor->id][i_0].num == item_cursor->RuleNum) {
									//넣을 셀에 동일한 내용이 있으므로 에러는 아니고 그냥 무시하고 지나감.
								}
								else {
									printf("Make_Action_Table의 다중값 상황발생3\n");
									getchar();
								}
							}
						} // if :FollwTable[rules[item_cursor->RuleNum].leftside.no][i_0]
					} // for : i_0
				} // else.
			} // else ( rules[item_cursor->RuleNum].rleng == ...

			item_cursor = item_cursor->link;
		} // while : item_cursor

		//다음 state_node 로 이동합니다.
		state_cursor = state_cursor->next;
	} // while : state_cursor
} // Make_Action_Table ( )

void print_Action_Table(void) {
	int i_0, i_1;
	FILE* file_ptr = NULL;

	file_ptr = fopen("action_table.txt", "w");

	fprintf(file_ptr, "  \t");
	for (i_0 = 0; i_0 < MaxTerminal; i_0++)
		fprintf(file_ptr, "%2s\t", Terminals_list[i_0].str);
	fprintf(file_ptr, "\n");

	for (i_0 = 0; i_0 < Number_Of_States; i_0++) {
		fprintf(file_ptr, "%3d\t", i_0);
		for (i_1 = 0; i_1 < MaxTerminal; i_1++) {
			fprintf(file_ptr, "%c", Action_Table[i_0][i_1].Kind);
			if (Action_Table[i_0][i_1].Kind == 's' || Action_Table[i_0][i_1].Kind == 'r')
				fprintf(file_ptr, "%2d\t", Action_Table[i_0][i_1].num);
			else
				fprintf(file_ptr, "\t");
		} // for : i_1
		fprintf(file_ptr, "\n");
	} // for : i_0

	fclose(file_ptr);
} // printf_Action_Table ( )

void Clear_Action_Table(void) {
	int i_0, i_1;

	for (i_0 = 0; i_0 < Number_Of_States; i_0++) {
		for (i_1 = 0; i_1 < MaxTerminal; i_1++) {
			Action_Table[i_0][i_1].Kind = 'e';
			Action_Table[i_0][i_1].num = 0;
		} // for : i_1
	} // for : i_0
} // Clear_Action_Table ( )

void Make_Goto_Table() {
	int to_state_id = -1;
	int i_0 = 0;
	state_node_ptr state_cursor = States_And_Arcs->State_Node_List;
	ty_ptr_item_node item_cursor = NULL;
	sym symbol;

	Clear_Goto_Table();
	while (state_cursor) {
		item_cursor = state_cursor->item_start;
		while (item_cursor) {
			symbol = rules[item_cursor->RuleNum].rightside[item_cursor->DotNum];

			if (rules[item_cursor->RuleNum].rleng > item_cursor->DotNum) {
				if (symbol.kind) {
					to_state_id = findToStateNodeId(States_And_Arcs->Arc_list, state_cursor->id, symbol);
					if (to_state_id == -1) {
						item_cursor = item_cursor->link;
						continue;
					} // if : to_state_id

					Goto_Table[state_cursor->id][symbol.no] = to_state_id;
				} // if : symbol.kind
			} // if : dot number

			item_cursor = item_cursor->link;
		} // while : item_cursor

		// 다음 state_node 로 이동합니다.
		state_cursor = state_cursor->next;
	} // while: state_cursor
} // Make_Goto_Table ( )

void Clear_Goto_Table(void) {
	int i_0, i_1;

	for (i_0 = 0; i_0 < MaxNumberOfStates; i_0++) {
		for (i_1 = 0; i_1 < MaxNonterminal; i_1++) {
			Goto_Table[i_0][i_1] = -1;
		} // for : i_1
	} // for : i_0
} // Clear_Goto_Table ( )

void print_Goto_Table(void) {
	int i_0, i_1;
	FILE* file_ptr = NULL;

	file_ptr = fopen("goto_table.txt", "w");

	fprintf(file_ptr, "   \t");
	for (i_0 = 0; i_0 < MaxNonterminal; i_0++) 
		fprintf(file_ptr, "%3s\t", Nonterminals_list[i_0].str);
	fprintf(file_ptr, "\n");

	for (i_0 = 0; i_0 < Number_Of_States; i_0++) {
		fprintf(file_ptr, "%3d\t", i_0);
		for (i_1 = 0; i_1 < MaxNonterminal; i_1++) {
			if (Goto_Table[i_0][i_1] != -1)
				fprintf(file_ptr, "%3d\t", Goto_Table[i_0][i_1]);
			else
				fprintf(file_ptr, " -1\t");
		} // for : i_1
		fprintf(file_ptr, "\n");
	} // for : i_0

	fclose(file_ptr);
} // print_Goto_Table ( )

void push_state(Ty_Node_Ptr Stack[], int State) {
	Ty_Node_Ptr NewNode;

	NewNode = (Ty_Node_Ptr)malloc(sizeof(struct nodetype));
	NewNode->state = State;
	Top++;
	Stack[Top] = NewNode;
}

void push_symbol(Ty_Node_Ptr Stack[], Ty_Node_Ptr NodeToPush) {
	Top++;
	Stack[Top] = NodeToPush;
}

Ty_Node_Ptr pop() {
	Ty_Node_Ptr rptr;
	if (Top < 0) {
		printf("Pop error. Empty stack.\n");
		getchar();
	}

	rptr = Stack[Top];
	Top--;
	return rptr;
} // pop

Ty_Node_Ptr Parsing(FILE* fps) {
	int i, kind, TempState, Finished = 0, State, RuleNo, RuleLeng;
	kind = 0;
	sym NextToken;
	Ty_Node_Ptr Root, NewNode, TempNode;

	push_state(Stack, 0);
	NextToken = get_next_token(fps);
	do {
		State = Stack[Top]->state;
		switch (Action_Table[State][NextToken.no].Kind) {
			case 's':
				NewNode = (Ty_Node_Ptr)malloc(sizeof(struct nodetype));
				NewNode->state = -1;
				NewNode->nodesym = NextToken;
				NewNode->child_cnt = 0; NewNode->children[0] = NULL;
				push_symbol(Stack, NewNode);
				TempState = Action_Table[State][NextToken.no].num;
				push_state(Stack, TempState);
				NextToken = get_next_token(fps);
				break;
			case 'r':
				NewNode = (Ty_Node_Ptr)malloc(sizeof(struct nodetype));
				NewNode->state = -1;
				RuleNo = Action_Table[State][NextToken.no].num;
				NewNode->nodesym = rules[RuleNo].leftside;
				RuleLeng = rules[RuleNo].rleng;
				for (i = 0; i < RuleLeng; i++) {
					TempNode = pop();
					TempNode = pop();
					NewNode->children[RuleLeng - 1 - i] = TempNode;
				}
				NewNode->child_cnt = RuleLeng;
				NewNode->rule_no_used = RuleNo;
				State = Stack[Top]->state;
				TempState = Goto_Table[State][NewNode->nodesym.no];
				push_symbol(Stack, NewNode);
				push_state(Stack, TempState);
				break;
			case 'a':
				Root = Stack[1];
				return Root; break;
			case 'e':
				printf("Error : Parser is attempting to access an error cell.\n"); getchar();
		} // switch
	} while (1);
} // Parsing

void print_parse_tree(FILE* fout, Ty_Node_Ptr curr, int standard, int first, int child) {
	int i, j, n = 0;
	if (first != 0) {
		for (i = 0; i < standard - 2; i++) {
			if (i % 8 == 5) {
				n++;
				if (bar[n])
					fprintf(fout, "|");
				else
					fprintf(fout, " ");
			}
			else
				fprintf(fout, " ");
		}
		
		if (standard != 0)
			fprintf(fout, "--");
	}

	fprintf(fout, "%3s", curr->nodesym.str);
	if (first == child)
		bar[standard / 8] = 0;
	if (curr->nodesym.kind == 0)
		fprintf(fout, "\n");
	else
		fprintf(fout, "-----");

	for (j = 0; j < curr->child_cnt; j++) {
		bar[standard / 8 + 1] = 1;
		print_parse_tree(fout, curr->children[j], standard + 8, j, curr->child_cnt - 1);
	}
	return;
} // void print_parse_tree

void read_grammar(const char* fileName) {
	FILE* fp;
	char line[500];
	char symstr[10];
	char* ret;
	int i, k, n_sym, n_rule, i_leftSymbol, i_rightSymbol, i_right, symkind;
	fp = fopen(fileName, "r");
	if (!fp) {
		printf("File open error of grammar file.\n");
		getchar();
	}

	ret = fgets(line, 500, fp);	//ignore line 1
	ret = fgets(line, 500, fp);	//ignore line 2
	ret = fgets(line, 500, fp);	//read nonterminals line.
	i = 0; n_sym = 0;
	do {
		while (line[i] == ' ' || line[i] == '\t') i++;	//skip spaces.
		if (line[i] == '\n') break;
		k = 0;
		while (line[i] != ' ' && line[i] != '\t' && line[i] != '\n') {
			symstr[k] = line[i]; i++; k++;
		}
		symstr[k] = '\0'; // a nonterminal string is finished.
		strcpy(Nonterminals_list[n_sym].str, symstr);
		Nonterminals_list[n_sym].kind = 1;
		Nonterminals_list[n_sym].no = n_sym;
		n_sym++;
	} while (1);
	MaxNonterminal = n_sym;

	i = 0; n_sym = 0;
	ret = fgets(line, 500, fp); // read terminals line.
	do {
		while (line[i] == ' ' || line[i] == '\t') i++; // skip spaces.
		if (line[i] == '\n') break;
		k = 0;
		while (line[i] != ' ' && line[i] != '\t' && line[i] != '\n') {
			symstr[k] = line[i]; i++; k++;
		}
		symstr[k] = '\0';
		strcpy(Terminals_list[n_sym].str, symstr);
		Terminals_list[n_sym].kind = 0;
		Terminals_list[n_sym].no = n_sym;
		n_sym++;
	} while (1);
	MaxTerminal = n_sym;

	ret = fgets(line, 500, fp); // ignore one line.
	n_rule = 0;
	do {
		ret = fgets(line, 500, fp);
		if (!ret)
			break; // no characters were read. So reading rules is terminated.

		//if the line inputed has only white spaces. we should skip this line.
		//this is determined as follows: length==0; or first char is not an alphabet
		i = 0;
		if (strlen(line) < 1)
			continue;
		else {
			while (line[i] == ' ' || line[i] == '\t') i++;
			if (!isalpha(line[i]))
				continue;
		}

		//take off left symbol of a rule.
		while (line[i] == ' ' || line[i] == '\t') i++; // skip spaces.
		k = 0;
		while (line[i] != ' ' && line[i] != '\t' && line[i] != '\n') {
			symstr[k] = line[i]; i++; k++;
		}
		symstr[k] = '\0'; // a nonterminal string is finished.
		i_leftSymbol = lookUp_nonterminal(symstr);
		if (i_leftSymbol < 0) {
			printf("Wrong left symbol of a rule.\n");
			getchar();
		}

		//left symbol is stored of the rule.
		rules[n_rule].leftside.kind = 1; rules[n_rule].leftside.no = i_leftSymbol;
		strcpy(rules[n_rule].leftside.str, symstr);

		//By three lines below, we move to first char of first sym of RHS.
		while (line[i] != '>') i++;
		i++;
		while (line[i] == ' ' || line[i] == '\t') i++;

		//Collect the symbols of the RHS of the rule.
		i_right = 0;
		do { //reading symbols of RHS
			k = 0;
			while (i < strlen(line) && (line[i] != ' ' && line[i] != '\t' && line[i] != '\n'))
			{
				symstr[k] = line[i]; i++; k++;
			}
			symstr[k] = '\0';
			if (strcmp(symstr, "epsilon") == 0) { // this is epsilon rule.
				rules[n_rule].rleng = 0;	// declare that this rule is an epsilon rule.
				break;
			}

			if (isupper(symstr[0])) {	//this is nonterminal.
				symkind = 1;
				i_rightSymbol = lookUp_nonterminal(symstr);
			}
			else {	//this is terminal
				symkind = 0;
				i_rightSymbol = lookUp_terminal(symstr);
			}

			if (i_rightSymbol < -1) {
				printf("Wrong right symbol of a rule.\n");
				getchar();
			}

			rules[n_rule].rightside[i_right].kind = symkind;
			rules[n_rule].rightside[i_right].no = i_rightSymbol;
			strcpy(rules[n_rule].rightside[i_right].str, symstr);

			i_right++;

			while (line[i] == ' ' || line[i] == '\t') i++;
			if (i >= strlen(line) || line[i] == '\n') // i>=strlen(line) is needed in case of eof
				break;	// finish reading rightand symbols.
		} while (1); // loop of reading symbols of RHS.

		rules[n_rule].rleng = i_right;
		n_rule++;
	} while (1); // loop of reading rules.

	totalNumberOfRules = n_rule;
	printf("Total number of rules = %d\n", totalNumberOfRules);
}	//read grammar.

int lookUp_nonterminal(char* str) {
	int i;
	for (i = 0; i < MaxNonterminal; i++)
		if (strcmp(str, Nonterminals_list[i].str) == 0)
			return i;
	return -1;
}

int lookUp_terminal(char* str) {
	int i;
	for (i = 0; i < MaxTerminal; i++)
		if (strcmp(str, Terminals_list[i].str) == 0)
			return i;
	return -1;
}

int Compute_first_of_any_string(sym alpha[], int first_result[]) {
	int i, k;
	for (i = 0; i < MaxTerminal + 2; i++)
		first_result[i] = 0; // initialize the first result of alpha

							 // Let alpha be Y0 Y1 ... Yn-1
	i = 0;
	do {
		if (alpha[i].kind == 0) {  // Yi is terminal
			first_result[alpha[i].no] = 1;
			break;
		}
		else if (alpha[i].kind == 1) { // Yi is nonterminal
			for (k = 0; k < MaxTerminal; k++)	 // copy first of Yi to first of alpha
				if (FirstTable[alpha[i].no][k] == 1) first_result[k] = 1;
			if (FirstTable[alpha[i].no][MaxTerminal] == 0)
				break; // first of Yi does not have epsilon.	
			else
				i = i + 1; // move to next Yi.
		}
		else if (alpha[i].kind == -1) {  // end of string mark.
			first_result[MaxTerminal] = 1; // if control reach here, all Yi's have epsilon in its first. Thus alpha has epsilon as first.
			break;
		}
	} while (1);
	return 1; // the first of alpha is passed thru parameter first_result.
} // end of function Compute_first_of_any_string

int Compute_first_of_one_nonterminal(sym X) {
	int i, j, k, CR = -1;
	element_stack es;
	element_stack Stack[100];
	int top = -1;

	int first_result[MaxSymbols];  // this is used for storing the processing result of symbol X.

	for (i = 0; i < MaxTerminal + 2; i++) first_result[i] = 0; // initialize.

Next_Rule:
	CR++;
	if (CR >= totalNumberOfRules) goto Process_Stop_Points;  // all X-rules were tried.
	if (!(rules[CR].leftside.kind == 1 && rules[CR].leftside.no == X.no)) // find a X-rule.
		goto Next_Rule;
	if (rules[CR].rleng == 0) { // this is epsilon rule
		first_result[MaxTerminal] = 1;  // add epsilon to the first of X.
		goto Next_Rule;
	}
	i = 0;

Next_Element:
	if (!(rules[CR].rightside[i].kind == 1 && rules[CR].rightside[i].no == X.no)) { // Yi differs from X
		if (rules[CR].rightside[i].kind == 0) {  // Yi is terminal
			first_result[rules[CR].rightside[i].no] = 1; // add Yi as first of X
			goto Next_Rule;
		}
		else {  // Yi is nonterminal
			if (FirstTable[rules[CR].rightside[i].no][MaxTerminal + 1] != 1) // first compute for Yi was not done.
				Compute_first_of_one_nonterminal(rules[CR].rightside[i]); // first computation for Yi is executed.
			for (k = 0; k < MaxTerminal; k++) // copy except epsilon.
				if (FirstTable[rules[CR].rightside[i].no][k] == 1)
					first_result[k] = 1;
			if (FirstTable[rules[CR].rightside[i].no][MaxTerminal] == 0) // epsilon is not in first of Yi?
				goto Next_Rule;
			if (i == rules[CR].rleng - 1) { // Yi is the last element of RHS.
				first_result[MaxTerminal] = 1;  // add epsilon to first of X.
				goto Next_Rule;
			}
			i++; goto Next_Element; // Yi was not the last one. So move to the next element.
		}
	}
	else { // Yi is X. So case 1 has occurred.
		es.cr = CR; es.elm = i;
		top++; Stack[top] = es;    //  push ( es ) ; // store the stopping point to the stack.
		goto Next_Rule;
	}

Process_Stop_Points:
	if (top < 0) { // Stack is empty. So, save the first result of X and terminate the function.
		for (k = 0; k < MaxTerminal + 1; k++)
			if (first_result[k] == 1)
				FirstTable[X.no][k] = 1;
		FirstTable[X.no][MaxTerminal + 1] = 1;  // place a done mark for this nonterminal.
		return 1;
	}

	CR = Stack[top].cr; j = Stack[top].elm; top--; // pop stack.

Process_X:
	if (first_result[MaxTerminal] == 0) // no epsilon in first of X yet.
		goto Process_Stop_Points;

Next_Sym:
	if (j == rules[CR].rleng - 1) { first_result[MaxTerminal] = 1; goto Process_Stop_Points; }
	j++;
	if (rules[CR].rightside[j].kind == 1 && rules[CR].rightside[j].no == X.no) goto Process_X;

	// Yj is not X.
	if (rules[CR].rightside[j].kind == 0) {  // Yj is terminal
		first_result[rules[CR].rightside[j].no] = 1; // add Yj as first of X
		goto Process_Stop_Points;
	}
	else {  // Yj is nonterminal
		if (FirstTable[rules[CR].rightside[j].no][MaxTerminal + 1] != 1) // first compute for Yj was not done.
			Compute_first_of_one_nonterminal(rules[CR].rightside[j]); // first computation for Yi is executed.
		for (k = 0; k < MaxTerminal; k++) // copy except epsilon.
			if (FirstTable[rules[CR].rightside[j].no][k] == 1)
				first_result[k] = 1;
		if (FirstTable[rules[CR].rightside[j].no][MaxTerminal] == 1) // epsilon is in first of Yj?
			goto Next_Sym;
		else
			goto Process_Stop_Points;
	}
} // end of function First_Compute_One_Symbol

  // This function computes the follow of a nonterminal with index idx_NT.
int Compute_follow_of_one_nonterminal(int idx_NT) {
	int i, j, k, m;
	int first_result[MaxSymbols]; // one row of First table.
	sym SymString[10];

	for (i = 0; i < totalNumberOfRules; i++) {
		for (j = 0; j < rules[i].rleng; j++)
		{    //  the symbol of index j of the RHS of rule i is to be processed in this iteration
			if (rules[i].rightside[j].kind == 0 || rules[i].rightside[j].no != idx_NT) continue; // skip this symbol j.
																								 // Now, position j has a nonterminal which is equal to idx_NT.
			if (j < rules[i].rleng - 1) {  // there are symbols after position j in RHS of rule i.
				m = 0;
				for (k = j + 1; k < rules[i].rleng; k++, m++) SymString[m] = rules[i].rightside[k];
				SymString[m].kind = -1;  // end of string marker.
				Compute_first_of_any_string(SymString, first_result); // Compute the first of the string after position j of rule i.
				for (k = 0; k < MaxTerminal; k++) // Copy the first symbols of the remaining string to the Follow of idx_NT.
					if (first_result[k] == 1) FollowTable[idx_NT][k] = 1;
			}

			if (j == rules[i].rleng - 1 || first_result[MaxTerminal] == 1) // j is last symbol or first result has epsilon
			{
				if (rules[i].leftside.no == idx_NT) continue; // No need of adding the follow of the left side symbol
				if (FollowTable[rules[i].leftside.no][MaxTerminal] == 0) // We need follow of the left side sym of rule i.
					Compute_follow_of_one_nonterminal(rules[i].leftside.no);
				for (k = 0; k < MaxTerminal; k++) // add follow of left side symbol to follow of idx_NT.
					if (FollowTable[rules[i].leftside.no][k] == 1) FollowTable[idx_NT][k] = 1;
			}
		} // end of for j=0.
	} // end of for i
	FollowTable[idx_NT][MaxTerminal] = 1;  // put the completion mark for this nonterminal.
	return 1;
} // end of function Compute_follow_of_one_nonterminal.


int main() {
	int i, j;
	sym a_nonterminal = { 1, 0 };

	read_grammar("Grammar_data.txt");
	strcpy(Terminals_list[MaxTerminal].str, "Epsilon");

	//initialize First and Follow tables.
	for (i = 0; i < MaxNonterminal; i++) {
		for (j = 0; j < MaxNonterminal + 1; j++) {
			FirstTable[i][j] = 0;
			FollowTable[i][j] = 0;
		}
		FirstTable[i][MaxTerminal + 1] = 0;
	}

	//Compute first of all nonterminals
	for (i = 0; i < MaxNonterminal; i++)
		if (FirstTable[i][MaxTerminal + 1] == 0) {
			a_nonterminal.no = i;
			Compute_first_of_one_nonterminal(a_nonterminal);
		}

	//Compute follow of all nonterminals;
	FollowTable[0][MaxTerminal - 1] = 1;
	for (i = 0; i < MaxNonterminal; i++) {
		if (FollowTable[i][MaxTerminal] == 0)
			Compute_follow_of_one_nonterminal(i);
	}

	//Print first of all nonterminals.
	for (i = 0; i < MaxNonterminal; i++) {
		printf("First(%s): ", Nonterminals_list[i].str);
		for (j = 0; j < MaxTerminal; j++) {
			if (FirstTable[i][j] == 1)
				printf("%s ", Terminals_list[j].str);
		}
		if (FirstTable[i][MaxTerminal] == 1)
			printf(" epsilon");
		printf("\n");
	}

	//Print follow of all nonterminals.
	printf("\n");
	for (i = 0; i < MaxNonterminal; i++) {
		printf("Follow(%s): ", Nonterminals_list[i].str);
		for (j = 0; j < MaxTerminal; j++) {
			if (FollowTable[i][j] == 1)
				printf("%s ", Terminals_list[j].str);
		}
		printf("\n");
	}

	ty_ptr_item_node ItemSet0, tptr;

	tptr = (ty_ptr_item_node)malloc(sizeof(type_item));
	tptr->RuleNum = 0;
	tptr->DotNum = 0;
	tptr->link = NULL;

	ItemSet0 = closure(tptr);

	makeGotoGraph(ItemSet0);
	printGotoGraph(States_And_Arcs);
	Make_Action_Table();
	print_Action_Table();
	Make_Goto_Table();
	print_Goto_Table();

	fps = fopen("source.txt", "r");
	Root = Parsing(fps);
	fclose(fps);

	FILE* fpo;
	fpo = fopen("output.txt", "w");
	print_parse_tree(fpo, Root, 0, 0, 0);
	fclose(fpo);

	printf("Program terminates.\n");
}