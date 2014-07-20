/* Generated by re2c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define	BSIZE	8192


enum ScanContition {
	EStateNormal,
	EStateComment,
	EStateSkiptoeol,
	EStateString,
};


typedef struct Scanner
{
	FILE			    *fp;
	unsigned char	    *cur, *tok, *lim, *eof;
	unsigned char 	    buffer[BSIZE];
	unsigned char       yych;
	enum ScanContition  cond;
	int                 state;
} Scanner;

int fill(Scanner *s, int len)
{
	if (!len)
	{
		s->cur = s->tok = s->lim = s->buffer;
		s->eof = 0;
	}
	if (!s->eof)
	{
		int got, cnt = s->tok - s->buffer;

		if (cnt > 0)
		{
			memcpy(s->buffer, s->tok, s->lim - s->tok);
			s->tok -= cnt;
			s->cur -= cnt;
			s->lim -= cnt;
		}
		cnt = BSIZE - cnt;
		if ((got = fread(s->lim, 1, cnt, s->fp)) != cnt)
		{
			s->eof = &s->lim[got];
		}
		s->lim += got;
	}
	else if (s->cur + len > s->eof)
	{
		return 0; /* not enough input data */
	}
	return -1;
}

void fputl(const char *s, size_t len, FILE *stream)
{
	while(len-- > 0)
	{
		fputc(*s++, stream);
	}
}

void scan(Scanner *s)
{
	fill(s, 0);

	for(;;)
	{
		s->tok = s->cur;


		if (s->state < 1) {
			if (s->state < 0) {
				goto yy0;
			} else {
				goto yyFillLabel0;
			}
		} else {
			if (s->state < 2) {
				goto yyFillLabel1;
			} else {
				if (s->state < 3) {
					goto yyFillLabel2;
				} else {
					goto yyFillLabel3;
				}
			}
		}
yy0:
		if (s->cond < 2) {
			if (s->cond < 1) {
				goto yyc_Normal;
			} else {
				goto yyc_Comment;
			}
		} else {
			if (s->cond < 3) {
				goto yyc_Skiptoeol;
			} else {
				goto yyc_String;
			}
		}
/* *********************************** */
yyc_Comment:

		s->state = 0;
		if ((s->lim - s->cur) < 2) if(fill(s, 2) >= 0) break;
yyFillLabel0:
		s->yych = *s->cur;
		if (s->yych != '*') goto yy4;
		++s->cur;
		if ((s->yych = *s->cur) == '/') goto yy5;
yy3:
		{
				goto yyc_Comment;
			}
yy4:
		s->yych = *++s->cur;
		goto yy3;
yy5:
		++s->cur;
		{
				s->cond = EStateNormal;
				continue;
			}
/* *********************************** */
yyc_Normal:
		s->state = 1;
		if ((s->lim - s->cur) < 4) if(fill(s, 4) >= 0) break;
yyFillLabel1:
		s->yych = *s->cur;
		if (s->yych <= '\'') {
			if (s->yych == '"') goto yy13;
			if (s->yych <= '&') goto yy15;
			goto yy12;
		} else {
			if (s->yych <= '/') {
				if (s->yych <= '.') goto yy15;
				goto yy11;
			} else {
				if (s->yych != '?') goto yy15;
			}
		}
		s->yych = *(s->tok = ++s->cur);
		if (s->yych == '?') goto yy26;
yy10:
		{
				fputc(*s->tok, stdout);
				continue;
			}
yy11:
		s->yych = *++s->cur;
		if (s->yych == '*') goto yy24;
		if (s->yych == '/') goto yy22;
		goto yy10;
yy12:
		s->yych = *(s->tok = ++s->cur);
		if (s->yych == '"') goto yy16;
		if (s->yych == '\\') goto yy18;
		goto yy10;
yy13:
		++s->cur;
		{
				fputc(*s->tok, stdout);
				s->state = EStateString;
				continue;
			}
yy15:
		s->yych = *++s->cur;
		goto yy10;
yy16:
		s->yych = *++s->cur;
		if (s->yych == '\'') goto yy20;
yy17:
		s->cur = s->tok;
		goto yy10;
yy18:
		s->yych = *++s->cur;
		if (s->yych != '"') goto yy17;
		s->yych = *++s->cur;
		if (s->yych != '\'') goto yy17;
yy20:
		++s->cur;
		{
				fputl("'\"'", 3, stdout);
				continue;
			}
yy22:
		++s->cur;
		{
				s->cond = EStateSkiptoeol;
				goto yyc_Skiptoeol;
			}
yy24:
		++s->cur;
		{
				s->cond = EStateComment;
				goto yyc_Comment;
			}
yy26:
		s->yych = *++s->cur;
		switch (s->yych) {
		case '!':	goto yy41;
		case '\'':	goto yy39;
		case '(':	goto yy27;
		case ')':	goto yy29;
		case '-':	goto yy43;
		case '/':	goto yy37;
		case '<':	goto yy31;
		case '=':	goto yy35;
		case '>':	goto yy33;
		default:	goto yy17;
		}
yy27:
		++s->cur;
		{
				fputc('[', stdout);
				continue;
			}
yy29:
		++s->cur;
		{
				fputc(']', stdout);
				continue;
			}
yy31:
		++s->cur;
		{
				fputc('{', stdout);
				continue;
			}
yy33:
		++s->cur;
		{
				fputc('}', stdout);
				continue;
			}
yy35:
		++s->cur;
		{
				fputc('#', stdout);
				continue;
			}
yy37:
		++s->cur;
		{
				fputc('\\', stdout);
				continue;
			}
yy39:
		++s->cur;
		{
				fputc('^', stdout);
				continue;
			}
yy41:
		++s->cur;
		{
				fputc('|', stdout);
				continue;
			}
yy43:
		++s->cur;
		{
				fputc('~', stdout);
				continue;
			}
/* *********************************** */
yyc_Skiptoeol:
		s->state = 2;
		if ((s->lim - s->cur) < 5) if(fill(s, 5) >= 0) break;
yyFillLabel2:
		s->yych = *s->cur;
		if (s->yych <= '>') {
			if (s->yych == '\n') goto yy50;
			goto yy52;
		} else {
			if (s->yych <= '?') goto yy47;
			if (s->yych == '\\') goto yy49;
			goto yy52;
		}
yy47:
		s->yych = *(s->tok = ++s->cur);
		if (s->yych == '?') goto yy57;
yy48:
		{
				goto yyc_Skiptoeol;
			}
yy49:
		s->yych = *(s->tok = ++s->cur);
		if (s->yych == '\n') goto yy55;
		if (s->yych == '\r') goto yy53;
		goto yy48;
yy50:
		++s->cur;
		{
				s->cond = EStateNormal;
				continue;
			}
yy52:
		s->yych = *++s->cur;
		goto yy48;
yy53:
		s->yych = *++s->cur;
		if (s->yych == '\n') goto yy55;
yy54:
		s->cur = s->tok;
		goto yy48;
yy55:
		++s->cur;
		{
				goto yyc_Skiptoeol;
			}
yy57:
		s->yych = *++s->cur;
		if (s->yych != '/') goto yy54;
		s->yych = *++s->cur;
		if (s->yych == '\n') goto yy60;
		if (s->yych != '\r') goto yy54;
		s->yych = *++s->cur;
		if (s->yych != '\n') goto yy54;
yy60:
		++s->cur;
		{
				goto yyc_Skiptoeol;
			}
/* *********************************** */
yyc_String:
		s->state = 3;
		if ((s->lim - s->cur) < 2) if(fill(s, 2) >= 0) break;
yyFillLabel3:
		s->yych = *s->cur;
		if (s->yych == '"') goto yy66;
		if (s->yych != '\\') goto yy68;
		++s->cur;
		if ((s->yych = *s->cur) != '\n') goto yy69;
yy65:
		{
				fputc(*s->tok, stdout);
				continue;
			}
yy66:
		++s->cur;
		{
				fputc(*s->tok, stdout);
				s->cond = EStateNormal;
				continue;
			}
yy68:
		s->yych = *++s->cur;
		goto yy65;
yy69:
		++s->cur;
		{
				fputl((const char*)s->tok, 2, stdout);
				continue;
			}

	}
}

int main(int argc, char **argv)
{
	Scanner in;
	char c;

	if (argc != 2)
	{
		fprintf(stderr, "%s <file>\n", argv[0]);
		return 1;;
	}

	memset((char*) &in, 0, sizeof(in));

	if (!strcmp(argv[1], "-"))
	{
		in.fp = stdin;
	}
	else if ((in.fp = fopen(argv[1], "r")) == NULL)
	{
		fprintf(stderr, "Cannot open file '%s'\n", argv[1]);
		return 1;
	}

 	in.cond = EStateNormal;
 	scan(&in);

	if (in.fp != stdin)
	{
		fclose(in.fp);
	}
	return 0;
}
