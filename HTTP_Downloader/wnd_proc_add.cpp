/*
	HTTP Downloader can download files through HTTP(S) and FTP(S) connections.
	Copyright (C) 2015-2020 Eric Kutcher

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "globals.h"
#include "utilities.h"
#include "connection.h"

#include "lite_ole32.h"
#include "lite_gdi32.h"
#include "lite_uxtheme.h"

#include "list_operations.h"

#include "drag_and_drop.h"
#include "folder_browser.h"

#include "string_tables.h"

#include "wnd_proc.h"

#define MIN_ADVANCED_HEIGHT		555
#define MIN_SIMPLE_HEIGHT		270

#define BTN_DOWNLOAD			1000
#define BTN_ADD_CANCEL			1001
#define EDIT_DOWNLOAD_PARTS		1002
#define EDIT_ADD_SPEED_LIMIT	1003
#define BTN_DOWNLOAD_DIRECTORY	1004
#define CHK_SEND_DATA			1005
#define CHK_SIMULATE_DOWNLOAD	1006
#define BTN_ADVANCED			1007

#define CB_REGEX_FILTER_PRESET	1008
#define BTN_APPLY_FILTER		1009

#define EDIT_ADD_URLS			1010

#define BTN_ADD_DOWNLOAD		1011

#define MENU_ADD_SPLIT_DOWNLOAD	10000
#define MENU_ADD_SPLIT_ADD		10001

HWND g_hWnd_add_urls = NULL;

HWND g_hWnd_edit_add = NULL;

HWND g_hWnd_static_download_directory = NULL;
HWND g_hWnd_download_directory = NULL;
HWND g_hWnd_btn_download_directory = NULL;

HWND g_hWnd_chk_simulate_download = NULL;

HWND g_hWnd_static_download_parts = NULL;
HWND g_hWnd_download_parts = NULL;
HWND g_hWnd_ud_download_parts = NULL;

HWND g_hWnd_static_add_speed_limit = NULL;
HWND g_hWnd_add_speed_limit = NULL;

HWND g_hWnd_static_ssl_version = NULL;
HWND g_hWnd_ssl_version = NULL;

HWND g_hWnd_btn_authentication = NULL;
HWND g_hWnd_static_username = NULL;
HWND g_hWnd_edit_username = NULL;
HWND g_hWnd_static_password = NULL;
HWND g_hWnd_edit_password = NULL;

HWND g_hWnd_advanced_add_tab = NULL;

HWND g_hWnd_static_cookies = NULL;
HWND g_hWnd_edit_cookies = NULL;

HWND g_hWnd_static_headers = NULL;
HWND g_hWnd_edit_headers = NULL;

HWND g_hWnd_chk_send_data = NULL;
HWND g_hWnd_edit_data = NULL;

HWND g_hWnd_chk_show_advanced_options = NULL;

HWND g_hWnd_btn_download = NULL;
HWND g_hWnd_btn_add_download = NULL;
HWND g_hWnd_cancel = NULL;

HWND g_hWnd_static_regex_filter = NULL;
HWND g_hWnd_regex_filter_preset = NULL;
HWND g_hWnd_regex_filter = NULL;
HWND g_hWnd_btn_apply_filter = NULL;

bool g_show_advanced = false;

wchar_t *t_download_directory = NULL;

WNDPROC URLProc = NULL;
WNDPROC AddTabProc = NULL;

bool use_drag_and_drop_add = true;	// Assumes OLE32_STATE_RUNNING is true.
IDropTarget *Add_DropTarget;

bool use_add_split = false;
HMENU g_hMenu_add_split = NULL;

unsigned char add_split_type = 0;	// 0 = Download, 1 = Add

HBRUSH g_add_tab_brush = NULL;
int g_add_tab_width = 0;
bool g_add_use_theme = true;

wchar_t add_limit_tooltip_text[ 32 ];
HWND g_hWnd_add_limit_tooltip = NULL;

LRESULT CALLBACK URLSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
		case WM_CHAR:
		{
			// Replace enter with "\r\n" instead of "\n".
			if ( wParam == VK_RETURN )
			{
				_SendMessageA( hWnd, EM_REPLACESEL, TRUE, ( LPARAM )"\r\n" );

				return 0;
			}
			else
			{
				// Ctrl is down and 'a' has been pressed.
				// Prevents the annoying beep that would happen if we use WM_KEYDOWN.
				if ( wParam == 1 )
				{
					_SendMessageW( hWnd, EM_SETSEL, 0, -1 );	// Select all text.

					return 0;
				}
			}
		}
		break;

		case WM_GETDLGCODE:
		{
			// Don't process the tab key in the edit control. (Allows us to tab between controls)
			if ( wParam == VK_TAB )
			{
				return DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTARROWS;
			}
		}
		break;

		case WM_KEYDOWN:
		{
			if ( hWnd == g_hWnd_edit_add )
			{
				if ( _GetKeyState( VK_CONTROL ) & 0x8000 )
				{
					if ( _GetKeyState( VK_SHIFT ) & 0x8000 && wParam == VK_RETURN )
					{
						// http://a.b
						if ( _SendMessageW( g_hWnd_edit_add, WM_GETTEXTLENGTH, 0, 0 ) >= 10 )
						{
							_SendMessageW( _GetParent( hWnd ), WM_COMMAND, BTN_DOWNLOAD, 0 );
						}

						return 0;
					}
				}
			}
		}
		break;
	}

	return _CallWindowProcW( URLProc, hWnd, msg, wParam, lParam );
}

LRESULT CALLBACK AddTabSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
		case WM_CTLCOLORSTATIC:
		{
			if ( g_add_use_theme && _IsThemeActive() == TRUE && ( HWND )lParam != g_hWnd_chk_show_advanced_options )
			{
				if ( ( HWND )lParam == g_hWnd_static_cookies ||
					 ( HWND )lParam == g_hWnd_static_headers ||
					 ( HWND )lParam == g_hWnd_chk_send_data )
				{
					_SetBkMode( ( HDC )wParam, TRANSPARENT );

					POINT pt;
					pt.x = 0; pt.y = 0;

					_MapWindowPoints( hWnd, ( HWND )lParam, &pt, 1 );
					_SetBrushOrgEx( ( HDC )wParam, pt.x, pt.y, NULL );

					return ( INT_PTR )g_add_tab_brush;
				}
			}
		}
		break;

		case WM_SIZE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			// Allow our controls to move in relation to the parent window.
			HDWP hdwp = _BeginDeferWindowPos( 6 );

			RECT rc_tab;
			_SendMessageW( hWnd, TCM_GETITEMRECT, 0, ( LPARAM )&rc_tab );

			_DeferWindowPos( hdwp, g_hWnd_static_cookies, HWND_TOP, 10, ( rc_tab.bottom - rc_tab.top ) + 10, rc.right - 20, 15, SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_edit_cookies, HWND_TOP, 10, ( rc_tab.bottom - rc_tab.top ) + 25, rc.right - 20, ( rc.bottom - rc_tab.bottom ) - 35, SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_headers, HWND_TOP, 10, ( rc_tab.bottom - rc_tab.top ) + 10, rc.right - 20, 15, SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_edit_headers, HWND_TOP, 10, ( rc_tab.bottom - rc_tab.top ) + 25, rc.right - 20, ( rc.bottom - rc_tab.bottom ) - 35, SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_chk_send_data, HWND_TOP, 10, ( rc_tab.bottom - rc_tab.top ) + 10, rc.right - 20, 20, SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_edit_data, HWND_TOP, 10, ( rc_tab.bottom - rc_tab.top ) + 30, rc.right - 20, ( rc.bottom - rc_tab.bottom ) - 40, SWP_NOZORDER );

			_EndDeferWindowPos( hdwp );
		}
		break;
		
		case WM_COMMAND:
		{
			if ( LOWORD( wParam ) == CHK_SEND_DATA )
			{
				_EnableWindow( g_hWnd_edit_data, ( _SendMessageW( g_hWnd_chk_send_data, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? TRUE : FALSE ) );
			}

			return 0;
		}
		break;
	}

	return _CallWindowProcW( AddTabProc, hWnd, msg, wParam, lParam );
}


void ShowHideAddTabs( int sw_type )
{
	int index = ( int )_SendMessageW( g_hWnd_advanced_add_tab, TCM_GETCURSEL, 0, 0 );		// Get the selected tab
	if ( index != -1 )
	{
		switch ( index )
		{
			case 0:
			{
				_ShowWindow( g_hWnd_static_cookies, sw_type );
				_ShowWindow( g_hWnd_edit_cookies, sw_type );
			}
			break;

			case 1:
			{
				_ShowWindow( g_hWnd_static_headers, sw_type );
				_ShowWindow( g_hWnd_edit_headers, sw_type );
			}
			break;

			case 2:
			{
				_ShowWindow( g_hWnd_chk_send_data, sw_type );
				_ShowWindow( g_hWnd_edit_data, sw_type );
			}
			break;
		}
	}
}

LRESULT CALLBACK AddURLsWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch ( msg )
    {
		case WM_CREATE:
		{
			DWORD enabled = ( g_use_regular_expressions ? 0 : WS_DISABLED );
			g_hWnd_static_regex_filter = _CreateWindowW( WC_STATIC, ST_V_RegEx_filter_, SS_OWNERDRAW | WS_CHILD | WS_VISIBLE | enabled, 85, 14, 90, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_regex_filter_preset = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_COMBOBOX, NULL, CBS_AUTOHSCROLL | CBS_DROPDOWNLIST | WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_VISIBLE | enabled, 180, 10, 80, 23, hWnd, ( HMENU )CB_REGEX_FILTER_PRESET, NULL, NULL );
			_SendMessageW( g_hWnd_regex_filter_preset, CB_ADDSTRING, 0, ( LPARAM )ST_V_Custom );
			_SendMessageW( g_hWnd_regex_filter_preset, CB_ADDSTRING, 0, ( LPARAM )ST_V_Images );
			_SendMessageW( g_hWnd_regex_filter_preset, CB_ADDSTRING, 0, ( LPARAM )ST_V_Music );
			_SendMessageW( g_hWnd_regex_filter_preset, CB_ADDSTRING, 0, ( LPARAM )ST_V_Videos );

			_SendMessageW( g_hWnd_regex_filter_preset, CB_SETCURSEL, 0, 0 );

			g_hWnd_regex_filter = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, L"", ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE | enabled, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			g_hWnd_btn_apply_filter = _CreateWindowW( WC_BUTTON, ST_V_Apply, WS_CHILD | WS_TABSTOP | WS_VISIBLE | enabled, 0, 0, 0, 0, hWnd, ( HMENU )BTN_APPLY_FILTER, NULL, NULL );


			HWND hWnd_static_urls = _CreateWindowW( WC_STATIC, ST_V_URL_s__, WS_CHILD | WS_VISIBLE, 10, 25, 70, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_edit_add = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, L"", WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL | ES_MULTILINE | WS_HSCROLL | WS_VSCROLL | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )EDIT_ADD_URLS, NULL, NULL );

			_SendMessageW( g_hWnd_edit_add, EM_LIMITTEXT, 0, 0 );	// Maximum size.

			g_hWnd_static_download_directory = _CreateWindowW( WC_STATIC, ST_V_Download_directory_, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_download_directory = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, cfg_default_download_directory, ES_AUTOHSCROLL | ES_READONLY | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_btn_download_directory = _CreateWindowW( WC_BUTTON, ST_V_BTN___, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_DOWNLOAD_DIRECTORY, NULL, NULL );


			g_hWnd_static_download_parts = _CreateWindowW( WC_STATIC, ST_V_Download_parts_, WS_CHILD, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			// Needs dimensions so that the spinner control can size itself.
			g_hWnd_download_parts = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP, 20, 203, 85, 23, hWnd, ( HMENU )EDIT_DOWNLOAD_PARTS, NULL, NULL );

			g_hWnd_ud_download_parts = _CreateWindowW( UPDOWN_CLASS, NULL, UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_download_parts, EM_LIMITTEXT, 3, 0 );
			_SendMessageW( g_hWnd_ud_download_parts, UDM_SETBUDDY, ( WPARAM )g_hWnd_download_parts, 0 );
            _SendMessageW( g_hWnd_ud_download_parts, UDM_SETBASE, 10, 0 );
			_SendMessageW( g_hWnd_ud_download_parts, UDM_SETRANGE32, 1, 100 );
			_SendMessageW( g_hWnd_ud_download_parts, UDM_SETPOS, 0, cfg_default_download_parts );


			g_hWnd_static_add_speed_limit = _CreateWindowW( WC_STATIC, ST_V_Download_speed_limit_bytes_, WS_CHILD, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_add_speed_limit = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, hWnd, ( HMENU )EDIT_ADD_SPEED_LIMIT, NULL, NULL );

			_SendMessageW( g_hWnd_add_speed_limit, EM_LIMITTEXT, 20, 0 );


			g_hWnd_add_limit_tooltip = _CreateWindowExW( WS_EX_TOPMOST, TOOLTIPS_CLASS, 0, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			add_limit_tooltip_text[ 0 ] = 0;

			TOOLINFO tti;
			_memzero( &tti, sizeof( TOOLINFO ) );
			tti.cbSize = sizeof( TOOLINFO );
			tti.uFlags = TTF_SUBCLASS;
			tti.hwnd = g_hWnd_add_speed_limit;
			tti.lpszText = add_limit_tooltip_text;

			_GetClientRect( hWnd, &tti.rect );
			_SendMessageW( g_hWnd_add_limit_tooltip, TTM_ADDTOOL, 0, ( LPARAM )&tti );


			char value[ 21 ];
			_memzero( value, sizeof( char ) * 21 );
			__snprintf( value, 21, "%I64u", cfg_default_speed_limit );
			_SendMessageA( g_hWnd_add_speed_limit, WM_SETTEXT, 0, ( LPARAM )value );


			g_hWnd_static_ssl_version = _CreateWindowW( WC_STATIC, ST_V_SSL___TLS_version_, WS_CHILD, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_ssl_version = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_COMBOBOX, NULL, CBS_AUTOHSCROLL | CBS_DROPDOWNLIST | WS_CHILD | WS_TABSTOP | WS_VSCROLL, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			_SendMessageW( g_hWnd_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_SSL_2_0 );
			_SendMessageW( g_hWnd_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_SSL_3_0 );
			_SendMessageW( g_hWnd_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_TLS_1_0 );
			_SendMessageW( g_hWnd_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_TLS_1_1 );
			_SendMessageW( g_hWnd_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_TLS_1_2 );

			_SendMessageW( g_hWnd_ssl_version, CB_SETCURSEL, cfg_default_ssl_version, 0 );

			g_hWnd_btn_authentication = _CreateWindowW( WC_BUTTON, ST_V_Authentication, BS_GROUPBOX | WS_CHILD, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			g_hWnd_static_username = _CreateWindowW( WC_STATIC, ST_V_Username_, WS_CHILD, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_edit_username = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			g_hWnd_static_password = _CreateWindowW( WC_STATIC, ST_V_Password_, WS_CHILD, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_edit_password = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_PASSWORD | ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			g_hWnd_advanced_add_tab = _CreateWindowExW( WS_EX_CONTROLPARENT, WC_TABCONTROL, NULL, WS_CHILD | /*WS_CLIPCHILDREN |*/ WS_TABSTOP, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			TCITEM ti;
			_memzero( &ti, sizeof( TCITEM ) );
			ti.mask = TCIF_TEXT;	// The tab will have text and an lParam value.

			ti.pszText = ( LPWSTR )ST_V_Cookies;
			_SendMessageW( g_hWnd_advanced_add_tab, TCM_INSERTITEM, 0, ( LPARAM )&ti );	// Insert a new tab at the end.

			ti.pszText = ( LPWSTR )ST_V_Headers;
			_SendMessageW( g_hWnd_advanced_add_tab, TCM_INSERTITEM, 1, ( LPARAM )&ti );	// Insert a new tab at the end.

			ti.pszText = ( LPWSTR )ST_V_POST_Data;
			_SendMessageW( g_hWnd_advanced_add_tab, TCM_INSERTITEM, 2, ( LPARAM )&ti );	// Insert a new tab at the end.

			g_hWnd_static_cookies = _CreateWindowW( WC_STATIC, ST_V_Cookies_, WS_CHILD, 0, 0, 0, 0, g_hWnd_advanced_add_tab, NULL, NULL, NULL );
			g_hWnd_edit_cookies = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, L"", WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL | ES_MULTILINE | WS_HSCROLL | WS_VSCROLL, 0, 0, 0, 0, g_hWnd_advanced_add_tab, NULL, NULL, NULL );

			g_hWnd_static_headers = _CreateWindowW( WC_STATIC, ST_V_Headers_, WS_CHILD, 0, 0, 0, 0, g_hWnd_advanced_add_tab, NULL, NULL, NULL );
			g_hWnd_edit_headers = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, L"", WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL | ES_MULTILINE | WS_HSCROLL | WS_VSCROLL, 0, 0, 0, 0, g_hWnd_advanced_add_tab, NULL, NULL, NULL );

			g_hWnd_chk_send_data = _CreateWindowW( WC_BUTTON, ST_V_Send_POST_Data_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, g_hWnd_advanced_add_tab, ( HMENU )CHK_SEND_DATA, NULL, NULL );
			g_hWnd_edit_data = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, L"", WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL | ES_MULTILINE | WS_HSCROLL | WS_VSCROLL | WS_DISABLED, 0, 0, 0, 0, g_hWnd_advanced_add_tab, NULL, NULL, NULL );

			g_hWnd_chk_simulate_download = _CreateWindowW( WC_BUTTON, ST_V_Simulate_download, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, hWnd, ( HMENU )CHK_SIMULATE_DOWNLOAD, NULL, NULL );

			g_hWnd_chk_show_advanced_options = _CreateWindowW( WC_BUTTON, ST_V_Advanced_options, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_ADVANCED, NULL, NULL );

			use_add_split = IsWindowsVersionOrGreater( HIBYTE( _WIN32_WINNT_VISTA ), LOBYTE( _WIN32_WINNT_VISTA ), 0 );

			if ( use_add_split )
			{
				g_hWnd_btn_download = _CreateWindowW( WC_BUTTON, ST_V_Download, BS_DEFSPLITBUTTON | BS_SPLITBUTTON | WS_CHILD | WS_DISABLED | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_DOWNLOAD, NULL, NULL );
			}
			else
			{
				g_hWnd_btn_download = _CreateWindowW( WC_BUTTON, ST_V_Download, BS_DEFPUSHBUTTON | WS_CHILD | WS_DISABLED | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_DOWNLOAD, NULL, NULL );
				g_hWnd_btn_add_download = _CreateWindowW( WC_BUTTON, ST_V_Add, WS_CHILD | WS_DISABLED | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_ADD_DOWNLOAD, NULL, NULL );
			}

			g_hWnd_cancel = _CreateWindowW( WC_BUTTON, ST_V_Cancel, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_ADD_CANCEL, NULL, NULL );

			g_hMenu_add_split = _CreatePopupMenu();

			MENUITEMINFO mii;
			_memzero( &mii, sizeof( MENUITEMINFO ) );
			mii.cbSize = sizeof( MENUITEMINFO );
			mii.fMask = MIIM_TYPE | MIIM_ID;

			mii.dwTypeData = ST_V_Download;
			mii.cch = ST_L_Download;
			mii.wID = MENU_ADD_SPLIT_DOWNLOAD;
			_InsertMenuItemW( g_hMenu_add_split, 0, TRUE, &mii );

			mii.dwTypeData = ST_V_Add;
			mii.cch = ST_L_Add;
			mii.wID = MENU_ADD_SPLIT_ADD;
			_InsertMenuItemW( g_hMenu_add_split, 1, TRUE, &mii );

			_SetFocus( g_hWnd_edit_add );

			_SendMessageW( g_hWnd_btn_download, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			if ( !use_add_split )
			{
				_SendMessageW( g_hWnd_btn_add_download, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			}
			_SendMessageW( g_hWnd_cancel, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( hWnd_static_urls, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_edit_add, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_static_ssl_version, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_ssl_version, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_static_add_speed_limit, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_add_speed_limit, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_static_download_parts, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_download_parts, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_chk_simulate_download, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_static_download_directory, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_download_directory, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_btn_download_directory, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_btn_authentication, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_static_username, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_edit_username, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_static_password, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_edit_password, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_advanced_add_tab, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_static_cookies, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_edit_cookies, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_static_headers, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_edit_headers, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_chk_send_data, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_edit_data, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_chk_show_advanced_options, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( g_hWnd_static_regex_filter, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_regex_filter_preset, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_regex_filter, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_btn_apply_filter, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			t_download_directory = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * MAX_PATH );
			_wmemcpy_s( t_download_directory, MAX_PATH, cfg_default_download_directory, g_default_download_directory_length );
			t_download_directory[ g_default_download_directory_length ] = 0;	// Sanity.

			URLProc = ( WNDPROC )_GetWindowLongPtrW( g_hWnd_edit_add, GWLP_WNDPROC );
			_SetWindowLongPtrW( g_hWnd_edit_add, GWLP_WNDPROC, ( LONG_PTR )URLSubProc );
			_SetWindowLongPtrW( g_hWnd_edit_cookies, GWLP_WNDPROC, ( LONG_PTR )URLSubProc );
			_SetWindowLongPtrW( g_hWnd_edit_headers, GWLP_WNDPROC, ( LONG_PTR )URLSubProc );
			_SetWindowLongPtrW( g_hWnd_edit_data, GWLP_WNDPROC, ( LONG_PTR )URLSubProc );

			AddTabProc = ( WNDPROC )_GetWindowLongPtrW( g_hWnd_advanced_add_tab, GWLP_WNDPROC );
			_SetWindowLongPtrW( g_hWnd_advanced_add_tab, GWLP_WNDPROC, ( LONG_PTR )AddTabSubProc );

			#ifndef UXTHEME_USE_STATIC_LIB
				if ( uxtheme_state == UXTHEME_STATE_SHUTDOWN )
				{
					g_add_use_theme = InitializeUXTheme();
				}
			#endif

			#ifndef OLE32_USE_STATIC_LIB
				if ( ole32_state == OLE32_STATE_SHUTDOWN )
				{
					use_drag_and_drop_add = InitializeOle32();
				}
			#endif

			if ( use_drag_and_drop_add )
			{
				_OleInitialize( NULL );

				RegisterDropWindow( g_hWnd_edit_add, &Add_DropTarget );
			}

			return 0;
		}
		break;

		case WM_DRAWITEM:
		{
			DRAWITEMSTRUCT *dis = ( DRAWITEMSTRUCT * )lParam;

			// The disabled static control causes flickering. The only way around it is to draw it ourselves.
			if ( dis->CtlType == ODT_STATIC )
			{
				if ( _IsWindowEnabled( dis->hwndItem ) == FALSE )
				{
					_SetTextColor( dis->hDC, _GetSysColor( COLOR_GRAYTEXT ) );
				}
				_DrawTextW( dis->hDC, ST_V_RegEx_filter_, ST_L_RegEx_filter_, &dis->rcItem, DT_NOPREFIX | DT_SINGLELINE | DT_END_ELLIPSIS | DT_RIGHT );
			}

			return TRUE;
		}
		break;

		case WM_SIZE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			// Allow our controls to move in relation to the parent window.
			HDWP hdwp = _BeginDeferWindowPos( ( use_add_split ? 23 : 24 ) );

			_DeferWindowPos( hdwp, g_hWnd_regex_filter, HWND_TOP, 265, 10, rc.right - 345, 23, SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_btn_apply_filter, HWND_TOP, rc.right - 75, 10, 65, 23, SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_edit_add, HWND_TOP, 10, 40, rc.right - 20, rc.bottom - ( g_show_advanced ? 418 : 133 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_download_directory, HWND_TOP, 10, rc.bottom - ( g_show_advanced ? 373 : 88 ), rc.right - 20, 15, SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_download_directory, HWND_TOP, 10, rc.bottom - ( g_show_advanced ? 358 : 73 ), rc.right - 60, 23, SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_btn_download_directory, HWND_TOP, rc.right - 45, rc.bottom - ( g_show_advanced ? 358 : 73 ), 35, 23, SWP_NOZORDER );

			//

			_DeferWindowPos( hdwp, g_hWnd_static_download_parts, HWND_TOP, 10, rc.bottom - 325, 115, 15, SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_download_parts, HWND_TOP, 10, rc.bottom - 310, 85, 23, SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_ud_download_parts, HWND_TOP, 95, rc.bottom - 310, 0, 0, SWP_NOZORDER | SWP_NOSIZE );

			_DeferWindowPos( hdwp, g_hWnd_static_add_speed_limit, HWND_TOP, 10, rc.bottom - 277, 200, 15, SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_add_speed_limit, HWND_TOP, 10, rc.bottom - 262, 200, 23, SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_ssl_version, HWND_TOP, 130, rc.bottom - 325, 115, 15, SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_ssl_version, HWND_TOP, 130, rc.bottom - 310, 100, 23, SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_btn_authentication, HWND_TOP, 250, rc.bottom - 325, 230, 65, SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_static_username, HWND_TOP, 260, rc.bottom - 310, 100, 15, SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_edit_username, HWND_TOP, 260, rc.bottom - 295, 100, 23, SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_static_password, HWND_TOP, 370, rc.bottom - 310, 100, 15, SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_edit_password, HWND_TOP, 370, rc.bottom - 295, 100, 23, SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_advanced_add_tab, HWND_TOP, 10, rc.bottom - 225, rc.right - 20, 150, SWP_NOZORDER );

			//

			_DeferWindowPos( hdwp, g_hWnd_chk_simulate_download, HWND_TOP, 10, rc.bottom - 65, 210, 20, SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_chk_show_advanced_options, HWND_TOP, 10, rc.bottom - 32, 210, 23, SWP_NOZORDER );

			if ( use_add_split )
			{
				_DeferWindowPos( hdwp, g_hWnd_btn_download, HWND_TOP, rc.right - 188, rc.bottom - 32, 93, 23, SWP_NOZORDER );
			}
			else
			{
				_DeferWindowPos( hdwp, g_hWnd_btn_download, HWND_TOP, rc.right - 260, rc.bottom - 32, 80, 23, SWP_NOZORDER );
				_DeferWindowPos( hdwp, g_hWnd_btn_add_download, HWND_TOP, rc.right - 175, rc.bottom - 32, 80, 23, SWP_NOZORDER );
			}
			_DeferWindowPos( hdwp, g_hWnd_cancel, HWND_TOP, rc.right - 90, rc.bottom - 32, 80, 23, SWP_NOZORDER );
			_EndDeferWindowPos( hdwp );

			//

			// This brush is refreshed whenever the tab changes size.
			// It's used to paint the background of static controls.
			// Windows XP has a gradient colored tab pane and setting the background of a static control to TRANSPARENT in WM_CTLCOLORSTATIC doesn't work well.
			if ( _IsWindowVisible( g_hWnd_advanced_add_tab ) == TRUE  && g_add_tab_width != ( rc.right - 20 ) )
			{
				g_add_tab_width = rc.right - 20;

				HBRUSH old_brush = g_add_tab_brush;

				HDC hDC = _GetDC( g_hWnd_advanced_add_tab );

				// Create a memory buffer to draw to.
				HDC hdcMem = _CreateCompatibleDC( hDC );

				HBITMAP hbm = _CreateCompatibleBitmap( hDC, g_add_tab_width, 150 );
				HBITMAP ohbm = ( HBITMAP )_SelectObject( hdcMem, hbm );
				_DeleteObject( ohbm );

				_SendMessageW( g_hWnd_advanced_add_tab, WM_PRINTCLIENT, ( WPARAM )hdcMem, ( LPARAM )( PRF_ERASEBKGND | PRF_CLIENT | PRF_NONCLIENT ) );

				g_add_tab_brush = _CreatePatternBrush( hbm );

				_DeleteObject( hbm );

				_DeleteDC( hdcMem );
				_ReleaseDC( g_hWnd_advanced_add_tab, hDC );

				if ( old_brush != NULL )
				{
					_DeleteObject( old_brush );
				}
			}

			/*rc.left = 5;
			rc.top = 5;
			rc.right -= 65;
			rc.bottom -= ( g_show_advanced ? 438 : 153 );	// Add 20 to each of these numbers from above.
			_SendMessageW( g_hWnd_edit_add, EM_SETRECT, 0, ( LPARAM )&rc );*/

			return 0;
		}
		break;

		case WM_GETMINMAXINFO:
		{
			// Set the minimum dimensions that the window can be sized to.
			( ( MINMAXINFO * )lParam )->ptMinTrackSize.x = 600;
			( ( MINMAXINFO * )lParam )->ptMinTrackSize.y = ( g_show_advanced ? MIN_ADVANCED_HEIGHT : MIN_SIMPLE_HEIGHT );

			return 0;
		}
		break;

		case WM_COMMAND:
		{
			switch ( LOWORD( wParam ) )
			{
				case IDOK:
				case BTN_DOWNLOAD:
				case BTN_ADD_DOWNLOAD:
				{
					unsigned char download_operations = ( _SendMessageW( g_hWnd_chk_simulate_download, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? DOWNLOAD_OPERATION_SIMULATE : DOWNLOAD_OPERATION_NONE );

					if ( !( download_operations & DOWNLOAD_OPERATION_SIMULATE ) && t_download_directory == NULL )
					{
						_MessageBoxW( hWnd, ST_V_You_must_supply_download_directory, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONWARNING );

						_SendMessageW( hWnd, WM_COMMAND, MAKEWPARAM( BTN_DOWNLOAD_DIRECTORY, 0 ), 0 );

						break;
					}

					unsigned int edit_length = ( unsigned int )_SendMessageW( g_hWnd_edit_add, WM_GETTEXTLENGTH, 0, 0 );

					// http://a.b
					if ( edit_length >= 10 )
					{
						ADD_INFO *ai = ( ADD_INFO * )GlobalAlloc( GPTR, sizeof( ADD_INFO ) );

						if ( !( download_operations & DOWNLOAD_OPERATION_SIMULATE ) )
						{
							ai->download_directory = t_download_directory;

							int t_download_directory_length = lstrlenW( t_download_directory );
							t_download_directory = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * MAX_PATH );
							_wmemcpy_s( t_download_directory, MAX_PATH, ai->download_directory, t_download_directory_length );
							t_download_directory[ t_download_directory_length ] = 0;	// Sanity.
						}

						ai->ssl_version = ( char )_SendMessageW( g_hWnd_ssl_version, CB_GETCURSEL, 0, 0 );

						ai->download_operations = download_operations | ( LOWORD( wParam ) == BTN_ADD_DOWNLOAD || add_split_type == 1 ? DOWNLOAD_OPERATION_ADD_STOPPED : DOWNLOAD_OPERATION_NONE );

						ai->method = ( _SendMessageW( g_hWnd_chk_send_data, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? METHOD_POST : METHOD_GET );

						char value[ 21 ];
						_SendMessageA( g_hWnd_download_parts, WM_GETTEXT, 11, ( LPARAM )value );
						ai->parts = ( unsigned char )_strtoul( value, NULL, 10 );

						_SendMessageA( g_hWnd_add_speed_limit, WM_GETTEXT, 21, ( LPARAM )value );
						ai->download_speed_limit = strtoull( value );

						// URLs
						wchar_t *edit = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( edit_length + 1 ) );
						_SendMessageW( g_hWnd_edit_add, WM_GETTEXT, edit_length + 1, ( LPARAM )edit );

						//int utf8_length = WideCharToMultiByte( CP_UTF8, 0, edit, -1, NULL, 0, NULL, NULL );	// Size includes NULL character.
						//ai->utf8_urls = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * utf8_length ); // Size includes the NULL character.
						//WideCharToMultiByte( CP_UTF8, 0, edit, -1, ai->utf8_urls, utf8_length, NULL, NULL );

						//GlobalFree( edit );

						ai->urls = edit;

						edit = NULL;

						int utf8_length = 0;

						// Username
						edit_length = ( unsigned int )_SendMessageW( g_hWnd_edit_username, WM_GETTEXTLENGTH, 0, 0 );
						if ( edit_length > 0 )
						{
							edit = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( edit_length + 1 ) );
							_SendMessageW( g_hWnd_edit_username, WM_GETTEXT, edit_length + 1, ( LPARAM )edit );

							utf8_length = WideCharToMultiByte( CP_UTF8, 0, edit, -1, NULL, 0, NULL, NULL );	// Size includes NULL character.
							ai->auth_info.username = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * utf8_length ); // Size includes the NULL character.
							WideCharToMultiByte( CP_UTF8, 0, edit, -1, ai->auth_info.username, utf8_length, NULL, NULL );

							GlobalFree( edit );
						}
						else
						{
							ai->auth_info.username = NULL;
						}

						// Password
						edit_length = ( unsigned int )_SendMessageW( g_hWnd_edit_password, WM_GETTEXTLENGTH, 0, 0 );
						if ( edit_length > 0 )
						{
							edit = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( edit_length + 1 ) );
							_SendMessageW( g_hWnd_edit_password, WM_GETTEXT, edit_length + 1, ( LPARAM )edit );

							utf8_length = WideCharToMultiByte( CP_UTF8, 0, edit, -1, NULL, 0, NULL, NULL );	// Size includes NULL character.
							ai->auth_info.password = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * utf8_length ); // Size includes the NULL character.
							WideCharToMultiByte( CP_UTF8, 0, edit, -1, ai->auth_info.password, utf8_length, NULL, NULL );

							GlobalFree( edit );
						}
						else
						{
							ai->auth_info.password = NULL;
						}

						edit_length = ( unsigned int )_SendMessageW( g_hWnd_edit_cookies, WM_GETTEXTLENGTH, 0, 0 );
						if ( edit_length > 0 )
						{
							edit = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( edit_length + 1 ) );
							_SendMessageW( g_hWnd_edit_cookies, WM_GETTEXT, edit_length + 1, ( LPARAM )edit );

							utf8_length = WideCharToMultiByte( CP_UTF8, 0, edit, -1, NULL, 0, NULL, NULL );	// Size includes NULL character.
							ai->utf8_cookies = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * utf8_length ); // Size includes the NULL character.
							WideCharToMultiByte( CP_UTF8, 0, edit, -1, ai->utf8_cookies, utf8_length, NULL, NULL );

							GlobalFree( edit );
						}
						else
						{
							ai->utf8_cookies = NULL;
						}

						// Must be at least 2 characters long. "a:" is a valid header name and value.
						edit_length = ( unsigned int )_SendMessageW( g_hWnd_edit_headers, WM_GETTEXTLENGTH, 0, 0 );
						if ( edit_length >= 2 )
						{
							edit = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( edit_length + 1 + 2 ) );	// Add 2 for \r\n
							_SendMessageW( g_hWnd_edit_headers, WM_GETTEXT, edit_length + 1, ( LPARAM )edit );

							wchar_t *edit_start = edit;

							// Skip newlines that might appear before the field name.
							while ( *edit_start == L'\r' || *edit_start == L'\n' )
							{
								--edit_length;
								++edit_start;
							}

							// Make sure the header has a colon somewhere in it and not at the very beginning.
							if ( edit_length >= 2 && *edit_start != L':' && _StrChrW( edit_start + 1, L':' ) != NULL )
							{
								// Make sure the header ends with a \r\n.
								if ( edit_start[ edit_length - 2 ] != '\r' && edit_start[ edit_length - 1 ] != '\n' )
								{
									edit_start[ edit_length ] = '\r';
									edit_start[ edit_length + 1 ] = '\n';
									edit_start[ edit_length + 2 ] = 0;	// Sanity.
								}

								utf8_length = WideCharToMultiByte( CP_UTF8, 0, edit_start, -1, NULL, 0, NULL, NULL );	// Size includes NULL character.
								ai->utf8_headers = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * utf8_length ); // Size includes the NULL character.
								WideCharToMultiByte( CP_UTF8, 0, edit_start, -1, ai->utf8_headers, utf8_length, NULL, NULL );
							}

							GlobalFree( edit );
						}
						else
						{
							ai->utf8_headers = NULL;
						}

						edit_length = ( unsigned int )_SendMessageW( g_hWnd_edit_data, WM_GETTEXTLENGTH, 0, 0 );
						if ( edit_length > 0 )
						{
							edit = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( edit_length + 1 ) );
							_SendMessageW( g_hWnd_edit_data, WM_GETTEXT, edit_length + 1, ( LPARAM )edit );

							utf8_length = WideCharToMultiByte( CP_UTF8, 0, edit, -1, NULL, 0, NULL, NULL );	// Size includes NULL character.
							ai->utf8_data = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * utf8_length ); // Size includes the NULL character.
							WideCharToMultiByte( CP_UTF8, 0, edit, -1, ai->utf8_data, utf8_length, NULL, NULL );

							GlobalFree( edit );
						}
						else
						{
							ai->utf8_data = NULL;
						}

						// ai is freed in AddURL.
						HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, AddURL, ( void * )ai, 0, NULL );
						if ( thread != NULL )
						{
							CloseHandle( thread );
						}
						else
						{
							GlobalFree( ai->utf8_headers );
							GlobalFree( ai->utf8_cookies );
							GlobalFree( ai->auth_info.username );
							GlobalFree( ai->auth_info.password );
							GlobalFree( ai->urls );
							GlobalFree( ai );
						}
					}

					_SendMessageW( hWnd, WM_CLOSE, 0, 0 );
				}
				break;

				case BTN_ADD_CANCEL:
				{
					_SendMessageW( hWnd, WM_CLOSE, 0, 0 );
				}
				break;

				case MENU_ADD_SPLIT_DOWNLOAD:
				{
					add_split_type = 0;

					_SendMessageW( g_hWnd_btn_download, WM_SETTEXT, 0, ( LPARAM )ST_V_Download );
				}
				break;

				case MENU_ADD_SPLIT_ADD:
				{
					add_split_type = 1;

					_SendMessageW( g_hWnd_btn_download, WM_SETTEXT, 0, ( LPARAM )ST_V_Add );
				}
				break;

				case EDIT_ADD_URLS:
				{
					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						BOOL enable = ( _SendMessageW( g_hWnd_edit_add, WM_GETTEXTLENGTH, 0, 0 ) > 0 ? TRUE : FALSE );
						_EnableWindow( g_hWnd_btn_download, enable );
						if ( !use_add_split )
						{
							_EnableWindow( g_hWnd_btn_add_download, enable );
						}
					}
				}
				break;

				case EDIT_DOWNLOAD_PARTS:
				{
					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						DWORD sel_start;

						char value[ 11 ];
						_SendMessageA( ( HWND )lParam, WM_GETTEXT, 11, ( LPARAM )value );
						unsigned long num = _strtoul( value, NULL, 10 );

						if ( num > 100 )
						{
							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )"100" );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}
						else if ( num == 0 )
						{
							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )"1" );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}
					}
				}
				break;

				case EDIT_ADD_SPEED_LIMIT:
				{
					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						DWORD sel_start;

						char value[ 21 ];
						_SendMessageA( ( HWND )lParam, WM_GETTEXT, 21, ( LPARAM )value );
						unsigned long long num = strtoull( value );

						if ( num == 0xFFFFFFFFFFFFFFFF )
						{
							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )"18446744073709551615" );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}

						if ( num > 0 )
						{
							unsigned int length = FormatSizes( add_limit_tooltip_text, 32, SIZE_FORMAT_AUTO, num );
							add_limit_tooltip_text[ length++ ] = L'/';
							add_limit_tooltip_text[ length++ ] = L's';
							add_limit_tooltip_text[ length ] = 0;
						}
						else
						{
							_wmemcpy_s( add_limit_tooltip_text, 32, ST_V_Unlimited, ST_L_Unlimited + 1 );
						}

						TOOLINFO ti;
						_memzero( &ti, sizeof( TOOLINFO ) );
						ti.cbSize = sizeof( TOOLINFO );
						ti.hwnd = g_hWnd_add_speed_limit;
						ti.lpszText = add_limit_tooltip_text;
						_SendMessageW( g_hWnd_add_limit_tooltip, TTM_UPDATETIPTEXT, 0, ( LPARAM )&ti );
					}
				}
				break;

				case CHK_SIMULATE_DOWNLOAD:
				{
					BOOL enable = ( _SendMessageW( g_hWnd_chk_simulate_download, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? FALSE : TRUE );
					_EnableWindow( g_hWnd_download_directory, enable );
					_EnableWindow( g_hWnd_btn_download_directory, enable );
				}
				break;

				case BTN_DOWNLOAD_DIRECTORY:
				{
					wchar_t *directory = NULL;

					_BrowseForFolder( hWnd, ST_V_Select_the_download_directory, &directory );

					if ( directory != NULL )
					{
						GlobalFree( t_download_directory );
						t_download_directory = directory;

						_SendMessageW( g_hWnd_download_directory, WM_SETTEXT, 0, ( LPARAM )t_download_directory );
					}
				}
				break;

				case BTN_ADVANCED:
				{
					g_show_advanced = !g_show_advanced;

					int sw_type = ( g_show_advanced ? SW_SHOW : SW_HIDE );

					_ShowWindow( g_hWnd_static_ssl_version, sw_type );
					_ShowWindow( g_hWnd_ssl_version, sw_type );
					_ShowWindow( g_hWnd_static_add_speed_limit, sw_type );
					_ShowWindow( g_hWnd_add_speed_limit, sw_type );
					_ShowWindow( g_hWnd_static_download_parts, sw_type );
					_ShowWindow( g_hWnd_download_parts, sw_type );
					_ShowWindow( g_hWnd_ud_download_parts, sw_type );
					_ShowWindow( g_hWnd_chk_simulate_download, sw_type );
					_ShowWindow( g_hWnd_btn_authentication, sw_type );
					_ShowWindow( g_hWnd_static_username, sw_type );
					_ShowWindow( g_hWnd_edit_username, sw_type );
					_ShowWindow( g_hWnd_static_password, sw_type );
					_ShowWindow( g_hWnd_edit_password, sw_type );
					_ShowWindow( g_hWnd_advanced_add_tab, sw_type );
					ShowHideAddTabs( sw_type );

					// Adjust the window height.
					RECT rc;
					_GetWindowRect( hWnd, &rc );

					if ( g_show_advanced )
					{
						_SetWindowPos( hWnd, NULL, 0, 0, rc.right - rc.left, ( MIN_ADVANCED_HEIGHT - MIN_SIMPLE_HEIGHT ) + ( rc.bottom - rc.top ), SWP_NOMOVE );
					}
					else
					{
						if ( _IsZoomed( hWnd ) == FALSE )
						{
							_SetWindowPos( hWnd, NULL, 0, 0, rc.right - rc.left, ( rc.bottom - rc.top ) - ( MIN_ADVANCED_HEIGHT - MIN_SIMPLE_HEIGHT ), SWP_NOMOVE );
						}
					}

					// Force a resize of our controls.
					_SendMessageW( hWnd, WM_SIZE, 0, 0 );
				}
				break;

				case CB_REGEX_FILTER_PRESET:
				{
					if ( HIWORD( wParam ) == CBN_SELCHANGE )
					{
						int index = ( int )_SendMessageW( ( HWND )lParam, CB_GETCURSEL, 0, 0 );

						wchar_t *filter = NULL;
						switch ( index )
						{
							case 1: { filter = L"(?i)(^(http|ftpe?)s?:\\/\\/[^\\/\\s]+\\/[^\\?#\\s]+\\.(jp(e?g|e)|gif|png|bmp|tiff?|dib|ico)(\\?|#|$))"; } break;
							case 2: { filter = L"(?i)(^(http|ftpe?)s?:\\/\\/[^\\/\\s]+\\/[^\\?#\\s]+\\.(mp3|wave?|flac?|ogg|m4a|wma|aac|midi?|ape|shn|wv|aiff?|oga)(\\?|#|$))"; } break;
							case 3: { filter = L"(?i)(^(http|ftpe?)s?:\\/\\/[^\\/\\s]+\\/[^\\?#\\s]+\\.(avi|mp[124]|m4v|mp(e?g|e)|mkv|webm|wmv|3gp|ogm|ogv|flv|vob)(\\?|#|$))"; } break;
						}

						_SendMessageW( g_hWnd_regex_filter, WM_SETTEXT, 0, ( LPARAM )filter );
					}
				}
				break;

				case BTN_APPLY_FILTER:
				{
					unsigned int edit_length = ( unsigned int )_SendMessageW( g_hWnd_edit_add, WM_GETTEXTLENGTH, 0, 0 );
					unsigned int regex_filter_length = ( unsigned int )_SendMessageW( g_hWnd_regex_filter, WM_GETTEXTLENGTH, 0, 0 );

					// http://a.b
					if ( edit_length >= 10 && regex_filter_length > 0 )
					{
						FILTER_INFO *fi = ( FILTER_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( FILTER_INFO ) );

						// URLs
						wchar_t *edit = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( edit_length + 1 ) );
						_SendMessageW( g_hWnd_edit_add, WM_GETTEXT, edit_length + 1, ( LPARAM )edit );

						fi->text = edit;

						// Filter
						edit = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( regex_filter_length + 1 ) );
						_SendMessageW( g_hWnd_regex_filter, WM_GETTEXT, regex_filter_length + 1, ( LPARAM )edit );

						fi->filter = edit;

						// fi is freed in filter_urls.
						HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, filter_urls, ( void * )fi, 0, NULL );
						if ( thread != NULL )
						{
							CloseHandle( thread );
						}
						else
						{
							GlobalFree( fi->filter );
							GlobalFree( fi->text );
							GlobalFree( fi );
						}
					}
				}
				break;
			}

			return 0;
		}
		break;

		case WM_NOTIFY:
		{
			// Get our listview codes.
			switch ( ( ( LPNMHDR )lParam )->code )
			{
				case TCN_SELCHANGING:		// The tab that's about to lose focus
				{
					//NMHDR *nmhdr = ( NMHDR * )lParam;

					ShowHideAddTabs( SW_HIDE );
				}
				break;

				case TCN_SELCHANGE:			// The tab that gains focus
				{
					NMHDR *nmhdr = ( NMHDR * )lParam;

					HWND hWnd_focused = _GetFocus();
					if ( hWnd_focused != hWnd && hWnd_focused != nmhdr->hwndFrom )
					{
						_SetFocus( _GetWindow( nmhdr->hwndFrom, GW_CHILD ) );
					}

					ShowHideAddTabs( SW_SHOW );
				}
				break;

				case BCN_DROPDOWN:
				{
					NMBCDROPDOWN *nmbcddd = ( NMBCDROPDOWN * )lParam;

					RECT rc;
					_GetWindowRect( nmbcddd->hdr.hwndFrom, &rc );

					_TrackPopupMenu( g_hMenu_add_split, 0, rc.left, rc.bottom, 0, hWnd, NULL );
				}
				break;
			}

			return FALSE;
		}
		break;

		case WM_PROPAGATE:
		{
			if ( wParam == -1 )
			{
				CL_ARGS *cla = ( CL_ARGS * )lParam;

				if ( cla != NULL )
				{
					if ( cla->download_directory != NULL && cla->download_directory_length < MAX_PATH )
					{
						if ( t_download_directory == NULL )
						{
							t_download_directory = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * MAX_PATH );
						}

						_wmemcpy_s( t_download_directory, MAX_PATH, cla->download_directory, cla->download_directory_length );
						t_download_directory[ cla->download_directory_length ] = 0;	// Sanity.

						_SendMessageW( g_hWnd_download_directory, WM_SETTEXT, 0, ( LPARAM )t_download_directory );
					}

					if ( cla->urls != NULL )
					{
						_SendMessageW( g_hWnd_edit_add, EM_REPLACESEL, 0, ( LPARAM )cla->urls );
					}

					if ( cla->cookies != NULL )
					{
						_SendMessageW( g_hWnd_edit_cookies, EM_REPLACESEL, 0, ( LPARAM )cla->cookies );
					}

					if ( cla->headers != NULL )
					{
						_SendMessageW( g_hWnd_edit_headers, EM_REPLACESEL, 0, ( LPARAM )cla->headers );
					}

					if ( cla->data != NULL )
					{
						_SendMessageW( g_hWnd_chk_send_data, BM_SETCHECK, BST_CHECKED, 0 );
						_SendMessageW( g_hWnd_edit_data, EM_REPLACESEL, 0, ( LPARAM )cla->data );
						_EnableWindow( g_hWnd_edit_data, TRUE );
					}

					if ( cla->parts > 0 )
					{
						_SendMessageW( g_hWnd_ud_download_parts, UDM_SETPOS, 0, cla->parts );
					}

					char value[ 21 ];
					_memzero( value, sizeof( char ) * 21 );
					__snprintf( value, 21, "%I64u", cla->download_speed_limit );
					_SendMessageA( g_hWnd_add_speed_limit, WM_SETTEXT, 0, ( LPARAM )value );

					if ( cla->ssl_version >= 0 )
					{
						_SendMessageW( g_hWnd_ssl_version, CB_SETCURSEL, cla->ssl_version, 0 );
					}

					if ( cla->username != NULL )
					{
						_SendMessageW( g_hWnd_edit_username, WM_SETTEXT, 0, ( LPARAM )cla->username );
					}

					if ( cla->password != NULL )
					{
						_SendMessageW( g_hWnd_edit_password, WM_SETTEXT, 0, ( LPARAM )cla->password );
					}

					if ( cla->download_operations & DOWNLOAD_OPERATION_SIMULATE )
					{
						_SendMessageW( g_hWnd_chk_simulate_download, BM_SETCHECK, BST_CHECKED, 0 );
						_EnableWindow( g_hWnd_download_directory, FALSE );
						_EnableWindow( g_hWnd_btn_download_directory, FALSE );
					}

					// http://a.b + \r\n
					_SetFocus( ( cla->urls_length >= 12 ? g_hWnd_btn_download : g_hWnd_edit_add ) );
				}
			}
			else if ( wParam != 0 )
			{
				char length = 0;

				if ( wParam == CF_UNICODETEXT || wParam == CF_HTML )
				{
					wchar_t *data = ( wchar_t * )lParam;
					if ( data != NULL ) { for ( ; length < 10 && *data != NULL; ++length, ++data ); }

					_SendMessageW( g_hWnd_edit_add, EM_REPLACESEL, 0, lParam );
				}
				else// if ( wParam == CF_TEXT )
				{
					char *data = ( char * )lParam;
					if ( data != NULL ) { for ( ; length < 10 && *data != NULL; ++length, ++data ); }

					_SendMessageA( g_hWnd_edit_add, EM_REPLACESEL, 0, lParam );
				}

				// Append a newline after our dropped text.
				_SendMessageW( g_hWnd_edit_add, EM_REPLACESEL, 0, ( LPARAM )L"\r\n" );

				// http://a.b
				_SetFocus( ( length == 10 ? g_hWnd_btn_download : g_hWnd_edit_add ) );
			}
			else
			{
				bool got_data = false;

				if ( _IsWindowVisible( hWnd ) == FALSE )
				{
					// If there's any HTML conent on the clipboard, then parse out any URLs.
					if ( _IsClipboardFormatAvailable( CF_HTML ) == TRUE )
					{
						if ( _OpenClipboard( g_hWnd_add_urls ) )
						{
							HANDLE cbh = _GetClipboardData( CF_HTML );

							char *data = ( char * )GlobalLock( cbh );
							if ( data != NULL )
							{
								wchar_t *wdata = ParseHTMLClipboard( data );
								if ( wdata != NULL )
								{
									_SendMessageW( g_hWnd_edit_add, EM_REPLACESEL, 0, ( LPARAM )wdata );

									// Append a newline after our pasted text.
									_SendMessageW( g_hWnd_edit_add, EM_REPLACESEL, 0, ( LPARAM )L"\r\n" );

									GlobalFree( wdata );

									got_data = true;
								}

								GlobalUnlock( cbh );
							}

							_CloseClipboard();
						}
					}

					// There were no URLs in the HTML content, or it's just regular text.
					if ( _IsClipboardFormatAvailable( CF_UNICODETEXT ) == TRUE && !got_data )
					{
						if ( _OpenClipboard( g_hWnd_add_urls ) )
						{
							HANDLE cbh = _GetClipboardData( CF_UNICODETEXT );

							wchar_t *data = ( wchar_t * )GlobalLock( cbh );
							if ( data != NULL )
							{
								// Make sure the text starts with http(s) protocol.
								char offset = 0;
								if ( _StrCmpNIW( data, L"http://", 7 ) == 0 )
								{
									offset = 7;
								}
								else if ( _StrCmpNIW( data, L"https://", 8 ) == 0 )
								{
									offset = 8;
								}
								else if ( _StrCmpNIW( data, L"ftp://", 6 ) == 0 )
								{
									offset = 6;
								}
								else if ( _StrCmpNIW( data, L"ftps://", 7 ) == 0 )
								{
									offset = 7;
								}
								else if ( _StrCmpNIW( data, L"ftpes://", 8 ) == 0 )
								{
									offset = 8;
								}

								// Make sure there's at least 3 characters after http(s)://
								if ( offset > 0 )
								{
									for ( char i = 0; i < 3; ++i, ++offset )
									{
										if ( *( data + offset ) == NULL )
										{
											offset = 0;
											break;
										}
									}
								}

								if ( offset != 0 )
								{
									_SendMessageW( g_hWnd_edit_add, EM_REPLACESEL, 0, ( LPARAM )data );

									// Append a newline after our pasted text.
									_SendMessageW( g_hWnd_edit_add, EM_REPLACESEL, 0, ( LPARAM )L"\r\n" );

									got_data = true;
								}

								GlobalUnlock( cbh );
							}

							_CloseClipboard();
						}
					}
				}

				_SetFocus( ( got_data ? g_hWnd_btn_download : g_hWnd_edit_add ) );
			}

			_ShowWindow( hWnd, SW_SHOWNORMAL );
			_SetForegroundWindow( hWnd );
		}
		break;

		case WM_FILTER_TEXT:
		{
			_SendMessageW( g_hWnd_edit_add, EM_SETSEL, 0, -1 );
			_SendMessageW( g_hWnd_edit_add, EM_REPLACESEL, TRUE, lParam );

			_SetFocus( g_hWnd_edit_add );

			GlobalFree( ( HGLOBAL )lParam );
		}
		break;

		case WM_ACTIVATE:
		{
			// 0 = inactive, > 0 = active
			g_hWnd_active = ( wParam == 0 ? NULL : hWnd );

            return FALSE;
		}
		break;

		case WM_CLOSE:
		{
			_ShowWindow( hWnd, SW_HIDE );

			_SendMessageW( g_hWnd_edit_add, WM_SETTEXT, 0, NULL );

			// Reset to default.

			_EnableWindow( g_hWnd_btn_download, FALSE );
			if ( !use_add_split )
			{
				_EnableWindow( g_hWnd_btn_add_download, FALSE );
			}

			_SendMessageW( g_hWnd_btn_download, BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE );
			_SendMessageW( g_hWnd_cancel, BM_SETSTYLE, 0, TRUE );
			_SendMessageW( g_hWnd_btn_download_directory, BM_SETSTYLE, 0, TRUE );
			_SendMessageW( g_hWnd_btn_apply_filter, BM_SETSTYLE, 0, TRUE );

			// Let's retain the last directory.
			//_SendMessageW( g_hWnd_download_directory, WM_SETTEXT, 0, ( LPARAM )cfg_default_download_directory );

			_SendMessageW( g_hWnd_ud_download_parts, UDM_SETPOS, 0, cfg_default_download_parts );
			_SendMessageW( g_hWnd_ssl_version, CB_SETCURSEL, cfg_default_ssl_version, 0 );

			char value[ 21 ];
			_memzero( value, sizeof( char ) * 21 );
			__snprintf( value, 21, "%I64u", cfg_default_speed_limit );
			_SendMessageA( g_hWnd_add_speed_limit, WM_SETTEXT, 0, ( LPARAM )value );

			_SendMessageW( g_hWnd_edit_username, WM_SETTEXT, 0, NULL );
			_SendMessageW( g_hWnd_edit_password, WM_SETTEXT, 0, NULL );

			_SendMessageW( g_hWnd_edit_cookies, WM_SETTEXT, 0, NULL );
			_SendMessageW( g_hWnd_edit_headers, WM_SETTEXT, 0, NULL );
			_SendMessageW( g_hWnd_edit_data, WM_SETTEXT, 0, NULL );

			_SendMessageW( g_hWnd_chk_send_data, BM_SETCHECK, BST_UNCHECKED, 0 );
			_EnableWindow( g_hWnd_edit_data, FALSE );

			_SendMessageW( g_hWnd_chk_simulate_download, BM_SETCHECK, BST_UNCHECKED, 0 );
			_EnableWindow( g_hWnd_download_directory, TRUE );
			_EnableWindow( g_hWnd_btn_download_directory, TRUE );

			_SendMessageW( g_hWnd_advanced_add_tab, TCM_SETCURFOCUS, 0, 0 );

			//

			return 0;
		}
		break;

		case WM_DESTROY:
		{
			if ( g_add_tab_brush != NULL )
			{
				_DeleteObject( g_add_tab_brush );
				g_add_tab_brush = NULL;
			}

			if ( use_drag_and_drop_add )
			{
				UnregisterDropWindow( g_hWnd_edit_add, Add_DropTarget );

				_OleUninitialize();
			}

			if ( t_download_directory != NULL )
			{
				GlobalFree( t_download_directory );
				t_download_directory = NULL;
			}

			_DestroyMenu( g_hMenu_add_split );

			g_hWnd_add_urls = NULL;

			g_show_advanced = false;

			return 0;
		}
		break;

		default:
		{
			return _DefWindowProcW( hWnd, msg, wParam, lParam );
		}
		break;
	}

	return TRUE;
}
