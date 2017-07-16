/*
	HTTP Downloader can download files through HTTP and HTTPS connections.
	Copyright (C) 2015-2017 Eric Kutcher

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

#include "lite_comdlg32.h"
#include "lite_ole32.h"
#include "lite_shell32.h"
#include "lite_zlib1.h"
#include "lite_normaliz.h"

#include "http_parsing.h"

#include "utilities.h"
#include "list_operations.h"

#include "cmessagebox.h"

#include "menus.h"

#include "doublylinkedlist.h"

HANDLE g_hIOCP = NULL;

WSAEVENT g_cleanup_event[ 1 ];

bool g_end_program = false;

DoublyLinkedList *context_list = NULL;

unsigned long total_downloading = 0;
DoublyLinkedList *download_queue = NULL;

DoublyLinkedList *active_download_list = NULL;	// List of active DOWNLOAD_INFO objects.

DoublyLinkedList *file_size_prompt_list = NULL;	// List of downloads that need to be prompted to continue.

DoublyLinkedList *rename_file_prompt_list = NULL;	// List of downloads that need to be prompted to continue.

void FreeContexts();

HANDLE g_timeout_semaphore = NULL;

CRITICAL_SECTION context_list_cs;				// Guard access to the global context list.
CRITICAL_SECTION active_download_list_cs;		// Guard access to the global active download list.
CRITICAL_SECTION download_queue_cs;				// Guard access to the download queue.
CRITICAL_SECTION file_size_prompt_list_cs;		// Guard access to the file size prompt list.
CRITICAL_SECTION rename_file_prompt_list_cs;	// Guard access to the rename file prompt list.
CRITICAL_SECTION cleanup_cs;

LPFN_CONNECTEX _ConnectEx = NULL;

bool file_size_prompt_active = false;
int g_file_size_cmb_ret = 0;	// Message box prompt for large files sizes.

bool rename_file_prompt_active = false;
int g_rename_file_cmb_ret = 0;	// Message box prompt to rename files.
int g_rename_file_cmb_ret2 = 0;	// Message box prompt to rename files.

bool g_timers_running = false;

// This should be done in a critical section.
void EnableTimers( bool timer_state )
{
	// Trigger the timers out of their infinite wait.
	if ( timer_state )
	{
		if ( !g_timers_running )
		{
			g_timers_running = true;

			if ( g_timeout_semaphore != NULL )
			{
				ReleaseSemaphore( g_timeout_semaphore, 1, NULL );
			}

			if ( g_timer_semaphore != NULL )
			{
				ReleaseSemaphore( g_timer_semaphore, 1, NULL );
			}
		}
	}
	else	// Let the timers complete their current task and then wait indefinitely.
	{
		UpdateMenus( true );

		g_timers_running = false;
	}
}

DWORD WINAPI Timeout( LPVOID WorkThreadContext )
{
	bool run_timer = g_timers_running;

	while ( !g_end_program )
	{
		// Check the timeout counter every second, or wait indefinitely if we're using the system default.
		WaitForSingleObject( g_timeout_semaphore, ( run_timer ? ( cfg_timeout > 0 ? 1000 : INFINITE ) : INFINITE ) );

		if ( g_end_program )
		{
			break;
		}

		// This will allow the timer to go through at least one loop after it's been disabled (g_timers_running == false).
		run_timer = g_timers_running;

		if ( TryEnterCriticalSection( &context_list_cs ) == TRUE )
		{
			DoublyLinkedList *context_node = context_list;

			// Go through the list of active connections.
			while ( context_node != NULL )
			{
				if ( g_end_program )
				{
					break;
				}

				SOCKET_CONTEXT *context = ( SOCKET_CONTEXT * )context_node->data;

				if ( context->cleanup == 0 && context->status != STATUS_ALLOCATING_FILE )
				{
					// See if we've reached the timeout limit.
					if ( ( context->timeout >= cfg_timeout ) && ( cfg_timeout > 0 ) )
					{
						// Ignore paused and queued downloads.
						if ( context->status == STATUS_PAUSED || context->status == STATUS_QUEUED )
						{
							InterlockedExchange( &context->timeout, 0 );	// Reset timeout counter.
						}
						else
						{
							EnterCriticalSection( &context->context_cs );

							context->timed_out = TIME_OUT_TRUE;

							if ( context->cleanup == 0 )
							{
								context->cleanup = 2;

								InterlockedIncrement( &context->pending_operations );

								context->overlapped_close.current_operation = ( context->ssl != NULL ? IO_Shutdown : IO_Close );

								PostQueuedCompletionStatus( g_hIOCP, 0, ( ULONG_PTR )context, ( OVERLAPPED * )&context->overlapped_close );
							}

							LeaveCriticalSection( &context->context_cs );
						}
					}
					else	// Increment the timeout counter.
					{
						InterlockedIncrement( &context->timeout );
					}
				}

				context_node = context_node->next;
			}

			LeaveCriticalSection( &context_list_cs );
		}
	}

	CloseHandle( g_timeout_semaphore );
	g_timeout_semaphore = NULL;

	ExitThread( 0 );
	return 0;
}

DWORD WINAPI IOCPDownloader( LPVOID pArgs )
{
	HANDLE *g_ThreadHandles = NULL;

	DWORD dwThreadCount = cfg_thread_count;

	if ( ws2_32_state == WS2_32_STATE_SHUTDOWN )
	{
		#ifndef WS2_32_USE_STATIC_LIB
			if ( !InitializeWS2_32() ){ goto HARD_CLEANUP; }
		#else
			StartWS2_32();
		#endif
	}

	// Loads the CreateEx function pointer. Required for overlapped connects.
	if ( !LoadConnectEx() )
	{
		goto HARD_CLEANUP;
	}

	// Load our SSL functions.
	if ( ssl_state == SSL_STATE_SHUTDOWN )
	{
		if ( SSL_library_init() == 0 )
		{
			goto HARD_CLEANUP;
		}
	}

	g_ThreadHandles = ( HANDLE * )GlobalAlloc( GMEM_FIXED, sizeof( HANDLE ) * dwThreadCount );
	for ( unsigned int i = 0; i < dwThreadCount; ++i )
	{
		g_ThreadHandles[ i ] = INVALID_HANDLE_VALUE;
	}

	g_cleanup_event[ 0 ] = _WSACreateEvent();
	if ( g_cleanup_event[ 0 ] == WSA_INVALID_EVENT )
	{
		goto CLEANUP;
	}

	g_hIOCP = CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, 0, 0 );
    if ( g_hIOCP == NULL )
	{
		goto CLEANUP;
	}

	_WSAResetEvent( g_cleanup_event[ 0 ] );

	// Spawn our IOCP worker threads.
	for ( DWORD dwCPU = 0; dwCPU < dwThreadCount; ++dwCPU )
	{
		HANDLE hThread;
		DWORD dwThreadId;

		// Create worker threads to service the overlapped I/O requests.
		hThread = _CreateThread( NULL, 0, IOCPConnection, g_hIOCP, 0, &dwThreadId );
		if ( hThread == NULL )
		{
			break;
		}

		g_ThreadHandles[ dwCPU ] = hThread;
		hThread = INVALID_HANDLE_VALUE;
	}

	if ( downloader_ready_semaphore != NULL )
	{
		ReleaseSemaphore( downloader_ready_semaphore, 1, NULL );
	}

	g_timeout_semaphore = CreateSemaphore( NULL, 0, 1, NULL );

	//CloseHandle( _CreateThread( NULL, 0, Timeout, NULL, 0, NULL ) );
	HANDLE timeout_handle = _CreateThread( NULL, 0, Timeout, NULL, 0, NULL );
	SetThreadPriority( timeout_handle, THREAD_PRIORITY_LOWEST );
	CloseHandle( timeout_handle );

	_WSAWaitForMultipleEvents( 1, g_cleanup_event, TRUE, WSA_INFINITE, FALSE );

	g_end_program = true;

	// Causes the IOCP worker threads to exit.
	if ( g_hIOCP != NULL )
	{
		for ( DWORD i = 0; i < dwThreadCount; ++i )
		{
			PostQueuedCompletionStatus( g_hIOCP, 0, 0, NULL );
		}
	}

	// Make sure IOCP worker threads have exited.
	if ( WaitForMultipleObjects( dwThreadCount, g_ThreadHandles, TRUE, 1000 ) == WAIT_OBJECT_0 )
	{
		for ( DWORD i = 0; i < dwThreadCount; ++i )
		{
			if ( g_ThreadHandles[ i ] != INVALID_HANDLE_VALUE )
			{
				CloseHandle( g_ThreadHandles[ i ] );
				g_ThreadHandles[ i ] = INVALID_HANDLE_VALUE;
			}
		}
	}

	if ( g_timeout_semaphore != NULL )
	{
		ReleaseSemaphore( g_timeout_semaphore, 1, NULL );
	}

	// Clean up our context list.
	FreeContexts();

	download_queue = NULL;
	total_downloading = 0;

	if ( g_hIOCP != NULL )
	{
		CloseHandle( g_hIOCP );
		g_hIOCP = NULL;
	}

CLEANUP:

	if ( g_ThreadHandles != NULL )
	{
		GlobalFree( g_ThreadHandles );
		g_ThreadHandles = NULL;
	}

	if ( g_cleanup_event[ 0 ] != WSA_INVALID_EVENT )
	{
		_WSACloseEvent( g_cleanup_event[ 0 ] );
		g_cleanup_event[ 0 ] = WSA_INVALID_EVENT;
	}

HARD_CLEANUP:

	if ( downloader_ready_semaphore != NULL )
	{
		ReleaseSemaphore( downloader_ready_semaphore, 1, NULL );
	}

	_ExitThread( 0 );
	return 0;
}


bool LoadConnectEx()
{
	bool ret = false;

	DWORD bytes = 0;
	GUID connectex_guid = WSAID_CONNECTEX;

	if ( _ConnectEx == NULL )
	{
		SOCKET tmp_socket = CreateSocket();
		if ( tmp_socket != INVALID_SOCKET )
		{
			// Load the ConnectEx extension function from the provider for this socket.
			ret = ( _WSAIoctl( tmp_socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &connectex_guid, sizeof( connectex_guid ), &_ConnectEx, sizeof( _ConnectEx ), &bytes, NULL, NULL ) == SOCKET_ERROR ? false : true );

			_closesocket( tmp_socket );
		}
	}
	else
	{
		ret = true;
	}

	return ret;
}

SOCKET CreateSocket( bool IPv6 )
{
	int nZero = 0;
	SOCKET socket = INVALID_SOCKET;

	socket = _WSASocketW( ( IPv6 ? AF_INET6 : AF_INET ), SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED ); 
	if ( socket != INVALID_SOCKET )
	{
		// Disable send buffering on the socket.
		_setsockopt( socket, SOL_SOCKET, SO_SNDBUF, ( char * )&nZero, sizeof( nZero ) );
	}

	return socket;
}

SECURITY_STATUS DecryptRecv( SOCKET_CONTEXT *context, DWORD &io_size )
{
	SECURITY_STATUS scRet = SEC_E_INTERNAL_ERROR;

	WSABUF wsa_decrypt;

	DWORD bytes_decrypted = 0;

	if ( context->ssl->rd.scRet == SEC_E_INCOMPLETE_MESSAGE )
	{
		context->ssl->cbIoBuffer += io_size;
	}
	else
	{
		context->ssl->cbIoBuffer = io_size;
	}

	io_size = 0;
	
	context->ssl->continue_decrypt = false;

	wsa_decrypt = context->wsabuf;

	// Decrypt our buffer.
	while ( context->ssl->pbIoBuffer != NULL /*&& context->ssl->cbIoBuffer > 0*/ )
	{
		scRet = SSL_WSARecv_Decrypt( context->ssl, &wsa_decrypt, bytes_decrypted );

		io_size += bytes_decrypted;

		wsa_decrypt.buf += bytes_decrypted;
		wsa_decrypt.len -= bytes_decrypted;

		switch ( scRet )
		{
			// We've successfully decrypted a portion of the buffer.
			case SEC_E_OK:
			{
				// Decrypt more records if there are any.
				continue;
			}
			break;

			// The message was decrypted, but not all of it was copied to our wsabuf.
			// There may be incomplete records left to decrypt. DecryptRecv must be called again after processing wsabuf.
			case SEC_I_CONTINUE_NEEDED:
			{
				context->ssl->continue_decrypt = true;

				return scRet;
			}
			break;

			case SEC_E_INCOMPLETE_MESSAGE:	// The message was incomplete. Request more data from the server.
			case SEC_I_RENEGOTIATE:			// Client wants us to perform another handshake.
			{
				return scRet;
			}
			break;

			//case SEC_I_CONTEXT_EXPIRED:
			default:
			{
				context->ssl->cbIoBuffer = 0;

				return scRet;
			}
			break;
		}
	}

	context->ssl->cbIoBuffer = 0;

	return scRet;
}

THREAD_RETURN FileSizePrompt( void *pArguments )
{
	DOWNLOAD_INFO *di = NULL;

	bool skip_processing = false;

	do
	{
		EnterCriticalSection( &file_size_prompt_list_cs );

		DoublyLinkedList *di_node = file_size_prompt_list;

		DLL_RemoveNode( &file_size_prompt_list, di_node );

		if ( di_node != NULL )
		{
			di = ( DOWNLOAD_INFO * )di_node->data;

			GlobalFree( di_node );
		}

		LeaveCriticalSection( &file_size_prompt_list_cs );

		if ( di != NULL )
		{
			EnterCriticalSection( &di->shared_cs );

			// If we don't want to prevent all large downloads, then prompt the user.
			if ( g_file_size_cmb_ret != CMBIDNOALL && g_file_size_cmb_ret != CMBIDYESALL )
			{
				wchar_t file_path[ MAX_PATH ];
				_wmemcpy_s( file_path, MAX_PATH, di->file_path, MAX_PATH );
				if ( di->filename_offset > 0 )
				{
					file_path[ di->filename_offset - 1 ] = L'\\';	// Replace the download directory NULL terminator with a directory slash.
				}

				wchar_t prompt_message[ MAX_PATH + 100 ];
				__snwprintf( prompt_message, MAX_PATH + 100, L"%s will be %llu bytes in size.\r\n\r\nDo you want to continue downloading this file?", file_path, di->file_size );
				g_file_size_cmb_ret = CMessageBoxW( g_hWnd_main, prompt_message, PROGRAM_CAPTION, CMB_ICONWARNING | CMB_YESNOALL );
			}

			SOCKET_CONTEXT *context = ( SOCKET_CONTEXT * )di->parts_list->data;

			// Close all large downloads.
			if ( g_file_size_cmb_ret == CMBIDNO || g_file_size_cmb_ret == CMBIDNOALL )
			{
				EnterCriticalSection( &context->context_cs );

				context->status = STATUS_SKIPPED;

				if ( context->cleanup == 0 )
				{
					context->cleanup = 1;	// Auto cleanup.

					InterlockedIncrement( &context->pending_operations );

					context->overlapped_close.current_operation = ( context->ssl != NULL ? IO_Shutdown : IO_Close );

					PostQueuedCompletionStatus( g_hIOCP, 0, ( ULONG_PTR )context, ( OVERLAPPED * )&context->overlapped_close );
				}

				LeaveCriticalSection( &context->context_cs );
			}
			else	// Continue where we left off when getting the content.
			{
				InterlockedIncrement( &context->pending_operations );

				context->overlapped.current_operation = IO_ResumeGetContent;

				PostQueuedCompletionStatus( g_hIOCP, context->current_bytes_read, ( ULONG_PTR )context, ( OVERLAPPED * )&context->overlapped );
			}

			LeaveCriticalSection( &di->shared_cs );
		}

		EnterCriticalSection( &file_size_prompt_list_cs );

		if ( file_size_prompt_list == NULL )
		{
			skip_processing = true;

			file_size_prompt_active = false;
		}

		LeaveCriticalSection( &file_size_prompt_list_cs );
	}
	while ( !skip_processing );

	_ExitThread( 0 );
	return 0;
}

THREAD_RETURN RenameFilePrompt( void *pArguments )
{
	DOWNLOAD_INFO *di = NULL;

	bool skip_processing = false;

	do
	{
		EnterCriticalSection( &rename_file_prompt_list_cs );

		DoublyLinkedList *di_node = rename_file_prompt_list;

		DLL_RemoveNode( &rename_file_prompt_list, di_node );

		if ( di_node != NULL )
		{
			di = ( DOWNLOAD_INFO * )di_node->data;

			GlobalFree( di_node );
		}

		LeaveCriticalSection( &rename_file_prompt_list_cs );

		if ( di != NULL )
		{
			EnterCriticalSection( &di->shared_cs );

			SOCKET_CONTEXT *context = ( SOCKET_CONTEXT * )di->parts_list->data;

			wchar_t prompt_message[ MAX_PATH + 100 ];

			wchar_t file_path[ MAX_PATH ];
			_wmemcpy_s( file_path, MAX_PATH, di->file_path, MAX_PATH );
			if ( di->filename_offset > 0 )
			{
				file_path[ di->filename_offset - 1 ] = L'\\';	// Replace the download directory NULL terminator with a directory slash.
			}

			// If the last return value was not set to remember our choice, then prompt again.
			if ( g_rename_file_cmb_ret != CMBIDRENAMEALL && g_rename_file_cmb_ret != CMBIDOVERWRITEALL && g_rename_file_cmb_ret != CMBIDSKIPALL )
			{
				__snwprintf( prompt_message, MAX_PATH + 100, L"%s already exists.\r\n\r\nWhat operation would you like to perform?", file_path );

				g_rename_file_cmb_ret = CMessageBoxW( g_hWnd_main, prompt_message, PROGRAM_CAPTION, CMB_ICONWARNING | CMB_RENAMEOVERWRITESKIPALL );
			}

			// Rename the file and try again.
			if ( g_rename_file_cmb_ret == CMBIDRENAME || g_rename_file_cmb_ret == CMBIDRENAMEALL )
			{
				// Creates a tree of active and queued downloads.
				dllrbt_tree *add_files_tree = CreateFilenameTree();

				bool rename_succeeded = RenameFile( di, add_files_tree );

				// The tree is only used to determine duplicate filenames.
				DestroyFilenameTree( add_files_tree );

				if ( !rename_succeeded )
				{
					if ( g_rename_file_cmb_ret2 != CMBIDOKALL )
					{
						__snwprintf( prompt_message, MAX_PATH + 100, L"%s could not be renamed.\r\n\r\nYou will need to choose a different save directory.", file_path );

						g_rename_file_cmb_ret2 = CMessageBoxW( g_hWnd_main, prompt_message, PROGRAM_CAPTION, CMB_ICONWARNING | CMB_OKALL );
					}

					EnterCriticalSection( &context->context_cs );

					context->status = STATUS_FILE_IO_ERROR;

					if ( context->cleanup == 0 )
					{
						context->cleanup = 1;	// Auto cleanup.

						InterlockedIncrement( &context->pending_operations );

						context->overlapped_close.current_operation = ( context->ssl != NULL ? IO_Shutdown : IO_Close );

						PostQueuedCompletionStatus( g_hIOCP, 0, ( ULONG_PTR )context, ( OVERLAPPED * )&context->overlapped_close );
					}

					LeaveCriticalSection( &context->context_cs );
				}
				else	// Continue where we left off when getting the content.
				{
					InterlockedIncrement( &context->pending_operations );

					context->overlapped.current_operation = IO_ResumeGetContent;

					PostQueuedCompletionStatus( g_hIOCP, context->current_bytes_read, ( ULONG_PTR )context, ( OVERLAPPED * )&context->overlapped );
				}
			}
			else if ( g_rename_file_cmb_ret == CMBIDFAIL || g_rename_file_cmb_ret == CMBIDSKIP || g_rename_file_cmb_ret == CMBIDSKIPALL ) // Skip the rename or overwrite if the return value fails, or the user selected skip.
			{
				EnterCriticalSection( &context->context_cs );

				context->status = STATUS_SKIPPED;

				if ( context->cleanup == 0 )
				{
					context->cleanup = 1;	// Auto cleanup.

					InterlockedIncrement( &context->pending_operations );

					context->overlapped_close.current_operation = ( context->ssl != NULL ? IO_Shutdown : IO_Close );

					PostQueuedCompletionStatus( g_hIOCP, 0, ( ULONG_PTR )context, ( OVERLAPPED * )&context->overlapped_close );
				}

				LeaveCriticalSection( &context->context_cs );
			}
			else	// Continue where we left off when getting the content.
			{
				InterlockedIncrement( &context->pending_operations );

				context->overlapped.current_operation = IO_ResumeGetContent;

				PostQueuedCompletionStatus( g_hIOCP, context->current_bytes_read, ( ULONG_PTR )context, ( OVERLAPPED * )&context->overlapped );
			}

			LeaveCriticalSection( &di->shared_cs );
		}

		EnterCriticalSection( &rename_file_prompt_list_cs );

		if ( rename_file_prompt_list == NULL )
		{
			skip_processing = true;

			rename_file_prompt_active = false;
		}

		LeaveCriticalSection( &rename_file_prompt_list_cs );
	}
	while ( !skip_processing );

	_ExitThread( 0 );
	return 0;
}

DWORD WINAPI IOCPConnection( LPVOID WorkThreadContext )
{
	HANDLE hIOCP = ( HANDLE )WorkThreadContext;
	OVERLAPPEDEX *overlapped = NULL;
	DWORD io_size = 0;
	SOCKET_CONTEXT *context = NULL;
	IO_OPERATION *current_operation = NULL;
	IO_OPERATION *next_operation = NULL;

	BOOL completion_status = TRUE;

	bool use_ssl = false;

	SECURITY_STATUS scRet = SEC_E_INTERNAL_ERROR;
	bool sent = false;
	int nRet = 0;
	DWORD dwFlags = 0;

	while ( true )
	{
		completion_status = GetQueuedCompletionStatus( hIOCP, &io_size, ( ULONG_PTR * )&context, ( OVERLAPPED ** )&overlapped, INFINITE );

		if ( g_end_program )
		{
			break;
		}

		if ( overlapped != NULL && overlapped->context != NULL )
		{
			context = overlapped->context;

			current_operation = &overlapped->current_operation;
			next_operation = &overlapped->next_operation;
		}
		else
		{
			continue;
		}

		InterlockedExchange( &context->timeout, 0 );	// Reset timeout counter.

		InterlockedDecrement( &context->pending_operations );

		use_ssl = ( context->ssl != NULL ? true : false );

		if ( completion_status == FALSE )
		{
			if ( context->pending_operations > 0 )
			{
				EnterCriticalSection( &context->context_cs );

				context->cleanup = 1;

				LeaveCriticalSection( &context->context_cs );

				continue;
			}
			else if ( *current_operation != IO_Shutdown && *current_operation != IO_Close )
			{
				if ( *current_operation == IO_Connect )	// We couldn't establish a connection.
				{
					context->timed_out = TIME_OUT_RETRY;
				}

				*current_operation = ( use_ssl ? IO_Shutdown : IO_Close );
			}
		}
		else
		{
			if ( *current_operation == IO_GetContent ||
				 *current_operation == IO_GetCONNECTResponse )
			{
				// If there's no more data that was read.
				// Can occur when no file size has been set and the connection header is set to close.
				if ( io_size == 0 )
				{
					if ( *current_operation != IO_GetContent )
					{
						// We don't need to shutdown the SSL/TLS connection since it will not have been established yet.
						*current_operation = IO_Close;
					}
					else
					{
						*current_operation = ( use_ssl ? IO_Shutdown : IO_Close );
					}
				}
				else
				{
					bool skip_process = false;

					if ( context->status == STATUS_STOPPED ||
						 context->status == STATUS_STOP_AND_REMOVE )	// Stop or Stop and Remove.
					{
						if ( *current_operation != IO_GetContent )
						{
							// We don't need to shutdown the SSL/TLS connection since it will not have been established yet.
							*current_operation = IO_Close;
						}
						else
						{
							*current_operation = ( use_ssl ? IO_Shutdown : IO_Close );
						}
					}
					else if ( context->status == STATUS_PAUSED )	// Pause.
					{
						context->current_bytes_read = io_size;

						skip_process = true;
					}

					if ( skip_process )
					{
						continue;
					}
				}
			}
		}

		switch ( *current_operation )
		{
			case IO_Connect:
			{
				bool connection_failed = false;

				EnterCriticalSection( &context->context_cs );

				if ( context->cleanup == 0 )
				{
					// Allow the connect socket to inherit the properties of the previously set properties.
					// Must be done so that shutdown() will work.
					nRet = _setsockopt( context->socket, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0 );
					if ( nRet != SOCKET_ERROR )
					{
						if ( context->request_info.protocol == PROTOCOL_HTTPS )
						{
							char shared_protocol = ( context->download_info != NULL ? context->download_info->ssl_version : 0 );
							DWORD protocol = 0;
							switch ( shared_protocol )
							{
								case 4:	protocol |= SP_PROT_TLS1_2_CLIENT;
								case 3:	protocol |= SP_PROT_TLS1_1_CLIENT;
								case 2:	protocol |= SP_PROT_TLS1_CLIENT;
								case 1:	protocol |= SP_PROT_SSL3_CLIENT;
								case 0:	{ if ( shared_protocol < 4 ) { protocol |= SP_PROT_SSL2_CLIENT; } }
							}

							SSL *ssl = SSL_new( protocol );
							if ( ssl == NULL )
							{
								connection_failed = true;
							}
							else
							{
								ssl->s = context->socket;

								context->ssl = ssl;
							}
						}

						if ( context->download_info != NULL )
						{
							EnterCriticalSection( &context->download_info->shared_cs );

							if ( !connection_failed )
							{
								context->download_info->status = STATUS_DOWNLOADING;

								context->status = STATUS_DOWNLOADING;
							}
							else
							{
								context->status = STATUS_FAILED;
							}

							LeaveCriticalSection( &context->download_info->shared_cs );
						}

						InterlockedIncrement( &context->pending_operations );

						if ( !connection_failed )
						{
							// If it's an HTTPS request and we're not going through a SSL/TLS proxy, then begin the SSL/TLS handshake.
							if ( context->request_info.protocol == PROTOCOL_HTTPS && !cfg_enable_proxy_s )
							{
								*next_operation = IO_HandshakeResponse;

								SSL_WSAConnect( context, overlapped, context->request_info.host, sent );
								if ( !sent )
								{
									connection_failed = true;
								}
							}
							else	// HTTP and tunneled HTTPS requests send/recv data normally.
							{
								*current_operation = IO_Write;

								context->wsabuf.buf = context->buffer;
								context->wsabuf.len = BUFFER_SIZE;

								// Tunneled HTTPS requests need to send a CONNECT response before sending/receiving data.
								if ( context->request_info.protocol == PROTOCOL_HTTPS && cfg_enable_proxy_s )
								{
									*next_operation = IO_GetCONNECTResponse;

									ConstructRequest( context, true );
								}
								else	// HTTP request.
								{
									*next_operation = IO_GetContent;

									ConstructRequest( context, false );
								}

								nRet = _WSASend( context->socket, &context->wsabuf, 1, NULL, dwFlags, ( WSAOVERLAPPED * )overlapped, NULL );
								if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
								{
									connection_failed = true;
								}
							}
						}
					}
					else
					{
						connection_failed = true;
					}
				}
				else if ( context->cleanup == 2 )	// If we've forced the cleanup, then allow it to continue its steps.
				{
					context->cleanup = 1;
				}
				else
				{
					connection_failed = true;
				}

				if ( connection_failed )
				{
					*current_operation = IO_Close;

					PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
				}

				LeaveCriticalSection( &context->context_cs );
			}
			break;

			case IO_HandshakeResponse:
			case IO_HandshakeReply:
			{
				EnterCriticalSection( &context->context_cs );

				if ( context->cleanup == 0 )
				{
					context->wsabuf.buf = context->buffer;
					context->wsabuf.len = BUFFER_SIZE;

					InterlockedIncrement( &context->pending_operations );

					if ( *current_operation == IO_HandshakeReply )
					{
						context->ssl->cbIoBuffer += io_size;

						if ( context->ssl->cbIoBuffer > 0 )
						{
							*current_operation = IO_HandshakeResponse;
							*next_operation = IO_HandshakeResponse;

							scRet = SSL_WSAConnect_Reply( context, overlapped, sent );
						}
						else
						{
							scRet = SEC_E_INTERNAL_ERROR;
						}
					}
					else
					{
						*current_operation = IO_HandshakeReply;

						scRet = SSL_WSAConnect_Response( context, overlapped, sent );
					}

					if ( scRet == SEC_E_OK )
					{
						if ( !sent )
						{
							InterlockedDecrement( &context->pending_operations );
						}

						// Post request.

						InterlockedIncrement( &context->pending_operations );

						context->wsabuf.buf = context->buffer;
						context->wsabuf.len = BUFFER_SIZE;

						*next_operation = IO_GetContent;

						ConstructRequest( context, false );

						SSL_WSASend( context, overlapped, &context->wsabuf, sent );
						if ( !sent )
						{
							*current_operation = IO_Shutdown;

							PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
						}
					}
					else if ( scRet != SEC_I_CONTINUE_NEEDED && scRet != SEC_E_INCOMPLETE_MESSAGE && scRet != SEC_I_INCOMPLETE_CREDENTIALS )
					{
						// Have seen SEC_E_ILLEGAL_MESSAGE (for a bad target name in InitializeSecurityContext), SEC_E_BUFFER_TOO_SMALL, and SEC_E_MESSAGE_ALTERED.

						if ( !sent )
						{
							InterlockedDecrement( &context->pending_operations );
						}

						InterlockedIncrement( &context->pending_operations );

						*current_operation = IO_Close;

						PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
					}
				}
				else if ( context->cleanup == 2 )	// If we've forced the cleanup, then allow it to continue its steps.
				{
					context->cleanup = 1;
				}
				else
				{
					InterlockedIncrement( &context->pending_operations );

					*current_operation = IO_Close;

					PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
				}

				LeaveCriticalSection( &context->context_cs );
			}
			break;

			case IO_GetCONNECTResponse:
			{
				EnterCriticalSection( &context->context_cs );

				if ( context->cleanup == 0 )
				{
					context->current_bytes_read = io_size + ( context->wsabuf.buf - context->buffer );

					context->wsabuf.buf = context->buffer;
					context->wsabuf.len = BUFFER_SIZE;

					context->wsabuf.buf[ context->current_bytes_read ] = 0;	// Sanity.

					int content_status = ParseHTTPHeader( context, context->wsabuf.buf, context->current_bytes_read );

					if ( content_status == CONTENT_STATUS_READ_MORE_HEADER )	// Request more header data.
					{
						InterlockedIncrement( &context->pending_operations );

						// wsabuf will be offset in ParseHTTPHeader to handle more data.
						nRet = _WSARecv( context->socket, &context->wsabuf, 1, NULL, &dwFlags, ( WSAOVERLAPPED * )overlapped, NULL );
						if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
						{
							*current_operation = IO_Close;

							PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
						}
					}
					else if ( content_status == CONTENT_STATUS_FAILED )
					{
						context->status = STATUS_FAILED;

						InterlockedIncrement( &context->pending_operations );

						// We don't need to shutdown the SSL/TLS connection since it will not have been established yet.
						*current_operation = IO_Close;

						PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
					}
					else// if ( content_status == CONTENT_STATUS_GET_CONTENT );
					{
						// Any 2XX status is valid.
						if ( context->header_info.http_status >= 200 && context->header_info.http_status <= 299 )
						{
							context->got_filename = 0;
							context->show_file_size_prompt = false;

							context->header_info.chunk_length = 0;
							context->header_info.end_of_header = NULL;
							context->header_info.http_status = 0;
							context->header_info.connection = CONNECTION_NONE;
							context->header_info.content_encoding = CONTENT_ENCODING_NONE;
							context->header_info.chunked_transfer = false;
							//context->header_info.etag = false;
							context->header_info.got_chunk_start = false;
							context->header_info.got_chunk_terminator = false;

							context->header_info.range_info->content_length = 0;	// We must reset this to get the real request length (not the length of the 2XX request).

							// Do not reset the other range_info values.

							//

							context->content_status = CONTENT_STATUS_NONE;

							InterlockedIncrement( &context->pending_operations );

							*next_operation = IO_HandshakeResponse;

							SSL_WSAConnect( context, overlapped, context->request_info.host, sent );
							if ( !sent )
							{
								*current_operation = IO_Shutdown;

								PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
							}
						}
						else	// Proxy can't/won't tunnel SSL/TLS connections, or authentication is required.
						{
							bool skip_close = false;

							// Authentication is required.
							if ( context->header_info.http_status == 407 )
							{
								if ( context->header_info.proxy_digest_info != NULL &&
								   ( context->header_info.proxy_digest_info->auth_type == AUTH_TYPE_BASIC || context->header_info.proxy_digest_info->auth_type == AUTH_TYPE_DIGEST ) &&
									 context->header_info.proxy_digest_info->nc == 0 )
								{
									bool use_keep_alive_connection = false;

									// If we have a keep-alive connection and were sent all of the data,
									// then we can reuse the connection and not have to flush any remnant data from the buffer.
									if ( context->header_info.connection == CONNECTION_KEEP_ALIVE )
									{
										char *response_buffer = context->header_info.end_of_header;
										int response_buffer_length = context->current_bytes_read - ( context->header_info.end_of_header - context->wsabuf.buf );

										// Look for a chunk terminator.
										if ( context->header_info.chunked_transfer )
										{
											if ( ( response_buffer_length >= 5 ) && ( _memcmp( response_buffer + ( response_buffer_length - 5 ), "0\r\n\r\n", 5 ) != 0 ) )
											{
												use_keep_alive_connection = true;
											}
										}
										else if ( response_buffer_length >= context->header_info.range_info->content_length )	// See if the response data length is the same as the content length.
										{
											use_keep_alive_connection = true;
										}
									}

									context->header_info.connection = ( use_keep_alive_connection ? CONNECTION_KEEP_ALIVE : CONNECTION_CLOSE );

									if ( MakeRequest( context, IO_GetCONNECTResponse, true ) == CONTENT_STATUS_FAILED )
									{
										context->status = STATUS_FAILED;
									}
									else	// Request was sent, don't close the connection below.
									{
										skip_close = true;
									}
								}
								else	// Exhausted the nonce count.
								{
									context->status = STATUS_PROXY_AUTH_REQUIRED;
								}
							}
							else	// Unhandled status response.
							{
								context->status = STATUS_FAILED;
							}

							if ( !skip_close )
							{
								InterlockedIncrement( &context->pending_operations );

								// We don't need to shutdown the SSL/TLS connection since it will not have been established yet.
								*current_operation = IO_Close;

								PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
							}
						}
					}
				}
				else if ( context->cleanup == 2 )	// If we've forced the cleanup, then allow it to continue its steps.
				{
					context->cleanup = 1;
				}
				else
				{
					InterlockedIncrement( &context->pending_operations );

					*current_operation = IO_Close;

					PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
				}

				LeaveCriticalSection( &context->context_cs );
			}
			break;

			case IO_GetContent:
			case IO_ResumeGetContent:
			{
				EnterCriticalSection( &context->context_cs );

				if ( context->cleanup == 0 )
				{
					int content_status = CONTENT_STATUS_FAILED;

					DWORD bytes_decrypted = io_size;

					if ( *current_operation == IO_GetContent )
					{
						context->current_bytes_read = 0;

						if ( use_ssl )
						{
							// We'll need to decrypt any remaining undecrypted data as well as copy the decrypted data to our wsabuf.
							if ( context->ssl->continue_decrypt )
							{
								bytes_decrypted = context->ssl->cbIoBuffer;
							}

							scRet = DecryptRecv( context, bytes_decrypted );
						}
					}

					if ( bytes_decrypted > 0 )
					{
						if ( *current_operation == IO_GetContent )
						{
							context->current_bytes_read = bytes_decrypted + ( context->wsabuf.buf - context->buffer );

							context->wsabuf.buf = context->buffer;
							context->wsabuf.len = BUFFER_SIZE;

							context->wsabuf.buf[ context->current_bytes_read ] = 0;	// Sanity.
						}
						else
						{
							*current_operation = IO_GetContent;
						}

						content_status = GetHTTPContent( context, context->wsabuf.buf, context->current_bytes_read );
					}
					else if ( use_ssl )
					{
						if ( scRet == SEC_E_INCOMPLETE_MESSAGE )
						{
							InterlockedIncrement( &context->pending_operations );

							//context->wsabuf.buf += bytes_decrypted;
							//context->wsabuf.len -= bytes_decrypted;

							SSL_WSARecv( context, overlapped, sent );
							if ( !sent )
							{
								InterlockedDecrement( &context->pending_operations );
							}
							else
							{
								content_status = CONTENT_STATUS_NONE;
							}
						}

						// SEC_I_CONTEXT_EXPIRED may occur here.
					}

					if ( content_status == CONTENT_STATUS_FAILED )
					{
						InterlockedIncrement( &context->pending_operations );

						*current_operation = ( use_ssl ? IO_Shutdown : IO_Close );

						PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
					}
					else if ( content_status == CONTENT_STATUS_HANDLE_RESPONSE )
					{
						context->content_status = CONTENT_STATUS_GET_CONTENT;

						if ( MakeRangeRequest( context ) == CONTENT_STATUS_FAILED )
						{
							InterlockedIncrement( &context->pending_operations );

							*current_operation = ( use_ssl ? IO_Shutdown : IO_Close );

							PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
						}
					}
					else if ( content_status == CONTENT_STATUS_READ_MORE_CONTENT || content_status == CONTENT_STATUS_READ_MORE_HEADER ) // Read more header information, or continue to read more content. Do not reset context->wsabuf since may have been offset to handle partial data.
					{
						InterlockedIncrement( &context->pending_operations );

						*current_operation = IO_GetContent;

						if ( use_ssl )
						{
							if ( context->ssl->continue_decrypt )
							{
								// We need to post a non-zero status to avoid our code shutting down the connection.
								// We'll use context->current_bytes_read for that, but it can be anything that's not zero.
								PostQueuedCompletionStatus( hIOCP, context->current_bytes_read, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
							}
							else
							{
								SSL_WSARecv( context, overlapped, sent );
								if ( !sent )
								{
									*current_operation = IO_Shutdown;

									PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
								}
							}
						}
						else
						{
							nRet = _WSARecv( context->socket, &context->wsabuf, 1, NULL, &dwFlags, ( WSAOVERLAPPED * )overlapped, NULL );
							if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
							{
								*current_operation = IO_Close;

								PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
							}
						}
					}
				}
				else if ( context->cleanup == 2 )	// If we've forced the cleanup, then allow it to continue its steps.
				{
					context->cleanup = 1;
				}
				else
				{
					InterlockedIncrement( &context->pending_operations );

					*current_operation = IO_Close;

					PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
				}

				LeaveCriticalSection( &context->context_cs );
			}
			break;

			case IO_WriteFile:
			{
				EnterCriticalSection( &context->context_cs );

				if ( context->cleanup == 0 )
				{
					EnterCriticalSection( &context->download_info->shared_cs );
					context->download_info->downloaded += io_size;				// The total amount of data (decoded) that was saved/simulated.
					LeaveCriticalSection( &context->download_info->shared_cs );

					EnterCriticalSection( &session_totals_cs );
					session_total_downloaded += io_size;
					LeaveCriticalSection( &session_totals_cs );

					context->header_info.range_info->file_write_offset += io_size;	// The size of the non-encoded/decoded data that we're writing to the file.

					// Make sure we've writen everything before we do anything else.
					if ( io_size < context->write_wsabuf.len )
					{
						EnterCriticalSection( &context->download_info->shared_cs );

						InterlockedIncrement( &context->pending_operations );

						context->write_wsabuf.buf += io_size;
						context->write_wsabuf.len -= io_size;

						BOOL bRet = WriteFile( context->download_info->hFile, context->write_wsabuf.buf, context->write_wsabuf.len, NULL, ( WSAOVERLAPPED * )overlapped );
						if ( bRet == FALSE && ( GetLastError() != ERROR_IO_PENDING ) )
						{
							InterlockedIncrement( &context->pending_operations );

							*current_operation = ( use_ssl ? IO_Shutdown : IO_Close );

							PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
						}

						LeaveCriticalSection( &context->download_info->shared_cs );
					}
					else
					{
						int content_status = context->content_status;

						// Reset so we don't try to process the header again.
						context->content_status = CONTENT_STATUS_GET_CONTENT;

						// We had set the overlapped structure for file operations, but now we need to reset it for socket operations.
						_memzero( &overlapped->overlapped, sizeof( WSAOVERLAPPED ) );

						//

						context->header_info.range_info->content_offset += context->content_offset;	// The true amount that was downloaded. Allows us to resume if we stop the download.
						context->content_offset = 0;

						if ( context->header_info.chunked_transfer )
						{
							if ( ( context->parts == 1 && context->header_info.connection == CONNECTION_KEEP_ALIVE && context->header_info.got_chunk_terminator ) ||
								 ( context->parts > 1 && ( context->header_info.range_info->content_offset >= ( ( context->header_info.range_info->range_end - context->header_info.range_info->range_start ) + 1 ) ) ) )
							{
								InterlockedIncrement( &context->pending_operations );

								*current_operation = ( use_ssl ? IO_Shutdown : IO_Close );

								PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );

								content_status = CONTENT_STATUS_NONE;
							}
						}
						else
						{
							// We need to force the keep-alive connections closed since the server will just keep it open after we've gotten all the data.
							if ( context->header_info.connection == CONNECTION_KEEP_ALIVE &&
							   ( context->header_info.range_info->content_length == 0 ||
							   ( context->header_info.range_info->content_offset >= ( ( context->header_info.range_info->range_end - context->header_info.range_info->range_start ) + 1 ) ) ) )
							{
								InterlockedIncrement( &context->pending_operations );

								*current_operation = ( use_ssl ? IO_Shutdown : IO_Close );

								PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );

								content_status = CONTENT_STATUS_NONE;
							}
						}

						//

						if ( content_status == CONTENT_STATUS_FAILED )
						{
							InterlockedIncrement( &context->pending_operations );

							*current_operation = ( use_ssl ? IO_Shutdown : IO_Close );

							PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
						}
						else if ( content_status == CONTENT_STATUS_HANDLE_RESPONSE )
						{
							if ( MakeRangeRequest( context ) == CONTENT_STATUS_FAILED )
							{
								InterlockedIncrement( &context->pending_operations );

								*current_operation = ( use_ssl ? IO_Shutdown : IO_Close );

								PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
							}
						}
						else if ( content_status == CONTENT_STATUS_READ_MORE_CONTENT || content_status == CONTENT_STATUS_READ_MORE_HEADER ) // Read more header information, or continue to read more content. Do not reset context->wsabuf since may have been offset to handle partial data.
						{
							InterlockedIncrement( &context->pending_operations );

							*current_operation = IO_GetContent;

							if ( use_ssl )
							{
								if ( context->ssl->continue_decrypt )
								{
									// We need to post a non-zero status to avoid our code shutting down the connection.
									// We'll use context->current_bytes_read for that, but it can be anything that's not zero.
									PostQueuedCompletionStatus( hIOCP, context->current_bytes_read, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
								}
								else
								{
									SSL_WSARecv( context, overlapped, sent );
									if ( !sent )
									{
										*current_operation = IO_Shutdown;

										PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
									}
								}
							}
							else
							{
								nRet = _WSARecv( context->socket, &context->wsabuf, 1, NULL, &dwFlags, ( WSAOVERLAPPED * )overlapped, NULL );
								if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
								{
									*current_operation = IO_Close;

									PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
								}
							}
						}
					}
				}
				else if ( context->cleanup == 2 )	// If we've forced the cleanup, then allow it to continue its steps.
				{
					context->cleanup = 1;
				}
				else
				{
					InterlockedIncrement( &context->pending_operations );

					*current_operation = IO_Close;

					PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
				}

				LeaveCriticalSection( &context->context_cs );
			}
			break;

			case IO_Write:
			{
				EnterCriticalSection( &context->context_cs );

				if ( context->cleanup == 0 )
				{
					// Make sure we've sent everything before we do anything else.
					if ( io_size < context->wsabuf.len )
					{
						InterlockedIncrement( &context->pending_operations );

						context->wsabuf.buf += io_size;
						context->wsabuf.len -= io_size;

						// We do a regular WSASend here since that's what we last did in SSL_WSASend.
						nRet = _WSASend( context->socket, &context->wsabuf, 1, NULL, dwFlags, ( WSAOVERLAPPED * )overlapped, NULL );
						if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
						{
							*current_operation = IO_Close;

							PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
						}
					}
					else	// All the data that we wanted to send has been sent. Post our next operation.
					{
						*current_operation = *next_operation;

						InterlockedIncrement( &context->pending_operations );

						context->wsabuf.buf = context->buffer;
						context->wsabuf.len = BUFFER_SIZE;

						if ( *current_operation == IO_HandshakeResponse ||
							 *current_operation == IO_Shutdown ||
							 *current_operation == IO_Close )
						{
							PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
						}
						else	// Read more data.
						{
							if ( *current_operation != IO_GetCONNECTResponse && use_ssl )
							{
								SSL_WSARecv( context, overlapped, sent );
								if ( !sent )
								{
									*current_operation = IO_Shutdown;

									PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
								}
							}
							else
							{
								nRet = _WSARecv( context->socket, &context->wsabuf, 1, NULL, &dwFlags, ( WSAOVERLAPPED * )overlapped, NULL );
								if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
								{
									*current_operation = IO_Close;

									PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
								}
							}
						}
					}
				}
				else if ( context->cleanup == 2 )	// If we've forced the cleanup, then allow it to continue its steps.
				{
					context->cleanup = 1;
				}
				else
				{
					InterlockedIncrement( &context->pending_operations );

					*current_operation = IO_Close;

					PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
				}

				LeaveCriticalSection( &context->context_cs );
			}
			break;

			case IO_Shutdown:
			{
				bool fall_through = true;

				EnterCriticalSection( &context->context_cs );

				*next_operation = IO_Close;

				InterlockedIncrement( &context->pending_operations );

				context->wsabuf.buf = context->buffer;
				context->wsabuf.len = BUFFER_SIZE;

				SSL_WSAShutdown( context, overlapped, sent );

				// We'll fall through the IO_Shutdown to IO_Close.
				if ( !sent )
				{
					InterlockedDecrement( &context->pending_operations );

					*current_operation = IO_Close;
				}
				else	// The shutdown sent data. IO_Close will be called in IO_Write.
				{
					fall_through = false;
				}

				/*if ( scRet != SEC_E_OK )
				{
					*current_operation = IO_Close;

					InterlockedIncrement( &context->pending_operations );

					PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
				}*/

				LeaveCriticalSection( &context->context_cs );

				if ( !fall_through )
				{
					break;
				}
			}

			case IO_Close:
			{
				if ( context->pending_operations > 0 )
				{
					EnterCriticalSection( &context->context_cs );

					context->cleanup = 1;

					LeaveCriticalSection( &context->context_cs );

					if ( context->socket != INVALID_SOCKET )
					{
						SOCKET s = context->socket;
						context->socket = INVALID_SOCKET;
						_shutdown( s, SD_BOTH );
						_closesocket( s );	// Saves us from having to post if there's already a pending IO operation. Should force the operation to complete.
					}
				}
				else
				{
					// Attempt to connect to a new address if we time out.
					if ( context->timed_out == TIME_OUT_RETRY && 
						 context->address_info != NULL &&
						 context->address_info->ai_next != NULL )
					{
						if ( context->socket != INVALID_SOCKET )
						{
							_shutdown( context->socket, SD_BOTH );
							_closesocket( context->socket );
							context->socket = INVALID_SOCKET;
						}

						if ( context->ssl != NULL )
						{
							SSL_free( context->ssl );
							context->ssl = NULL;
						}

						addrinfoW *old_address_info = context->address_info;
						context->address_info = context->address_info->ai_next;
						old_address_info->ai_next = NULL;

						_FreeAddrInfoW( old_address_info );

						// If we're going to restart the download, then we need to reset these values.
						context->header_info.chunk_length = 0;
						context->header_info.end_of_header = NULL;
						context->header_info.http_status = 0;
						context->header_info.connection = CONNECTION_NONE;
						context->header_info.content_encoding = CONTENT_ENCODING_NONE;
						context->header_info.chunked_transfer = false;
						//context->header_info.etag = false;
						context->header_info.got_chunk_start = false;
						context->header_info.got_chunk_terminator = false;

						context->header_info.range_info->content_length = 0;	// We must reset this to get the real request length (not the length of the 401/407 request).

						if ( context->header_info.range_info != NULL )
						{
							context->header_info.range_info->range_start += context->header_info.range_info->content_offset;	// Begin where we left off.
							context->header_info.range_info->content_offset = 0;	// Reset.
						}

						context->content_status = CONTENT_STATUS_NONE;

						context->timed_out = TIME_OUT_FALSE;

						context->status = STATUS_CONNECTING;

						// Connect to the remote server.
						if ( !CreateConnection( context, context->request_info.host, context->request_info.port ) )
						{
							context->status = STATUS_FAILED;
						}
						else
						{
							break;
						}
					}

					CleanupConnection( context );
				}
			}
			break;
		}
	}

	ExitThread( 0 );
	return 0;
}

SOCKET_CONTEXT *CreateSocketContext()
{
	SOCKET_CONTEXT *context = ( SOCKET_CONTEXT * )GlobalAlloc( GPTR, sizeof( SOCKET_CONTEXT ) );
	if ( context )
	{
		context->wsabuf.buf = context->buffer;
		context->wsabuf.len = BUFFER_SIZE;

		context->socket = INVALID_SOCKET;

		context->overlapped.context = context;
		context->overlapped_close.context = context;

		InitializeCriticalSection( &context->context_cs );
	}

	return context;
}

bool CreateConnection( SOCKET_CONTEXT *context, char *host, unsigned short port )
{
	if ( context == NULL || host == NULL )
	{
		return false;
	}

	int nRet = 0;

	struct addrinfoW hints;

	bool use_ipv6 = false;

	wchar_t *whost = NULL, *t_whost = NULL;
	wchar_t wcs_ip[ 16 ];
	wchar_t wport[ 6 ];

	if ( context->address_info == NULL )
	{
		// Resolve the remote host.
		_memzero( &hints, sizeof( addrinfoW ) );
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_IP;

		if ( cfg_enable_proxy && context->request_info.protocol == PROTOCOL_HTTP )
		{
			__snwprintf( wport, 6, L"%hu", cfg_port );

			if ( cfg_address_type == 0 )
			{
				whost = ( g_punycode_hostname != NULL ? g_punycode_hostname : cfg_hostname );
			}
			else
			{
				struct sockaddr_in src_addr;
				_memzero( &src_addr, sizeof( sockaddr_in ) );

				src_addr.sin_family = AF_INET;
				src_addr.sin_addr.s_addr = _htonl( cfg_ip_address );

				DWORD wcs_ip_length = 16;
				_WSAAddressToStringW( ( SOCKADDR * )&src_addr, sizeof( struct sockaddr_in ), NULL, wcs_ip, &wcs_ip_length );

				whost = wcs_ip;
			}
		}
		else if ( cfg_enable_proxy_s && context->request_info.protocol == PROTOCOL_HTTPS )
		{
			__snwprintf( wport, 6, L"%hu", cfg_port_s );

			if ( cfg_address_type_s == 0 )
			{
				whost = ( g_punycode_hostname_s != NULL ? g_punycode_hostname_s : cfg_hostname_s );
			}
			else
			{
				struct sockaddr_in src_addr;
				_memzero( &src_addr, sizeof( sockaddr_in ) );

				src_addr.sin_family = AF_INET;
				src_addr.sin_addr.s_addr = _htonl( cfg_ip_address_s );

				DWORD wcs_ip_length = 16;
				_WSAAddressToStringW( ( SOCKADDR * )&src_addr, sizeof( struct sockaddr_in ), NULL, wcs_ip, &wcs_ip_length );

				whost = wcs_ip;
			}
		}
		else
		{
			__snwprintf( wport, 6, L"%hu", port );

			int whost_length = MultiByteToWideChar( CP_UTF8, 0, host, -1, NULL, 0 );	// Include the NULL terminator.
			whost = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * whost_length );
			MultiByteToWideChar( CP_UTF8, 0, host, -1, whost, whost_length );

			t_whost = whost;
		}

		nRet = _GetAddrInfoW( whost, wport, &hints, &context->address_info );
		if ( nRet == WSAHOST_NOT_FOUND )
		{
			use_ipv6 = true;

			hints.ai_family = AF_INET6;	// Try IPv6
			nRet = _GetAddrInfoW( whost, wport, &hints, &context->address_info );
		}

		if ( nRet != 0 )
		{
			GlobalFree( t_whost );
			return false;
		}
		GlobalFree( t_whost );
	}

	SOCKET socket = CreateSocket( use_ipv6 );
	if ( socket == INVALID_SOCKET )
	{
		return false;
	}

	context->socket = socket;

	g_hIOCP = CreateIoCompletionPort( ( HANDLE )socket, g_hIOCP, 0/*( ULONG_PTR )context*/, 0 );
	if ( g_hIOCP == NULL )
	{
		return false;
	}

	// Socket must be bound before we can use it with ConnectEx.
	struct sockaddr_in ipv4_addr;
	struct sockaddr_in6 ipv6_addr;

	if ( use_ipv6 )
	{
		_memzero( &ipv6_addr, sizeof( ipv6_addr ) );
		ipv6_addr.sin6_family = AF_INET6;
		//ipv6_addr.sin6_addr = in6addr_any;	// This assignment requires the CRT, but it's all zeros anyway and it gets set by _memzero().
		//ipv6_addr.sin6_port = 0;
		nRet = _bind( socket, ( SOCKADDR * )&ipv6_addr, sizeof( ipv6_addr ) );
	}
	else
	{
		_memzero( &ipv4_addr, sizeof( ipv4_addr ) );
		ipv4_addr.sin_family = AF_INET;
		//ipv4_addr.sin_addr.s_addr = INADDR_ANY;
		//ipv4_addr.sin_port = 0;
		nRet = _bind( socket, ( SOCKADDR * )&ipv4_addr, sizeof( ipv4_addr ) );
	}

	if ( nRet == SOCKET_ERROR )
	{
		return false;
	}

	// Attempt to connect to the host.
	InterlockedIncrement( &context->pending_operations );

	context->overlapped.current_operation = IO_Connect;

	DWORD lpdwBytesSent = 0;
	BOOL bRet = _ConnectEx( socket, context->address_info->ai_addr, ( int )context->address_info->ai_addrlen, NULL, 0, &lpdwBytesSent, ( OVERLAPPED * )&context->overlapped );
	if ( bRet == FALSE && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
	{
		InterlockedDecrement( &context->pending_operations );

		/*if ( context->address_info != NULL )
		{
			_FreeAddrInfoW( context->address_info );
		}*/

		return false;
	}

	/*if ( context->address_info != NULL )
	{
		_FreeAddrInfoW( context->address_info );
	}*/

	return true;
}

void StartDownload( DOWNLOAD_INFO *di, bool check_if_file_exists )
{
	if ( di == NULL )
	{
		return;
	}

	unsigned char add_state = 0;

	PROTOCOL protocol = PROTOCOL_UNKNOWN;
	wchar_t *host = NULL;
	wchar_t *resource = NULL;
	unsigned short port = 0;

	if ( check_if_file_exists )
	{
		bool skip_start = false;

		EnterCriticalSection( &di->shared_cs );

		wchar_t prompt_message[ MAX_PATH + 100 ];

		wchar_t file_path[ MAX_PATH ];
		_wmemcpy_s( file_path, MAX_PATH, di->file_path, MAX_PATH );
		if ( di->filename_offset > 0 )
		{
			file_path[ di->filename_offset - 1 ] = L'\\';	// Replace the download directory NULL terminator with a directory slash.
		}

		if ( GetFileAttributes( file_path ) != INVALID_FILE_ATTRIBUTES )
		{
			// If the last return value was not set to remember our choice, then prompt again.
			if ( g_rename_file_cmb_ret != CMBIDRENAMEALL && g_rename_file_cmb_ret != CMBIDOVERWRITEALL && g_rename_file_cmb_ret != CMBIDSKIPALL )
			{
				__snwprintf( prompt_message, MAX_PATH + 100, L"%s already exists.\r\n\r\nWhat operation would you like to perform?", file_path );

				g_rename_file_cmb_ret = CMessageBoxW( g_hWnd_main, prompt_message, PROGRAM_CAPTION, CMB_ICONWARNING | CMB_RENAMEOVERWRITESKIPALL );
			}

			// Rename the file and try again.
			if ( g_rename_file_cmb_ret == CMBIDRENAME || g_rename_file_cmb_ret == CMBIDRENAMEALL )
			{
				// Creates a tree of active and queued downloads.
				dllrbt_tree *add_files_tree = CreateFilenameTree();

				bool rename_succeeded = RenameFile( di, add_files_tree );

				// The tree is only used to determine duplicate filenames.
				DestroyFilenameTree( add_files_tree );

				if ( !rename_succeeded )
				{
					if ( g_rename_file_cmb_ret2 != CMBIDOKALL )
					{
						__snwprintf( prompt_message, MAX_PATH + 100, L"%s could not be renamed.\r\n\r\nYou will need to choose a different save directory.", file_path );

						g_rename_file_cmb_ret2 = CMessageBoxW( g_hWnd_main, prompt_message, PROGRAM_CAPTION, CMB_ICONWARNING | CMB_OKALL );
					}

					di->status = STATUS_SKIPPED;

					skip_start = true;
				}
			}
			else if ( g_rename_file_cmb_ret == CMBIDFAIL || g_rename_file_cmb_ret == CMBIDSKIP || g_rename_file_cmb_ret == CMBIDSKIPALL ) // Skip the rename or overwrite if the return value fails, or the user selected skip.
			{
				di->status = STATUS_SKIPPED;

				skip_start = true;
			}
		}

		LeaveCriticalSection( &di->shared_cs );

		if ( skip_start )
		{
			return;
		}
	}

	ParseURL_W( di->url, protocol, &host, port, &resource );

	int host_length = lstrlenW( host );
	int resource_length = lstrlenW( resource );

	if ( normaliz_state == NORMALIZ_STATE_RUNNING )
	{
		int punycode_length = _IdnToAscii( 0, host, host_length, NULL, 0 );

		if ( punycode_length > host_length )
		{
			wchar_t *punycode = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( punycode_length + 1 ) );
			host_length = _IdnToAscii( 0, host, host_length, punycode, punycode_length );
			punycode[ host_length ] = 0;	// Sanity.

			GlobalFree( host );
			host = punycode;
		}
	}

	unsigned char part = 1;

	EnterCriticalSection( &cleanup_cs );

	DoublyLinkedList *range_node = di->range_info;
	while ( range_node != NULL )
	{
		RANGE_INFO *range_info = ( RANGE_INFO * )range_node->data;

		// Check if our range still needs to download.
		if ( range_info != NULL && ( range_info->content_offset <= ( range_info->range_end - range_info->range_start ) ) )
		{
			// Check the state of our downloads/queue once.
			if ( add_state == 0 )
			{
				if ( total_downloading < cfg_max_downloads )
				{
					add_state = 1;	// Create the connection.

					// Set the start time only if we've manually started the download.
					if ( di->retries == 0 )
					{
						FILETIME ft;
						GetSystemTimeAsFileTime( &ft );
						di->start_time.LowPart = ft.dwLowDateTime;
						di->start_time.HighPart = ft.dwHighDateTime;
					}

					di->status = STATUS_CONNECTING;	// Connecting.

					EnableTimers( true );

					EnterCriticalSection( &active_download_list_cs );

					// Add to the global active download list.
					di->download_node.data = di;
					DLL_AddNode( &active_download_list, &di->download_node, -1 );

					++total_downloading;

					LeaveCriticalSection( &active_download_list_cs );
				}
				else
				{
					add_state = 2;	// Queue the download.

					di->status = STATUS_QUEUED;	// Queued.

					EnterCriticalSection( &download_queue_cs );
					
					// Add to the global download queue.
					di->queue_node.data = di;
					DLL_AddNode( &download_queue, &di->queue_node, -1 );

					LeaveCriticalSection( &download_queue_cs );
				}
			}

			di->last_downloaded = di->downloaded;

			if ( add_state == 1 )
			{
				// Save the request information, the header information (if we got any), and create a new connection.
				SOCKET_CONTEXT *context = CreateSocketContext();

				context->processed_header = di->processed_header;

				context->part = part;
				context->parts = di->parts;

				// If we've processed the header, then we would have already gotten a content disposition filename.
				if ( di->processed_header )
				{
					context->got_filename = 1;
				}

				context->request_info.port = port;
				context->request_info.protocol = protocol;

				int cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, host, host_length + 1, NULL, 0, NULL, NULL );
				char *utf8_cfg_val = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * cfg_val_length ); // Size includes the null character.
				WideCharToMultiByte( CP_UTF8, 0, host, host_length + 1, utf8_cfg_val, cfg_val_length, NULL, NULL );

				context->request_info.host = utf8_cfg_val;

				cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, resource, resource_length + 1, NULL, 0, NULL, NULL );
				utf8_cfg_val = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * cfg_val_length ); // Size includes the null character.
				WideCharToMultiByte( CP_UTF8, 0, resource, resource_length + 1, utf8_cfg_val, cfg_val_length, NULL, NULL );

				context->request_info.resource = utf8_cfg_val;

				context->download_info = di;

				range_info->range_start += range_info->content_offset;	// Begin where we left off.
				range_info->content_offset = 0;	// Reset.

				context->header_info.range_info = range_info;

				if ( di->cookies != NULL )
				{
					char *new_cookies = NULL;

					// This value will be saved
					if ( !ParseCookieValues( di->cookies, &context->header_info.cookie_tree, &new_cookies ) )
					{
						GlobalFree( new_cookies );
						new_cookies = NULL;
					}

					// If we got a new cookie.
					if ( new_cookies != NULL )
					{
						// Then see if the new cookie is not blank.
						if ( new_cookies[ 0 ] != NULL )
						{
							context->header_info.cookies = new_cookies;
						}
						else	// Otherwise, if the cookie is blank, then free it.
						{
							GlobalFree( new_cookies );
						}
					}
				}

				// Add to the parts list.
				context->parts_node.data = context;
				DLL_AddNode( &context->download_info->parts_list, &context->parts_node, -1 );

				context->context_node.data = context;

				EnterCriticalSection( &context_list_cs );

				// Add to the global download list.
				DLL_AddNode( &context_list, &context->context_node, 0 );

				LeaveCriticalSection( &context_list_cs );

				EnterCriticalSection( &di->shared_cs );

				++( di->active_parts );

				LeaveCriticalSection( &di->shared_cs );

				context->status = STATUS_CONNECTING;

				if ( !CreateConnection( context, context->request_info.host, context->request_info.port ) )
				{
					context->status = STATUS_FAILED;

					CleanupConnection( context );
				}
			}
			else if ( add_state == 2 )
			{
				add_state = 3;	// Skip adding anymore values to the queue.

				EnterCriticalSection( &di->shared_cs );

				di->active_parts = 0;

				LeaveCriticalSection( &di->shared_cs );
			}

			++part;
			range_node = range_node->next;
		}
		else	// Remove the range if there's nothing to download.
		{
			DoublyLinkedList *tmp_range_node = range_node;

			range_node = range_node->next;

			DLL_RemoveNode( &di->range_info, tmp_range_node );

			GlobalFree( tmp_range_node->data );
			GlobalFree( tmp_range_node );
		}
	}

	LeaveCriticalSection( &cleanup_cs );

	GlobalFree( host );
	GlobalFree( resource );
}

dllrbt_tree *CreateFilenameTree()
{
	DOWNLOAD_INFO *di = NULL;
	wchar_t *filename = NULL;

	// Make a tree of active and queued downloads to find filenames that need to be renamed.
	dllrbt_tree *filename_tree = dllrbt_create( dllrbt_compare_w );

	EnterCriticalSection( &download_queue_cs );

	DoublyLinkedList *tmp_node = download_queue;
	while ( tmp_node != NULL )
	{
		di = ( DOWNLOAD_INFO * )tmp_node->data;
		if ( di != NULL )
		{
			filename = GlobalStrDupW( di->file_path + di->filename_offset );

			if ( dllrbt_insert( filename_tree, ( void * )filename, ( void * )filename ) != DLLRBT_STATUS_OK )
			{
				GlobalFree( filename );
			}
		}

		tmp_node = tmp_node->next;
	}

	LeaveCriticalSection( &download_queue_cs );

	EnterCriticalSection( &active_download_list_cs );

	tmp_node = active_download_list;
	while ( tmp_node != NULL )
	{
		di = ( DOWNLOAD_INFO * )tmp_node->data;
		if ( di != NULL )
		{
			filename = GlobalStrDupW( di->file_path + di->filename_offset );

			if ( dllrbt_insert( filename_tree, ( void * )filename, ( void * )filename ) != DLLRBT_STATUS_OK )
			{
				GlobalFree( filename );
			}
		}

		tmp_node = tmp_node->next;
	}

	LeaveCriticalSection( &active_download_list_cs );

	return filename_tree;
}

void DestroyFilenameTree( dllrbt_tree *filename_tree )
{
	// The tree is only used to determine duplicate filenames.
	node_type *node = dllrbt_get_head( filename_tree );
	while ( node != NULL )
	{
		wchar_t *filename = ( wchar_t * )node->val;

		if ( filename != NULL )
		{
			GlobalFree( filename );
		}

		node = node->next;
	}
	dllrbt_delete_recursively( filename_tree );
}

bool RenameFile( DOWNLOAD_INFO *di, dllrbt_tree *filename_tree )
{
	unsigned int rename_count = 0;

	// The maximum folder path length is 248 (including the trailing '\').
	// The maximum file name length in the case above is 11 (not including the NULL terminator).
	// The total is 259 characters (not including the NULL terminator).
	// MAX_PATH is 260.

	// We don't want to overwrite the download info until the very end.
	wchar_t new_file_path[ MAX_PATH ];
	_wmemcpy_s( new_file_path, MAX_PATH, di->file_path, MAX_PATH );

	new_file_path[ di->filename_offset - 1 ] = L'\\';	// Replace the download directory NULL terminator with a directory slash.

	do
	{
		while ( dllrbt_find( filename_tree, ( void * )( new_file_path + di->filename_offset ), false ) != NULL )
		{
			// If there's a file extension, then put the counter before it.
			int ret = __snwprintf( new_file_path + di->file_extension_offset, MAX_PATH - di->file_extension_offset - 1, L" (%lu)%s", ++rename_count, di->file_path + di->file_extension_offset );

			// Can't rename.
			if ( ret < 0 )
			{
				return false;
			}
		}

		// Add the new filename to the add files tree.
		wchar_t *filename = GlobalStrDupW( new_file_path + di->filename_offset );

		if ( dllrbt_insert( filename_tree, ( void * )filename, ( void * )filename ) != DLLRBT_STATUS_OK )
		{
			GlobalFree( filename );
		}
	}
	while ( GetFileAttributes( new_file_path ) != INVALID_FILE_ATTRIBUTES );

	// Set the new filename.
	_wmemcpy_s( di->file_path + di->filename_offset, MAX_PATH - di->filename_offset, new_file_path + di->filename_offset, MAX_PATH - di->filename_offset );
	di->file_path[ MAX_PATH - 1 ] = 0;	// Sanity.

	// Get the new file extension offset.
	di->file_extension_offset = di->filename_offset + get_file_extension_offset( di->file_path + di->filename_offset, lstrlenW( di->file_path + di->filename_offset ) );

	return true;
}

ICON_INFO *CacheIcon( DOWNLOAD_INFO *di, SHFILEINFO *sfi )
{
	ICON_INFO *ii = NULL;

	// Cache our file's icon.
	EnterCriticalSection( &icon_cache_cs );
	ii = ( ICON_INFO * )dllrbt_find( icon_handles, ( void * )( di->file_path + di->file_extension_offset ), true );
	if ( ii == NULL )
	{
		bool destroy = true;
		#ifndef OLE32_USE_STATIC_LIB
			if ( ole32_state == OLE32_STATE_SHUTDOWN )
			{
				destroy = InitializeOle32();
			}
		#endif

		if ( destroy )
		{
			_CoInitializeEx( NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE );
		}

		// Use an unknown file type icon for extensionless files.
		_SHGetFileInfoW( ( di->file_path[ di->file_extension_offset ] != 0 ? di->file_path + di->file_extension_offset : L" " ), FILE_ATTRIBUTE_NORMAL, sfi, sizeof( SHFILEINFO ), SHGFI_USEFILEATTRIBUTES | SHGFI_ICON | SHGFI_SMALLICON );

		if ( destroy )
		{
			_CoUninitialize();
		}

		ii = ( ICON_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( DOWNLOAD_INFO ) );

		ii->file_extension = GlobalStrDupW( di->file_path + di->file_extension_offset );
		ii->icon = sfi->hIcon;

		ii->count = 1;

		if ( dllrbt_insert( icon_handles, ( void * )ii->file_extension, ( void * )ii ) != DLLRBT_STATUS_OK )
		{
			DestroyIcon( ii->icon );
			GlobalFree( ii->file_extension );
			GlobalFree( ii );
			ii = NULL;
		}
	}
	else
	{
		++( ii->count );
	}
	LeaveCriticalSection( &icon_cache_cs );

	return ii;
}

DWORD WINAPI AddURL( void *add_info )
{
	if ( add_info == NULL )
	{
		_ExitThread( 0 );
		return 0;
	}

	EnterCriticalSection( &worker_cs );

	in_worker_thread = true;

	ProcessingList( true );

	ADD_INFO *ai = ( ADD_INFO * )add_info;

	wchar_t *url_list = ai->urls;

	wchar_t *host = NULL;
	wchar_t *resource = NULL;
	
	PROTOCOL protocol = PROTOCOL_UNKNOWN;
	unsigned short port = 0;

	int host_length = 0;
	int resource_length = 0;

	int username_length = 0;
	int password_length = 0;
	int cookies_length = 0;

	if ( ai->auth_info.username != NULL )
	{
		username_length = lstrlenA( ai->auth_info.username );
	}

	if ( ai->auth_info.password != NULL )
	{
		password_length = lstrlenA( ai->auth_info.password );
	}

	if ( ai->utf8_cookies != NULL )
	{
		cookies_length = lstrlenA( ai->utf8_cookies );
	}

	SHFILEINFO *sfi = ( SHFILEINFO * )GlobalAlloc( GMEM_FIXED, sizeof( SHFILEINFO ) );

	// Creates a tree of active and queued downloads.
	dllrbt_tree *add_files_tree = CreateFilenameTree();

	while ( url_list != NULL )
	{
		// Stop processing and exit the thread.
		if ( kill_worker_thread_flag )
		{
			break;
		}

		// Find the end of the current url.
		wchar_t *current_url = url_list;

		// Remove anything before our URL (spaces, tabs, newlines, etc.)
		while ( *current_url != 0 && *current_url != L'h' )
		{
			++current_url;
		}

		url_list = _StrStrW( current_url, L"\r\n" );
		if ( url_list != NULL )
		{
			*url_list = 0;	// Sanity.

			url_list += 2;
		}

		// Remove whitespace at the end of our URL.
		int current_url_length = lstrlenW( current_url );
		while ( current_url_length-- > 0 )
		{
			if ( current_url[ current_url_length ] != L' ' && current_url[ current_url_length ] != L'\t' && current_url[ current_url_length ] != L'\f' )
			{
				break;
			}
			else
			{
				current_url[ current_url_length ] = 0;	// Sanity.
			}
		}

		// Reset.
		protocol = PROTOCOL_UNKNOWN;
		host = NULL;
		resource = NULL;
		port = 0;

		host_length = 0;
		resource_length = 0;

		ParseURL_W( current_url, protocol, &host, port, &resource );

		if ( ( protocol == PROTOCOL_HTTP || protocol == PROTOCOL_HTTPS ) && host != NULL && resource != NULL && port != 0 )
		{
			host_length = lstrlenW( host );
			resource_length = lstrlenW( resource );

			DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )GlobalAlloc( GPTR, sizeof( DOWNLOAD_INFO ) );

			if ( !ai->simulate_download )
			{
				di->filename_offset = lstrlenW( ai->download_directory );
				_wmemcpy_s( di->file_path, MAX_PATH, ai->download_directory, di->filename_offset );
				di->file_path[ di->filename_offset ] = 0;	// Sanity.

				++di->filename_offset;	// Include the NULL terminator.
			}
			else
			{
				di->filename_offset = 1;
			}

			unsigned int directory_length = 0;
			wchar_t *directory = url_decode_w( resource, resource_length, &directory_length );

			int w_filename_length = 0;

			if ( directory != NULL )
			{
				// No file was found in the resource path. Use the host for the filename.
				if ( directory_length == 1 )
				{
					w_filename_length = min( host_length, ( int )( MAX_PATH - di->filename_offset - 1 ) );

					_wmemcpy_s( di->file_path + di->filename_offset, MAX_PATH - di->filename_offset, host, w_filename_length );
					di->file_path[ di->filename_offset + w_filename_length ] = 0;	// Sanity.

					EscapeFilename( di->file_path + di->filename_offset );
				}
				else
				{
					wchar_t *previous_directory = NULL;
					wchar_t *last_directory = directory;
					wchar_t *current_directory = directory;

					// Skip to the last directory.
					// Exit if we've reached the start of any URL parameters, or fragment.
					while ( *current_directory != NULL )
					{
						if ( *current_directory == L'?' || *current_directory == L'#' )
						{
							*current_directory = 0;	// Sanity.

							break;
						}
						else if ( *current_directory == L'/' )
						{
							previous_directory = last_directory;
							last_directory = current_directory + 1;
						}
						
						++current_directory;
					}

					if ( *last_directory != NULL )
					{
						w_filename_length = lstrlenW( last_directory );
						w_filename_length = min( w_filename_length, ( int )( MAX_PATH - di->filename_offset - 1 ) );

						_wmemcpy_s( di->file_path + di->filename_offset, MAX_PATH - di->filename_offset, last_directory, w_filename_length );
						di->file_path[ di->filename_offset + w_filename_length ] = 0;	// Sanity.

						EscapeFilename( di->file_path + di->filename_offset );
					}
					else if ( previous_directory != NULL )
					{
						// Remove any trailing slash.
						wchar_t *end_of_filename = _StrChrW( previous_directory, L'/' );
						if ( end_of_filename != NULL )
						{
							*end_of_filename = 0;	// Sanity.
						}

						if ( previous_directory != end_of_filename )
						{
							w_filename_length = lstrlenW( previous_directory );
						}
						else
						{
							w_filename_length = host_length;
							previous_directory = host;
						}
						w_filename_length = min( w_filename_length, ( int )( MAX_PATH - di->filename_offset - 1 ) );

						_wmemcpy_s( di->file_path + di->filename_offset, MAX_PATH - di->filename_offset, previous_directory, w_filename_length );
						di->file_path[ di->filename_offset + w_filename_length ] = 0;	// Sanity.

						EscapeFilename( di->file_path + di->filename_offset );
					}
					else
					{
						w_filename_length = 11;
						_wmemcpy_s( di->file_path + di->filename_offset, MAX_PATH - di->filename_offset, L"NO_FILENAME\0", 12 );
					}
				}

				GlobalFree( directory );
			}
			else
			{
				w_filename_length = 11;
				_wmemcpy_s( di->file_path + di->filename_offset, MAX_PATH - di->filename_offset, L"NO_FILENAME\0", 12 );
			}

			di->file_extension_offset = di->filename_offset + get_file_extension_offset( di->file_path + di->filename_offset, w_filename_length );

			di->hFile = INVALID_HANDLE_VALUE;

			InitializeCriticalSection( &di->shared_cs );

			di->url = GlobalStrDupW( current_url );

			// Cache our file's icon.
			ICON_INFO *ii = CacheIcon( di, sfi );

			if ( ii != NULL )
			{
				di->icon = &ii->icon;
			}

			di->parts = ai->parts;

			di->ssl_version = ai->ssl_version;

			di->simulate_download = ai->simulate_download;

			if ( ai->auth_info.username != NULL )
			{
				di->auth_info.username = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( username_length + 1 ) );
				_memcpy_s( di->auth_info.username, username_length + 1, ai->auth_info.username, username_length );
				di->auth_info.username[ username_length ] = 0;	// Sanity.
			}

			if ( ai->auth_info.password != NULL )
			{
				di->auth_info.password = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( password_length + 1 ) );
				_memcpy_s( di->auth_info.password, password_length + 1, ai->auth_info.password, password_length );
				di->auth_info.password[ password_length ] = 0;	// Sanity.
			}

			if ( ai->utf8_cookies != NULL )
			{
				di->cookies = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( cookies_length + 1 ) );
				_memcpy_s( di->cookies, cookies_length + 1, ai->utf8_cookies, cookies_length );
				di->cookies[ cookies_length ] = 0;	// Sanity.
			}

			SYSTEMTIME st;
			FILETIME ft;

			GetLocalTime( &st );
			SystemTimeToFileTime( &st, &ft );

			di->add_time.LowPart = ft.dwLowDateTime;
			di->add_time.HighPart = ft.dwHighDateTime;

			int buffer_length = 0;

			#ifndef NTDLL_USE_STATIC_LIB
				//buffer_length = 64;	// Should be enough to hold most translated values.
				buffer_length = __snwprintf( NULL, 0, L"%s, %s %d, %04d %d:%02d:%02d %s", GetDay( st.wDayOfWeek ), GetMonth( st.wMonth ), st.wDay, st.wYear, ( st.wHour > 12 ? st.wHour - 12 : ( st.wHour != 0 ? st.wHour : 12 ) ), st.wMinute, st.wSecond, ( st.wHour >= 12 ? L"PM" : L"AM" ) ) + 1;	// Include the NULL character.
			#else
				buffer_length = _scwprintf( L"%s, %s %d, %04d %d:%02d:%02d %s", GetDay( st.wDayOfWeek ), GetMonth( st.wMonth ), st.wDay, st.wYear, ( st.wHour > 12 ? st.wHour - 12 : ( st.wHour != 0 ? st.wHour : 12 ) ), st.wMinute, st.wSecond, ( st.wHour >= 12 ? L"PM" : L"AM" ) ) + 1;	// Include the NULL character.
			#endif

			di->w_add_time = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * buffer_length );

			__snwprintf( di->w_add_time, buffer_length, L"%s, %s %d, %04d %d:%02d:%02d %s", GetDay( st.wDayOfWeek ), GetMonth( st.wMonth ), st.wDay, st.wYear, ( st.wHour > 12 ? st.wHour - 12 : ( st.wHour != 0 ? st.wHour : 12 ) ), st.wMinute, st.wSecond, ( st.wHour >= 12 ? L"PM" : L"AM" ) );

			// Add our range info to the download info.
			RANGE_INFO *ri = ( RANGE_INFO * )GlobalAlloc( GPTR, sizeof( RANGE_INFO ) );
			DoublyLinkedList *range_node = DLL_CreateNode( ( void * )ri );
			DLL_AddNode( &di->range_info, range_node, -1 );

			EnterCriticalSection( &cleanup_cs );

			StartDownload( di, !di->simulate_download );

			LVITEM lvi;
			_memzero( &lvi, sizeof( LVITEM ) );
			lvi.mask = LVIF_PARAM;
			lvi.iItem = _SendMessageW( g_hWnd_files, LVM_GETITEMCOUNT, 0, 0 );
			lvi.lParam = ( LPARAM )di;
			_SendMessageW( g_hWnd_files, LVM_INSERTITEM, 0, ( LPARAM )&lvi );

			download_history_changed = true;

			LeaveCriticalSection( &cleanup_cs );
		}

		GlobalFree( host );
		GlobalFree( resource );
	}

	// The tree is only used to determine duplicate filenames.
	DestroyFilenameTree( add_files_tree );

	GlobalFree( sfi );

	GlobalFree( ai->utf8_cookies );
	GlobalFree( ai->auth_info.username );
	GlobalFree( ai->auth_info.password );
	GlobalFree( ai->download_directory );
	GlobalFree( ai->urls );
	GlobalFree( ai );

	ProcessingList( false );

	// Release the semaphore if we're killing the thread.
	if ( worker_semaphore != NULL )
	{
		ReleaseSemaphore( worker_semaphore, 1, NULL );
	}

	in_worker_thread = false;

	LeaveCriticalSection( &worker_cs );

	_ExitThread( 0 );
	return 0;
}

void StartQueuedItem()
{
	EnterCriticalSection( &download_queue_cs );

	if ( download_queue != NULL )
	{
		DoublyLinkedList *download_queue_node = download_queue;

		DOWNLOAD_INFO *di = NULL;

		// Run through our download queue and start the first context that hasn't been paused or stopped.
		// Continue to dequeue if we haven't hit our maximum allowed active downloads.
		while ( download_queue_node != NULL )
		{
			di = ( DOWNLOAD_INFO * )download_queue_node->data;

			download_queue_node = download_queue_node->next;

			if ( di != NULL )
			{
				// Remove the item from the download queue.
				DLL_RemoveNode( &download_queue, &di->queue_node );
				di->queue_node.data = NULL;

				StartDownload( di, false );

				// Exit the loop if we've hit our maximum allowed active downloads.
				if ( total_downloading >= cfg_max_downloads )
				{
					break;
				}
			}
		}
	}

	LeaveCriticalSection( &download_queue_cs );
}

void CleanupConnection( SOCKET_CONTEXT *context )
{
	if ( context != NULL )
	{
		bool skip_cleanup = false;

		EnterCriticalSection( &context->context_cs );

		if ( context->cleanup == 2 )
		{
			skip_cleanup = true;
		}

		context->cleanup = 1;

		LeaveCriticalSection( &context->context_cs );

		if ( skip_cleanup )
		{
			return;
		}

		bool retry_context_connection = false;

		// This critical section must encompass the (context->download_info != NULL) section below so that any listview manipulation (like remove_items(...))
		// doesn't affect the queuing/starting proceedure.
		EnterCriticalSection( &cleanup_cs );

		EnterCriticalSection( &context_list_cs );

		// Remove from the global download list.
		DLL_RemoveNode( &context_list, &context->context_node );

		LeaveCriticalSection( &context_list_cs );

		// Remove from the parts list.
		if ( !g_end_program )
		{
			if ( context->download_info != NULL )
			{
				if ( context->retries < cfg_retry_parts_count &&
				   ( context->status == STATUS_CONNECTING ||
					 context->status == STATUS_DOWNLOADING ||
					 context->status == STATUS_PAUSED ) )
				{
					if ( context->header_info.range_info != NULL &&  
					   ( context->header_info.range_info->content_offset < ( ( context->header_info.range_info->range_end - context->header_info.range_info->range_start ) + 1 ) ) )
					{
						++context->retries;

						if ( context->socket != INVALID_SOCKET )
						{
							_shutdown( context->socket, SD_BOTH );
							_closesocket( context->socket );
							context->socket = INVALID_SOCKET;
						}

						if ( context->ssl != NULL )
						{
							SSL_free( context->ssl );
							context->ssl = NULL;
						}

						EnterCriticalSection( &context_list_cs );

						DLL_AddNode( &context_list, &context->context_node, 0 );

						LeaveCriticalSection( &context_list_cs );

						// If we're going to restart the download, then we need to reset these values.
						context->header_info.chunk_length = 0;
						context->header_info.end_of_header = NULL;
						context->header_info.http_status = 0;
						context->header_info.connection = CONNECTION_NONE;
						context->header_info.content_encoding = CONTENT_ENCODING_NONE;
						context->header_info.chunked_transfer = false;
						//context->header_info.etag = false;
						context->header_info.got_chunk_start = false;
						context->header_info.got_chunk_terminator = false;

						context->header_info.range_info->content_length = 0;	// We must reset this to get the real request length (not the length of the 401/407 request).

						if ( context->header_info.range_info != NULL )
						{
							context->header_info.range_info->range_start += context->header_info.range_info->content_offset;	// Begin where we left off.
							context->header_info.range_info->content_offset = 0;	// Reset.
						}

						context->content_status = CONTENT_STATUS_NONE;

						// Remember if we timed out in case we failed to connect.
						unsigned char timed_out = context->timed_out;

						context->timed_out = TIME_OUT_FALSE;

						context->status = STATUS_CONNECTING;

						// Connect to the remote server.
						if ( !CreateConnection( context, context->request_info.host, context->request_info.port ) )
						{
							context->status = STATUS_FAILED;

							context->timed_out = timed_out;

							EnterCriticalSection( &context_list_cs );

							DLL_RemoveNode( &context_list, &context->context_node );

							LeaveCriticalSection( &context_list_cs );
						}
						else
						{
							retry_context_connection = true;
						}
					}
				}

				if ( !retry_context_connection )
				{
					EnterCriticalSection( &download_queue_cs );

					// If the context we're cleaning up is in the download queue.
					if ( context->download_info->queue_node.data != NULL )
					{
						DLL_RemoveNode( &download_queue, &context->download_info->queue_node );
						context->download_info->queue_node.data = NULL;
					}

					LeaveCriticalSection( &download_queue_cs );

					EnterCriticalSection( &context->download_info->shared_cs );

					DLL_RemoveNode( &context->download_info->parts_list, &context->parts_node );

					if ( context->download_info->active_parts > 0 )
					{
						--context->download_info->active_parts;

						// There are no more active connections.
						if ( context->download_info->active_parts == 0 )
						{
							bool incomplete_download = false;

							// Go through our range list and see if any connections have not fully completed.
							DoublyLinkedList *range_node = context->download_info->range_info;
							while ( range_node != NULL )
							{
								RANGE_INFO *range_info = ( RANGE_INFO * )range_node->data;
								if ( range_info->content_offset < ( ( range_info->range_end - range_info->range_start ) + 1 ) )
								{
									incomplete_download = true;

									break;
								}
								
								range_node = range_node->next;
							}

							if ( incomplete_download )
							{
								if ( context->status == STATUS_CONNECTING ||
									 context->status == STATUS_DOWNLOADING ||
									 context->status == STATUS_PAUSED )
								{
									// If any of our connections timed out (after we have no more active connections), then set our status to timed out.
									if ( context->timed_out != TIME_OUT_FALSE )
									{
										context->download_info->status = STATUS_TIMED_OUT;
									}
									else
									{
										context->download_info->status = STATUS_STOPPED;
									}
								}
								else
								{
									incomplete_download = false;

									context->download_info->status = context->status;
								}
							}
							else
							{
								context->download_info->status = STATUS_COMPLETED;
							}

							EnterCriticalSection( &active_download_list_cs );

							// Remove the node from the active download list.
							DLL_RemoveNode( &active_download_list, &context->download_info->download_node );
							context->download_info->download_node.data = NULL;

							context->download_info->last_downloaded = context->download_info->downloaded;

							--total_downloading;

							LeaveCriticalSection( &active_download_list_cs );

							context->download_info->time_remaining = 0;
							context->download_info->speed = 0;

							if ( context->download_info->hFile != INVALID_HANDLE_VALUE )
							{
								CloseHandle( context->download_info->hFile );
								context->download_info->hFile = INVALID_HANDLE_VALUE;
							}

							FILETIME ft;
							GetSystemTimeAsFileTime( &ft );
							ULARGE_INTEGER current_time;
							current_time.HighPart = ft.dwHighDateTime;
							current_time.LowPart = ft.dwLowDateTime;

							context->download_info->time_elapsed = ( current_time.QuadPart - context->download_info->start_time.QuadPart ) / FILETIME_TICKS_PER_SECOND;

							// Stop and Remove.
							if ( context->status == STATUS_STOP_AND_REMOVE )
							{
								// Find the icon info
								EnterCriticalSection( &icon_cache_cs );
								dllrbt_iterator *itr = dllrbt_find( icon_handles, ( void * )( context->download_info->file_path + context->download_info->file_extension_offset ), false );
								// Free its values and remove it from the tree if there are no other items using it.
								if ( itr != NULL )
								{
									ICON_INFO *ii = ( ICON_INFO * )( ( node_type * )itr )->val;

									if ( ii != NULL )
									{
										if ( --ii->count == 0 )
										{
											DestroyIcon( ii->icon );
											GlobalFree( ii->file_extension );
											GlobalFree( ii );

											dllrbt_remove( icon_handles, itr );
										}
									}
									else
									{
										dllrbt_remove( icon_handles, itr );
									}
								}
								LeaveCriticalSection( &icon_cache_cs );

								GlobalFree( context->download_info->url );
								GlobalFree( context->download_info->w_add_time );
								GlobalFree( context->download_info->cookies );
								//GlobalFree( context->download_info->etag );
								GlobalFree( context->download_info->auth_info.username );
								GlobalFree( context->download_info->auth_info.password );

								while ( context->download_info->range_info != NULL )
								{
									DoublyLinkedList *range_node = context->download_info->range_info;
									context->download_info->range_info = context->download_info->range_info->next;

									GlobalFree( range_node->data );
									GlobalFree( range_node );
								}

								DeleteCriticalSection( &context->download_info->shared_cs );

								GlobalFree( context->download_info );
								context->download_info = NULL;
							}
							else
							{
								if ( incomplete_download )
								{
									if ( context->download_info->retries < cfg_retry_downloads_count )
									{
										++context->download_info->retries;

										StartDownload( context->download_info, false );
									}
								}

								LeaveCriticalSection( &context->download_info->shared_cs );
							}

							// Start any items that are in our download queue.
							if ( total_downloading < cfg_max_downloads )
							{
								StartQueuedItem();
							}

							// Turn off our timers if we're not currently downloading anything.
							if ( total_downloading == 0 )
							{
								EnableTimers( false );
							}
						}
						else
						{
							LeaveCriticalSection( &context->download_info->shared_cs );
						}
					}
					else
					{
						LeaveCriticalSection( &context->download_info->shared_cs );
					}
				}
			}
		}

		if ( !retry_context_connection )
		{
			if ( context->socket != INVALID_SOCKET )
			{
				_shutdown( context->socket, SD_BOTH );
				_closesocket( context->socket );
				context->socket = INVALID_SOCKET;
			}

			if ( context->ssl != NULL ) { SSL_free( context->ssl ); }

			if ( context->address_info != NULL ) { _FreeAddrInfoW( context->address_info ); }

			if ( context->decompressed_buf != NULL ) { GlobalFree( context->decompressed_buf ); }
			if ( zlib1_state == ZLIB1_STATE_RUNNING ) { _inflateEnd( &context->stream ); }

			FreeAuthInfo( &context->header_info.digest_info );
			FreeAuthInfo( &context->header_info.proxy_digest_info );

			if ( context->header_info.chunk_buffer != NULL ) { GlobalFree( context->header_info.chunk_buffer ); }

			if ( context->header_info.cookies != NULL ) { GlobalFree( context->header_info.cookies ); }
			if ( context->header_info.cookie_tree != NULL )
			{
				node_type *node = dllrbt_get_head( context->header_info.cookie_tree );
				while ( node != NULL )
				{
					COOKIE_CONTAINER *cc = ( COOKIE_CONTAINER * )node->val;
					if ( cc != NULL )
					{
						GlobalFree( cc->cookie_name );
						GlobalFree( cc->cookie_value );
						GlobalFree( cc );
					}

					node = node->next;
				}

				dllrbt_delete_recursively( context->header_info.cookie_tree );
			}

			if ( context->request_info.host != NULL ) { GlobalFree( context->request_info.host ); }
			if ( context->request_info.resource != NULL ) { GlobalFree( context->request_info.resource ); }

			// context->download_info is freed in WM_DESTROY.

			DeleteCriticalSection( &context->context_cs );

			GlobalFree( context );
		}

		LeaveCriticalSection( &cleanup_cs );
	}
}

void FreeAuthInfo( AUTH_INFO **auth_info )
{
	if ( *auth_info != NULL )
	{
		if ( ( *auth_info )->domain != NULL ) { GlobalFree( ( *auth_info )->domain ); }
		if ( ( *auth_info )->nonce != NULL ) { GlobalFree( ( *auth_info )->nonce ); }
		if ( ( *auth_info )->opaque != NULL ) { GlobalFree( ( *auth_info )->opaque ); }
		if ( ( *auth_info )->qop != NULL ) { GlobalFree( ( *auth_info )->qop ); }
		if ( ( *auth_info )->realm != NULL ) { GlobalFree( ( *auth_info )->realm ); }

		GlobalFree( *auth_info );
	}

	*auth_info = NULL;
}

// Free all context structures in the global list of context structures.
void FreeContexts()
{
	DoublyLinkedList *context_node = context_list;
	DoublyLinkedList *del_context_node = NULL;

	while ( context_node != NULL )
	{
		del_context_node = context_node;
		context_node = context_node->next;

		CleanupConnection( ( SOCKET_CONTEXT * )del_context_node->data );
	}

	context_list = NULL;

	return;
}
