%include {
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "perplex.h"
#include "token_type.h"

static void
writeHeader(FILE *templateFile, FILE *headerFile)
{
    int c;
    while ((c = fgetc(templateFile)) != EOF) {
	if (c == '%') {
	    if ((c = fgetc(templateFile)) == '%') {
		/* found %%, end of header section */
		return;
	    } else {
		fprintf(headerFile, "%c", '%');
	    }
	}
	fprintf(headerFile, "%c", c);
    }
}

static void
writeCodeOpen(appData_t *appData)
{
    fprintf(appData->out, "{\n");
}

static void
writeCodeClose(appData_t *appData)
{
    /* prevent fall-through to next rule */
    fprintf(appData->out, "IGNORE_TOKEN;\n}\n");
}

static void
writeString(appData_t *appData, char *string)
{
    fprintf(appData->out, "%s", string);
    free(string);
}

static void
writeDefinitions(appData_t *appData)
{
    FILE *templateFile = appData->scanner_template;
    FILE *headerFile = appData->header;
    FILE *outFile = appData->out;
    int c;

    /* write header file from template */
    if (headerFile != NULL) {
	fprintf(headerFile, "#ifndef PERPLEX_H\n");
	fprintf(headerFile, "#define PERPLEX_H\n");

	writeHeader(templateFile, headerFile);
	fprintf(headerFile, "#endif\n");
	fclose(headerFile);
    }

    /* write implementation file from template */
    if (appData->usingConditions) {
	fprintf(outFile, "#define PERPLEX_USING_CONDITIONS\n");
    }

    fprintf(outFile, "\n");
    while ((c = fgetc(templateFile)) != EOF) {
	if (c == '%') {
	    if ((c = fgetc(templateFile)) == '%') {
		/* found %%, ignore */
		continue;
	    } else {
		fprintf(outFile, "%%");
	    }
	}
	fprintf(outFile, "%c", c);
    }
    if (appData->usingConditions) {
	fprintf(outFile, "re2c:condenumprefix = \"\";\n");
	fprintf(outFile, "re2c:cond:goto = \"continue;\";\n");
	fprintf(outFile, "re2c:define:YYCONDTYPE = int;\n");
	fprintf(outFile, "re2c:define:YYGETCONDITION:naked = 1;\n");
    }
    fprintf(outFile, "\n");
    fclose(templateFile);
}

static int
isDefinition(const char *str)
{
    if (str == NULL) {
	return 0;
    }
    return (str[strlen(str) - 1] == ';');
}
}

%token_type {YYSTYPE}
%extra_argument {appData_t *appData}

%type string {char*}
%type word {char*}
%type name_or_def {char*}
%type opt_special {char*}
%type special {char*}

/* later tokens have greater precedence */
%nonassoc EMPTY_RULE_LIST.
%nonassoc TOKEN_WORD.
%nonassoc TOKEN_CODE_END.
%nonassoc TOKEN_CODE_START.
%right TOKEN_EMPTY_COND.
%right TOKEN_CONDITION.
%right TOKEN_NAME.

/* suppress compiler warning about unused variable */
%destructor file {ParseARG_STORE;}

/* three section perplex input file */
file ::= def_section rule_section code_section.

/* DEFINITIONS SECTION */
def_section ::= string TOKEN_SEPARATOR.
{
    writeDefinitions(appData);
    fprintf(appData->out, "\n/* start rules */\n");
}

/* RULES SECTION */
rule_section ::= rule_list.

rule_list ::= /* empty */. [EMPTY_RULE_LIST]
rule_list ::= rule_list rules. [TOKEN_CODE_END]
rule_list ::= rule_list start_scope rules end_scope.

rules ::= rule.
rules ::= rules rule.

start_scope ::= TOKEN_START_SCOPE(COND).
{
    appData->conditions = COND.string;
}

end_scope ::= TOKEN_END_SCOPE.
{
    free(appData->conditions);
    appData->conditions = NULL;
}

rule ::= opt_condition pattern_or_name opt_special(OS) opt_code.
{
    if (isDefinition(OS)) {
	fprintf(appData->out, "\n");
    } else {
	writeCodeClose(appData);
    }

    if(OS != NULL) {
	free(OS);
    }
}
rule ::= empty_condition opt_special opt_code.
{
    writeCodeClose(appData);
}

opt_condition ::= /* empty */. [TOKEN_WORD]
{
    if (appData->conditions) {
	fprintf(appData->out, appData->conditions);
    }
}
opt_condition ::= TOKEN_CONDITION(C). [TOKEN_WORD]
{
    writeString(appData, C.string);
}
empty_condition ::= TOKEN_EMPTY_COND.
{
    fprintf(appData->out, "<> ");
}

opt_special(OS) ::= /* empty */.
{
    OS = NULL;
    writeCodeOpen(appData);
}
opt_special(OS) ::= special(S).
{
    if (!isDefinition(S)) {
	writeCodeOpen(appData);
    }
    OS = S;
}

special(S) ::= TOKEN_SPECIAL_OP(OP) name_or_def(ND).
{
    writeString(appData, OP.string);
    fprintf(appData->out, " %s", ND);
    S = ND;
}

pattern_or_name ::= TOKEN_PATTERN(PATTERN). {
    writeString(appData, PATTERN.string);
}

pattern_or_name ::= TOKEN_NAME(NAME). {
    writeString(appData, NAME.string);
}

name_or_def(N) ::= TOKEN_NAME(NAME).
{
    N = NAME.string;
}
name_or_def(D) ::= TOKEN_DEFINITION(DEF).
{
    D = DEF.string;
}

opt_code ::= /* empty */. [TOKEN_CODE_END]
opt_code ::= TOKEN_CODE_START string TOKEN_CODE_END.

/* CODE SECTION */
code_section ::= /* empty */.
{
    /* close re2c block, while block, and scanner routine */
    fprintf(appData->out, "\n*/\n    }\n}\n");
}

code_section ::= code_section_start string.

code_section_start ::= TOKEN_SEPARATOR.
{
    /* close re2c block, while block, and scanner routine */
    fprintf(appData->out, "\n*/\n    }\n}\n");

    fprintf(appData->out, "\n/* start code */\n");
}

/* strings */
string ::= /* empty */.
string ::= string word(WORD).
{
    writeString(appData, WORD);
}

/* A word is a character sequence that ends in a whitespace character,
 * which does not contain any whitespace chracters except inside quotes.
 * Example words include:
 * abc
 * "abc def"'\n'?
 * abc[ \t\n]
 */
word(A) ::= TOKEN_WORD(B). { A = B.string; }
