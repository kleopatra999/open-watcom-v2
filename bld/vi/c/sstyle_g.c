/****************************************************************************
*
*                            Open Watcom Project
*
*    Portions Copyright (c) 1983-2002 Sybase, Inc. All Rights Reserved.
*
*  ========================================================================
*
*    This file contains Original Code and/or Modifications of Original
*    Code as defined in and that are subject to the Sybase Open Watcom
*    Public License version 1.0 (the 'License'). You may not use this file
*    except in compliance with the License. BY USING THIS FILE YOU AGREE TO
*    ALL TERMS AND CONDITIONS OF THE LICENSE. A copy of the License is
*    provided with the Original Code and Modifications, and is also
*    available at www.sybase.com/developer/opensource.
*
*    The Original Code and all software distributed under the License are
*    distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
*    EXPRESS OR IMPLIED, AND SYBASE AND ALL CONTRIBUTORS HEREBY DISCLAIM
*    ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
*    NON-INFRINGEMENT. Please see the License for the specific language
*    governing rights and limitations under the License.
*
*  ========================================================================
*
* Description:  Support for (W)GML syntax.
*
****************************************************************************/


#include "vi.h"
#include "sstyle.h"
#include "sstyle_g.h"

#include "clibext.h"


/*----- LOCALS -----*/

static  ss_flags_g  flags;
static  char        *firstNonWS;
static  char        *firstChar;


static int isGMLComment( char *string )
{
    if( !strnicmp( string, ".*", 2 ) || !strnicmp( string, ":cmt.", 5 ) ) {
        return( 1 );
    } else {
        return( 0 );
    }
}

void InitGMLLine( char *text )
{
    firstChar = text;
    while( *text != '\0' && isspace( *text ) ) {
        text++;
    }
    firstNonWS = text;
}

static void getWhiteSpace( ss_block *ss_new, char *start )
{
    char    *text = start + 1;

    while( isspace( *text ) ) {
        text++;
    }
    ss_new->type = SE_WHITESPACE;
    ss_new->len = text - start;
}

static void getText( ss_block *ss_new, char *start )
{
    char    *text = start + 1;
    char    save_char;

    // gather up symbol
    while( isalnum( *text ) || (*text == '_') ) {
        text++;
    }
    ss_new->type = SE_IDENTIFIER;

    // see if symbol is a keyword
    if( flags.inGMLKeyword ) {
        save_char = *text;
        *text = '\0';
        if( IsKeyword( start, true ) ) {
            ss_new->type = SE_KEYWORD;
        }
        *text = save_char;
        // only check first word after a ":" (should check all tokens?)
        flags.inGMLKeyword = false;
    }

    ss_new->len = text - start;
}

static void getSymbol( ss_block *ss_new )
{
    ss_new->type = SE_SYMBOL;
    ss_new->len = 1;
}

static void getBeyondText( ss_block *ss_new )
{
    ss_new->type = SE_WHITESPACE;
    ss_new->len = BEYOND_TEXT;
}

static void getGMLComment( ss_block *ss_new, char *start, int skip )
{
    char    *text = start + skip;
    char    comment1;
    char    comment2;
    char    comment3;

    for( ;; ) {
        // check for "-->"
        comment1 = text[0];
        if( comment1 == '\0' ) {
            break;
        }

        comment2 = '\0';
        comment3 = '\0';
        if( comment1 != '\0' ) {
            comment2 = text[1];
        }
        if( comment2 != '\0' ) {
            comment3 = text[2];
        }

        if( comment1 == '-' && comment2 == '-' && comment3 == '>' ) {
            flags.inGMLComment = false;
            text += 3;
            break;
        }
        text++;
    }
    ss_new->type = SE_COMMENT;
    ss_new->len = text - start;
}

static void getString( ss_block *ss_new, char *start, int skip )
{
    char    *nstart = start + skip;
    char    *text = nstart;

    ss_new->type = SE_STRING;
    while( *text != '\0' && *text != '"' ) {
        text++;
    }
    if( *text == '\0' ) {
        // string continued on next line
        flags.inString = true;
    } else {
        text++;
        // definitely finished string
        flags.inString = false;
    }
    ss_new->len = text - start;
}

void InitGMLFlagsGivenValues( ss_flags_g *newFlags )
{
    flags = *newFlags;
}

void GetGMLFlags( ss_flags_g *storeFlags )
{
    *storeFlags = flags;
}

void InitGMLFlags( linenum line_no )
{
    /* unused parameters */ (void)line_no;

    flags.inGMLComment = false;
    flags.inGMLKeyword = false;
    flags.inString = false;
}

void GetGMLBlock( ss_block *ss_new, char *start, int line )
{
    /* unused parameters */ (void)line;

    if( start[0] == '\0' ) {
        if( firstNonWS == start ) {
            // line is empty -
            // do not flag following line as having anything to do
            // with an unterminated "
            flags.inString = false;
        }
        getBeyondText( ss_new );
        return;
    }

    if( isGMLComment( firstChar ) ) {
        getGMLComment( ss_new, start, 0 );
        return;
    }
    if( flags.inGMLComment ) {
        getGMLComment( ss_new, start, 0 );
        return;
    }
    if( flags.inString ) {
        getString( ss_new, start, 0 );
        return;
    }

    if( isspace( start[0] ) ) {
        getWhiteSpace( ss_new, start );
        return;
    }

    switch( start[0] ) {
    case '"':
        getString( ss_new, start, 1 );
        return;
    case ':':
        flags.inGMLKeyword = true;
        break;
    case '.':
        flags.inGMLKeyword = false;
        break;
    }

    if( isalnum( start[0] ) || ( start[0] == '_' ) ) {
        getText( ss_new, start );
    } else {
        getSymbol( ss_new );
    }
}
