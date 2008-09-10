/*                       F N M A T C H . C
 * BRL-CAD
 *
 * Copyright (c) 2008 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 *
 *
 * Based off of OpenBSD's fnmatch.c v 1.13 2006/03/31
 *
 *
 * Copyright (c) 1989, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Guido van Rossum.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Function fnmatch() as specified in POSIX 1003.2-1992, section B.6.
 * Compares a filename or pathname to a pattern.
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define _BU_FNMATCH_H_

#define BU_FNM_NOMATCH     1       /* Match failed. */
#define BU_FNM_NOSYS       2       /* Function not supported (unused). */

#define BU_FNM_NOESCAPE    0x01    /* Disable backslash escaping. */
#define BU_FNM_PATHNAME    0x02    /* Slash must be matched by slash. */
#define BU_FNM_PERIOD      0x04    /* Period must be matched by period. */
#define BU_FNM_LEADING_DIR 0x08    /* Ignore /<tail> after Imatch. */
#define BU_FNM_CASEFOLD    0x10    /* Case insensitive search. */
#define BU_FNM_IGNORECASE  BU_FNM_CASEFOLD
#define BU_FNM_FILE_NAME   BU_FNM_PATHNAME

#define	BU_FNM_EOS	'\0'

#define	BU_FNM_RANGE_MATCH	1
#define	BU_FNM_RANGE_NOMATCH	0
#define	BU_FNM_RANGE_ERROR	(-1)


static int
bu_rangematch(const char *pattern, char test, int flags, char **newp)
{
	int negate, ok;
	char c, c2;

	/*
	 * A bracket expression starting with an unquoted circumflex
	 * character produces unspecified results (IEEE 1003.2-1992,
	 * 3.13.2).  This implementation treats it like '!', for
	 * consistency with the regular expression syntax.
	 * J.T. Conklin (conklin@ngai.kaleida.com)
	 */
	if ((negate = (*pattern == '!' || *pattern == '^')))
		++pattern;

	if (flags & BU_FNM_CASEFOLD)
		test = (char)tolower((unsigned char)test);

	/*
	 * A right bracket shall lose its special meaning and represent
	 * itself in a bracket expression if it occurs first in the list.
	 * -- POSIX.2 2.8.3.2
	 */
	ok = 0;
	c = *pattern++;
	do {
		if (c == '\\' && !(flags & BU_FNM_NOESCAPE))
			c = *pattern++;
		if (c == BU_FNM_EOS)
			return (BU_FNM_RANGE_ERROR);
		if (c == '/' && (flags & BU_FNM_PATHNAME))
			return (BU_FNM_RANGE_NOMATCH);
		if ((flags & BU_FNM_CASEFOLD))
			c = (char)tolower((unsigned char)c);
		if (*pattern == '-'
		    && (c2 = *(pattern+1)) != BU_FNM_EOS && c2 != ']') {
			pattern += 2;
			if (c2 == '\\' && !(flags & BU_FNM_NOESCAPE))
				c2 = *pattern++;
			if (c2 == BU_FNM_EOS)
				return (BU_FNM_RANGE_ERROR);
			if (flags & BU_FNM_CASEFOLD)
				c2 = (char)tolower((unsigned char)c2);
			if (c <= test && test <= c2)
				ok = 1;
		} else if (c == test)
			ok = 1;
	} while ((c = *pattern++) != ']');

	*newp = (char *)pattern;
	return (ok == negate ? BU_FNM_RANGE_NOMATCH : BU_FNM_RANGE_MATCH);
}


int
bu_fnmatch(const char *pattern, const char *string, int flags)
{
	const char *stringstart;
	char *newp;
	char c, test;

	for (stringstart = string;;)
		switch (c = *pattern++) {
		case BU_FNM_EOS:
			if ((flags & BU_FNM_LEADING_DIR) && *string == '/')
				return (0);
			return (*string == BU_FNM_EOS ? 0 : BU_FNM_NOMATCH);
		case '?':
			if (*string == BU_FNM_EOS)
				return (BU_FNM_NOMATCH);
			if (*string == '/' && (flags & BU_FNM_PATHNAME))
				return (BU_FNM_NOMATCH);
			if (*string == '.' && (flags & BU_FNM_PERIOD) &&
			    (string == stringstart ||
			    ((flags & BU_FNM_PATHNAME) && *(string - 1) == '/')))
				return (BU_FNM_NOMATCH);
			++string;
			break;
		case '*':
			c = *pattern;
			/* Collapse multiple stars. */
			while (c == '*')
				c = *++pattern;

			if (*string == '.' && (flags & BU_FNM_PERIOD) &&
			    (string == stringstart ||
			    ((flags & BU_FNM_PATHNAME) && *(string - 1) == '/')))
				return (BU_FNM_NOMATCH);

			/* Optimize for pattern with * at end or before /. */
			if (c == BU_FNM_EOS) {
				if (flags & BU_FNM_PATHNAME)
					return ((flags & BU_FNM_LEADING_DIR) ||
					    strchr(string, '/') == NULL ?
					    0 : BU_FNM_NOMATCH);
				else
					return (0);
			} else if (c == '/' && (flags & BU_FNM_PATHNAME)) {
				if ((string = strchr(string, '/')) == NULL)
					return (BU_FNM_NOMATCH);
				break;
			}

			/* General case, use recursion. */
			while ((test = *string) != BU_FNM_EOS) {
				if (!fnmatch(pattern, string, flags & ~BU_FNM_PERIOD))
					return (0);
				if (test == '/' && (flags & BU_FNM_PATHNAME))
					break;
				++string;
			}
			return (BU_FNM_NOMATCH);
		case '[':
			if (*string == BU_FNM_EOS)
				return (BU_FNM_NOMATCH);
			if (*string == '/' && (flags & BU_FNM_PATHNAME))
				return (BU_FNM_NOMATCH);
			if (*string == '.' && (flags & BU_FNM_PERIOD) &&
			    (string == stringstart ||
			    ((flags & BU_FNM_PATHNAME) && *(string - 1) == '/')))
				return (BU_FNM_NOMATCH);

			switch (bu_rangematch(pattern, *string, flags, &newp)) {
			case BU_FNM_RANGE_ERROR:
				/* not a good range, treat as normal text */
				goto normal;
			case BU_FNM_RANGE_MATCH:
				pattern = newp;
				break;
			case BU_FNM_RANGE_NOMATCH:
				return (BU_FNM_NOMATCH);
			}
			++string;
			break;
		case '\\':
			if (!(flags & BU_FNM_NOESCAPE)) {
				if ((c = *pattern++) == BU_FNM_EOS) {
					c = '\\';
					--pattern;
				}
			}
			/* FALLTHROUGH */
		default:
		normal:
			if (c != *string && !((flags & BU_FNM_CASEFOLD) &&
				 (tolower((unsigned char)c) ==
				 tolower((unsigned char)*string))))
				return (BU_FNM_NOMATCH);
			++string;
			break;
		}
	/* NOTREACHED */
}

