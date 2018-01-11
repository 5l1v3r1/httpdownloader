/*
	HTTP Downloader can download files through HTTP and HTTPS connections.
	Copyright (C) 2015-2018 Eric Kutcher

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

#include "connection.h"

#include "lite_ole32.h"
#include "lite_gdi32.h"

#include "drag_and_drop.h"

#include "string_tables.h"

#define BTN_DOWNLOAD			1000
#define BTN_ADD_CANCEL			1001
#define EDIT_DOWNLOAD_PARTS		1003
#define BTN_DOWNLOAD_DIRECTORY	1004
#define CHK_SIMULATE_DOWNLOAD	1005
#define BTN_ADVANCED			1006

HWND g_hWnd_add_urls = NULL;

HWND g_hWnd_edit_add = NULL;

HWND g_hWnd_static_download_directory = NULL;
HWND g_hWnd_download_directory = NULL;
HWND g_hWnd_btn_download_directory = NULL;

HWND g_hWnd_chk_simulate_download = NULL;

HWND g_hWnd_static_download_parts = NULL;
HWND g_hWnd_download_parts = NULL;
HWND g_hWnd_ud_download_parts = NULL;

HWND g_hWnd_static_ssl_version = NULL;
HWND g_hWnd_ssl_version = NULL;

HWND g_hWnd_btn_authentication = NULL;
HWND g_hWnd_static_username = NULL;
HWND g_hWnd_edit_username = NULL;
HWND g_hWnd_static_password = NULL;
HWND g_hWnd_edit_password = NULL;

HWND g_hWnd_static_cookies = NULL;
HWND g_hWnd_edit_cookies = NULL;

HWND g_hWnd_static_headers = NULL;
HWND g_hWnd_edit_headers = NULL;

HWND g_hWnd_btn_advanced = NULL;

HWND g_hWnd_btn_download = NULL;
HWND g_hWnd_cancel = NULL;

bool g_show_advanced = false;

wchar_t *t_download_directory = NULL;

WNDPROC URLProc = NULL;

IDropTarget *Add_DropTarget;

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
	}

	return _CallWindowProcW( URLProc, hWnd, msg, wParam, lParam );
}

LRESULT CALLBACK AddURLsWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch ( msg )
    {
		case WM_CREATE:
		{
			HWND hWnd_static_urls = _CreateWindowW( WC_STATIC, ST_URL_s__, WS_CHILD | WS_VISIBLE, 20, 20, 100, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_edit_add = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, L"", WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL | ES_MULTILINE | WS_HSCROLL | WS_VSCROLL | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_edit_add, EM_LIMITTEXT, 0, 0 );	// Maximum size.

			g_hWnd_static_download_directory = _CreateWindowW( WC_STATIC, ST_Download_directory_, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_download_directory = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, cfg_default_download_directory, ES_AUTOHSCROLL | ES_READONLY | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_btn_download_directory = _CreateWindowW( WC_BUTTON, ST_BTN___, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_DOWNLOAD_DIRECTORY, NULL, NULL );


			g_hWnd_static_download_parts = _CreateWindowW( WC_STATIC, ST_Download_parts_, WS_CHILD, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_download_parts = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, hWnd, ( HMENU )EDIT_DOWNLOAD_PARTS, NULL, NULL );

			// Keep this unattached. Looks ugly inside the text box.
			g_hWnd_ud_download_parts = _CreateWindowW( UPDOWN_CLASS, NULL, /*UDS_ALIGNRIGHT |*/ UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_download_parts, EM_LIMITTEXT, 3, 0 );
			_SendMessageW( g_hWnd_ud_download_parts, UDM_SETBUDDY, ( WPARAM )g_hWnd_download_parts, 0 );
            _SendMessageW( g_hWnd_ud_download_parts, UDM_SETBASE, 10, 0 );
			_SendMessageW( g_hWnd_ud_download_parts, UDM_SETRANGE32, 1, 100 );
			_SendMessageW( g_hWnd_ud_download_parts, UDM_SETPOS, 0, cfg_default_download_parts );


			g_hWnd_static_ssl_version = _CreateWindowW( WC_STATIC, ST_SSL___TLS_version_, WS_CHILD, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_ssl_version = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_COMBOBOX, NULL, CBS_AUTOHSCROLL | CBS_DROPDOWNLIST | WS_CHILD | WS_TABSTOP | WS_VSCROLL, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			_SendMessageW( g_hWnd_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_SSL_2_0 );
			_SendMessageW( g_hWnd_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_SSL_3_0 );
			_SendMessageW( g_hWnd_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_TLS_1_0 );
			_SendMessageW( g_hWnd_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_TLS_1_1 );
			_SendMessageW( g_hWnd_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_TLS_1_2 );

			_SendMessageW( g_hWnd_ssl_version, CB_SETCURSEL, cfg_default_ssl_version, 0 );

			g_hWnd_btn_authentication = _CreateWindowW( WC_BUTTON, ST_Authentication, BS_GROUPBOX | WS_CHILD, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			g_hWnd_static_username = _CreateWindowW( WC_STATIC, ST_Username_, WS_CHILD, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_edit_username = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			g_hWnd_static_password = _CreateWindowW( WC_STATIC, ST_Password_, WS_CHILD, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_edit_password = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_PASSWORD | ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			g_hWnd_static_cookies = _CreateWindowW( WC_STATIC, ST_Cookies_, WS_CHILD, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_edit_cookies = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, L"", WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL | ES_MULTILINE | WS_HSCROLL | WS_VSCROLL, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			g_hWnd_static_headers = _CreateWindowW( WC_STATIC, ST_Headers_, WS_CHILD, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_edit_headers = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, L"", WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL | ES_MULTILINE | WS_HSCROLL | WS_VSCROLL, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			g_hWnd_chk_simulate_download = _CreateWindowW( WC_BUTTON, ST_Simulate_download, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, hWnd, ( HMENU )CHK_SIMULATE_DOWNLOAD, NULL, NULL );

			g_hWnd_btn_advanced = _CreateWindowW( WC_BUTTON, ST_Advanced_BB, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_ADVANCED, NULL, NULL );

			g_hWnd_btn_download = _CreateWindowW( WC_BUTTON, ST_Download, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_DOWNLOAD, NULL, NULL );
			g_hWnd_cancel = _CreateWindowW( WC_BUTTON, ST_Cancel, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_ADD_CANCEL, NULL, NULL );

			_SetFocus( g_hWnd_edit_add );

			_SendMessageW( g_hWnd_btn_download, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_cancel, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( hWnd_static_urls, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_edit_add, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_static_ssl_version, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_ssl_version, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_static_download_parts, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_download_parts, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_simulate_download, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_static_download_directory, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_download_directory, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_btn_download_directory, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_btn_authentication, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_static_username, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_edit_username, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_static_password, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_edit_password, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_static_cookies, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_edit_cookies, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_static_headers, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_edit_headers, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_btn_advanced, WM_SETFONT, ( WPARAM )hFont, 0 );

			t_download_directory = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * MAX_PATH );
			_wmemcpy_s( t_download_directory, MAX_PATH, cfg_default_download_directory, g_default_download_directory_length );
			t_download_directory[ g_default_download_directory_length ] = 0;	// Sanity.

			URLProc = ( WNDPROC )_GetWindowLongW( g_hWnd_edit_add, GWL_WNDPROC );
			_SetWindowLongW( g_hWnd_edit_add, GWL_WNDPROC, ( LONG )URLSubProc );
			_SetWindowLongW( g_hWnd_edit_cookies, GWL_WNDPROC, ( LONG )URLSubProc );
			_SetWindowLongW( g_hWnd_edit_headers, GWL_WNDPROC, ( LONG )URLSubProc );

			#ifndef OLE32_USE_STATIC_LIB
				if ( ole32_state == OLE32_STATE_SHUTDOWN )
				{
					use_drag_and_drop = InitializeOle32();
				}
			#endif

			if ( use_drag_and_drop )
			{
				_OleInitialize( NULL );

				RegisterDropWindow( g_hWnd_edit_add, &Add_DropTarget );
			}

			return 0;
		}
		break;

		case WM_CTLCOLORSTATIC:
		{
			//if ( ( HWND )lParam != g_hWnd_chk_simulate_download )
			//{
				return ( LRESULT )( _GetSysColorBrush( COLOR_WINDOW ) );
			//}
			//else
			//{
			//	return _DefWindowProcW( hWnd, msg, wParam, lParam );
			//}
		}
		break;

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hDC = _BeginPaint( hWnd, &ps );

			RECT client_rc, frame_rc;
			_GetClientRect( hWnd, &client_rc );

			// Create a memory buffer to draw to.
			HDC hdcMem = _CreateCompatibleDC( hDC );

			HBITMAP hbm = _CreateCompatibleBitmap( hDC, client_rc.right - client_rc.left, client_rc.bottom - client_rc.top );
			HBITMAP ohbm = ( HBITMAP )_SelectObject( hdcMem, hbm );
			_DeleteObject( ohbm );
			_DeleteObject( hbm );

			// Fill the background.
			HBRUSH color = _CreateSolidBrush( ( COLORREF )_GetSysColor( COLOR_MENU ) );
			_FillRect( hdcMem, &client_rc, color );
			_DeleteObject( color );

			frame_rc = client_rc;
			frame_rc.left += 10;
			frame_rc.right -= 10;
			frame_rc.top += 10;
			frame_rc.bottom -= 40;

			// Fill the frame.
			color = _CreateSolidBrush( ( COLORREF )_GetSysColor( COLOR_WINDOW ) );
			_FillRect( hdcMem, &frame_rc, color );
			_DeleteObject( color );

			// Draw the frame's border.
			_DrawEdge( hdcMem, &frame_rc, EDGE_ETCHED, BF_RECT );

			// Draw our memory buffer to the main device context.
			_BitBlt( hDC, client_rc.left, client_rc.top, client_rc.right, client_rc.bottom, hdcMem, 0, 0, SRCCOPY );

			_DeleteDC( hdcMem );
			_EndPaint( hWnd, &ps );

			return 0;
		}
		break;

		case WM_SIZE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			// Allow our controls to move in relation to the parent window.
			HDWP hdwp = _BeginDeferWindowPos( 22 );
			_DeferWindowPos( hdwp, g_hWnd_edit_add, HWND_TOP, 20, 35, rc.right - 40, rc.bottom - ( g_show_advanced ? 420 : 130 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_download_directory, HWND_TOP, 20, rc.bottom - ( g_show_advanced ? 375 : 85 ), 150, 15, SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_download_directory, HWND_TOP, 20, rc.bottom - ( g_show_advanced ? 360 : 70 ), rc.right - 80, 20, SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_btn_download_directory, HWND_TOP, rc.right - 55, rc.bottom - ( g_show_advanced ? 360 : 70 ), 35, 20, SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_download_parts, HWND_TOP, 20, rc.bottom - 330, 100, 15, SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_download_parts, HWND_TOP, 20, rc.bottom - 315, 85, 20, SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_ud_download_parts, HWND_TOP, 105, rc.bottom - 316, _GetSystemMetrics( SM_CXVSCROLL ), 22, SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_ssl_version, HWND_TOP, 130, rc.bottom - 330, 100, 15, SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_ssl_version, HWND_TOP, 130, rc.bottom - 315, 100, 20, SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_btn_authentication, HWND_TOP, 240, rc.bottom - 330, 230, 65, SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_static_username, HWND_TOP, 250, rc.bottom - 315, 100, 15, SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_edit_username, HWND_TOP, 250, rc.bottom - 300, 100, 20, SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_static_password, HWND_TOP, 360, rc.bottom - 315, 100, 15, SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_edit_password, HWND_TOP, 360, rc.bottom - 300, 100, 20, SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_cookies, HWND_TOP, 20, rc.bottom - 265, 100, 15, SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_edit_cookies, HWND_TOP, 20, rc.bottom - 250, rc.right - 40, 75, SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_headers, HWND_TOP, 20, rc.bottom - 165, 100, 15, SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_edit_headers, HWND_TOP, 20, rc.bottom - 150, rc.right - 40, 75, SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_chk_simulate_download, HWND_TOP, 20, rc.bottom - 65, 120, 20, SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_btn_advanced, HWND_TOP, 10, rc.bottom - 32, 95, 23, SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_btn_download, HWND_TOP, rc.right - 175, rc.bottom - 32, 80, 23, SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_cancel, HWND_TOP, rc.right - 90, rc.bottom - 32, 80, 23, SWP_NOZORDER );
			_EndDeferWindowPos( hdwp );

			rc.left = 5;
			rc.top = 5;
			rc.right -= 65;
			rc.bottom -= ( g_show_advanced ? 440 : 150 );
			_SendMessageW( g_hWnd_edit_add, EM_SETRECT, 0, ( LPARAM )&rc );

			return 0;
		}
		break;

		case WM_GETMINMAXINFO:
		{
			// Set the minimum dimensions that the window can be sized to.
			( ( MINMAXINFO * )lParam )->ptMinTrackSize.x = 505;
			( ( MINMAXINFO * )lParam )->ptMinTrackSize.y = ( g_show_advanced ? 530 : 240 );

			return 0;
		}
		break;

		case WM_COMMAND:
		{
			switch( LOWORD( wParam ) )
			{
				case BTN_DOWNLOAD:
				{
					unsigned char download_operations = ( _SendMessageW( g_hWnd_chk_simulate_download, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? DOWNLOAD_OPERATION_SIMULATE : DOWNLOAD_OPERATION_NONE );

					if ( !( download_operations & DOWNLOAD_OPERATION_SIMULATE ) && t_download_directory == NULL )
					{
						_MessageBoxW( hWnd, ST_You_must_supply_download_directory, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONWARNING );

						_SendMessageW( hWnd, WM_COMMAND, MAKEWPARAM( BTN_DOWNLOAD_DIRECTORY, 0 ), 0 );

						break;
					}

					unsigned int edit_length = _SendMessageW( g_hWnd_edit_add, WM_GETTEXTLENGTH, 0, 0 );

					// http://a.b
					if ( edit_length >= 10 )
					{
						ADD_INFO *ai = ( ADD_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( ADD_INFO ) );

						ai->download_directory = t_download_directory;
						t_download_directory = NULL;

						ai->ssl_version = ( char )_SendMessageW( g_hWnd_ssl_version, CB_GETCURSEL, 0, 0 );

						ai->download_operations = download_operations;

						char value[ 11 ];
						_SendMessageA( g_hWnd_download_parts, WM_GETTEXT, 11, ( LPARAM )value );
						ai->parts = ( unsigned char )_strtoul( value, NULL, 10 );

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
						edit_length = _SendMessageW( g_hWnd_edit_username, WM_GETTEXTLENGTH, 0, 0 );
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
						edit_length = _SendMessageW( g_hWnd_edit_password, WM_GETTEXTLENGTH, 0, 0 );
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

						edit_length = _SendMessageW( g_hWnd_edit_cookies, WM_GETTEXTLENGTH, 0, 0 );
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
						edit_length = _SendMessageW( g_hWnd_edit_headers, WM_GETTEXTLENGTH, 0, 0 );
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

						// ai is freed in AddURL.
						CloseHandle( _CreateThread( NULL, 0, AddURL, ( void * )ai, 0, NULL ) );
					}

					_SendMessageW( hWnd, WM_CLOSE, 0, 0 );
				}
				break;

				case BTN_ADD_CANCEL:
				{
					_SendMessageW( hWnd, WM_CLOSE, 0, 0 );
				}
				break;

				case EDIT_DOWNLOAD_PARTS:
				{
					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						DWORD sel_start = 0;

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

				case CHK_SIMULATE_DOWNLOAD:
				{
					BOOL enable = ( _SendMessageW( g_hWnd_chk_simulate_download, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? FALSE : TRUE );
					_EnableWindow( g_hWnd_download_directory, enable );
					_EnableWindow( g_hWnd_btn_download_directory, enable );
				}
				break;

				case BTN_DOWNLOAD_DIRECTORY:
				{
					// Open a browse for folder dialog box.
					BROWSEINFO bi;
					_memzero( &bi, sizeof( BROWSEINFO ) );
					bi.hwndOwner = hWnd;
					bi.lpszTitle = ST_Select_the_download_directory;
					bi.ulFlags = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_VALIDATE;

					bool destroy = true;
					#ifndef OLE32_USE_STATIC_LIB
						if ( ole32_state == OLE32_STATE_SHUTDOWN )
						{
							destroy = InitializeOle32();
						}
					#endif

					if ( destroy )
					{
						// OleInitialize calls CoInitializeEx
						_OleInitialize( NULL );
					}

					LPITEMIDLIST lpiidl = _SHBrowseForFolderW( &bi );
					if ( lpiidl )
					{
						if ( t_download_directory == NULL )
						{
							t_download_directory = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * MAX_PATH );
						}
						_memzero( t_download_directory, sizeof( wchar_t ) * MAX_PATH );

						// Get the directory path from the id list.
						_SHGetPathFromIDListW( lpiidl, ( LPTSTR )t_download_directory );

						if ( t_download_directory[ 0 ] != NULL )
						{
							_SendMessageW( g_hWnd_download_directory, WM_SETTEXT, 0, ( LPARAM )t_download_directory );
						}

						if ( destroy )
						{
							_CoTaskMemFree( lpiidl );
						}
						else	// Warn of leak if we can't free.
						{
							_MessageBoxW( NULL, L"Item ID List was not freed.", PROGRAM_CAPTION, 0 );
						}
					}

					if ( destroy )
					{
						_OleUninitialize();
					}
				}
				break;

				case BTN_ADVANCED:
				{
					g_show_advanced = !g_show_advanced;

					int sw_type = SW_HIDE;

					if ( g_show_advanced )
					{
						sw_type = SW_SHOW;

						_SendMessageW( g_hWnd_btn_advanced, WM_SETTEXT, 0, ( LPARAM )ST_Advanced_AB );
					}
					else
					{
						_SendMessageW( g_hWnd_btn_advanced, WM_SETTEXT, 0, ( LPARAM )ST_Advanced_BB );
					}

					_ShowWindow( g_hWnd_static_ssl_version, sw_type );
					_ShowWindow( g_hWnd_ssl_version, sw_type );
					_ShowWindow( g_hWnd_static_download_parts, sw_type );
					_ShowWindow( g_hWnd_download_parts, sw_type );
					_ShowWindow( g_hWnd_ud_download_parts, sw_type );
					_ShowWindow( g_hWnd_chk_simulate_download, sw_type );
					_ShowWindow( g_hWnd_btn_authentication, sw_type );
					_ShowWindow( g_hWnd_static_username, sw_type );
					_ShowWindow( g_hWnd_edit_username, sw_type );
					_ShowWindow( g_hWnd_static_password, sw_type );
					_ShowWindow( g_hWnd_edit_password, sw_type );
					_ShowWindow( g_hWnd_static_cookies, sw_type );
					_ShowWindow( g_hWnd_edit_cookies, sw_type );
					_ShowWindow( g_hWnd_static_headers, sw_type );
					_ShowWindow( g_hWnd_edit_headers, sw_type );

					// Adjust the window height.
					RECT rc;
					_GetWindowRect( hWnd, &rc );

					if ( g_show_advanced && ( rc.bottom - rc.top ) < 430 )
					{
						_SetWindowPos( hWnd, NULL, 0, 0, rc.right - rc.left, 430, SWP_NOMOVE );
					}
					else	// Force a resize of our controls.
					{
						_SendMessageW( hWnd, WM_SIZE, 0, 0 );
					}
				}
				break;
			}

			return 0;
		}

		case WM_PROPAGATE:
		{
			if ( wParam == CF_UNICODETEXT || wParam == CF_HTML )
			{
				SendMessageW( g_hWnd_edit_add, EM_REPLACESEL, 0, lParam );
			}
			else// if ( wParam == CF_TEXT )
			{
				SendMessageA( g_hWnd_edit_add, EM_REPLACESEL, 0, lParam );
			}

			// Append a newline after our dropped text.
			SendMessageW( g_hWnd_edit_add, EM_REPLACESEL, 0, ( LPARAM )L"\r\n" );

			_ShowWindow( hWnd, SW_SHOWNORMAL );

			_SetForegroundWindow( hWnd );
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
			_DestroyWindow( hWnd );

			return 0;
		}
		break;

		case WM_DESTROY:
		{
			if ( use_drag_and_drop )
			{
				UnregisterDropWindow( g_hWnd_edit_add, Add_DropTarget );

				_OleUninitialize();
			}

			if ( t_download_directory != NULL )
			{
				GlobalFree( t_download_directory );
				t_download_directory = NULL;
			}

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
