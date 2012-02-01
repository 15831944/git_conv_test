/*                        S S C A N F . C
 * BRL-CAD
 *
 * Copyright (c) 2012 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * Copyright (c) 1990, 1993 The Regents of the University of California.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided
 * with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote
 * products derived from this software without specific prior written
 * permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/** @file sscanf.c
 *
 * Custom sscanf and vsscanf.
 *
 */

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "bu.h"

/*
 * Flags used during conversion.
 */
#define	LONG		0x00001	/* l: long or double */
#define	LONGDBL		0x00002	/* L: long double */
#define	SHORT		0x00004	/* h: short */
#define	SUPPRESS	0x00008	/* *: suppress assignment */
#define	POINTER		0x00010	/* p: void * (as hex) */
#define	NOSKIP		0x00020	/* [ or c: do not skip blanks */
#define	LONGLONG	0x00400	/* ll: long long (+ deprecated q: quad) */
#define	INTMAXT		0x00800	/* j: intmax_t */
#define	PTRDIFFT	0x01000	/* t: ptrdiff_t */
#define	SIZET		0x02000	/* z: size_t */
#define	SHORTSHORT	0x04000	/* hh: char */
#define	UNSIGNED	0x08000	/* %[oupxX] conversions */
#define	BUVLS		0x20000	/* V: struct bu_vls */

/*
 * The following are used in integral conversions only:
 * SIGNOK, NDIGITS, PFXOK, and NZDIGITS
 */
#define	SIGNOK		0x00040	/* +/- is (still) legal */
#define	NDIGITS		0x00080	/* no digits detected */
#define	PFXOK		0x00100	/* 0x prefix is (still) legal */
#define	NZDIGITS	0x00200	/* no zero digits detected */
#define	HAVESIGN	0x10000	/* sign detected */

/*
 * Conversion types.
 */
#define	CT_CHAR		0	/* %c conversion */
#define	CT_CCL		1	/* %[...] conversion */
#define	CT_STRING	2	/* %s conversion */
#define	CT_INT		3	/* %[dioupxX] conversion */
#define	CT_FLOAT	4	/* %[efgEFG] conversion */

#define CCL_TABLE_SIZE 256
#define CCL_ACCEPT 1
#define CCL_REJECT 0

static const char*
get_ccl_table(char *tab, const char *fmt)
{
    int i, v, curr, lower, upper;

    BU_ASSERT(tab != NULL);
    BU_ASSERT(fmt != NULL);

#define SET_CCL_ENTRY(e) tab[e] = v;

    curr = *fmt++;
    if (curr == '^') {
	/* accept all chars by default */
	memset(tab, CCL_ACCEPT, CCL_TABLE_SIZE);
	v = CCL_REJECT;
	curr = *fmt++;
    } else {
	/* reject all chars by default */
	memset(tab, CCL_REJECT, CCL_TABLE_SIZE);
	v = CCL_ACCEPT;
    }

    if (curr == '\0') {
	/* format ended before closing ] */
	return fmt - 1;
    }

    SET_CCL_ENTRY(curr);

    /* set table entries from fmt */
    while (1) {
	lower = curr;
	curr = *fmt++;

	if (curr == '\0') {
	    return fmt - 1;
	}
	if (curr == ']') {
	    return fmt;
	}

	/* '-' usually used to specify a range */
	if (curr == '-') {
	    upper = *fmt;

	    if (upper == ']' || upper < lower) {
		/* ordinary '-' */
		SET_CCL_ENTRY(curr);
		continue;
	    }

	    /* Range. Set everything in the range. */
	    for (i = 0; i < CCL_TABLE_SIZE; ++i) {
		if (lower < i && i <= upper) {
		    SET_CCL_ENTRY(i);
		}
	    }

	    /* need to skip upper since it is not an ordinary character */
	    curr = upper;
	    ++fmt;
	} else {
	    /* ordinary char */
	    SET_CCL_ENTRY(curr);
	}
    }
    /* NOTREACHED */
}

/* Copy part of a string, everything from srcStart to srcEnd (exclusive),
 * into a new buffer. Returns the allocated buffer.
 */
static char*
getSubstring(const char *srcStart, const char *srcEnd)
{
    size_t size;
    char *sub;

    size = (size_t)srcEnd - (size_t)srcStart + 1;
    sub = (char*)bu_malloc(size * sizeof(char), "getSubstring");
    bu_strlcpy(sub, srcStart, size);

    return sub;
}

/* append %n to format string to get consumed count */
static void
append_n(char **fmt) {
    char *result;
    int nLen, size, resultSize;

    size = strlen(*fmt) + 1;
    nLen = strlen("%n");

    resultSize = size + nLen;
    result = (char*)bu_malloc(resultSize * sizeof(char), "append_n");

    bu_strlcpy(result, *fmt, resultSize);
    bu_strlcat(result, "%n", resultSize);

    bu_free(*fmt, "append_n");
    *fmt = result;
}

/* The basic strategy of this routine is to break the formatted scan into
 * pieces, one for each word of the format string.
 *
 * New two-word format strings are created by appending "%n" (request to sscanf
 * for consumed char count) to each word of the provided format string. sscanf
 * is called with each two-word format string, followed by an appropriately
 * cast pointer from the vararg list, followed by the local consumed count
 * pointer.
 *
 * Each time sscanf successfully returns, the total assignment count and the
 * total consumed character count are updated. The consumed character count is
 * used to offset the read position of the source string on successive calls to
 * sscanf. The total assignment count is the ultimate return value of the
 * routine.
 */
int
bu_vsscanf(const char *src, const char *fmt, va_list ap)
{
    int c;
    long flags;
    int numCharsConsumed, partConsumed;
    int numFieldsAssigned, partAssigned;
    char *partFmt;
    const char *wordStart;
    size_t i, width;
    char ccl_tab[CCL_TABLE_SIZE];

    BU_ASSERT(src != NULL);
    BU_ASSERT(fmt != NULL);

    numFieldsAssigned = 0;
    numCharsConsumed = 0;
    partConsumed = 0;
    partAssigned = 0;
    partFmt = NULL;

#define UPDATE_COUNTS \
    numCharsConsumed += partConsumed; \
    numFieldsAssigned += partAssigned;

#define FREE_FORMAT_PART \
    if (partFmt != NULL) { \
	bu_free(partFmt, "bu_sscanf partFmt"); \
	partFmt = NULL; \
    }

#define GET_FORMAT_PART \
    FREE_FORMAT_PART; \
    partFmt = getSubstring(wordStart, fmt); \
    append_n(&partFmt);

#define EXIT_DUE_TO_INPUT_FAILURE \
    FREE_FORMAT_PART; \
    if (numFieldsAssigned == 0) { \
	return EOF; \
    } \
    return numFieldsAssigned;

#define EXIT_DUE_TO_MATCH_FAILURE \
    FREE_FORMAT_PART; \
    return numFieldsAssigned;

    while (1) {
	/* Mark word start, then skip to first non-white char. */
	wordStart = fmt;
	do {
	    c = *fmt;
	    if (c == '\0') {
		/* Found EOI before next word; implies fmt contains trailing
		 * whitespace or is empty. No worries; exit normally.
		 */
		goto exit;
	    }
	    ++fmt;
	} while (isspace(c));

	if (c != '%') {
	    /* Must have found literal sequence. Find where it ends. */
	    while (1) {
		c = *fmt;
		if (c == '\0' || isspace(c) || c == '%') {
		    break;
		}
		++fmt;
	    }
	    /* scan literal sequence */
	    GET_FORMAT_PART;
	    partAssigned = sscanf(&src[numCharsConsumed], partFmt, &partConsumed);
	    if (partAssigned < 0) {
		EXIT_DUE_TO_INPUT_FAILURE;
	    }
	    UPDATE_COUNTS;
	    continue;
	}

	/* Found conversion specification. Parse it. */

	width = 0;
	flags = 0;
again:
	c = *fmt++;
	switch (c) {

	/* Literal '%'. */
	case '%':
	    GET_FORMAT_PART;
	    partAssigned = sscanf(&src[numCharsConsumed], partFmt, &partConsumed);
	    if (partAssigned < 0) {
		EXIT_DUE_TO_INPUT_FAILURE;
	    }
	    UPDATE_COUNTS;
	    continue;
	

	/* MODIFIER */
	case '*':
	    flags |= SUPPRESS;
	    goto again;
	case 'j':
	    flags |= INTMAXT;
	    goto again;
	case 'l':
	    if (!(flags & LONG)) {
		/* First occurance of 'l' in this conversion specifier. */
		flags |= LONG;
	    } else {
		/* Since LONG is set, the previous conversion character must
		 * have been 'l'. With this second 'l', we know we have an "ll"
		 * modifer, not an 'l' modifer. We need to replace the
		 * incorrect flag with the correct one.
		 */
		flags &= ~LONG;
		flags |= LONGLONG;
	    }
	    goto again;
	case 'q':
	    /* Quad conversion specific to 4.4BSD and Linux libc5 only.
	     * Should probably print a warning here to use ll or L instead.
	     */
	    flags |= LONGLONG;
	    goto again;
	case 't':
	    flags |= PTRDIFFT;
	    goto again;
	case 'z':
	    flags |= SIZET;
	    goto again;
	case 'L':
	    flags |= LONGDBL;
	    goto again;
	case 'h':
	    if (!(flags & SHORT)) {
		/* First occurance of 'h' in this conversion specifier. */
		flags |= SHORT;
	    } else {
		/* Since SHORT is set, the previous conversion character must
		 * have been 'h'. With this second 'h', we know we have an "hh"
		 * modifer, not an 'h' modifer. We need to replace the
		 * incorrect flag with the correct one.
		 */
		flags &= ~SHORT;
		flags |= SHORTSHORT;
	    }
	    goto again;
	case 'V':
	    flags |= BUVLS;
	    goto again;


	/* MAXIMUM FIELD WIDTH */
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
#define NUMERIC_CHAR_TO_INT(c) (c - '0')
	    width = (width * 10) + NUMERIC_CHAR_TO_INT(c);
	    goto again;


	/* CONVERSION */
	case 'd':
	    c = CT_INT;
	    break;
	case 'i':
	    c = CT_INT;
	    break;
	case 'o':
	    c = CT_INT;
	    flags |= UNSIGNED;
	    break;
	case 'u':
	    c = CT_INT;
	    flags |= UNSIGNED;
	    break;
	case 'p':
	case 'x':
	case 'X':
	    if (c == 'p') {
		flags |= POINTER;
	    }
	    flags |= PFXOK;
	    flags |= UNSIGNED;
	    c = CT_INT;
	    break;
	case 'A': case 'E': case 'F': case 'G':
	case 'a': case 'e': case 'f': case 'g':
	    c = CT_FLOAT;
	    break;
	case 'S':
	    /* XXX This may be a BSD extension. */
	    flags |= LONG;
	    /* FALLTHROUGH */
	case 's':
	    c = CT_STRING;
	    break;
	case '[':
	    fmt = get_ccl_table(ccl_tab, fmt);
	    flags |= NOSKIP;
	    c = CT_CCL;
	    break;
	case 'C':
	    /* XXX This may be a BSD extension. */
	    flags |= LONG;
	    /* FALLTHROUGH */
	case 'c':
	    flags |= NOSKIP;
	    c = CT_CHAR;
	    break;
	case 'n':
	    if (flags & SUPPRESS) {
		/* This is legal, but doesn't really make sense. Caller
		 * requested assignment of the current count of consumed
		 * characters, but then suppressed the assignment they
		 * requested!
	         */
		continue;
	    }

	    /* Store current count of consumed characters to whatever kind of
	     * int pointer was provided.
	     */
	    if (flags & SHORTSHORT) {
		*va_arg(ap, char *) = numCharsConsumed;
	    } else if (flags & SHORT) {
		*va_arg(ap, short *) = numCharsConsumed;
	    } else if (flags & LONG) {
		*va_arg(ap, long *) = numCharsConsumed;
	    } else if (flags & LONGLONG) {
		*va_arg(ap, long long *) = numCharsConsumed;
	    } else if (flags & INTMAXT) {
		*va_arg(ap, intmax_t *) = numCharsConsumed;
	    } else if (flags & SIZET) {
		*va_arg(ap, size_t *) = numCharsConsumed;
	    } else if (flags & PTRDIFFT) {
		*va_arg(ap, ptrdiff_t *) = numCharsConsumed;
	    } else {
		*va_arg(ap, int *) = numCharsConsumed;
	    }
	    continue;

	case '\0':
	    /* Format string ends with bare '%'. Returning EOF regardless of
	     * successfull assignments is a backwards compatability behavior.
	     */
	    FREE_FORMAT_PART;
	    return EOF;

	default:
	    EXIT_DUE_TO_MATCH_FAILURE;
	}

	/* Done parsing conversion specification.
	 * Now do the actual conversion.
	 */

	GET_FORMAT_PART;

#define SSCANF_TYPE(type) \
    if (flags & SUPPRESS) { \
	partAssigned = sscanf(&src[numCharsConsumed], partFmt, \
		&partConsumed); \
    } else { \
	partAssigned = sscanf(&src[numCharsConsumed], partFmt, \
		va_arg(ap, type), &partConsumed); \
    }

#define SSCANF_SIGNED_UNSIGNED(type) \
if (flags & UNSIGNED) { \
    SSCANF_TYPE(unsigned type); \
} else { \
    SSCANF_TYPE(type); \
}
	partAssigned = partConsumed = 0;

	switch (c) {

	case CT_CHAR:
	case CT_CCL:
	case CT_STRING:

	    /* %Vc or %V[...] or %Vs conversion */
	    if (flags & BUVLS) {
		struct bu_vls *vls = NULL;
		int conversion = c;

		if (src[numCharsConsumed] == '\0') {
		    EXIT_DUE_TO_INPUT_FAILURE;
		}

		/* skip leading whitespace for %Vs */
		if (conversion == CT_STRING) {
		    while (1) {
			c = src[numCharsConsumed];
			if (c == '\0' || !isspace(c)) {
			    break;
			}
			++numCharsConsumed;
		    }
		}

		/* set default width */
		if (width == 0) {
		    if (conversion == CT_CHAR) {
			width = 1;
		    } else {
			/* infinity */
			width = ~(width & 0);
		    }
		}

		if (!(flags & SUPPRESS)) {
		    vls = va_arg(ap, struct bu_vls*);
		    bu_vls_trunc(vls, 0);
		}

		/* Copy characters from src to vls. Stop at width, non-matching
		 * character, or EOI.
		 */
		for (i = 0; i < width; ++i) {
		    c = src[numCharsConsumed + i];

		    /* stop at EOI */
		    if (c == '\0') {
			break;
		    }

		    /* stop at non-matching */
		    if (ccl_tab[c] == CCL_REJECT && conversion == CT_CCL) {
			break;
		    }
		    if (isspace(c) && conversion == CT_STRING) {
			break;
		    }

		    if (!(flags & SUPPRESS)) {
			/* copy valid char to vls */
			bu_vls_putc(vls, c);
		    }
		    ++partConsumed;
		}

		if (!(flags & SUPPRESS) && partConsumed > 0) {
		    /* successful assignment */
		    ++partAssigned;
		}
	    } /* BUVLS */

	    /* %c or %[...] or %s conversion */
	    else if (flags & LONG) {
		SSCANF_TYPE(wchar_t*);
	    } else {
		SSCANF_TYPE(char*);
	    }
	    break;

	/* %[dioupxX] conversion */
	case CT_INT:
	    if (flags & SHORT) {
		SSCANF_SIGNED_UNSIGNED(short int*);
	    } else if (flags & SHORTSHORT) {
		SSCANF_SIGNED_UNSIGNED(char*);
	    } else if (flags & LONG) {
		SSCANF_SIGNED_UNSIGNED(long int*);
	    } else if (flags & LONGLONG) {
		SSCANF_SIGNED_UNSIGNED(long long int*);
	    } else if (flags & POINTER) {
		SSCANF_TYPE(void*);
	    } else if (flags & PTRDIFFT) {
		SSCANF_TYPE(ptrdiff_t*);
	    } else if (flags & SIZET) {
		SSCANF_TYPE(size_t*);
	    } else if (flags & INTMAXT) {
		if (flags & UNSIGNED) {
		    SSCANF_TYPE(uintmax_t*);
		} else {
		    SSCANF_TYPE(intmax_t*);
		}
	    } else {
		SSCANF_TYPE(int*);
	    }
	    break;

	/* %[eEfg] conversion */
	case CT_FLOAT:
	    if (flags & LONG) {
		SSCANF_TYPE(double*);
	    } else if (flags & LONGDBL) {
		SSCANF_TYPE(long double*);
	    } else {
		SSCANF_TYPE(float*);
	    }
	}

	/* check for read error or bad source string */
	if (partAssigned == EOF) {
	    EXIT_DUE_TO_INPUT_FAILURE;
	}

        /* check that assignment was successful */
	if (!(flags & SUPPRESS) && partAssigned < 1) {
	    EXIT_DUE_TO_MATCH_FAILURE;
	}

	/* Conversion successful, on to the next one! */
	UPDATE_COUNTS;

    } /* while (1) */

exit:
    FREE_FORMAT_PART;
    return numFieldsAssigned;
} /* bu_vsscanf */

int
bu_sscanf(const char *src, const char *fmt, ...)
{
    int ret;
    va_list ap;

    va_start(ap, fmt);
    ret = bu_vsscanf(src, fmt, ap);
    va_end(ap);

    return ret;
}

/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
