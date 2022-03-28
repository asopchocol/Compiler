#include<stdio.h>
#include<string.h>
#include<ctype.h>
#define Max_symbols 500
#define MaxRules 500

typedef struct ssym {
	int kind;
	int no;
	char str[10];
} sym;		//1이면 terminal 0이면 nonterminal Kind. no = number terminal과 nontermianl들은 각자의 고유번호를 가지고있음.

typedef struct orule {
	sym leftside;
	sym rightside[10];
	int rleng;
} onerule;
typedef struct twoints {
	int cr;
	int elm;
} element_stack;

int Compute_first_of_one_nonterminal(sym);
int Compute_follow_of_one_nonterminal(int);
int Compute_first_of_any_string(sym[], int[]);

int MaxTerminal;
int MaxNonterminal;
sym Nonterminals_list[Max_symbols];
sym Terminals_list[Max_symbols];

int totalNumberOfRules;
onerule rules[MaxRules];

int FirstTable[Max_symbols][Max_symbols];
int FollowTable[Max_symbols][Max_symbols];

void read_grammar(const char* fileName);
int lookUp_nonterminal(char* str);
int lookUp_terminal(char* str);
int Compute_first_of_any_string(sym alpha[], int first_result[]);
int Compute_first_of_one_nonterminal(sym X);
int Compute_follow_of_one_nonterminal(int idx_NT);

int main()
{
	int i, j;
	sym a_nonterminal = { 1, 0 };
	int first_result[Max_symbols];

	read_grammar("G_case1.txt");
	strcpy(Terminals_list[MaxTerminal].str, "Epsilon");

	//initialize First and Follow tables;
	for (i = 0; i < MaxNonterminal; i++) {
		for (j = 0; j < MaxNonterminal + 1; j++) {
			FirstTable[i][j] = 0;
			FollowTable[i][j] = 0;
		}
		FirstTable[i][MaxTerminal + 1] = 0;
	}

	//Compute first of all nonterminals
	for(i=0;i<MaxNonterminal;i++)
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

	printf("Program terminates.\n");
} //main.

void read_grammar(const char *fileName) {
	FILE *fp;
	char line[500];
	char symstr[10];
	char *ret;
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

int lookUp_nonterminal(char *str) {
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

	int first_result[Max_symbols];  // this is used for storing the processing result of symbol X.

	for (i = 0; i<MaxTerminal + 2; i++) first_result[i] = 0; // initialize.

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
	int first_result[Max_symbols]; // one row of First table.
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
