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
* Description:  Utility routines for semantic actions.
*
****************************************************************************/


#include "global.h"
#include "rcerrors.h"
#include "rcrtns.h"
#include "semutil.h"
#include "rccore.h"

#include "clibext.h"


void ReportCopyError( RcStatus status, int read_msg, const char *filename, int err_code )
{
    switch( status ) {
    case RS_READ_ERROR:
        RcError( read_msg, filename, strerror( err_code ) );
        break;
    case RS_READ_INCMPLT:
        RcError( ERR_UNEXPECTED_EOF, filename );
        break;
    case RS_WRITE_ERROR:
        RcError( ERR_WRITTING_RES_FILE, CurrResFile.filename, strerror( err_code ) );
        break;
    default:
        RcError( ERR_INTERNAL, INTERR_UNKNOWN_RCSTATUS );
        break;
    }
}

/*
 * CopyData -
 */
RcStatus CopyData( WResFileOffset offset, uint_32 length, WResFileID fid,
                void *buff, unsigned buffsize, int *err_code )
/***************************************************************************/
{
    WResFileSSize   numread;

    if( RESSEEK( fid, offset, SEEK_SET ) == -1 ) {
        *err_code = errno;
        return( RS_READ_ERROR );
    }

    while( length > 0 ) {
        if( buffsize > length )
            buffsize = (unsigned)length;
        numread = RESREAD( fid, buff, buffsize );
        if( (WResFileSize)numread != buffsize ) {
            *err_code = errno;
            return( RESIOERR( fid, numread ) ? RS_READ_ERROR : RS_READ_INCMPLT );
        }
        length -= buffsize;
        if( (WResFileSize)RESWRITE( CurrResFile.fid, buff, buffsize ) != buffsize ) {
            *err_code = errno;
            return( RS_WRITE_ERROR );
        }
    }

    return( RS_OK );
} /* CopyData */
