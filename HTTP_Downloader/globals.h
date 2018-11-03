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

#ifndef _GLOBALS_H
#define _GLOBALS_H

// Pretty window.
#pragma comment( linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"" )

#define STRICT
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <commctrl.h>
#include <process.h>

#include "lite_user32.h"
#include "lite_shell32.h"
#include "lite_ntdll.h"

#include "dllrbt.h"

#include "resource.h"

#define MIN_WIDTH			640
#define MIN_HEIGHT			480

#define SNAP_WIDTH			10		// The minimum distance at which our windows will attach together.

#define WM_DESTROY_ALT		WM_APP		// Allows non-window threads to call DestroyWindow.
#define WM_PROPAGATE		WM_APP + 1
#define WM_CHANGE_CURSOR	WM_APP + 2	// Updates the window cursor.

#define WM_TRAY_NOTIFY		WM_APP + 3
#define WM_EXIT				WM_APP + 4
#define WM_ALERT			WM_APP + 5

#define FILETIME_TICKS_PER_SECOND	10000000LL

#define NUM_COLUMNS			14

#define _wcsicmp_s( a, b ) ( ( a == NULL && b == NULL ) ? 0 : ( a != NULL && b == NULL ) ? 1 : ( a == NULL && b != NULL ) ? -1 : lstrcmpiW( a, b ) )
#define _stricmp_s( a, b ) ( ( a == NULL && b == NULL ) ? 0 : ( a != NULL && b == NULL ) ? 1 : ( a == NULL && b != NULL ) ? -1 : lstrcmpiA( a, b ) )

#define SAFESTRA( s ) ( s != NULL ? s : "" )
#define SAFESTR2A( s1, s2 ) ( s1 != NULL ? s1 : ( s2 != NULL ? s2 : "" ) )

#define SAFESTRW( s ) ( s != NULL ? s : L"" )
#define SAFESTR2W( s1, s2 ) ( s1 != NULL ? s1 : ( s2 != NULL ? s2 : L"" ) )

#define is_close( a, b ) ( _abs( a - b ) < SNAP_WIDTH )

#define is_digit_a( c ) ( c - '0' + 0U <= 9U )
#define is_digit_w( c ) ( c - L'0' + 0U <= 9U )

#define GET_X_LPARAM( lp )	( ( int )( short )LOWORD( lp ) )
#define GET_Y_LPARAM( lp )	( ( int )( short )HIWORD( lp ) )

#define PROGRAM_CAPTION		L"HTTP Downloader"
#define PROGRAM_CAPTION_A	"HTTP Downloader"

#define HOME_PAGE			L"https://erickutcher.github.io/#HTTP_Downloader"

#define SIZE_FORMAT_BYTE		0
#define SIZE_FORMAT_KILOBYTE	1
#define SIZE_FORMAT_MEGABYTE	2
#define SIZE_FORMAT_GIGABYTE	3
#define SIZE_FORMAT_AUTO		4

struct sortinfo
{
	HWND hWnd;
	int column;
	unsigned char direction;
};

struct ICON_INFO
{
	wchar_t *file_extension;
	HICON icon;
	unsigned int count;
};

struct SEARCH_INFO
{
	wchar_t *text;
	unsigned char type;			// 0 = Filename, 1 = URL
	unsigned char case_flag;	// 0x00 = None, 0x01 = Match case, 0x02 = Match whole word.
	bool search_all;
};

union QFILETIME
{
	FILETIME ft;
	ULONGLONG ull;
};

// These are all variables that are shared among the separate .cpp files.

// Object handles.
extern HWND g_hWnd_main;				// Handle to our main window.
extern HWND g_hWnd_add_urls;
extern HWND g_hWnd_options;
extern HWND g_hWnd_update_download;
extern HWND g_hWnd_search;
extern HWND g_hWnd_url_drop_window;

extern HWND g_hWnd_toolbar;
extern HWND g_hWnd_files;

extern HWND g_hWnd_active;				// Handle to the active window. Used to handle tab stops.

extern HFONT hFont;

extern dllrbt_tree *icon_handles;

extern bool	can_fast_allocate;			// Prevent the pre-allocation from zeroing the file.

extern int row_height;

extern bool skip_list_draw;

extern int g_file_size_cmb_ret;			// Message box prompt for large files sizes.

extern HANDLE worker_semaphore;			// Blocks shutdown while a worker thread is active.
extern bool in_worker_thread;
extern bool kill_worker_thread_flag;	// Allow for a clean shutdown.

extern bool download_history_changed;

extern HANDLE downloader_ready_semaphore;

extern CRITICAL_SECTION worker_cs;				// Worker thread critical section.

extern CRITICAL_SECTION session_totals_cs;

extern CRITICAL_SECTION icon_cache_cs;

extern wchar_t *base_directory;
extern unsigned int base_directory_length;

extern unsigned int g_default_download_directory_length;

extern NOTIFYICONDATA g_nid;					// Tray icon information.

extern UINT CF_HTML;	// Clipboard format.

extern unsigned long long session_total_downloaded;
extern unsigned long long session_downloaded_speed;

extern unsigned long long last_session_total_downloaded;
extern unsigned long long last_session_downloaded_speed;

// Settings

extern int cfg_pos_x;
extern int cfg_pos_y;
extern int cfg_width;
extern int cfg_height;

extern char cfg_min_max;

extern int cfg_column_width1;
extern int cfg_column_width2;
extern int cfg_column_width3;
extern int cfg_column_width4;
extern int cfg_column_width5;
extern int cfg_column_width6;
extern int cfg_column_width7;
extern int cfg_column_width8;
extern int cfg_column_width9;
extern int cfg_column_width10;
extern int cfg_column_width11;
extern int cfg_column_width12;
extern int cfg_column_width13;
extern int cfg_column_width14;

extern char cfg_column_order1;
extern char cfg_column_order2;
extern char cfg_column_order3;
extern char cfg_column_order4;
extern char cfg_column_order5;
extern char cfg_column_order6;
extern char cfg_column_order7;
extern char cfg_column_order8;
extern char cfg_column_order9;
extern char cfg_column_order10;
extern char cfg_column_order11;
extern char cfg_column_order12;
extern char cfg_column_order13;
extern char cfg_column_order14;

extern bool cfg_show_toolbar;
extern bool cfg_show_status_bar;

extern unsigned char cfg_t_downloaded;	// 0 = Bytes, 1 = KB, 2 = MB, 3 = GB, 4 = auto
extern unsigned char cfg_t_file_size;	// 0 = Bytes, 1 = KB, 2 = MB, 3 = GB, 4 = auto
extern unsigned char cfg_t_down_speed;	// 0 = Bytes, 1 = KB, 2 = MB, 3 = GB, 4 = auto

extern unsigned char cfg_t_status_downloaded;	// 0 = Bytes, 1 = KB, 2 = MB, 3 = GB, 4 = auto
extern unsigned char cfg_t_status_down_speed;	// 0 = Bytes, 1 = KB, 2 = MB, 3 = GB, 4 = auto

extern int cfg_drop_pos_x;	// URL drop window.
extern int cfg_drop_pos_y;	// URL drop window.

extern bool cfg_tray_icon;
extern bool cfg_close_to_tray;
extern bool cfg_minimize_to_tray;

extern bool cfg_always_on_top;
extern bool cfg_enable_download_history;
extern bool cfg_enable_quick_allocation;
extern bool cfg_set_filetime;
extern bool cfg_use_one_instance;
extern bool cfg_enable_drop_window;
extern bool cfg_download_immediately;

extern bool cfg_play_sound;
extern wchar_t *cfg_sound_file_path;

extern unsigned long cfg_thread_count;
extern unsigned long g_max_threads;

extern unsigned char cfg_max_downloads;

extern unsigned char cfg_retry_downloads_count;
extern unsigned char cfg_retry_parts_count;

extern unsigned char cfg_default_ssl_version;
extern unsigned char cfg_default_download_parts;

extern unsigned char cfg_max_redirects;

extern wchar_t *cfg_default_download_directory;

// Server

extern bool cfg_enable_server;
extern unsigned char cfg_server_address_type;
extern unsigned long cfg_server_ip_address;
extern wchar_t *cfg_server_hostname;
extern unsigned short cfg_server_port;

extern unsigned char cfg_server_ssl_version;

extern bool cfg_server_enable_ssl;

extern unsigned char cfg_certificate_type;

extern wchar_t *cfg_certificate_pkcs_file_name;
extern wchar_t *cfg_certificate_pkcs_password;

extern wchar_t *cfg_certificate_cer_file_name;
extern wchar_t *cfg_certificate_key_file_name;

extern bool cfg_use_authentication;
extern wchar_t *cfg_authentication_username;
extern wchar_t *cfg_authentication_password;
extern unsigned char cfg_authentication_type;

// HTTP proxy
extern bool cfg_enable_proxy;
extern unsigned char cfg_address_type;	// 0 = Host name, 1 = IP address
extern unsigned long cfg_ip_address;
extern wchar_t *cfg_hostname;
extern unsigned short cfg_port;

extern wchar_t *cfg_proxy_auth_username;
extern wchar_t *cfg_proxy_auth_password;

extern wchar_t *g_punycode_hostname;

extern char *g_proxy_auth_username;
extern char *g_proxy_auth_password;
extern char *g_proxy_auth_key;
extern unsigned long g_proxy_auth_key_length;

// HTTPS proxy
extern bool cfg_enable_proxy_s;
extern unsigned char cfg_address_type_s;	// 0 = Host name, 1 = IP address
extern unsigned long cfg_ip_address_s;
extern wchar_t *cfg_hostname_s;
extern unsigned short cfg_port_s;

extern wchar_t *cfg_proxy_auth_username_s;
extern wchar_t *cfg_proxy_auth_password_s;

extern wchar_t *g_punycode_hostname_s;

extern char *g_proxy_auth_username_s;
extern char *g_proxy_auth_password_s;
extern char *g_proxy_auth_key_s;
extern unsigned long g_proxy_auth_key_length_s;

//

extern unsigned short cfg_timeout;

extern char *download_columns[ NUM_COLUMNS ];
extern int *download_columns_width[ NUM_COLUMNS ];

extern unsigned char g_total_columns;

extern HANDLE g_timeout_semaphore;	// For updating the connection states.
extern HANDLE g_timer_semaphore;	// For updating the listview.

extern bool g_timers_running;

extern SYSTEMTIME g_compile_time;

#endif
