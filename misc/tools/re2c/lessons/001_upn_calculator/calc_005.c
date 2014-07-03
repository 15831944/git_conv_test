/* Generated by re2c */
#line 1 "calc_005.re"
/* re2c lesson 001_upn_calculator, calc_005, (c) M. Boerger 2006 - 2007 */
#line 19 "calc_005.re"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define DEBUG(stmt) stmt

int  stack[4];
int  depth = 0;

int push_num(const char *t, const char *l, int radix)
{
	int num = 0;
	
	if (depth >= sizeof(stack))
	{
		return 3;
	}

	--t;
	while(++t < l)
	{
		num = num * radix + (*t - '0');
	}
	DEBUG(printf("Num: %d\n", num));

	stack[depth++] = num;
	return 0;
}

int stack_add()
{
	if (depth < 2) return 4;
	
	--depth;
	stack[depth-1] = stack[depth-1] + stack[depth];
	return 0;
}

int stack_sub()
{
	if (depth < 2) return 4;

	--depth;
	stack[depth-1] = stack[depth-1] - stack[depth];
	return 0;
}

int scan(char *s, int l)
{
	char *p = s;
	char *q = 0;
	char *t;
	int res = 0;

#define YYCTYPE         char
#define YYCURSOR        p
#define YYLIMIT         (s+l+1)
#define YYMARKER        q
#define YYFILL(n)		{ return depth == 1 ? 0 : 2; }
	
	while(!res)
	{
		t = p;

#line 71 "<stdout>"
		{
			YYCTYPE yych;

			if ((YYLIMIT - YYCURSOR) < 2) YYFILL(2);
			yych = *YYCURSOR;
			switch (yych) {
			case '\t':
			case ' ':	goto yy2;
			case '+':	goto yy7;
			case '-':	goto yy9;
			case '0':	goto yy4;
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':	goto yy6;
			default:	goto yy11;
			}
yy2:
			++YYCURSOR;
			yych = *YYCURSOR;
			goto yy19;
yy3:
#line 91 "calc_005.re"
			{ continue; }
#line 101 "<stdout>"
yy4:
			++YYCURSOR;
			switch ((yych = *YYCURSOR)) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':	goto yy15;
			default:	goto yy5;
			}
yy5:
#line 93 "calc_005.re"
			{ res = push_num(t, p, 10); continue; }
#line 120 "<stdout>"
yy6:
			yych = *++YYCURSOR;
			goto yy14;
yy7:
			++YYCURSOR;
#line 94 "calc_005.re"
			{ res = stack_add();		continue; }
#line 128 "<stdout>"
yy9:
			++YYCURSOR;
#line 95 "calc_005.re"
			{ res = stack_sub();		continue; }
#line 133 "<stdout>"
yy11:
			++YYCURSOR;
#line 96 "calc_005.re"
			{ res = 1; 					continue; }
#line 138 "<stdout>"
yy13:
			++YYCURSOR;
			if (YYLIMIT <= YYCURSOR) YYFILL(1);
			yych = *YYCURSOR;
yy14:
			switch (yych) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':	goto yy13;
			default:	goto yy5;
			}
yy15:
			++YYCURSOR;
			if (YYLIMIT <= YYCURSOR) YYFILL(1);
			yych = *YYCURSOR;
			switch (yych) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':	goto yy15;
			default:	goto yy17;
			}
yy17:
#line 92 "calc_005.re"
			{ res = push_num(t, p, 8);	continue; }
#line 177 "<stdout>"
yy18:
			++YYCURSOR;
			if (YYLIMIT <= YYCURSOR) YYFILL(1);
			yych = *YYCURSOR;
yy19:
			switch (yych) {
			case '\t':
			case ' ':	goto yy18;
			default:	goto yy3;
			}
		}
#line 97 "calc_005.re"

	}
	return res;
}

int main(int argc, char **argv)
{
	if (argc > 1)
	{
		char *inp;
		int res = 0, argp = 0, len;
		
		while(!res && ++argp < argc)
		{
			inp = argv[argp];
			len = strlen(inp);
			if (inp[0] == '\"' && inp[len-1] == '\"')
			{
				++inp;
				len -=2;
			}
			res = scan(inp, len);
		}
		switch(res)
		{
		case 0:
			printf("Result: %d\n", stack[0]);
			return 0;
		case 1:
			fprintf(stderr, "Illegal character in input.\n");
			return 1;
		case 2:
			fprintf(stderr, "Premature end of input.\n");
			return 2;
		case 3:
			fprintf(stderr, "Stack overflow.\n");
			return 3;
		case 4:
			fprintf(stderr, "Stack underflow.\n");
			return 4;
		}
	}
	else
	{
		fprintf(stderr, "%s <expr>\n", argv[0]);
		return 0;
	}
}
