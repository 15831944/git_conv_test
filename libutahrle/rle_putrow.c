/*
 * This software is copyrighted as noted below.  It may be freely copied,
 * modified, and redistributed, provided that the copyright notice is 
 * preserved on all copies.
 * 
 * There is no warranty or other guarantee of fitness for this software,
 * it is provided solely "as is".  Bug reports or fixes may be sent
 * to the author, who may or may not act on them as he desires.
 *
 * You may not include this software in a program or other software product
 * without supplying the source, or without informing the end-user that the 
 * source is available for no extra charge.
 *
 * If you modify this software, you should include a notice giving the
 * name of the person performing the modification, the date of modification,
 * and the reason for such modification.
 *
 *  Modified at BRL 16-May-88 by Mike Muuss to avoid Alliant STDC desire
 *  to have all "void" functions so declared.
 */
/* 
 * rle_putrow.c - Save a row of the fb to a file.
 * 
 * Author:	Spencer W. Thomas
 * 		Computer Science Dept.
 * 		University of Utah
 * Date:	1 April 1981
 * Copyright (c) 1981,1986 Spencer W. Thomas
 *
 * $Id$
 */
 
#include "stdio.h"
#include "rle_put.h"
#include "rle.h"
#ifdef USE_STDLIB_H
#include <stdlib.h>
#else

#ifndef VOID_STAR
extern char * malloc();
#else
extern void *malloc();
#endif
extern void free();

#endif /* USE_STDLIB_H */

static int findruns();

#define FASTRUNS		/* Faster run finding */
#ifdef vax
#define LOCC			/* Use vax instructions for more speed */
#endif

#define	FALSE	0
#define	TRUE	1

/*****************************************************************
 * TAG( rle_putrow )
 * Write a scanline to the output file.
 * 
 * Inputs:
 *	rows:		Pointer to vector of pointers to
 *			rle_pixel arrays containing the pixel information.
 *			If NULL, rowlen scanlines are skipped.
 *	rowlen:		The number of pixels in the scanline, or the
 *			number of scanlines to skip (see above).
 * Outputs:
 * 	Run length encoded information is written to the_hdr.rle_file.
 * Assumptions:
 * 	I'm sure there are lots of assumptions in here.
 * Algorithm:
 * 	[read the code :-]
 */

void
rle_putrow(rows, rowlen, the_hdr)
register rle_pixel *rows[];
int rowlen;
register rle_hdr * the_hdr;
{
    register int i, j;
    int nrun;
    register rle_pixel *row;
    int mask;
    char bits[256];
    short   state,
	    dstart,
    	    dend,
	    rstart = 0,
	    runval = 0;			/* shut up lint */

    if (rows == NULL)
    {
	the_hdr->priv.put.nblank += rowlen;
	return;
    }
    /* 
     * If not done already, allocate space to remember runs of
     * non-background color.  A run of bg color must be at least 2
     * bytes long to count, so there can be at most rowlen/3 of them.
     */
    if ( the_hdr->priv.put.brun == NULL )
    {
	the_hdr->priv.put.brun =
	    (short (*)[2])malloc(
		(unsigned)((rowlen/3 + 1) * 2 * sizeof(short)) );
	if ( the_hdr->priv.put.brun == NULL )
	{
	    fprintf( stderr, "Malloc failed in rle_putrow\n" );
	    exit(1);
	}
    }
    /* Unpack bitmask in the_hdr struct */
    for ( i=0; i < the_hdr->ncolors; i++ )
	bits[i] = RLE_BIT( *the_hdr, i );
    bits[255] = RLE_BIT( *the_hdr, -1 );

    /* 
     * If saving only non-background pixels, find runs of them.  Note
     * that the alpha channel is considered to be background iff it is
     * zero.
     */
#ifdef	FASTRUNS
    if ( the_hdr->background )
    {
	/* 
	 * Find runs in each color individually, merging them as we go.
	 */
	nrun = 0;		/* start out with no runs */
	/* Alpha channel first */
	if ( the_hdr->alpha )
	    nrun = findruns( rows[-1], rowlen, 0, nrun,
			    the_hdr->priv.put.brun );
	/* Now the color channels */
	for ( i = 0; i < the_hdr->ncolors; i++ )
	    if ( bits[i] )
		nrun = findruns( rows[i], rowlen, the_hdr->bg_color[i],
				 nrun, the_hdr->priv.put.brun );
    }
    else
    {
	the_hdr->priv.put.brun[0][0] = 0;
	the_hdr->priv.put.brun[0][1] = rowlen-1;
	nrun = 1;
    }
#else				/* FASTRUNS */
    if (the_hdr->background)	/* find non-background runs */
    {
	j = 0;
	for (i=0; i<rowlen; i++)
	    if (!same_color( i, rows, the_hdr->bg_color,
			     the_hdr->ncolors, bits ) ||
		(the_hdr->alpha && rows[-1][i] != 0))
	    {
		if (j > 0 && i - the_hdr->priv.put.brun[j-1][1] <= 4)
		    j--;
		else
		    the_hdr->priv.put.brun[j][0] = i; /* start of run */
		for ( i++;
		      i < rowlen && 
			( !same_color( i, rows, the_hdr->bg_color,
					 the_hdr->ncolors, bits ) ||
			  (the_hdr->alpha && rows[-1][i] != 0) );
		      i++)
		    ;			/* find the end of this run */
		the_hdr->priv.put.brun[j][1] = i-1;    /* last in run */
		j++;
	    }
	nrun = j;
    }
    else
    {
	the_hdr->priv.put.brun[0][0] = 0;
	the_hdr->priv.put.brun[0][1] = rowlen-1;
	nrun = 1;
    }
#endif				/* FASTRUNS */
    if (nrun > 0)
    {
	if (the_hdr->priv.put.nblank > 0)
	{
	    SkipBlankLines(the_hdr->priv.put.nblank);
	    the_hdr->priv.put.nblank = 0;
	}
	for ( mask = (the_hdr->alpha ? -1 : 0);
	      mask < the_hdr->ncolors;
	      mask++)			/* do all colors */
	{
	    if ( ! bits[mask & 0xff] )
	    {
		continue;
	    }
	    row = rows[mask];
	    SetColor(mask);
	    if (the_hdr->priv.put.brun[0][0] > 0)
	    {
		SkipPixels(the_hdr->priv.put.brun[0][0], FALSE, FALSE);
	    }
	    for (j=0; j<nrun; j++)
	    {
		state = DATA;
		dstart = the_hdr->priv.put.brun[j][0];
		dend = the_hdr->priv.put.brun[j][1];
		for (i=dstart; i<=dend; i++)
		{
		    switch(state)
		    {
		    case DATA:
			if (i > dstart && runval == row[i])
			{
			    state = RUN2;	/* 2 in a row, may be a run */
			}
			else
			{
			    runval = row[i];	/* maybe a run starts here? */
			    rstart = i;
			}
			break;
	    
		    case RUN2:
			if (runval == row[i])
			{
			    state  = RUN3;	/* 3 in a row may be a run */
			}
			else
			{
			    state = DATA;	/* Nope, back to data */
			    runval = row[i];	/* but maybe a new run here? */
			    rstart = i;
			}
			break;

		    case RUN3:
			if (runval == row[i])	/* 3 in a row is a run */
			{
			    state = INRUN;
			    putdata(row + dstart, rstart - dstart);
#ifdef FASTRUNS
#ifdef LOCC
			    /* Shortcut to find end of run! */
			    i = dend - skpc( (char *)row + i, dend + 1 - i,
					     runval );
#else
			    while ( row[++i] == runval && i <= dend)
				; /* not quite so good, but not bad */
			    i--;
#endif /* LOCC */
#endif /* FASTRUNS */
			}
			else
			{
			    state = DATA;		/* not a run, */
			    runval = row[i];	/* but may this starts one */
			    rstart = i;
			}
			break;
	    
		    case INRUN:
			if (runval != row[i])	/* if run out */
			{
			    state = DATA;
			    putrun(runval, i - rstart, FALSE);
			    runval = row[i];	/* who knows, might be more */
			    rstart = i;
			    dstart = i;	/* starting a new 'data' run */
			}
			break;
		    }
		}
		if (state == INRUN)
		    putrun(runval, i - rstart, TRUE);	/* last bit */
		else
		    putdata(row + dstart, i - dstart);

		if (j < nrun-1)
		    SkipPixels(
			    the_hdr->priv.put.brun[j+1][0] - dend - 1,
			    FALSE, state == INRUN);
		else
		{
		    if (rowlen - dend > 0)
			SkipPixels(
			    rowlen - dend - 1,
			    TRUE, state == INRUN);
		}
	    }

	    if ( mask != the_hdr->ncolors - 1 )
		NewScanLine(FALSE);
	}
    }

    /* Increment to next scanline */
    the_hdr->priv.put.nblank++;

    /* flush every scanline */
    fflush( the_hdr->rle_file );
}


/*****************************************************************
 * TAG( rle_skiprow )
 * 
 * Skip rows in RLE file.
 * Inputs:
 * 	the_hdr:    	Header struct for RLE output file.
 *  	nrow:	    	Number of rows to skip.
 * Outputs:
 * 	Increments the nblank field in the the_hdr struct, so that a Skiplines
 *  	code will be output the next time rle_putrow or rle_putraw is called.
 * Assumptions:
 * 	Only effective when called between rle_putrow or rle_putraw calls (or
 *  	some other routine that follows the same conventions.
 * Algorithm:
 *	[None]
 */
void
rle_skiprow( the_hdr, nrow )
rle_hdr *the_hdr;
int nrow;
{
    the_hdr->priv.put.nblank += nrow;
}


/*****************************************************************
 * TAG( rle_put_init )
 * 
 * Initialize the header structure for writing scanlines. 
 * Inputs:
 *	[None]
 * Outputs:
 * 	the_hdr:	Private portions initialized for output.
 * Assumptions:
 *	[None]
 * Algorithm:
 *	[None]
 */
void
rle_put_init( the_hdr )
register rle_hdr *the_hdr;
{
    the_hdr->dispatch = RUN_DISPATCH;
    the_hdr->priv.put.nblank = 0;	/* Reinit static vars */
    /* Would like to be able to free previously allocated storage,
     * but can't count on a non-NULL value being a valid pointer.
     */
    the_hdr->priv.put.brun = NULL;
    the_hdr->priv.put.fileptr = 0;

    /* Only save alpha if alpha AND alpha channel bit are set. */
    if ( the_hdr->alpha )
	the_hdr->alpha = (RLE_BIT( *the_hdr, -1 ) != 0);
    else
	RLE_CLR_BIT( *the_hdr, -1 );
}

/*****************************************************************
 * TAG( rle_put_setup )
 * 
 * Initialize for writing RLE, and write header to output file.
 * Inputs:
 * 	the_hdr:	Describes output image.
 * Outputs:
 * 	the_hdr:	Initialized.
 * Assumptions:
 *	Lots of them.
 * Algorithm:
 *	[None]
 */
void
rle_put_setup( the_hdr )
register rle_hdr * the_hdr;
{
    rle_put_init( the_hdr );
    Setup();
}

/*ARGSUSED*/
void
DefaultBlockHook(the_hdr)
rle_hdr * the_hdr;
{
    					/* Do nothing */
}

/*****************************************************************
 * TAG( rle_puteof )
 * Write an EOF code into the output file.
 */
void
rle_puteof( the_hdr )
register rle_hdr * the_hdr;
{
    /* Don't puteof twice. */
    if ( the_hdr->dispatch == NO_DISPATCH )
	return;
    PutEof();
    fflush( the_hdr->rle_file );
    /* Free storage allocated by rle_put_init. */
    if ( the_hdr->priv.put.brun != NULL )
    {
	free( the_hdr->priv.put.brun );
	the_hdr->priv.put.brun = NULL;
    }
    /* Signal that puteof has been called. */
    the_hdr->dispatch = NO_DISPATCH;
}

#ifndef FASTRUNS
/*****************************************************************
 * TAG( same_color )
 * 
 * Determine if the color at the given index position in the scan rows
 * is the same as the background color.
 * Inputs:
 * 	index:	    Index to the pixel position in each row.
 *	rows:	    array of pointers to the scanlines
 *	bg_color:   the background color
 *	ncolors:    number of color elements/pixel
 * Outputs:
 * 	TRUE if the color at row[*][i] is the same as bg_color[*].
 * Assumptions:
 *	[None]
 * Algorithm:
 *	[None]
 */
static int
same_color( index, rows, bg_color, ncolors, bits )
register rle_pixel *rows[];
register int bg_color[];
char *bits;
{
    register int i;

    for ( i = 0; i < ncolors; i++, bits++ )
	if ( *bits &&
	     rows[i][index] != bg_color[i] )
	    return 0;
    return 1;				/* all the same */
}
#endif /* !FASTRUNS */

/*****************************************************************
 * TAG( findruns )
 * 
 * Find runs not a given color in the row.
 * Inputs:
 * 	row:		Row of pixel values
 *	rowlen:		Number of pixels in the row.
 *	color:		Color to compare against.
 *	nrun:		Number of runs already found (in different colors).
 *	brun:		Runs found in other color channels already.
 * Outputs:
 * 	brun:		Modified to reflect merging of runs in this color.
 *	Returns number of runs in brun.
 * Assumptions:
 *
 * Algorithm:
 * 	Search for occurences of pixels not of the given color outside the
 *	runs already found.  When some are found, add a new run or extend
 *	an existing one.  Adjacent runs with fewer than two pixels intervening
 *	are merged.
 */
static int
findruns( row, rowlen, color, nrun, brun )
register rle_pixel *row;
int rowlen, color, nrun;
short (*brun)[2];
{
    int i = 0, lower, upper;
    register int s, j;

#ifdef DEBUG
    fprintf( stderr, "findruns( " );
    for ( s = 0; s < rowlen; s++ )
	fprintf( stderr, "%2x.%s", row[s], (s % 20 == 19) ? "\n\t" : "" );
    if ( s % 20 != 0 )
	fprintf( stderr, "\n\t" );
    fprintf( stderr, "%d, %d, %d, \n\t", rowlen, color, nrun );
    for ( j = 0; j < nrun; j++ )
	fprintf( stderr, "(%3d,%3d) %s", brun[j][0], brun[j][1],
		(j % 6 == 5) ? "\n\t" : "" );
    fprintf( stderr, ")\n" );
#endif

    while ( i <= nrun )
    {
	/* Assert: 0 <= i <= rowlen
	 * brun[i] is the run following the "blank" space being
	 * searched.  If i == rowlen, search after brun[i-1].
	 */

	/* get lower and upper bounds of search */

	if ( i == 0 )
	    lower = 0;
	else
	    lower = brun[i-1][1] + 1;

	if ( i == nrun )
	    upper = rowlen - 1;
	else
	    upper = brun[i][0] - 1;

#ifdef DEBUG
	fprintf( stderr, "Searching before run %d from %d to %d\n",
		i, lower, upper );
#endif
	/* Search for beginning of run != color */
#if  defined(LOCC)&defined(vax)
	s = upper - skpc( (char *)row + lower, upper - lower + 1, color ) + 1;
#else
	for ( s = lower; s <= upper; s++ )
	    if ( row[s] != color )
		break;
#endif

	if ( s <= upper )	/* found a new run? */
	{
	    if ( s > lower + 1 || i == 0 ) /* disjoint from preceding run? */
	    {
#ifdef DEBUG
		fprintf( stderr, "Found new run starting at %d\n", s );
#endif
		/* Shift following runs up */
		for ( j = nrun; j > i; j-- )
		{
		    brun[j][0] = brun[j-1][0];
		    brun[j][1] = brun[j-1][1];
		}
		brun[i][0] = s;
		nrun++;
	    }
	    else
	    {
		i--;		/* just add to preceding run */
#ifdef DEBUG
		fprintf( stderr, "Adding to previous run\n" );
#endif
	    }

#if defined(LOCC)&defined(vax)
	    s = upper - locc( (char *)row + s, upper - s + 1, color ) + 1;
#else
	    for ( ; s <= upper; s++ )
		if ( row[s] == color )
		    break;
#endif
	    brun[i][1] = s - 1;

#ifdef DEBUG
	    fprintf( stderr, "Ends at %d", s - 1 );
#endif
	    if ( s >= upper && i < nrun - 1 ) /* merge with following run */
	    {
		brun[i][1] = brun[i+1][1];
		/* move following runs back down */
		for ( j = i + 2; j < nrun; j++ )
		{
		    brun[j-1][0] = brun[j][0];
		    brun[j-1][1] = brun[j][1];
		}
		nrun--;
#ifdef DEBUG
		fprintf( stderr, ", add to next run" );
#endif
	    }
#ifdef DEBUG
	    putc( '\n', stderr );
#endif
	}
	
	/* Search in next space */
	i++;
    }

    return nrun;
}


/*****************************************************************
 * TAG( rgb_to_bw )
 * 
 * Perform the NTSC Y transform on RGB data to get B&W data.
 * Inputs:
 * 	red_row, green_row, blue_row:	Given RGB pixel data.
 *	rowlen:	    Number of pixels in the rows.
 * Outputs:
 * 	bw_row:	    Output B&W data.  May coincide with one of the
 *		    inputs.
 * Assumptions:
 *	[None]
 * Algorithm:
 * 	BW = .30*R + .59*G + .11*B
 */
void
rgb_to_bw( red_row, green_row, blue_row, bw_row, rowlen )
rle_pixel *red_row;
rle_pixel *green_row;
rle_pixel *blue_row;
rle_pixel *bw_row;
int rowlen;
{
    register int x, bw;

    for (x=0; x<rowlen; x++)
    {
	/* 68000 won't store float > 127 into byte? */
	/* HP compiler blows it */
	bw = .30*red_row[x] + .59*green_row[x] + .11*blue_row[x];
	bw_row[x] = bw;
    }
}

#ifdef LOCC
/*ARGSUSED*/
locc( p, l, c )
register char *p;
register int l;
register int c;
{
    asm( "locc	r9,r10,(r11)" );
#ifdef lint
    c = (int) p;		/* why doesn't ARGSUSED work? */
    l = c;
    return l;			/* Needs return value, at least */
#endif
}

/*ARGSUSED*/
skpc( p, l, c )
register char *p;
register int l;
register int c;
{
    asm( "skpc r9,r10,(r11)" );
#ifdef lint
    c = (int) p;		/* why doesn't ARGSUSED work? */
    l = c;
    return l;			/* Needs return value, at least */
#endif
}
#endif
