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
*    provided with the Original Code and Modifications, and is also707
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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#include "guiwind.h"
#include <stdlib.h>
#include <string.h>
#include "guiutil.h"
#include "guixutil.h"
#include "guicontr.h"
#include "guicolor.h"
#include "guistyle.h"
#include "guixdlg.h"
#include "guitextm.h"
#include "guixscal.h"
#include "guiscale.h"
#include "guistr.h"
#include "guixwind.h"
//#include "guixhook.h"
#include "ctl3dcvr.h"
#include "guipaint.h"
#include "guimapky.h"
#include "guirdlg.h"
#ifndef __OS2_PM__
#include "windlg.h"
#else
#include "os2dlg.h"
#endif


/* Local Window callback functions prototypes */
WINEXPORT WPI_DLGRESULT CALLBACK GUIInitDialogFuncDlgProc( HWND hwnd, WPI_MSG message, WPI_PARAM1 wparam, WPI_PARAM2 lparam );

extern  bool            EditControlHasFocus;

static  char            *Font           = NULL;         /* name of font used in dialogs  */
static  int             FontPointSize   = 0;            /* point size of fonts used in dialogs   */
static  WPI_TEXTMETRIC  GUIDialogtm;                    /* tm of dialog font */
static  gui_coord       SizeDialog      = { 128, 128 }; /* of test dialog        */
static  gui_coord       SizeScreen      = { 0, 0 };     /* of test dialog        */

extern  controls_struct GUIControls[];

void GUISetJapanese( void )
{
#ifndef __OS2_PM__
    char *  newfont;

    if( GUIIsDBCS() ) {
  #if 0
        newfont = "";
        FontPointSize = 0;
  #else
        newfont = "�l�r ����";
        FontPointSize = 10;
  #endif
        if( Font != NULL ) {
            GUIMemFree( Font );
        }
        Font = GUIStrDup( newfont, NULL );
    }
#endif
}

/*
 * GUIInitControl
 */

void GUIInitControl( control_item *item, gui_window *wnd, gui_ctl_id *focus_id )
{
    HWND        ctrl;

    if( !item->hwnd ) {
        item->hwnd = _wpi_getdlgitem( wnd->hwnd, item->id );
    }
    ctrl = item->hwnd;
    if( ( focus_id != NULL ) && item->style & GUI_FOCUS ) {
        *focus_id = item->id;
    }
    /* will subclass if required */
    item->win_call_back = GUIDoSubClass( ctrl, item->control_class );
    (void)CvrCtl3dSubclassCtl( ctrl );
    switch( item->control_class ) {
    case GUI_CHECK_BOX :
    case GUI_RADIO_BUTTON :
        if( item->checked ) {
            GUISendMessage( ctrl, BM_SETCHECK, (WPI_PARAM1)true, 0 );
        }
        break;
    case GUI_EDIT_COMBOBOX :
        GUISetText( wnd, item->id, item->text );
        break;
    }
}

/*
 * InitDialog -- initialize the check boxes and radio buttons on the
 *               dialog box.  Also set the HWND properly for the dialog
 *               box and all of its controls.  Add text to list boxes and
 *               combo boxes.
 */

static bool InitDialog( gui_window *wnd )
{
    control_item        *item;
    gui_ctl_id          focus_id;

    focus_id = 0;
    for( item = wnd->controls; item != NULL; item = item->next ) {
        GUIInitControl( item, wnd, &focus_id );
    }
    if( focus_id != 0 ) {
        GUISetFocus( wnd, focus_id );
    }
    GUISetRowCol( wnd, NULL );
    GUIEVENTWND( wnd, GUI_INIT_DIALOG, NULL );
    wnd->flags |= SENT_INIT;
   /* must return false or Windows will set input focus to the
    * first control with a group style
    */
    return( false );
}

bool GUIProcessControlNotification( gui_ctl_id id, int wNotify, gui_window *wnd )
{
    unsigned            check;
    control_item        *item;
    HWND                cntl;

    if( wnd == NULL ) {
        return( false );
    }

    item = GUIGetControlByID( wnd, id );

    if( item != NULL ) {
        switch( item->control_class ) {
        case GUI_EDIT :
        case GUI_EDIT_MLE :
            switch( wNotify ) {
            case EN_SETFOCUS :
                EditControlHasFocus = true;
                break;
            case EN_KILLFOCUS :
                EditControlHasFocus = false;
                GUIEVENTWND( wnd, GUI_CONTROL_NOT_ACTIVE, &id );
                break;
            }
            break;
        case GUI_RADIO_BUTTON :
        case GUI_CHECK_BOX :
            // if this dialog was created from a resource then we
            // assume that the creator of said resource set up the
            // tab and cursor groups with a dialog editor
            if( ( wnd->flags & IS_RES_DIALOG ) == 0 ) {
                if( item->control_class == GUI_RADIO_BUTTON ) {
                    GUICheckRadioButton( wnd, id );
                } else {
                    cntl = _wpi_getdlgitem( wnd->hwnd, id );
                    check = (unsigned)GUISendMessage( cntl, BM_GETCHECK, 0, 0 );
                    if( item->style & GUI_CONTROL_3STATE ) {
                        check = ( ( check & 3 ) + 1 ) % 3;
                        GUISendMessage( cntl, BM_SETCHECK, (WPI_PARAM1)check, 0 );
                    } else {
                        GUISendMessage( cntl, BM_SETCHECK, (WPI_PARAM1)!check, 0 );
                    }
                }
            }
            /* fall through */
        case GUI_PUSH_BUTTON :
        case GUI_DEFPUSH_BUTTON :
            switch( wNotify ) {
            case BN_CLICKED :
                GUIEVENTWND( wnd, GUI_CONTROL_CLICKED, &id );
                return( true );
            case BN_DOUBLECLICKED :
                GUIEVENTWND( wnd, GUI_CONTROL_DCLICKED, &id );
                return( true );
            }
            break;
        case GUI_LISTBOX :
            switch( wNotify ) {
            case LBN_SELCHANGE :
                GUIEVENTWND( wnd, GUI_CONTROL_CLICKED, &id );
                return( true );
            case LBN_DBLCLK :
                GUIEVENTWND( wnd, GUI_CONTROL_DCLICKED, &id );
                return( true );
            case LBN_KILLFOCUS :
                GUIEVENTWND( wnd, GUI_CONTROL_NOT_ACTIVE, &id );
                return( true );
            }
            break;
        case GUI_COMBOBOX :
        case GUI_EDIT_COMBOBOX :
            switch( wNotify ) {
            case CBN_SELCHANGE :
                GUIEVENTWND( wnd, GUI_CONTROL_CLICKED, &id );
                return( true );
#ifndef __OS2_PM__
            case CBN_DBLCLK :
                GUIEVENTWND( wnd, GUI_CONTROL_DCLICKED, &id );
                return( true );
#endif
            case CBN_KILLFOCUS :
                GUIEVENTWND( wnd, GUI_CONTROL_NOT_ACTIVE, &id );
                return( true );
            }
            break;
        }
    }

    return( false );
}

/*
 * GUIProcessControlMsg
 */

bool GUIProcessControlMsg( WPI_PARAM1 wparam, WPI_PARAM2 lparam, gui_window *wnd, bool *pret )
{
    gui_ctl_id  id;
    bool        ret;
#ifndef __OS2_PM__
    bool        rc;
    int         notify_code;

    lparam=lparam;

    ret = false;

    id = LOWORD( wparam );
    notify_code = GET_WM_COMMAND_CMD( wparam, lparam );
    switch( notify_code ) {
    case EN_KILLFOCUS :
    case CBN_KILLFOCUS :
    case BN_CLICKED :
    case BN_DOUBLECLICKED :  /* same as LBN_KILLFOCUS */
    case LBN_SELCHANGE :     /* same as CBN_SELCHANGE */
    case LBN_DBLCLK :        /* same as CBN_DCLICK */
        rc = GUIProcessControlNotification( id, notify_code, wnd );
        if( pret != NULL ) {
            *pret = rc;
        }
        ret = true;
        break;
    }
#else
    pret = pret;

    ret = false;

    id = _wpi_getid( wparam );
    if( SHORT1FROMMP( lparam ) == CMDSRC_PUSHBUTTON ) {
        GUIEVENTWND( wnd, GUI_CONTROL_CLICKED, &id );
        ret = true;
    }
#endif
    return( ret );
}

/*
 * GUIDialogDlgProc - callback function for all dynamically created dialog
 *                 boxes
 */

WPI_DLGRESULT CALLBACK GUIDialogDlgProc( HWND hwnd, WPI_MSG message, WPI_PARAM1 wparam, WPI_PARAM2 lparam )
{
    gui_ctl_id          id;
    bool                escape_pressed;
    gui_window          *wnd;
    bool                msg_processed;
    bool                ret;
    gui_coord           size;
    WPI_POINT           pnt;
    HWND                child;
    HWND                hfocus;
    control_item        *item;
    gui_event           gui_ev;
    gui_key_state       key_state;
#ifdef __OS2_PM__
    WORD                key_flags;
#endif

    msg_processed = false;
    ret = false;

    if( message == WM_INITDIALOG ) {
        wnd = (gui_window *)lparam;
        wnd->hwnd = hwnd;
        wnd->hwnd_frame = hwnd;
#ifdef __OS2_PM__
        wnd->hwnd_pinfo.normal_pres =
            _wpi_createos2normpres( GUIMainHInst, hwnd );
#endif
        _wpi_getclientrect( hwnd, &wnd->hwnd_client_rect );
        wnd->root_client_rect = wnd->hwnd_client_rect;
        GUIInsertCtrlWnd( wnd );
        if( wnd->flags & IS_RES_DIALOG ) {
            GUIInsertResDialogControls( wnd );
        }
    } else {
        wnd = GUIGetCtrlWnd( hwnd );
    }

    switch( message ) {
    case WM_SIZE :
        if( wnd ) {
            _wpi_getclientrect( hwnd, &wnd->hwnd_client_rect );
            wnd->root_client_rect = wnd->hwnd_client_rect;
            size.x = _wpi_getwmsizex( wparam, lparam );
            size.y = _wpi_getwmsizey( wparam, lparam );
            GUIScreenToScaleR( &size );
            GUISetRowCol( wnd, &size );
            GUIEVENTWND( wnd, GUI_RESIZE, &size );
        }
        break;
#if defined(__NT__)
    case WM_CTLCOLORBTN :
    case WM_CTLCOLORDLG :
    //case WM_CTLCOLORLISTBOX :
    case WM_CTLCOLORSTATIC :
    //case WM_CTLCOLOREDIT :
        // May come along before WM_INITDIALOG
        if( wnd != NULL ) {
            SetBkColor( (HDC)wparam, GetNearestColor( (HDC)wparam,
                        GUIGetBack( wnd, GUI_BACKGROUND ) ) );
            return( (WPI_DLGRESULT)wnd->bk_brush );
        }
        break;
#endif
    case WM_SYSCOLORCHANGE:
        (void)CvrCtl3dColorChange();
        break;
    case WM_INITDIALOG:
        /* must return false or Windows will set input focus to the
         * first control with a group style
         */
        msg_processed = true;
        hfocus = _wpi_getfocus();
        InitDialog( wnd );
#ifdef __OS2_PM__
        if( hfocus != _wpi_getfocus() ) {
            ret = true;
        }
#endif
        if( wnd->flags & IS_RES_DIALOG ) {
            if( hfocus == _wpi_getfocus() ) {
                // if the focus did not change then let the
                // windowing system set the focus
                ret = true;
            }
        }
        break;
#ifdef __OS2_PM__
    case WM_CONTROL :
        GUIProcessControlNotification( SHORT1FROMMP( wparam ), SHORT2FROMMP( wparam ), wnd );
        break;
    case WM_RBUTTONDOWN :
        WPI_MAKEPOINT( wparam, lparam, pnt );
        child = PM1632WinWindowFromPoint( hwnd, &pnt, false );
        item = NULL;
        if( child ) {
            item = GUIGetControlByHwnd( wnd, child );
            if( item != NULL && item->id != 0 ) {
                msg_processed = GUIEVENTWND( wnd, GUI_CONTROL_RCLICKED, &item->id );
            }
        }
        if( item == NULL || item->id == 0 ) {
            msg_processed |= !SendPointEvent( wparam, lparam, wnd, GUI_RBUTTONDOWN, false );
        }
        break;
#else
    case WM_PARENTNOTIFY :
        if( LOWORD(wparam) == WM_RBUTTONDOWN ) {
            WPI_MAKEPOINT( wparam, lparam, pnt );
            _wpi_mapwindowpoints( hwnd, HWND_DESKTOP, &pnt, 1 );
            child = _wpi_windowfrompoint( pnt );
            item = GUIGetControlByHwnd( wnd, child );
            if( item != NULL && item->id != 0 && ( _wpi_getparent( child ) == hwnd ) ) {
                msg_processed = GUIEVENTWND( wnd, GUI_CONTROL_RCLICKED, &item->id );
            }
        }
        break;
    case WM_RBUTTONDOWN:
        msg_processed = !SendPointEvent( wparam, lparam, wnd, GUI_RBUTTONDOWN, false );
        break;
#endif
    case WM_RBUTTONUP:
        msg_processed = !SendPointEvent( wparam, lparam, wnd, GUI_RBUTTONUP, false );
        break;
    case WM_RBUTTONDBLCLK:
        msg_processed = !SendPointEvent( wparam, lparam, wnd, GUI_RBUTTONDBLCLK, false );
        break;
    case WM_LBUTTONDOWN:
        msg_processed = !SendPointEvent( wparam, lparam, wnd, GUI_LBUTTONDOWN, false );
        break;
    case WM_LBUTTONUP:
        msg_processed = !SendPointEvent( wparam, lparam, wnd, GUI_LBUTTONUP, false );
        break;
    case WM_LBUTTONDBLCLK:
        msg_processed = !SendPointEvent( wparam, lparam, wnd, GUI_LBUTTONDBLCLK, false );
        break;
    case WM_COMMAND :
        escape_pressed = false;
        id = _wpi_getid( wparam );
        if( _wpi_ismenucommand( wparam, lparam ) ) {  /* from menu */
            if( wnd != NULL ) {
#ifndef __OS2_PM__
                escape_pressed = ( !GET_WM_COMMAND_CMD( wparam, lparam ) && ( id == IDCANCEL ) );
#endif
                if( !escape_pressed ) {
                    GUIEVENTWND( wnd, GUI_CONTROL_CLICKED, &id );
                }
            }
        } else {
#ifdef __OS2_PM__
            escape_pressed = (((ULONG)lparam == 1) && (id == IDCANCEL));
#else
            escape_pressed = ( !GET_WM_COMMAND_CMD( wparam, lparam ) && ( id == IDCANCEL ) );
#endif
            if( !escape_pressed ) {
                GUIProcessControlMsg( wparam, lparam, wnd, &ret );
            }
        }
        if( escape_pressed ) {
            GUIEVENTWND( wnd, GUI_DIALOG_ESCAPE, NULL );
            GUICloseDialog( wnd );
        }
        msg_processed = true;
        break;
    case WM_CLOSE :
        _wpi_enddialog( hwnd, TRUE );
        msg_processed = true;
        break;
    case WM_MOVE :
        GUIInvalidatePaintHandles( wnd );
        break;
    case WM_DESTROY :
        if( wnd != NULL ) {
            GUIEVENTWND( wnd, GUI_DESTROY, NULL );
        }
#ifndef __OS2_PM__
        break;
    case WM_NCDESTROY :
#endif
        if( wnd != NULL ) {
            GUIFreeWindowMemory( wnd, false, true );
        }
        break;
    case WM_PAINT :
        if( !_wpi_isiconic( hwnd ) ) {
            GUIPaint( wnd, hwnd, true );
            msg_processed = true;
        }
        break;
#ifndef __OS2_PM__
    case WM_VKEYTOITEM :
        msg_processed = true;
        GUIGetKeyState( &key_state.state );
        if( !GUIWindowsMapKey( wparam, lparam, &key_state.key ) ) {
            return( -1 );
        }
        ret = GUIEVENTWND( wnd, GUI_KEYTOITEM, &key_state );
        break;
#endif
    // the following code allows GUI dialogs to receive key msgs
    // this code must eventually be rationalized with the code in
    // guixwind.c
#ifndef __OS2_PM__
    case WM_SYSKEYDOWN :
    case WM_SYSKEYUP :
    case WM_KEYDOWN :
    case WM_KEYUP :
        gui_ev = GUI_KEYUP;
        if( ( message == WM_KEYDOWN  ) || ( message == WM_SYSKEYDOWN  ) ) {
            gui_ev = GUI_KEYDOWN;
        }
        GUIGetKeyState( &key_state.state );
        if( GUIWindowsMapKey( wparam, lparam, &key_state.key ) ) {
            if( GUIEVENTWND( wnd, gui_ev, &key_state ) ) {
                return( false );
            }
        }
        break;
#else
    case WM_CHAR :
        key_flags = SHORT1FROMMP( wparam );
        gui_ev = GUI_KEYDOWN;
        if( key_flags & KC_KEYUP ) {
            gui_ev = GUI_KEYUP;
        }
        GUIGetKeyState( &key_state.state );
        if( GUIWindowsMapKey( wparam, lparam, &key_state.key ) ) {
            if( GUIEVENTWND( wnd, gui_ev, &key_state ) ) {
                return( false );
            }
        }
        break;
#endif
    }

#ifdef __OS2_PM__
    if( !msg_processed ) {
        return( _wpi_defdlgproc( hwnd, message, wparam, lparam ) );
    }
#endif
    _wpi_dlgreturn( ret );
}

/*
 * ToDialogUnits -- convert the given coordinate to dialog units
 */

static void ToDialogUnits( gui_coord *coord )
{
    if( coord != NULL ) {
        coord->x = GUIMulDiv( coord->x, SizeDialog.x, SizeScreen.x );
        coord->y = GUIMulDiv( coord->y, SizeDialog.y, SizeScreen.y );
    }
}

/*
 * AdjustToDialogUnits -- convert the given coordinate to dialog units and
 *                        adjust
 */

static void AdjustToDialogUnits( gui_coord *coord )
{
    ToDialogUnits( coord );
}

static void AdjustForFrame( gui_coord *pos, gui_coord *size )
{
#ifndef __OS2_PM__
    int xframe, yframe, ycaption;

    xframe = 0;
    yframe = 0;
    ycaption = 0;
    if( ( pos != NULL )  || ( size != NULL ) ) {
        xframe   = _wpi_getsystemmetrics( SM_CXDLGFRAME );
        yframe   = _wpi_getsystemmetrics( SM_CYDLGFRAME );
        ycaption = _wpi_getsystemmetrics( SM_CYCAPTION );
    }

    if( pos != NULL ) {
        pos->x += xframe;
        pos->y += ( yframe + ycaption );
    }

    if( size != NULL ) {
        size->x -= ( 2 * xframe );
        size->y -= ( 2 * yframe + ycaption );
    }
#else
    pos = pos;
    size = size;
#endif
}

static void GUIDlgCalcLocation( gui_rect *rect, gui_coord *pos, gui_coord *size )
{
    pos->x = rect->x;
    pos->y = rect->y;
    size->x = rect->width;
    size->y = rect->height;
    GUIScaleToScreenR( pos );
    GUIScaleToScreenR( size );
}

/*
 * GUIXCreateDialog -- create a dialog with the specified controls
 */

bool GUIXCreateDialog( gui_create_info *dlg_info, gui_window *wnd,
                       int num_controls, gui_control_info *controls_info,
                       bool sys, res_name_or_id dlg_id )
{
    gui_coord           pos, size;
    gui_coord           parent_pos;
    TEMPLATE_HANDLE     old_dlgtemplate;
    TEMPLATE_HANDLE     new_dlgtemplate;
    int                 i;
    const char          *captiontext;
    long                style;
    HWND                parent_hwnd;
    gui_control_info    *ctl_info;
    bool                got_first_focus;
    bool                in_group;
    LONG                dlg_style;
#ifdef __OS2_PM__
    void                *pctldata;
    int                 pctldatalen;
    ENTRYFDATA          edata;
#endif
    size_t              templatelen;

    wnd->flags |= IS_DIALOG;
    wnd->parent = dlg_info->parent;
    wnd->root_pinfo.force_count = NUMBER_OF_FORCED_REPAINTS;
    wnd->hwnd_pinfo.force_count = NUMBER_OF_FORCED_REPAINTS;

    parent_hwnd = HWND_DESKTOP;
    if( dlg_info->parent != NULL ) {
        parent_hwnd = dlg_info->parent->hwnd;
    }

    if( !GUISetupStruct( wnd, dlg_info, &parent_pos, &size, parent_hwnd, NULL ) ) {
        return( false );
    }

    wnd->flags |= HAS_CAPTION;
    if( dlg_id != NULL ) {
        wnd->flags |= IS_RES_DIALOG;
        return( GUIDoCreateResDialog( dlg_id, parent_hwnd, (void *)wnd ) );
    }

    AdjustForFrame( &parent_pos, &size );
    AdjustToDialogUnits( &parent_pos );
    AdjustToDialogUnits( &size );

    if( sys ) {
        dlg_style = SYSTEM_MODAL_STYLE;
    } else {
        dlg_style = MODAL_STYLE;
    }

    old_dlgtemplate = DialogTemplate( dlg_style | DS_SETFONT,
                           parent_pos.x, parent_pos.y, size.x, size.y,
                           LIT( Empty ), LIT( Empty ), dlg_info->title,
                           FontPointSize, Font, &templatelen );
    if( old_dlgtemplate == NULL ) {
        return( false );
    }

    got_first_focus = false;
    in_group = false;
    for( i = 0; i < num_controls; i++ ) {
        ctl_info = &controls_info[i];
#ifdef __OS2_PM__
        pctldata = NULL;
        pctldatalen = 0;
        if( ctl_info->control_class == GUI_EDIT ) {
            edata.cb = sizeof( ENTRYFDATA );
            edata.cchEditLimit = 2048;
            edata.ichMinSel = 0;
            edata.ichMaxSel = 0;
            pctldata = &edata;
            pctldatalen = sizeof( edata );
        }
#endif
        GUIDlgCalcLocation( &ctl_info->rect, &pos, &size );
        AdjustToDialogUnits( &pos );
        AdjustToDialogUnits( &size );
        if( ctl_info->text == NULL ) {
            captiontext = LIT( Empty );
        } else {
            captiontext = ctl_info->text;
        }
        style = GUISetControlStyle( ctl_info );
        if( !in_group ) {
            style |= WS_GROUP;
        }
        if( style & GUI_GROUP ) {
            in_group = !in_group;
        }
#ifdef __OS2_PM__
        new_dlgtemplate = AddControl( old_dlgtemplate, pos.x, pos.y, size.x, size.y, ctl_info->id,
                          style, GUIControls[ctl_info->control_class].classname, captiontext,
                          pctldata, (BYTE)pctldatalen, &templatelen );
#else
        new_dlgtemplate = AddControl( old_dlgtemplate, pos.x, pos.y, size.x, size.y, ctl_info->id,
                          style, GUIControls[ctl_info->control_class].classname, captiontext,
                          NULL, 0, &templatelen );
#endif
        if( new_dlgtemplate == NULL  ) {
            GlobalFree( old_dlgtemplate );
            return( false );
        }
        if( GUIControlInsert( wnd, ctl_info->control_class, NULLHANDLE, ctl_info, NULL ) == NULL ) {
            GlobalFree( old_dlgtemplate );
            return( false );
        }
        old_dlgtemplate = new_dlgtemplate;
    }
    new_dlgtemplate = DoneAddingControls( old_dlgtemplate );
    DynamicDialogBox( GUIDialogDlgProc, GUIMainHInst, parent_hwnd, new_dlgtemplate, (WPI_PARAM2)wnd );
    return( true );
}

/*
 * GUICloseDialog -- close the given dialog, freeing all associated memory
 */

void GUICloseDialog( gui_window * wnd )
{
    GUISendMessage( wnd->hwnd, WM_CLOSE, 0, 0 );
}

/****************************************************************************/
/* From here down is used to get sizing right, called by GUIWndInit         */
/****************************************************************************/

static WPI_FONT         DlgFont;

/*
 * GUIInitDialogFuncDlgProc - callback function the test dialog box used to get the
 *                     dialog box font info and client size info
 *
 */

WPI_DLGRESULT CALLBACK GUIInitDialogFuncDlgProc( HWND hwnd, WPI_MSG message, WPI_PARAM1 wparam, WPI_PARAM2 lparam )
{
    WPI_PRES            hdc;
    WPI_RECT            rect;
    bool                ret;

    lparam = lparam;
    ret    = false;

    switch( message ) {
#ifndef __OS2_PM__
    case WM_SETFONT :
        DlgFont = (WPI_FONT)wparam;
        break;
#endif
    case WM_INITDIALOG:
        hdc = _wpi_getpres( hwnd );
#ifndef __OS2_PM__
        _wpi_selectfont( hdc, DlgFont );
#else
        DlgFont = (WPI_FONT)NULL;
#endif
        _wpi_gettextmetrics( hdc, &GUIDialogtm );
        _wpi_releasepres( hwnd, hdc );

        _wpi_getclientrect( hwnd, &rect );
        SizeScreen.x = _wpi_getwidthrect( rect );
        SizeScreen.y = _wpi_getheightrect( rect );
        _wpi_enddialog( hwnd, TRUE );
        break;
#ifdef __OS2_PM__
    default:
        return( _wpi_defdlgproc( hwnd, message, wparam, lparam ) );
#endif
    }

    _wpi_dlgreturn( ret );
}

void GUIFiniDialog( void )
{
    if( Font != NULL ) {
        GUIMemFree( Font );
        Font = NULL;
    }
}

void GUIInitDialog( void )
{
    TEMPLATE_HANDLE     old_dlgtemplate;
    TEMPLATE_HANDLE     new_dlgtemplate;
    char                *cp;
    bool                font_set;
    size_t              templatelen;

    font_set = false;
#ifdef __OS2_PM__
    Font = GUIStrDup( LIT( OS2_Dialog_Font ), NULL );
#else
    Font = GUIStrDup( LIT( Windows_Dialog_Font ), NULL );
#endif
    if( Font ) {
        cp = strrchr( Font, '.' );
        if( cp ) {
            *cp = '\0';
            cp++;
            FontPointSize = atoi( cp );
            font_set = true;
        }
    }

    if( !font_set ) {
        FontPointSize = 0;
        if( Font != NULL ) {
            GUIMemFree( Font );
        }
        Font = GUIStrDup( "", NULL );
    }

    // create a dialog of known dialog units and use the resulting
    // size of the client area to scale subsequent screen units
    old_dlgtemplate = DialogTemplate( MODAL_STYLE | DS_SETFONT,
                           SizeDialog.x, SizeDialog.y,
                           SizeDialog.x, SizeDialog.y, LIT( Empty ),
                           LIT( Empty ), LIT( Empty ), FontPointSize, Font, &templatelen );
    if( old_dlgtemplate != NULL ) {
        new_dlgtemplate = DoneAddingControls( old_dlgtemplate );
        DynamicDialogBox( GUIInitDialogFuncDlgProc, GUIMainHInst, NULLHANDLE, new_dlgtemplate, 0 );
    }
}

/*
 * ScaleGrow -- adjust the estimate given to the user according to the
 *              known discrepency beween a dialog size asked for and
 *              what was created in this font.
 */

#ifdef THIS_CODE_IS_STUPID
static void ScaleGrow( gui_coord * coord )
{
    if( coord != NULL ) {
        coord->x = GUIMulDiv( coord->x, ActualSize.x, ExpectedSize.x );
        coord->y = GUIMulDiv( coord->y, ActualSize.y, ExpectedSize.y );
    }
}
#endif

/*
 * GUIGetDlgTextMetrics -- get the metrics of the text used in dialog boxes
 */

void GUIGetDlgTextMetrics( gui_text_metrics * metrics )
{
    if( metrics != NULL ) {
        GUISetMetrics( metrics, &GUIDialogtm );
#ifdef THIS_CODE_IS_STUPID
        ScaleGrow( &metrics->avg );
        ScaleGrow( &metrics->max );
#endif
    }
}
