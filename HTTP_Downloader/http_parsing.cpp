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

#include "http_parsing.h"

#include "globals.h"
#include "utilities.h"
#include "dllrbt.h"

#include "lite_ole32.h"
#include "lite_zlib1.h"

#include "cmessagebox.h"

// This basically skips past an expression string when searching for a particular character.
// end is set if the end of the string is reached and the character is not found.
char *FindCharExcludeExpression( char *start, char **end, char character )
{
	unsigned char opened_quote = 0;

	char *pos = start;

	while ( pos != NULL )
	{
		// If an end was supplied, then search until we reach it. If not, then search until we reach the NULL terminator.
		if ( ( *end != NULL && pos < *end ) || ( *end == NULL && *pos != NULL ) )
		{
			if ( opened_quote == 0 )
			{
				// Exit if we've found the end of the value.
				if ( *pos == character )
				{
					break;
				}
				else if ( *pos == '\'' || *pos == '\"' )	// A single or double quote has been opened.
				{
					opened_quote = *pos;
				}
			}
			else	// Find the single or double quote's pair (closing quote).
			{
				if ( *pos == opened_quote || *pos == opened_quote )
				{
					opened_quote = 0;
				}
			}

			++pos;
		}
		else
		{
			// Make sure there's no open quote if we've reached the end.
			if ( *end == NULL && opened_quote == 0 )
			{
				*end = pos;
			}

			pos = NULL;
		}
	}

	return pos;
}

char *GetHeaderValue( char *header, char *field_name, unsigned long field_name_length, char **value_start, char **value_end )
{
	char *field_name_start = NULL;
	char *field_end = NULL;
	char *itr_field = header;

	while ( true )
	{
		// Find the end of the field.
		field_end = _StrStrA( itr_field, "\r\n" );
		if ( field_end != NULL && ( field_end != itr_field ) )	// Ensures we don't go past the last "\r\n".
		{
			// Skip whitespace that might appear before the field name.
			while ( *itr_field == ' ' || *itr_field == '\t' || *itr_field == '\f' )
			{
				++itr_field;
			}

			field_name_start = itr_field;

			while ( true )
			{
				// Find the end of the field name.
				if ( itr_field < field_end )
				{
					// The field name will end with a colon.
					if ( *itr_field == ':' )
					{
						// We found the field name.
						if ( ( itr_field - field_name_start ) == field_name_length &&
							 ( _StrCmpNIA( field_name_start, field_name, field_name_length ) == 0 ) )
						{
							++itr_field;

							// Skip whitespace that might appear before the field value.
							while ( *itr_field == ' ' || *itr_field == '\t' || *itr_field == '\f' )
							{
								++itr_field;
							}

							// Skip whitespace that could appear before the "\r\n", but after the field value.
							while ( ( field_end - 1 ) >= itr_field )
							{
								if ( *( field_end - 1 ) != ' ' && *( field_end - 1 ) != '\t' && *( field_end - 1 ) != '\f' )
								{
									break;
								}

								--field_end;
							}

							*value_start = itr_field;
							*value_end = field_end;

							return field_name_start;
						}
						else	// Bad/wrong field name. Move to the next field.
						{
							itr_field = field_end + 2;

							break;
						}
					}

					++itr_field;
				}
				else	// Bad/wrong field name. Move to the next field.
				{
					itr_field = field_end + 2;

					break;
				}
			}
		}
		else	// A complete field end was not found, or we reached the end of the header.
		{
			break;
		}
	}

	return NULL;
}

char *GetDigestValue( char *digest_value, char *digest_value_name, unsigned long digest_value_name_length, char **value_start, char **value_end )
{
	char *digest_value_name_start = NULL;
	char *digest_value_end = NULL;
	char *itr_digest_value = digest_value;

	char *digest_end = NULL;

	while ( true )
	{
		// Find the end of the digest value.
		// If the second parameter is NULL, then it'll be set to the end of the string and FindCharExcludeExpression will return NULL.
		digest_value_end = FindCharExcludeExpression( itr_digest_value, &digest_end, ',' );

		if ( digest_value_end == NULL && digest_end != NULL )
		{
			digest_value_end = digest_end;
		}

		if ( digest_value_end != NULL && ( digest_value_end != itr_digest_value ) )	// Ensures we don't go past the last digest value.
		{
			// Skip whitespace that might appear before the field name.
			while ( *itr_digest_value == ' ' || *itr_digest_value == '\t' || *itr_digest_value == '\f' )
			{
				++itr_digest_value;
			}

			digest_value_name_start = itr_digest_value;

			while ( true )
			{
				// Find the end of the diget value name.
				if ( itr_digest_value < digest_value_end )
				{
					// The digest value name will end with a equals.
					if ( *itr_digest_value == '=' )
					{
						// We found the digest value name.
						if ( ( itr_digest_value - digest_value_name_start ) == digest_value_name_length &&
							 ( _StrCmpNIA( digest_value_name_start, digest_value_name, digest_value_name_length ) == 0 ) )
						{
							++itr_digest_value;

							// Skip whitespace that might appear before the digest value.
							while ( *itr_digest_value == ' ' || *itr_digest_value == '\t' || *itr_digest_value == '\f' )
							{
								++itr_digest_value;
							}

							// Skip whitespace that could appear before the ",", but after the digest value.
							while ( ( digest_value_end - 1 ) >= itr_digest_value )
							{
								if ( *( digest_value_end - 1 ) != ' ' && *( digest_value_end - 1 ) != '\t' && *( digest_value_end - 1 ) != '\f' )
								{
									break;
								}

								--digest_value_end;
							}

							*value_start = itr_digest_value;
							*value_end = digest_value_end;

							return digest_value_name_start;
						}
						else	// Bad/wrong digest value name. Move to the next digest value.
						{
							if ( digest_end != NULL )
							{
								return NULL;
							}
							else
							{
								itr_digest_value = digest_value_end + 1;

								break;
							}
						}
					}

					++itr_digest_value;
				}
				else	// Bad/wrong digest value name. Move to the next digest value.
				{
					if ( digest_end != NULL )
					{
						return NULL;
					}
					else
					{
						itr_digest_value = digest_value_end + 1;

						break;
					}
				}
			}
		}
		else	// A complete digest value end was not found, or we reached the end of the digest.
		{
			break;
		}
	}

	return NULL;
}

dllrbt_tree *CopyCookieTree( dllrbt_tree *cookie_tree )
{
	dllrbt_tree *new_cookie_tree = NULL;

	node_type *node = dllrbt_get_head( cookie_tree );
	while ( node != NULL )
	{
		COOKIE_CONTAINER *cc = ( COOKIE_CONTAINER * )node->val;

		if ( cc != NULL )
		{
			if ( new_cookie_tree == NULL )
			{
				new_cookie_tree = dllrbt_create( dllrbt_compare_a );
			}

			COOKIE_CONTAINER *new_cc = ( COOKIE_CONTAINER * )GlobalAlloc( GMEM_FIXED, sizeof( COOKIE_CONTAINER ) );

			new_cc->name_length = cc->name_length;
			new_cc->value_length = cc->value_length;

			new_cc->cookie_name = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( new_cc->name_length + 1 ) );
			_memcpy_s( new_cc->cookie_name, new_cc->name_length + 1, cc->cookie_name, new_cc->name_length );
			new_cc->cookie_name[ new_cc->name_length ] = 0;	// Sanity.

			new_cc->cookie_value = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( new_cc->value_length + 1 ) );
			_memcpy_s( new_cc->cookie_value, new_cc->value_length + 1, cc->cookie_value, new_cc->value_length );
			new_cc->cookie_value[ new_cc->value_length ] = 0;	// Sanity.

			// If anything other than OK or duplicate was returned, then free the cookie container.
			if ( dllrbt_insert( new_cookie_tree, ( void * )new_cc->cookie_name, ( void * )new_cc ) != DLLRBT_STATUS_OK )
			{
				GlobalFree( new_cc->cookie_name );
				GlobalFree( new_cc->cookie_value );
				GlobalFree( new_cc );
			}
		}

		node = node->next;
	}

	return new_cookie_tree;
}

void ConstructCookie( dllrbt_tree *cookie_tree, char **cookies )
{
	if ( cookie_tree == NULL )
	{
		return;
	}

	unsigned int total_cookie_length = 0;

	// Get the length of all cookies.
	node_type *node = dllrbt_get_head( cookie_tree );
	while ( node != NULL )
	{
		COOKIE_CONTAINER *cc = ( COOKIE_CONTAINER * )node->val;

		if ( cc != NULL )
		{
			total_cookie_length += ( cc->name_length + cc->value_length + 2 );
		}

		node = node->next;
	}

	*cookies = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( total_cookie_length + 1 ) );

	int cookie_length = 0;
	int count = 0;

	// Construct the cookie string.
	node = dllrbt_get_head( cookie_tree );
	while ( node != NULL )
	{
		COOKIE_CONTAINER *cc = ( COOKIE_CONTAINER * )node->val;

		if ( cc != NULL )
		{
			// Add "; " at the end of the cookie string (before the current cookie).
			if ( count > 0 )
			{
				*( *cookies + cookie_length++ ) = ';';
				*( *cookies + cookie_length++ ) = ' ';
			}

			++count;

			_memcpy_s( *cookies + cookie_length, total_cookie_length  - cookie_length, cc->cookie_name, cc->name_length );
			cookie_length += cc->name_length;

			_memcpy_s( *cookies + cookie_length, total_cookie_length  - cookie_length, cc->cookie_value, cc->value_length );
			cookie_length += cc->value_length;
		}

		node = node->next;
	}

	*( *cookies + cookie_length ) = 0;	// Sanity
}

bool ParseCookieValues( char *cookie_list, dllrbt_tree **cookie_tree, char **cookies )
{
	if ( cookie_list == NULL )
	{
		return false;
	}

	/*if ( end_of_header == NULL )
	{
		end_of_header = fields_tolower( decoded_buffer );	// Normalize the header fields. Returns the end of header.
		if ( end_of_header == NULL )
		{
			return false;
		}
	}
	*( end_of_header + 2 ) = 0;	// Make our search string shorter.*/

	char *end_of_header = cookie_list + lstrlenA( cookie_list );

	char *cookie_end = cookie_list;
	char *cookie_search = cookie_list;

	while ( cookie_search < end_of_header )
	{
		/*cookie_search = _StrStrA( cookie_end, "set-cookie:" );
		if ( cookie_search == NULL || cookie_search >= end_of_header )
		{
			break;			// No more cookies found.
		}
		cookie_search += 11;

		// Skip whitespace that could appear after the ":", but before the field value.
		while ( cookie_search < end_of_header )
		{
			if ( cookie_search[ 0 ] != ' ' && cookie_search[ 0 ] != '\t' )
			{
				break;
			}

			++cookie_search;
		}*/

		cookie_end = _StrStrA( cookie_search, "\r\n" );
		if ( cookie_end == NULL )
		{
			//*( end_of_header + 2 ) = '\r';	// Restore the end of header.
			//return false;	// If this is true, then it means we found the Set-Cookie field, but no termination character. We can't continue.

			cookie_end = end_of_header;
		}

		char *cookie_value_end = cookie_end;
		// Skip whitespace that could appear before the "\r\n", but after the field value.
		while ( ( cookie_value_end - 1 ) >= cookie_search )
		{
			if ( *( cookie_value_end - 1 ) != ' ' && *( cookie_value_end - 1 ) != '\t' )
			{
				break;
			}

			--cookie_value_end;
		}

		char *cookie_name_end = _StrChrA( cookie_search, '=' );
		if ( cookie_name_end != NULL && cookie_name_end < cookie_value_end )
		{
			// Get the cookie name and add it to the tree.
			if ( *cookie_tree == NULL )
			{
				*cookie_tree = dllrbt_create( dllrbt_compare_a );
			}

			// Go back to the end of the cookie name if there's any whitespace after it and before the "=".
			while ( ( cookie_name_end - 1 ) >= cookie_search )
			{
				if ( *( cookie_name_end - 1 ) != ' ' && *( cookie_name_end - 1 ) != '\t' )
				{
					break;
				}

				--cookie_name_end;
			}

			COOKIE_CONTAINER *cc = ( COOKIE_CONTAINER * )GlobalAlloc( GMEM_FIXED, sizeof( COOKIE_CONTAINER ) );

			cc->name_length = ( cookie_name_end - cookie_search );
			cc->cookie_name = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( cc->name_length + 1 ) );

			_memcpy_s( cc->cookie_name, cc->name_length + 1, cookie_search, cc->name_length );
			cc->cookie_name[ cc->name_length ] = 0;	// Sanity.

			// See if the cookie has attributes after it.
			char *cookie_attributes = strnchr( cookie_name_end, ';', ( cookie_value_end - cookie_name_end ) );
			if ( cookie_attributes != NULL && cookie_attributes < cookie_value_end )
			{
				cookie_search = cookie_attributes;
			}
			else
			{
				cookie_search = cookie_value_end;
			}

			cc->value_length = ( cookie_search - cookie_name_end );
			cc->cookie_value = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( cc->value_length + 1 ) );

			_memcpy_s( cc->cookie_value, cc->value_length + 1, cookie_name_end, cc->value_length );
			cc->cookie_value[ cc->value_length ] = 0;	// Sanity.

			// Attempt to add the container to the tree.
			dllrbt_status status = dllrbt_insert( *cookie_tree, ( void * )cc->cookie_name, ( void * )cc );
			if ( status == DLLRBT_STATUS_DUPLICATE_KEY )
			{
				// If there's a duplicate, find it.
				dllrbt_iterator *itr = dllrbt_find( *cookie_tree, ( void * )cc->cookie_name, false );

				// Free its values and remove it from the tree.
				if ( itr != NULL )
				{
					COOKIE_CONTAINER *occ = ( COOKIE_CONTAINER * )( ( node_type * )itr )->val;
					if ( occ != NULL )
					{
						GlobalFree( occ->cookie_name );
						GlobalFree( occ->cookie_value );
						GlobalFree( occ );
					}

					dllrbt_remove( *cookie_tree, itr );
				}

				// Try adding the new cc again. If it fails, then just free it.
				if ( dllrbt_insert( *cookie_tree, ( void * )cc->cookie_name, ( void * )cc ) != DLLRBT_STATUS_OK )
				{
					GlobalFree( cc->cookie_name );
					GlobalFree( cc->cookie_value );
					GlobalFree( cc );
				}
			}
			else if ( status != DLLRBT_STATUS_OK )	// If anything other than OK or duplicate was returned, then free the cookie container.
			{
				GlobalFree( cc->cookie_name );
				GlobalFree( cc->cookie_value );
				GlobalFree( cc );
			}
		}

		//cookie_end += 2;
		cookie_search += 2;
	}

	ConstructCookie( *cookie_tree, cookies );

	//*( end_of_header + 2 ) = '\r';	// Restore the end of header.

	return true;
}

// Modifies decoded_buffer
bool ParseCookies( char *header, dllrbt_tree **cookie_tree, char **cookies, char *end_of_header = 0 )
{
	char *set_cookie_header = NULL;
	char *set_cookie_header_end = header;

	while ( GetHeaderValue( set_cookie_header_end, "Set-Cookie", 10, &set_cookie_header, &set_cookie_header_end ) != NULL )
	{
		char *cookie_name_end = _StrChrA( set_cookie_header, '=' );
		if ( cookie_name_end != NULL && cookie_name_end < set_cookie_header_end )
		{
			// Get the cookie name and add it to the tree.
			if ( *cookie_tree == NULL )
			{
				*cookie_tree = dllrbt_create( dllrbt_compare_a );
			}

			// Go back to the end of the cookie name if there's any whitespace after it and before the "=".
			while ( ( cookie_name_end - 1 ) >= set_cookie_header )
			{
				if ( *( cookie_name_end - 1 ) != ' ' && *( cookie_name_end - 1 ) != '\t' && *( cookie_name_end - 1 ) != '\f' )
				{
					break;
				}

				--cookie_name_end;
			}

			COOKIE_CONTAINER *cc = ( COOKIE_CONTAINER * )GlobalAlloc( GMEM_FIXED, sizeof( COOKIE_CONTAINER ) );

			cc->name_length = ( cookie_name_end - set_cookie_header );
			cc->cookie_name = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( cc->name_length + 1 ) );

			_memcpy_s( cc->cookie_name, cc->name_length + 1, set_cookie_header, cc->name_length );
			cc->cookie_name[ cc->name_length ] = 0;	// Sanity.

			// See if the cookie has attributes after it.
			char *cookie_attributes = strnchr( cookie_name_end, ';', ( set_cookie_header_end - cookie_name_end ) );
			if ( cookie_attributes != NULL && cookie_attributes < set_cookie_header_end )
			{
				set_cookie_header = cookie_attributes;
			}
			else
			{
				set_cookie_header = set_cookie_header_end;
			}

			cc->value_length = ( set_cookie_header - cookie_name_end );
			cc->cookie_value = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( cc->value_length + 1 ) );

			_memcpy_s( cc->cookie_value, cc->value_length + 1, cookie_name_end, cc->value_length );
			cc->cookie_value[ cc->value_length ] = 0;	// Sanity.

			// Attempt to add the container to the tree.
			dllrbt_status status = dllrbt_insert( *cookie_tree, ( void * )cc->cookie_name, ( void * )cc );
			if ( status == DLLRBT_STATUS_DUPLICATE_KEY )
			{
				// If there's a duplicate, find it.
				dllrbt_iterator *itr = dllrbt_find( *cookie_tree, ( void * )cc->cookie_name, false );

				// Free its values and remove it from the tree.
				if ( itr != NULL )
				{
					COOKIE_CONTAINER *occ = ( COOKIE_CONTAINER * )( ( node_type * )itr )->val;
					if ( occ != NULL )
					{
						GlobalFree( occ->cookie_name );
						GlobalFree( occ->cookie_value );
						GlobalFree( occ );
					}

					dllrbt_remove( *cookie_tree, itr );
				}

				// Try adding the new cc again. If it fails, then just free it.
				if ( dllrbt_insert( *cookie_tree, ( void * )cc->cookie_name, ( void * )cc ) != DLLRBT_STATUS_OK )
				{
					GlobalFree( cc->cookie_name );
					GlobalFree( cc->cookie_value );
					GlobalFree( cc );
				}
			}
			else if ( status != DLLRBT_STATUS_OK )	// If anything other than OK or duplicate was returned, then free the cookie container.
			{
				GlobalFree( cc->cookie_name );
				GlobalFree( cc->cookie_value );
				GlobalFree( cc );
			}
		}

		set_cookie_header_end += 2;
	}

	ConstructCookie( *cookie_tree, cookies );

	return true;
}

bool ParseURL_A( char *url, PROTOCOL &protocol, char **host, unsigned int &host_length, unsigned short &port, char **resource, unsigned int &resource_length )
{
	if ( url == NULL )
	{
		return false;
	}

	char *str_pos_start = url;

	// Find the start of the host. (Resource is an absolute URI)
	// Don't include ":" in the search in case it's a stupid protocol relative URI.
	char *str_pos_end = _StrStrA( str_pos_start, "//" );
	if ( str_pos_end != NULL )
	{
		protocol = PROTOCOL_RELATIVE;

		if ( ( str_pos_end - str_pos_start ) == 5 && _StrCmpNA( str_pos_start, "http:", 5 ) == 0 )
		{
			protocol = PROTOCOL_HTTP;
			port = 80;
		}
		else if ( ( str_pos_end - str_pos_start ) == 6 && _StrCmpNA( str_pos_start, "https:", 6 ) == 0 )
		{
			protocol = PROTOCOL_HTTPS;
			port = 443;
		}

		str_pos_start = str_pos_end + 2;

		// Find the end of the host.
		str_pos_end = _StrChrA( str_pos_start, '/' );
		if ( str_pos_end != NULL )
		{
			// Save the resource.
			resource_length = lstrlenA( str_pos_end );

			*resource = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( resource_length + 1 ) );
			_memcpy_s( *resource, resource_length + 1, str_pos_end, resource_length );
			*( *resource + resource_length ) = 0;	// Sanity.
		}
		else
		{
			// See if there's a query string (this would technically not be valid). Would look like: www.test.com?foo=bar
			str_pos_end = _StrChrA( str_pos_start, '?' );
			if ( str_pos_end != NULL )
			{
				resource_length = lstrlenA( str_pos_end ) + 1;	// Include the starting /.

				*resource = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( resource_length + 1 ) );	
				*( *resource ) = '/';
				_memcpy_s( *resource + 1, resource_length, str_pos_end, resource_length );
				*( *resource + resource_length ) = 0;	// Sanity.
			}
			else
			{
				str_pos_end = str_pos_start + lstrlenA( str_pos_start );	// New end of host.

				resource_length = 1;

				// Include the / as the resource.
				*resource = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * 2 );
				*( *resource ) = '/';
				*( *resource + 1 ) = 0;	// Sanity.
			}
		}

		// Find the beginning of a port (if it was included).
		char *str_port_start = str_pos_end - 1;
		while ( str_port_start >= str_pos_start )
		{
			if ( *str_port_start == ':' )
			{
				// If we have a well formed IPv6 address, then see if there was a port assigned to it.
				if ( *str_pos_start == '[' && str_port_start > str_pos_start && *( str_port_start - 1 ) != ']' )
				{
					break;
				}

				char tmp_end = *str_pos_end;
				*str_pos_end = 0;	// Temporary string terminator.
				int num = _strtoul( str_port_start + 1, NULL, 10 );
				*str_pos_end = tmp_end;	// Restore string.

				port = ( num > 65535 ? 0 : num );

				str_pos_end = str_port_start;	// New end of host.

				break;
			}

			--str_port_start;
		}

		host_length = str_pos_end - str_pos_start;

		// Save the host.
		*host = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( host_length + 1 ) );
		_memcpy_s( *host, host_length + 1, str_pos_start, host_length );
		*( *host + host_length ) = 0;	// Sanity
	}
	else if ( url[ 0 ] == '/' )	// Resource is a relative URI that starts with a L'/'
	{
		// Save the resource.
		resource_length = lstrlenA( url );

		*resource = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( resource_length + 1 ) );
		_memcpy_s( *resource, resource_length + 1, url, resource_length );
		*( *resource + resource_length ) = 0;	// Sanity.
	}
	else	// Resource is a relative URI that does not start with a L'/'
	{
		resource_length = lstrlenA( url ) + 1;	// Include the L'/'

		*resource = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( resource_length + 1 ) );
		*resource[ 0 ] = '/';
		_memcpy_s( *resource + 1, resource_length, url, resource_length );
		*( *resource + resource_length ) = 0;	// Sanity.
	}

	return true;
}

bool ParseURL_W( wchar_t *url, PROTOCOL &protocol, wchar_t **host, unsigned int &host_length, unsigned short &port, wchar_t **resource, unsigned int &resource_length )
{
	if ( url == NULL )
	{
		return false;
	}

	wchar_t *str_pos_start = url;

	// Find the start of the host. (Resource is an absolute URI)
	// Don't include ":" in the search in case it's a stupid protocol relative URI.
	wchar_t *str_pos_end = _StrStrW( str_pos_start, L"//" );
	if ( str_pos_end != NULL )
	{
		protocol = PROTOCOL_RELATIVE;

		if ( ( str_pos_end - str_pos_start ) == 5 && _StrCmpNW( str_pos_start, L"http:", 5 ) == 0 )
		{
			protocol = PROTOCOL_HTTP;
			port = 80;
		}
		else if ( ( str_pos_end - str_pos_start ) == 6 && _StrCmpNW( str_pos_start, L"https:", 6 ) == 0 )
		{
			protocol = PROTOCOL_HTTPS;
			port = 443;
		}

		str_pos_start = str_pos_end + 2;

		// Find the end of the host.
		str_pos_end = _StrChrW( str_pos_start, L'/' );
		if ( str_pos_end != NULL )
		{
			// Save the resource.
			resource_length = lstrlenW( str_pos_end );

			*resource = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( resource_length + 1 ) );
			_wmemcpy_s( *resource, resource_length + 1, str_pos_end, resource_length );
			*( *resource + resource_length ) = 0;	// Sanity.
		}
		else
		{
			// See if there's a query string (this would technically not be valid). Would look like: www.test.com?foo=bar
			str_pos_end = _StrChrW( str_pos_start, L'?' );
			if ( str_pos_end != NULL )
			{
				resource_length = lstrlenW( str_pos_end ) + 1;	// Include the starting /.

				*resource = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( resource_length + 1 ) );	
				*( *resource ) = L'/';
				_wmemcpy_s( *resource + 1, resource_length, str_pos_end, resource_length );
				*( *resource + resource_length ) = 0;	// Sanity.
			}
			else
			{
				str_pos_end = str_pos_start + lstrlenW( str_pos_start );	// New end of host.

				resource_length = 1;

				// Include the / as the resource.
				*resource = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * 2 );
				*( *resource ) = L'/';
				*( *resource + 1 ) = 0;	// Sanity.
			}
		}

		// Find the beginning of a port (if it was included).
		wchar_t *str_port_start = str_pos_end - 1;
		while ( str_port_start >= str_pos_start )
		{
			if ( *str_port_start == L':' )
			{
				// If we have a well formed IPv6 address, then see if there was a port assigned to it.
				if ( *str_pos_start == L'[' && str_port_start > str_pos_start && *( str_port_start - 1 ) != L']' )
				{
					break;
				}

				wchar_t tmp_end = *str_pos_end;
				*str_pos_end = 0;	// Temporary string terminator.
				int num = _wcstoul( str_port_start + 1, NULL, 10 );
				*str_pos_end = tmp_end;	// Restore string.

				port = ( num > 65535 ? 0 : num );

				str_pos_end = str_port_start;	// New end of host.

				break;
			}

			--str_port_start;
		}

		host_length = str_pos_end - str_pos_start;

		// Save the host.
		*host = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( host_length + 1 ) );
		_wmemcpy_s( *host, host_length + 1, str_pos_start, host_length );
		*( *host + host_length ) = 0;	// Sanity
	}
	else if ( url[ 0 ] == L'/' )	// Resource is a relative URI that starts with a L'/'
	{
		// Save the resource.
		resource_length = lstrlenW( url );

		*resource = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( resource_length + 1 ) );
		_wmemcpy_s( *resource, resource_length + 1, url, resource_length );
		*( *resource + resource_length ) = 0;	// Sanity.
	}
	else	// Resource is a relative URI that does not start with a L'/'
	{
		resource_length = lstrlenW( url ) + 1;	// Include the L'/'

		*resource = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( resource_length + 1 ) );
		*resource[ 0 ] = L'/';
		_wmemcpy_s( *resource + 1, resource_length, url, resource_length );
		*( *resource + resource_length ) = 0;	// Sanity.
	}

	return true;
}

unsigned int DecompressStream( SOCKET_CONTEXT *context, char *buffer, unsigned int buffer_size )
{
	int stream_ret = Z_OK;
	unsigned int total_data_length = 0;

	if ( context->decompressed_buf == NULL )
	{
		context->decompressed_buf_size = ZLIB_CHUNK;
		context->decompressed_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * context->decompressed_buf_size );	// Allocate 16 kilobytes.

		_memzero( &context->stream, sizeof( z_stream ) );
		context->stream.zalloc = zGlobalAlloc;
		context->stream.zfree = zGlobalFree;

		// -MAX_WBITS		= deflate
		// MAX_WBITS		= zlib
		// MAX_WBITS + 16	= gzip
		// MAX_WBITS + 32	= Detects gzip or zlib
		stream_ret = _inflateInit2( &context->stream, ( context->header_info.content_encoding == CONTENT_ENCODING_GZIP ? MAX_WBITS + 16 : ( context->header_info.content_encoding == CONTENT_ENCODING_DEFLATE ? -MAX_WBITS : MAX_WBITS ) ) );	// 1 = gzip, 2 = deflate, everything else = default
	}

	context->stream.next_in = ( Bytef * )buffer;
	context->stream.avail_in = buffer_size;
	if ( context->stream.avail_in == 0 )
	{
		stream_ret = Z_DATA_ERROR;
	}

	bool retry = false;

	while ( stream_ret == Z_OK && context->stream.avail_in > 0 )
	{
		context->stream.next_out = ( Bytef * )context->decompressed_buf + total_data_length;
		context->stream.avail_out = context->decompressed_buf_size - total_data_length;

		stream_ret = _inflate( &context->stream, Z_NO_FLUSH );
		if ( stream_ret == Z_NEED_DICT )
		{
			stream_ret = Z_DATA_ERROR;
		}
		else if ( stream_ret == Z_DATA_ERROR )
		{
			if ( retry )
			{
				break;
			}
			
			_memzero( &context->stream, sizeof( z_stream ) );
			context->stream.zalloc = zGlobalAlloc;
			context->stream.zfree = zGlobalFree;

			context->stream.next_in = ( Bytef * )buffer;
			context->stream.avail_in = buffer_size;

			// See if it's a zlib wrapped compression.
			stream_ret = _inflateInit2( &context->stream, MAX_WBITS );

			retry = true;

			continue;
		}

		total_data_length = ( context->decompressed_buf_size - context->stream.avail_out );

		// Allocate more memory if we have any remaining data in our buffer to decompress.
		if ( stream_ret == Z_OK && context->stream.avail_in > 0 )
		{
			context->decompressed_buf_size += ZLIB_CHUNK;
			char *realloc_buffer = ( char * )GlobalReAlloc( context->decompressed_buf, sizeof( char ) * context->decompressed_buf_size, GMEM_MOVEABLE );
			if ( realloc_buffer != NULL )
			{
				context->decompressed_buf = realloc_buffer;
			}
		}
	}

	return total_data_length;
}


//
//
//
//
// HEADER FIELD FUNCTIONS
//
//
//
//

unsigned short GetHTTPStatus( char *header )
{
	// The status should be surrounded by two single spaces. Example: " 200 "
	char *status_start = _StrChrA( header, ' ' );
	if ( status_start != NULL )
	{
		++status_start;
		char *status_end = _StrChrA( status_start, ' ' );
		if ( status_end != NULL )
		{
			// Status response.
			char cstatus[ 4 ];
			_memcpy_s( cstatus, 4, status_start, ( ( status_end - status_start ) > 3 ? 3 : ( status_end - status_start ) ) );
			cstatus[ 3 ] = 0; // Sanity
			
			return ( unsigned short )_strtoul( cstatus, NULL, 10 );
		}
	}

	return 0;
}

unsigned char GetHTTPMethod( char *header )
{
	unsigned char method_type = METHOD_NONE;

	char *method_end = _StrChrA( header, ' ' );
	if ( method_end != NULL )
	{
		*method_end = 0;	// Temporary end.

		if ( ( method_end - header ) == 3 && _StrCmpNIA( header, "GET", 3 ) == 0 )
		{
			method_type = METHOD_GET;
		}
		else if ( ( method_end - header ) == 4 && _StrCmpNIA( header, "POST", 4 ) == 0 )
		{
			method_type = METHOD_POST;
		}
		else if ( ( method_end - header ) == 7 && _StrCmpNIA( header, "CONNECT", 7 ) == 0 )
		{
			method_type = METHOD_CONNECT;
		}
		else if ( ( method_end - header ) == 4 && _StrCmpNIA( header, "HEAD", 4 ) == 0 )
		{
			method_type = METHOD_HEAD;
		}
		else if ( ( method_end - header ) == 3 && _StrCmpNIA( header, "PUT", 3 ) == 0 )
		{
			method_type = METHOD_PUT;
		}
		else if ( ( method_end - header ) == 6 && _StrCmpNIA( header, "DELETE", 6 ) == 0 )
		{
			method_type = METHOD_DELETE;
		}
		else if ( ( method_end - header ) == 7 && _StrCmpNIA( header, "OPTIONS", 7 ) == 0 )
		{
			method_type = METHOD_OPTIONS;
		}
		else if ( ( method_end - header ) == 5 && _StrCmpNIA( header, "TRACE", 5 ) == 0 )
		{
			method_type = METHOD_TRACE;
		}
		else if ( ( method_end - header ) == 5 && _StrCmpNIA( header, "PATCH", 5 ) == 0 )
		{
			method_type = METHOD_PATCH;
		}
		else
		{
			method_type = METHOD_UNHANDLED;
		}

		*method_end = ' ';	// Restore.
	}

	return method_type;
}

bool GetTransferEncoding( char *header )
{
	char *transfer_encoding_header = NULL;
	char *transfer_encoding_header_end = NULL;

	if ( GetHeaderValue( header, "Transfer-Encoding", 17, &transfer_encoding_header, &transfer_encoding_header_end ) != NULL )
	{
		if ( ( transfer_encoding_header_end - transfer_encoding_header ) == 7 && _StrCmpNIA( transfer_encoding_header, "chunked", 7 ) == 0 )
		{
			return true;
		}
	}

	return false;
}

unsigned long long GetContentLength( char *header )
{
	char *content_length_header = NULL;
	char *content_length_header_end = NULL;

	if ( GetHeaderValue( header, "Content-Length", 14, &content_length_header, &content_length_header_end ) != NULL )
	{
		int content_length_length = ( content_length_header_end - content_length_header );
		if ( content_length_length > 20 )
		{
			content_length_length = 20;
		}

		char clength[ 21 ];
		_memzero( clength, 21 );
		_memcpy_s( clength, 21, content_length_header, content_length_length );
		clength[ 20 ] = 0;	// Sanity

		return strtoull( clength );
	}

	return 0;
}

void GetContentRange( char *header, RANGE_INFO *range_info )
{
	char *content_range_header = NULL;
	char *content_range_header_end = NULL;

	if ( GetHeaderValue( header, "Content-Range", 13, &content_range_header, &content_range_header_end ) != NULL )
	{
		char *content_range_value = _StrStrIA( content_range_header, "bytes" );
		if ( content_range_value != NULL )
		{
			content_range_value += 5;

			char *content_range_value_end = _StrChrA( content_range_value, '-' );
			char *tmp_content_range_value_end = content_range_value_end;
			if ( content_range_value_end != NULL )
			{
				// Skip whitespace.
				/*while ( content_range_value < content_range_value_end )
				{
					if ( *content_range_value != ' ' && *content_range_value != '\t' && *content_range_value != '\f' )
					{
						break;
					}

					++content_range_value;
				}*/
				while ( *content_range_value == ' ' || *content_range_value == '\t' || *content_range_value == '\f' )
				{
					++content_range_value;
				}

				// Skip whitespace that could appear before the '-', but after the field value.
				while ( ( content_range_value_end - 1 ) >= content_range_value )
				{
					if ( *( content_range_value_end - 1 ) != ' ' && *( content_range_value_end - 1 ) != '\t' && *( content_range_value_end - 1 ) != '\f' )
					{
						break;
					}

					--content_range_value_end;
				}

				char clength[ 21 ];
				_memzero( clength, 21 );
				_memcpy_s( clength, 21, content_range_value, ( ( content_range_value_end - content_range_value ) > 20 ? 20 : ( content_range_value_end - content_range_value ) ) );
				clength[ 20 ] = 0;	// Sanity

				range_info->range_start = strtoull( clength );

				content_range_value = tmp_content_range_value_end + 1;

				content_range_value_end = _StrChrA( content_range_value, '/' );
				tmp_content_range_value_end = content_range_value_end;
				if ( content_range_value_end != NULL )
				{
					// Skip whitespace.
					/*while ( content_range_value < content_range_value_end )
					{
						if ( *content_range_value != ' ' && *content_range_value != '\t' && *content_range_value != '\f' )
						{
							break;
						}

						++content_range_value;
					}*/
					while ( *content_range_value == ' ' || *content_range_value == '\t' || *content_range_value == '\f' )
					{
						++content_range_value;
					}

					// Skip whitespace that could appear before the '/', but after the field value.
					while ( ( content_range_value_end - 1 ) >= content_range_value )
					{
						if ( *( content_range_value_end - 1 ) != ' ' && *( content_range_value_end - 1 ) != '\t' && *( content_range_value_end - 1 ) != '\f' )
						{
							break;
						}

						--content_range_value_end;
					}

					char clength[ 21 ];
					_memzero( clength, 21 );
					_memcpy_s( clength, 21, content_range_value, ( ( content_range_value_end - content_range_value ) > 20 ? 20 : ( content_range_value_end - content_range_value ) ) );
					clength[ 20 ] = 0;	// Sanity

					range_info->range_end = strtoull( clength );

					content_range_value = tmp_content_range_value_end + 1;

					// Skip whitespace.
					/*while ( content_range_value < content_range_header_end )
					{
						if ( *content_range_value != ' ' && *content_range_value != '\t' && *content_range_value != '\f' )
						{
							break;
						}

						++content_range_value;
					}*/
					while ( *content_range_value == ' ' || *content_range_value == '\t' || *content_range_value == '\f' )
					{
						++content_range_value;
					}

					clength[ 21 ];
					_memzero( clength, 21 );
					_memcpy_s( clength, 21, content_range_value, ( ( content_range_header_end - content_range_value ) > 20 ? 20 : ( content_range_header_end - content_range_value ) ) );
					clength[ 20 ] = 0;	// Sanity

					range_info->content_length = strtoull( clength );
				}
			}
		}
	}
}

void GetLocation( char *header, URL_LOCATION *url_location )
{
	char *location_header = NULL;
	char *location_header_end = NULL;

	if ( GetHeaderValue( header, "Location", 8, &location_header, &location_header_end ) != NULL )
	{
		// Temporary string terminator.
		char tmp_end = *location_header_end;
		*location_header_end = 0;	// Sanity

		unsigned int host_length = 0;
		unsigned int resource_length = 0;

		ParseURL_A( location_header, url_location->protocol, &url_location->host, host_length, url_location->port, &url_location->resource, resource_length );

		char *a_resource = url_location->resource;
		unsigned int a_resource_length = 0;

		while ( *a_resource != NULL )
		{
			if ( *a_resource == '#' )
			{
				*a_resource = 0;
				resource_length = a_resource - url_location->resource;

				break;
			}

			++a_resource;
		}

		/*a_resource = url_encode_a( url_location->resource, resource_length, &a_resource_length );

		// Did we encode anything?
		if ( resource_length != a_resource_length )
		{
			GlobalFree( url_location->resource );
			url_location->resource = a_resource;
		}
		else
		{
			GlobalFree( a_resource );
		}*/

		*location_header_end = tmp_end;	// Restore.
	}
}

unsigned char GetConnection( char *header )
{
	unsigned char connection_type = CONNECTION_NONE;

	char *connection_header = NULL;
	char *connection_header_end = NULL;

	if ( GetHeaderValue( header, "Connection", 10, &connection_header, &connection_header_end ) != NULL )
	{
		char tmp_end = *connection_header_end;
		*connection_header_end = 0;	// Sanity

		// We need to search for this because there might be additional values for the field.
		char *connection = _StrStrIA( connection_header, "keep-alive" );
		if ( connection != NULL )
		{
			connection_type = CONNECTION_KEEP_ALIVE;
		}
		else	// If we found the keep-alive value, then there shouldn't be a close. Wouldn't make sense.
		{
			connection = _StrStrIA( connection_header, "close" );
			if ( connection != NULL )
			{
				connection_type = CONNECTION_CLOSE;
			}
		}

		*connection_header_end = tmp_end;	// Restore.
	}

	return connection_type;
}

unsigned char GetContentEncoding( char *header )
{
	char *content_encoding_header = NULL;
	char *content_encoding_header_end = NULL;

	if ( GetHeaderValue( header, "Content-Encoding", 16, &content_encoding_header, &content_encoding_header_end ) != NULL )
	{
		if ( ( content_encoding_header_end - content_encoding_header ) == 4 && _StrCmpNIA( content_encoding_header, "gzip", 4 ) == 0 )
		{
			return CONTENT_ENCODING_GZIP;
		}
		else if ( ( content_encoding_header_end - content_encoding_header ) == 7 && _StrCmpNIA( content_encoding_header, "deflate", 7 ) == 0 )
		{
			return CONTENT_ENCODING_DEFLATE;
		}
		else
		{
			return CONTENT_ENCODING_UNHANDLED;	// Unhandled.
		}
	}

	return CONTENT_ENCODING_NONE;
}

void GetAuthorization( char *header, AUTH_INFO *auth_info )
{
	char *authorization_header = NULL;
	char *authorization_header_end = NULL;

	if ( GetHeaderValue( header, "Authorization", 13, &authorization_header, &authorization_header_end ) != NULL )
	{
		if ( _StrStrIA( authorization_header, "Basic " ) != NULL )	// The protocol doesn't specify whether "Basic" is case-sensitive or not. Note that the protocol requires a single space (SP) after "Basic".
		{
			authorization_header += 6;

			auth_info->auth_type = AUTH_TYPE_BASIC;

			int basic_value_length = authorization_header_end - authorization_header;

			// We'll just use the digest's nonce value to store the key. No need to introduce any extra memory.
			auth_info->nonce = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( basic_value_length + 1 ) );
			_memcpy_s( auth_info->nonce, basic_value_length + 1, authorization_header, basic_value_length );
			auth_info->nonce[ basic_value_length ] = 0;	// Sanity
		}
		else if ( _StrStrIA( authorization_header, "Digest " ) != NULL )	// The protocol doesn't specify whether "Digest" is case-sensitive or not. Note that the protocol requires a single space (SP) after "Digest".
		{
			char tmp_end = *authorization_header_end;
			*authorization_header_end = 0;	// Sanity

			authorization_header += 7;

			char *digest_value = NULL;
			char *digest_value_end = NULL;

			if ( GetDigestValue( authorization_header, "username", 8, &digest_value, &digest_value_end ) != NULL )
			{
				char delimiter = digest_value[ 0 ];
				if ( delimiter == '\"' || delimiter == '\'' )
				{
					++digest_value;

					if ( *( digest_value_end - 1 ) == delimiter )
					{
						--digest_value_end;
					}
				}

				int digest_value_length = digest_value_end - digest_value;

				auth_info->username = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( digest_value_length + 1 ) );
				_memcpy_s( auth_info->username, digest_value_length + 1, digest_value, digest_value_length );
				auth_info->username[ digest_value_length ] = 0;	// Sanity
			}

			if ( GetDigestValue( authorization_header, "realm", 5, &digest_value, &digest_value_end ) != NULL )
			{
				char delimiter = digest_value[ 0 ];
				if ( delimiter == '\"' || delimiter == '\'' )
				{
					++digest_value;

					if ( *( digest_value_end - 1 ) == delimiter )
					{
						--digest_value_end;
					}
				}

				int digest_value_length = digest_value_end - digest_value;

				auth_info->realm = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( digest_value_length + 1 ) );
				_memcpy_s( auth_info->realm, digest_value_length + 1, digest_value, digest_value_length );
				auth_info->realm[ digest_value_length ] = 0;	// Sanity
			}

			if ( GetDigestValue( authorization_header, "nonce", 5, &digest_value, &digest_value_end ) != NULL )
			{
				char delimiter = digest_value[ 0 ];
				if ( delimiter == '\"' || delimiter == '\'' )
				{
					++digest_value;

					if ( *( digest_value_end - 1 ) == delimiter )
					{
						--digest_value_end;
					}
				}

				int digest_value_length = digest_value_end - digest_value;

				auth_info->nonce = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( digest_value_length + 1 ) );
				_memcpy_s( auth_info->nonce, digest_value_length + 1, digest_value, digest_value_length );
				auth_info->nonce[ digest_value_length ] = 0;	// Sanity
			}

			if ( GetDigestValue( authorization_header, "uri", 3, &digest_value, &digest_value_end ) != NULL )
			{
				char delimiter = digest_value[ 0 ];
				if ( delimiter == '\"' || delimiter == '\'' )
				{
					++digest_value;

					if ( *( digest_value_end - 1 ) == delimiter )
					{
						--digest_value_end;
					}
				}

				int digest_value_length = digest_value_end - digest_value;

				auth_info->uri = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( digest_value_length + 1 ) );
				_memcpy_s( auth_info->uri, digest_value_length + 1, digest_value, digest_value_length );
				auth_info->uri[ digest_value_length ] = 0;	// Sanity
			}

			if ( GetDigestValue( authorization_header, "algorithm", 9, &digest_value, &digest_value_end ) != NULL )
			{
				char delimiter = digest_value[ 0 ];
				if ( delimiter == '\"' || delimiter == '\'' )
				{
					++digest_value;

					if ( *( digest_value_end - 1 ) == delimiter )
					{
						--digest_value_end;
					}
				}

				if ( ( digest_value_end - digest_value ) == 8 && _StrCmpNIA( digest_value, "MD5-sess", 8 ) == 0 )
				{
					auth_info->algorithm = 2;
				}
				else if ( ( digest_value_end - digest_value ) == 3 && _StrCmpNIA( digest_value, "MD5", 3 ) == 0 )
				{
					auth_info->algorithm = 1;
				}
				else
				{
					auth_info->algorithm = 3;	// Unhandled.
				}
			}

			if ( GetDigestValue( authorization_header, "response", 8, &digest_value, &digest_value_end ) != NULL )
			{
				char delimiter = digest_value[ 0 ];
				if ( delimiter == '\"' || delimiter == '\'' )
				{
					++digest_value;

					if ( *( digest_value_end - 1 ) == delimiter )
					{
						--digest_value_end;
					}
				}

				int digest_value_length = digest_value_end - digest_value;

				auth_info->response = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( digest_value_length + 1 ) );
				_memcpy_s( auth_info->response, digest_value_length + 1, digest_value, digest_value_length );
				auth_info->response[ digest_value_length ] = 0;	// Sanity
			}

			/*if ( GetDigestValue( authorization_header, "domain", 6, &digest_value, &digest_value_end ) != NULL )
			{
				char delimiter = digest_value[ 0 ];
				if ( delimiter == '\"' || delimiter == '\'' )
				{
					++digest_value;

					if ( *( digest_value_end - 1 ) == delimiter )
					{
						--digest_value_end;
					}
				}

				int digest_value_length = digest_value_end - digest_value;

				auth_info->domain = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( digest_value_length + 1 ) );
				_memcpy_s( auth_info->domain, digest_value_length + 1, digest_value, digest_value_length );
				auth_info->domain[ digest_value_length ] = 0;	// Sanity
			}*/

			if ( GetDigestValue( authorization_header, "opaque", 6, &digest_value, &digest_value_end ) != NULL )
			{
				char delimiter = digest_value[ 0 ];
				if ( delimiter == '\"' || delimiter == '\'' )
				{
					++digest_value;

					if ( *( digest_value_end - 1 ) == delimiter )
					{
						--digest_value_end;
					}
				}

				int digest_value_length = digest_value_end - digest_value;

				auth_info->opaque = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( digest_value_length + 1 ) );
				_memcpy_s( auth_info->opaque, digest_value_length + 1, digest_value, digest_value_length );
				auth_info->opaque[ digest_value_length ] = 0;	// Sanity
			}

			if ( GetDigestValue( authorization_header, "qop", 3, &digest_value, &digest_value_end ) != NULL )
			{
				char delimiter = digest_value[ 0 ];
				if ( delimiter == '\"' || delimiter == '\'' )
				{
					++digest_value;

					if ( *( digest_value_end - 1 ) == delimiter )
					{
						--digest_value_end;
					}
				}

				int digest_value_length = digest_value_end - digest_value;

				auth_info->qop = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( digest_value_length + 1 ) );
				_memcpy_s( auth_info->qop, digest_value_length + 1, digest_value, digest_value_length );
				auth_info->qop[ digest_value_length ] = 0;	// Sanity

				char tmp_end = *digest_value_end;
				*digest_value_end = 0;	// Sanity

				// We need to search for this because there might be additional values for the field.
				char *qop_type_search = _StrStrIA( digest_value, "auth-int" );
				if ( qop_type_search != NULL )
				{
					qop_type_search[ 0 ] = '-';	// Set the string so we don't get a partial search match below.
					auth_info->qop_type = 2;
				}

				// If auth is specified, then we'll use that instead.
				do
				{
					qop_type_search = _StrStrIA( digest_value, "auth" );
					if ( qop_type_search != NULL )
					{
						if ( qop_type_search[ 4 ] == NULL ||
							 qop_type_search[ 4 ] == ' '  ||
							 qop_type_search[ 4 ] == '\t' ||
							 qop_type_search[ 4 ] == '\f' ||
							 qop_type_search[ 4 ] == ',' )
						{
							auth_info->qop_type = 1;
							break;
						}
						else
						{
							qop_type_search[ 0 ] = '-';	// Set the string so we don't get a partial search match below.
						}
					}
				}
				while ( qop_type_search != NULL );

				*digest_value_end = tmp_end;	// Restore.
			}

			if ( GetDigestValue( authorization_header, "nc", 2, &digest_value, &digest_value_end ) != NULL )
			{
				char delimiter = digest_value[ 0 ];
				if ( delimiter == '\"' || delimiter == '\'' )
				{
					++digest_value;

					if ( *( digest_value_end - 1 ) == delimiter )
					{
						--digest_value_end;
					}
				}

				char cnc[ 21 ];
				_memzero( cnc, 21 );
				_memcpy_s( cnc, 21, digest_value, ( ( digest_value_end - digest_value ) > 20 ? 20 : ( digest_value_end - digest_value ) ) );
				cnc[ 20 ] = 0;	// Sanity

				auth_info->nc = ( unsigned int )strtoull( cnc, true );
			}

			if ( GetDigestValue( authorization_header, "cnonce", 6, &digest_value, &digest_value_end ) != NULL )
			{
				char delimiter = digest_value[ 0 ];
				if ( delimiter == '\"' || delimiter == '\'' )
				{
					++digest_value;

					if ( *( digest_value_end - 1 ) == delimiter )
					{
						--digest_value_end;
					}
				}

				int digest_value_length = digest_value_end - digest_value;

				auth_info->cnonce = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( digest_value_length + 1 ) );
				_memcpy_s( auth_info->cnonce, digest_value_length + 1, digest_value, digest_value_length );
				auth_info->cnonce[ digest_value_length ] = 0;	// Sanity
			}

			*authorization_header_end = tmp_end;	// Restore.

			auth_info->auth_type = AUTH_TYPE_DIGEST;
		}
	}
}

void GetAuthenticate( char *header, unsigned char auth_header_type, AUTH_INFO *auth_info )
{
	char *authenticate_header = NULL;
	char *authenticate_header_end = NULL;

	char *authentication_field = NULL;

	if ( auth_header_type == 1 )
	{
		authentication_field = GetHeaderValue( header, "WWW-Authenticate", 16, &authenticate_header, &authenticate_header_end );
	}
	else if ( auth_header_type == 2 )
	{
		authentication_field = GetHeaderValue( header, "Proxy-Authenticate", 18, &authenticate_header, &authenticate_header_end );
	}

	if ( authentication_field != NULL )
	{
		if ( _StrStrIA( authenticate_header, "Basic " ) != NULL )	// The protocol doesn't specify whether "Basic" is case-sensitive or not. Note that the protocol requires a single space (SP) after "Basic".
		{
			auth_info->auth_type = AUTH_TYPE_BASIC;
		}
		else if ( _StrStrIA( authenticate_header, "Digest " ) != NULL )	// The protocol doesn't specify whether "Digest" is case-sensitive or not. Note that the protocol requires a single space (SP) after "Digest".
		{
			char tmp_end = *authenticate_header_end;
			*authenticate_header_end = 0;	// Sanity

			authenticate_header += 7;

			char *digest_value = NULL;
			char *digest_value_end = NULL;

			if ( GetDigestValue( authenticate_header, "realm", 5, &digest_value, &digest_value_end ) != NULL )
			{
				char delimiter = digest_value[ 0 ];
				if ( delimiter == '\"' || delimiter == '\'' )
				{
					++digest_value;

					if ( *( digest_value_end - 1 ) == delimiter )
					{
						--digest_value_end;
					}
				}

				int digest_value_length = digest_value_end - digest_value;

				auth_info->realm = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( digest_value_length + 1 ) );
				_memcpy_s( auth_info->realm, digest_value_length + 1, digest_value, digest_value_length );
				auth_info->realm[ digest_value_length ] = 0;	// Sanity
			}

			if ( GetDigestValue( authenticate_header, "nonce", 5, &digest_value, &digest_value_end ) != NULL )
			{
				char delimiter = digest_value[ 0 ];
				if ( delimiter == '\"' || delimiter == '\'' )
				{
					++digest_value;

					if ( *( digest_value_end - 1 ) == delimiter )
					{
						--digest_value_end;
					}
				}

				int digest_value_length = digest_value_end - digest_value;

				auth_info->nonce = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( digest_value_length + 1 ) );
				_memcpy_s( auth_info->nonce, digest_value_length + 1, digest_value, digest_value_length );
				auth_info->nonce[ digest_value_length ] = 0;	// Sanity
			}

			if ( GetDigestValue( authenticate_header, "algorithm", 9, &digest_value, &digest_value_end ) != NULL )
			{
				char delimiter = digest_value[ 0 ];
				if ( delimiter == '\"' || delimiter == '\'' )
				{
					++digest_value;

					if ( *( digest_value_end - 1 ) == delimiter )
					{
						--digest_value_end;
					}
				}

				if ( ( digest_value_end - digest_value ) == 8 && _StrCmpNIA( digest_value, "MD5-sess", 8 ) == 0 )
				{
					auth_info->algorithm = 2;
				}
				else if ( ( digest_value_end - digest_value ) == 3 && _StrCmpNIA( digest_value, "MD5", 3 ) == 0 )
				{
					auth_info->algorithm = 1;
				}
				else
				{
					auth_info->algorithm = 3;	// Unhandled.
				}
			}

			if ( GetDigestValue( authenticate_header, "domain", 6, &digest_value, &digest_value_end ) != NULL )
			{
				char delimiter = digest_value[ 0 ];
				if ( delimiter == '\"' || delimiter == '\'' )
				{
					++digest_value;

					if ( *( digest_value_end - 1 ) == delimiter )
					{
						--digest_value_end;
					}
				}

				int digest_value_length = digest_value_end - digest_value;

				auth_info->domain = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( digest_value_length + 1 ) );
				_memcpy_s( auth_info->domain, digest_value_length + 1, digest_value, digest_value_length );
				auth_info->domain[ digest_value_length ] = 0;	// Sanity
			}

			if ( GetDigestValue( authenticate_header, "opaque", 6, &digest_value, &digest_value_end ) != NULL )
			{
				char delimiter = digest_value[ 0 ];
				if ( delimiter == '\"' || delimiter == '\'' )
				{
					++digest_value;

					if ( *( digest_value_end - 1 ) == delimiter )
					{
						--digest_value_end;
					}
				}

				int digest_value_length = digest_value_end - digest_value;

				auth_info->opaque = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( digest_value_length + 1 ) );
				_memcpy_s( auth_info->opaque, digest_value_length + 1, digest_value, digest_value_length );
				auth_info->opaque[ digest_value_length ] = 0;	// Sanity
			}

			if ( GetDigestValue( authenticate_header, "qop", 3, &digest_value, &digest_value_end ) != NULL )
			{
				char delimiter = digest_value[ 0 ];
				if ( delimiter == '\"' || delimiter == '\'' )
				{
					++digest_value;

					if ( *( digest_value_end - 1 ) == delimiter )
					{
						--digest_value_end;
					}
				}

				int digest_value_length = digest_value_end - digest_value;

				auth_info->qop = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( digest_value_length + 1 ) );
				_memcpy_s( auth_info->qop, digest_value_length + 1, digest_value, digest_value_length );
				auth_info->qop[ digest_value_length ] = 0;	// Sanity

				char tmp_end = *digest_value_end;
				*digest_value_end = 0;	// Sanity

				// We need to search for this because there might be additional values for the field.
				char *qop_type_search = _StrStrIA( digest_value, "auth-int" );
				if ( qop_type_search != NULL )
				{
					qop_type_search[ 0 ] = '-';	// Set the string so we don't get a partial search match below.
					auth_info->qop_type = 2;
				}

				// If auth is specified, then we'll use that instead.
				do
				{
					qop_type_search = _StrStrIA( digest_value, "auth" );
					if ( qop_type_search != NULL )
					{
						if ( qop_type_search[ 4 ] == NULL ||
							 qop_type_search[ 4 ] == ' '  ||
							 qop_type_search[ 4 ] == '\t' ||
							 qop_type_search[ 4 ] == '\f' ||
							 qop_type_search[ 4 ] == ',' )
						{
							auth_info->qop_type = 1;
							break;
						}
						else
						{
							qop_type_search[ 0 ] = '-';	// Set the string so we don't get a partial search match below.
						}
					}
				}
				while ( qop_type_search != NULL );

				*digest_value_end = tmp_end;	// Restore.
			}

			*authenticate_header_end = tmp_end;	// Restore.

			auth_info->auth_type = AUTH_TYPE_DIGEST;
		}
		else
		{
			auth_info->auth_type = AUTH_TYPE_UNHANDLED;	// Unhandled.
		}
	}
}

char *GetContentDisposition( char *header, unsigned int &filename_length )
{
	char *content_disposition_header = NULL;
	char *content_disposition_header_end = NULL;

	if ( GetHeaderValue( header, "Content-Disposition", 19, &content_disposition_header, &content_disposition_header_end ) != NULL )
	{
		// Case insensitive. The RFC doesn't mention anything about it.
		content_disposition_header = _StrStrIA( content_disposition_header, "filename" );
		if ( content_disposition_header != NULL )
		{
			content_disposition_header = content_disposition_header + 8;

			// Filename was encoded.
			if ( *content_disposition_header = '*' )
			{
				// Language opening.
				content_disposition_header = _StrChrA( content_disposition_header + 1, '\'' );
				if ( content_disposition_header != NULL )
				{
					// Language closing.
					content_disposition_header = _StrChrA( content_disposition_header + 1, '\'' );
				}
			}
			else
			{
				content_disposition_header = _StrChrA( content_disposition_header, '=' );
			}

			if ( content_disposition_header != NULL )
			{
				++content_disposition_header;

				// Skip whitespace.
				/*while ( content_disposition_header < content_disposition_header_end )
				{
					if ( *content_disposition_header != ' ' && *content_disposition_header != '\t' && *content_disposition_header != '\f' )
					{
						break;
					}

					++content_disposition_header;
				}*/
				while ( *content_disposition_header == ' ' || *content_disposition_header == '\t' || *content_disposition_header == '\f' )
				{
					++content_disposition_header;
				}

				// Skip whitespace that could appear before the "\r\n", but after the field value.
				while ( ( content_disposition_header_end - 1 ) >= content_disposition_header )
				{
					if ( *( content_disposition_header_end - 1 ) != ' ' && *( content_disposition_header_end - 1 ) != '\t' && *( content_disposition_header_end - 1 ) != '\f' )
					{
						break;
					}

					--content_disposition_header_end;
				}

				char delimiter = content_disposition_header[ 0 ];
				if ( delimiter == '\"' || delimiter == '\'' )
				{
					++content_disposition_header;

					if ( *( content_disposition_header_end - 1 ) == delimiter )
					{
						--content_disposition_header_end;
					}
				}

				return url_decode_a( content_disposition_header, content_disposition_header_end - content_disposition_header, &filename_length );
			}
		}
	}

	return NULL;
}
/*
char *GetETag( char *header )
{
	char *etag_header = NULL;
	char *etag_header_end = NULL;

	if ( GetHeaderValue( header, "ETag", 4, &etag_header, &etag_header_end ) != NULL )
	{
		int etag_length = etag_header_end - etag_header;

		// Save the etag.
		char *etag = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( etag_length + 1 ) );
		_memcpy_s( etag, etag_length + 1, etag_header, etag_length );
		etag[ etag_length ] = 0;	// Sanity

		return etag;
	}

	return NULL;
}
*/

//
//
//
//
// HEADER FIELD FUNCTIONS
//
//
//
//
int ParseHTTPHeader( SOCKET_CONTEXT *context, char *header_buffer, unsigned int header_buffer_length, bool request )
{
	if ( context == NULL )
	{
		return CONTENT_STATUS_FAILED;
	}

	char *end_of_header = NULL;
	char *last_field = NULL;

	unsigned int buffer_offset = 0;
	unsigned char tmp_terminator = 0;

	unsigned long long content_length = 0;

	// Try to find the end of the last valid field.
	char *next_field = _StrStrA( header_buffer, "\r\n" );
	if ( next_field != NULL )
	{
		do
		{
			next_field += 2;

			last_field = next_field;

			if ( ( unsigned int )( ( next_field + 2 ) - header_buffer ) <= header_buffer_length )
			{
				// See if we can find the header terminator after the field terminator.
				if ( next_field[ 0 ] == '\r' && next_field[ 1 ] == '\n' )
				{
					end_of_header = next_field + 2;

					break;
				}
				else	// Find the next field if we didn't find the header terminator.
				{
					next_field = _StrStrA( next_field, "\r\n" );
				}
			}
			else	// Not enough characters to search for anything else.
			{
				break;
			}
		}
		while ( next_field != NULL );

		// The last field terminator should be valid if the header terminator was not found.
		if ( end_of_header == NULL )
		{
			end_of_header = last_field;

			// We'll end up moving the remaining bytes (after this offset) to the beginning of the buffer.
			buffer_offset = ( last_field - header_buffer );

			// The only remaining field is the terminating "\r\n".
			if ( buffer_offset == 2 && header_buffer[ 0 ] == '\r' && header_buffer[ 1 ] == '\n' )
			{
				buffer_offset = 0;
			}
			else
			{
				tmp_terminator = header_buffer[ buffer_offset ];
				header_buffer[ buffer_offset ] = 0;	// Sanity.
			}
		}
	}
	else
	{
		context->wsabuf.buf += header_buffer_length;
		context->wsabuf.len -= header_buffer_length;

		if ( context->wsabuf.len > 0 )
		{
			return CONTENT_STATUS_READ_MORE_HEADER;	// Need more header data.
		}
		else
		{
			return CONTENT_STATUS_FAILED;	// Buffer is too small to work with.
		}
	}

	if ( end_of_header != NULL )
	{
		if ( !request )
		{
			if ( context->header_info.http_status == 0 )
			{
				context->header_info.http_status = GetHTTPStatus( header_buffer );

				if ( context->header_info.http_status == 0 )
				{
					return CONTENT_STATUS_FAILED;
				}
			}
		}
		else
		{
			if ( context->header_info.http_method == METHOD_NONE )
			{
				context->header_info.http_method = GetHTTPMethod( header_buffer );

				if ( context->header_info.http_method == METHOD_NONE )
				{
					return CONTENT_STATUS_FAILED;
				}
			}

			if ( context->header_info.digest_info == NULL )
			{
				context->header_info.digest_info = ( AUTH_INFO * )GlobalAlloc( GPTR, sizeof( AUTH_INFO ) );
			}

			if ( context->header_info.digest_info != NULL &&
				 context->header_info.digest_info->auth_type == AUTH_TYPE_NONE )
			{
				GetAuthorization( header_buffer, context->header_info.digest_info );
			}
		}

		char *new_cookies = NULL;

		// This value will be saved
		if ( !ParseCookies( header_buffer, &context->header_info.cookie_tree, &new_cookies, end_of_header ) )
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
				// If it's not, then free the old cookie and update it to the new one.
				GlobalFree( context->header_info.cookies );
				context->header_info.cookies = new_cookies;
			}
			else	// Otherwise, if the cookie is blank, then free it.
			{
				GlobalFree( new_cookies );
			}
		}

		if ( !context->header_info.chunked_transfer )
		{
			context->header_info.chunked_transfer = GetTransferEncoding( header_buffer );
		}

		if ( context->header_info.range_info->content_length == 0 )
		{
			content_length = GetContentLength( header_buffer );

			GetContentRange( header_buffer, context->header_info.range_info );

			// Set the content_length and range content_length to the same value. (Use the greater of the two values.)
			if ( content_length > context->header_info.range_info->content_length )
			{
				context->header_info.range_info->content_length = content_length;
			}
		}

		if ( context->header_info.url_location.host == NULL )
		{
			GetLocation( header_buffer, &context->header_info.url_location );

			// This is so stupid. Match the relative URI protocol/port with the request URI protocol/port.
			if ( context->header_info.url_location.protocol == PROTOCOL_RELATIVE )
			{
				// Only set the port of it wasn't included.
				if ( context->header_info.url_location.port == 0 )
				{
					context->header_info.url_location.port = context->request_info.port;
				}

				context->header_info.url_location.protocol = context->request_info.protocol;
			}
		}

		if ( context->header_info.connection == CONNECTION_NONE )
		{
			context->header_info.connection = GetConnection( header_buffer );

			// If the Connection header field does not exist, then by default, it'll be keep-alive.
			if ( context->header_info.connection == CONNECTION_NONE )
			{
				context->header_info.connection = CONNECTION_KEEP_ALIVE;
			}
		}

		if ( context->header_info.content_encoding == CONTENT_ENCODING_NONE )
		{
			context->header_info.content_encoding = GetContentEncoding( header_buffer );
		}

		if ( context->header_info.http_status == 401 )
		{
			if ( context->header_info.digest_info == NULL )
			{
				context->header_info.digest_info = ( AUTH_INFO * )GlobalAlloc( GPTR, sizeof( AUTH_INFO ) );
			}

			if ( context->header_info.digest_info != NULL &&
				 context->header_info.digest_info->auth_type == AUTH_TYPE_NONE )
			{
				GetAuthenticate( header_buffer, 1, context->header_info.digest_info );
			}
		}

		if ( context->header_info.http_status == 407 )
		{
			if ( ( cfg_enable_proxy && context->request_info.protocol == PROTOCOL_HTTP ) ||
				 ( cfg_enable_proxy_s && context->request_info.protocol == PROTOCOL_HTTPS ) )
			{
				if ( context->header_info.proxy_digest_info == NULL )
				{
					context->header_info.proxy_digest_info = ( AUTH_INFO * )GlobalAlloc( GPTR, sizeof( AUTH_INFO ) );
				}

				if ( context->header_info.proxy_digest_info != NULL &&
					 context->header_info.proxy_digest_info->auth_type == AUTH_TYPE_NONE )
				{
					GetAuthenticate( header_buffer, 2, context->header_info.proxy_digest_info );
				}
			}
		}

		if ( context->got_filename == 0 )
		{
			unsigned int filename_length = 0;

			char *filename = GetContentDisposition( header_buffer, filename_length );
			if ( filename != NULL )
			{
				// Remove directory paths.
				char *tmp_filename = filename;
				while ( filename_length > 0 )
				{
					if ( filename[ --filename_length ] == '\\' || filename[ filename_length ] == '/' )
					{
						tmp_filename = filename + ( filename_length + 1 );
						break;
					}
				}

				if ( context->download_info != NULL )
				{
					EnterCriticalSection( &icon_cache_cs );
					// Find the icon info
					dllrbt_iterator *itr = dllrbt_find( icon_handles, ( void * )( context->download_info->file_path + context->download_info->file_extension_offset ), false );

					// Free its values and remove it from the tree if there are no other items using it.
					if ( itr != NULL )
					{
						ICON_INFO *ii = ( ICON_INFO * )( ( node_type * )itr )->val;
						if ( ii != NULL )
						{
							context->download_info->icon = NULL;

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

					EnterCriticalSection( &context->download_info->shared_cs );

					int w_filename_length = MultiByteToWideChar( CP_UTF8, 0, tmp_filename, -1, context->download_info->file_path + context->download_info->filename_offset, MAX_PATH - context->download_info->filename_offset - 1 ) - 1;
					if ( w_filename_length == -1 && GetLastError() == ERROR_INSUFFICIENT_BUFFER )
					{
						w_filename_length = MAX_PATH - context->download_info->filename_offset - 1;
					}

					EscapeFilename( context->download_info->file_path + context->download_info->filename_offset );

					context->download_info->file_extension_offset = context->download_info->filename_offset + get_file_extension_offset( context->download_info->file_path + context->download_info->filename_offset, w_filename_length );

					wchar_t file_path[ MAX_PATH ];
					_wmemcpy_s( file_path, MAX_PATH, context->download_info->file_path, MAX_PATH );
					if ( context->download_info->filename_offset > 0 )
					{
						file_path[ context->download_info->filename_offset - 1 ] = L'\\';	// Replace the download directory NULL terminator with a directory slash.
					}

					// Make sure any existing file hasn't started downloading.
					if ( !( context->download_info->download_operations & DOWNLOAD_OPERATION_SIMULATE ) &&
						  GetFileAttributes( file_path ) != INVALID_FILE_ATTRIBUTES &&
						  context->download_info->downloaded == 0 )
					{
						context->got_filename = 2;
					}
					else	// No need to rename.
					{
						context->got_filename = 1;
					}

					LeaveCriticalSection( &context->download_info->shared_cs );

					SHFILEINFO *sfi = ( SHFILEINFO * )GlobalAlloc( GMEM_FIXED, sizeof( SHFILEINFO ) );

					// Cache our file's icon.
					ICON_INFO *ii = CacheIcon( context->download_info, sfi );

					EnterCriticalSection( &context->download_info->shared_cs );

					context->download_info->icon = ( ii != NULL ? &ii->icon : NULL );

					LeaveCriticalSection( &context->download_info->shared_cs );

					GlobalFree( sfi );
				}

				GlobalFree( filename );
			}
		}

		/*if ( !context->header_info.etag )
		{
			char *etag = GetETag( header_buffer );
			if ( etag != NULL )
			{
				if ( context->download_info != NULL )
				{
					EnterCriticalSection( &context->download_info->shared_cs );

					if ( context->download_info->etag != NULL )
					{
						GlobalFree( context->download_info->etag );
					}

					context->download_info->etag = etag;

					LeaveCriticalSection( &context->download_info->shared_cs );

					context->header_info.etag = true;
				}
				else
				{
					GlobalFree( etag );
				}
			}
		}*/
	}

	// If we have an incomplete header, then store the last header field fragment and request more data.
	if ( buffer_offset > 0 )
	{
		header_buffer[ buffer_offset ] = tmp_terminator;	// Restore.

		// Move the fragment at the end of the buffer to the beginning.
		int fragment_length = header_buffer_length - buffer_offset;
		_memmove( context->buffer, context->buffer + buffer_offset, fragment_length );

		context->wsabuf.buf += fragment_length;
		context->wsabuf.len -= fragment_length;

		return CONTENT_STATUS_READ_MORE_HEADER;	// Need more header data.
	}
	else
	{
		context->header_info.end_of_header = end_of_header;
	}

	return CONTENT_STATUS_GET_CONTENT;
}

int GetHTTPHeader( SOCKET_CONTEXT *context, char *header_buffer, unsigned int header_buffer_length )
{
	if ( context == NULL )
	{
		return CONTENT_STATUS_FAILED;
	}

	int content_status = ParseHTTPHeader( context, header_buffer, header_buffer_length );

	if ( content_status == CONTENT_STATUS_READ_MORE_HEADER )		// Request more header data.
	{
		return CONTENT_STATUS_READ_MORE_HEADER;
	}
	else if ( content_status == CONTENT_STATUS_FAILED )				// File rename error with content-disposition header field, or user selected No to all when file size prompt was displayed. We don't want to continue.
	{
		return CONTENT_STATUS_FAILED;
	}
	else// if ( content_status == CONTENT_STATUS_GET_CONTENT );
	{
		// If resource is not NULL and we got a 3XX status.
		// Handle redirects before processing any data.
		if ( context->header_info.http_status >= 300 &&
			 context->header_info.http_status <= 399 &&
			 context->header_info.url_location.resource != NULL &&
			 cfg_max_redirects > 0 && context->request_info.redirect_count < cfg_max_redirects )
		{
			// If we're going to redirect, then allow the file to be renamed.
			unsigned int filename_length = lstrlenA( context->header_info.url_location.resource );

			char *filename = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( filename_length + 1 ) );
			if ( filename != NULL )
			{
				_memcpy_s( filename, filename_length + 1, context->header_info.url_location.resource, filename_length );
				filename[ filename_length ] = 0;	// Sanity.

				char *directory_ptr = filename;
				char *current_directory = filename;
				char *last_directory = NULL;

				// Iterate forward because '/' can be found after '#'.
				while ( *directory_ptr != NULL )
				{
					if ( *directory_ptr == '?' || *directory_ptr == '#' )
					{
						*directory_ptr = 0;	// Sanity.

						break;
					}
					else if ( *directory_ptr == '/' )
					{
						last_directory = current_directory;
						current_directory = directory_ptr + 1; 
					}

					++directory_ptr;
				}

				if ( *current_directory == NULL )
				{
					// Adjust for '/'. current_directory will always be at least 1 greater than last_directory.
					if ( last_directory != NULL && ( current_directory - 1 ) - last_directory > 0 )
					{
						*( --current_directory ) = 0;	// Sanity.
						current_directory = last_directory;
					}
					else	// No filename could be made from the resource path. Use the host name instead.
					{
						if ( context->header_info.url_location.host != NULL )
						{
							current_directory = context->header_info.url_location.host;
						}
						else if ( context->request_info.host != NULL )	// Relative redirection.
						{
							current_directory = context->request_info.host;
						}
						else	// Shouldn't happen.
						{
							current_directory = "NO_FILENAME";
						}
					}
				}

				if ( context->download_info != NULL )
				{
					EnterCriticalSection( &icon_cache_cs );
					// Find the icon info
					dllrbt_iterator *itr = dllrbt_find( icon_handles, ( void * )( context->download_info->file_path + context->download_info->file_extension_offset ), false );

					// Free its values and remove it from the tree if there are no other items using it.
					if ( itr != NULL )
					{
						ICON_INFO *ii = ( ICON_INFO * )( ( node_type * )itr )->val;
						if ( ii != NULL )
						{
							context->download_info->icon = NULL;

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

					EnterCriticalSection( &context->download_info->shared_cs );

					int w_filename_length = MultiByteToWideChar( CP_UTF8, 0, current_directory, -1, context->download_info->file_path + context->download_info->filename_offset, MAX_PATH - context->download_info->filename_offset - 1 ) - 1;
					if ( w_filename_length == -1 && GetLastError() == ERROR_INSUFFICIENT_BUFFER )
					{
						w_filename_length = MAX_PATH - context->download_info->filename_offset - 1;
					}

					EscapeFilename( context->download_info->file_path + context->download_info->filename_offset );

					context->download_info->file_extension_offset = context->download_info->filename_offset + get_file_extension_offset( context->download_info->file_path + context->download_info->filename_offset, w_filename_length );

					wchar_t file_path[ MAX_PATH ];
					_wmemcpy_s( file_path, MAX_PATH, context->download_info->file_path, MAX_PATH );
					if ( context->download_info->filename_offset > 0 )
					{
						file_path[ context->download_info->filename_offset - 1 ] = L'\\';	// Replace the download directory NULL terminator with a directory slash.
					}

					// Make sure any existing file hasn't started downloading.
					if ( !( context->download_info->download_operations & DOWNLOAD_OPERATION_SIMULATE ) &&
						  GetFileAttributes( file_path ) != INVALID_FILE_ATTRIBUTES &&
						  context->download_info->downloaded == 0 )
					{
						context->got_filename = 2;
					}
					else	// No need to rename.
					{
						context->got_filename = 1;
					}

					LeaveCriticalSection( &context->download_info->shared_cs );

					SHFILEINFO *sfi = ( SHFILEINFO * )GlobalAlloc( GMEM_FIXED, sizeof( SHFILEINFO ) );

					// Cache our file's icon.
					ICON_INFO *ii = CacheIcon( context->download_info, sfi );

					EnterCriticalSection( &context->download_info->shared_cs );

					context->download_info->icon = ( ii != NULL ? &ii->icon : NULL );

					LeaveCriticalSection( &context->download_info->shared_cs );

					GlobalFree( sfi );
				}

				GlobalFree( filename );
			}

			// The connection will get closed in here.
			return HandleRedirect( context );
		}
		else if ( context->header_info.http_status == 401 ||
				  context->header_info.http_status == 407 )	// Handle authorization requests before processing any data.
		{
			AUTH_INFO *auth_info = NULL;

			if ( context->header_info.http_status == 401 )
			{
				auth_info = context->header_info.digest_info;
			}
			else// if ( context->header_info.http_status == 407 )
			{
				if ( ( cfg_enable_proxy && context->request_info.protocol == PROTOCOL_HTTP ) ||
					 ( cfg_enable_proxy_s && context->request_info.protocol == PROTOCOL_HTTPS ) )
				{
					auth_info = context->header_info.proxy_digest_info;
				}
			}

			// We're only going to try 1 time if nonce count is 0.
			// nonce count is meant for digest authorization, but we'll reuse it for basic as well.
			// See void ConstructRequest(SOCKET_CONTEXT *)
			if ( auth_info != NULL &&
			   ( auth_info->auth_type == AUTH_TYPE_BASIC || auth_info->auth_type == AUTH_TYPE_DIGEST ) &&
				 auth_info->nc == 0 )
			{
				bool use_keep_alive_connection = false;

				// If we have a keep-alive connection and we've sent all of the data,
				// then we can reuse the connection and not have to flush any remnant data from the buffer.
				if ( context->header_info.connection == CONNECTION_KEEP_ALIVE )
				{
					header_buffer_length -= ( context->header_info.end_of_header - header_buffer );
					header_buffer = context->header_info.end_of_header;

					// Look for a chunk terminator.
					if ( context->header_info.chunked_transfer )
					{
						if ( ( header_buffer_length >= 5 ) && ( _memcmp( header_buffer + ( header_buffer_length - 5 ), "0\r\n\r\n", 5 ) != 0 ) )
						{
							use_keep_alive_connection = true;
						}
					}
					else if ( header_buffer_length >= context->header_info.range_info->content_length )	// See if the response data length is the same as the content length.
					{
						use_keep_alive_connection = true;
					}
				}

				context->header_info.connection = ( use_keep_alive_connection ? CONNECTION_KEEP_ALIVE : CONNECTION_CLOSE );

				return MakeRequest( context, IO_GetContent, false );
			}
			else	// We have all of the request and the first authorization attempt failed.
			{
				if ( context->header_info.http_status == 401 )
				{
					context->status = STATUS_AUTH_REQUIRED;
				}
				else// if ( context->header_info.http_status == 407 )
				{
					context->status = STATUS_PROXY_AUTH_REQUIRED;
				}

				return CONTENT_STATUS_FAILED;
			}
		}
		else
		{
			if ( context->header_info.http_status == 206 )
			{
				// If our range connections have been made. Start retrieving their content.
				if ( context->processed_header )
				{
					return CONTENT_STATUS_GET_CONTENT;
				}
				else	// The range connections have not been made. We've only requested the length (Range: 0-0) so far.
				{
					// Make sure we can split the download into enough parts.
					if ( context->header_info.range_info->content_length < context->parts )
					{
						context->parts = ( context->header_info.range_info->content_length > 0 ? ( unsigned char )context->header_info.range_info->content_length : 1 );
					}
				}
			}
			else	// Non-range request.
			{
				// If we indended to make a range request and the status is not 206.
				//if ( context->parts > 1 /*&& ( context->header_info.range_info->range_start > 0 || context->header_info.range_info->range_end > 0 )*/ )
				if ( context->parts > 1 && context->processed_header )
				{
					return CONTENT_STATUS_FAILED;
				}
				else
				{
					if ( context->download_info != NULL )
					{
						EnterCriticalSection( &context->download_info->shared_cs );

						context->download_info->processed_header = true;

						LeaveCriticalSection( &context->download_info->shared_cs );
					}

					context->parts = 1;	// If the content is not a range (206), then allow only one connection.

					context->processed_header = true;	// Assume we created our range request.
				}
			}

			if ( context->header_info.content_encoding != CONTENT_ENCODING_NONE )
			{
				context->parts = 1;

				context->header_info.range_info->content_length = 0;	// The true content length is indeterminate when compressed.

				context->header_info.connection = CONNECTION_CLOSE;
			}

			// Set out range info even if we use one part.
			if ( context->header_info.range_info->content_length > 0 && context->header_info.range_info->range_end == 0 )
			{
				context->header_info.range_info->range_start = 0;
				context->header_info.range_info->range_end = context->header_info.range_info->content_length - 1;
			}

			// Check the file size threshold (4GB).
			if ( context->header_info.range_info->content_length > MAX_FILE_SIZE )
			{
				context->show_file_size_prompt = true;
			}

			EnterCriticalSection( &context->download_info->shared_cs );

			context->download_info->parts = context->parts;

			context->download_info->file_size = context->header_info.range_info->content_length;

			if ( context->ssl == NULL )
			{
				context->download_info->ssl_version = -1;
			}

			LeaveCriticalSection( &context->download_info->shared_cs );
		}

		return content_status;
	}
}

// If we received a location URL, then we'll need to redirect to it.
int HandleRedirect( SOCKET_CONTEXT *context )
{
	int ret = CONTENT_STATUS_FAILED;

	if ( context != NULL )
	{
		// Allow up to 100 redirects before we stop. The servers should get their act together.
		//if ( context->request_info.redirect_count < cfg_max_redirects )
		//{
			SOCKET_CONTEXT *redirect_context = CreateSocketContext();

			redirect_context->processed_header = context->processed_header;

			redirect_context->part = context->part;
			redirect_context->parts = context->parts;

			//

			if ( context->header_info.url_location.host != NULL )	// Handle absolute URIs.
			{
				redirect_context->request_info.host = context->header_info.url_location.host;
				redirect_context->request_info.port = context->header_info.url_location.port;
				redirect_context->request_info.resource = context->header_info.url_location.resource;
				redirect_context->request_info.protocol = context->header_info.url_location.protocol;

				redirect_context->request_info.redirect_count = context->request_info.redirect_count + 1;
				
				context->header_info.url_location.host = NULL;
				context->header_info.url_location.resource = NULL;
			}
			else	// Handle relative URIs.
			{
				redirect_context->request_info = context->request_info;
				redirect_context->request_info.resource = context->header_info.url_location.resource;
				
				++redirect_context->request_info.redirect_count;

				context->request_info.host = NULL;	// We don't want to free the host, but the resource is ok to free.
				context->header_info.url_location.resource = NULL;
			}

			redirect_context->header_info.cookie_tree = context->header_info.cookie_tree;
			redirect_context->header_info.cookies = context->header_info.cookies;
			redirect_context->header_info.chunk_buffer = context->header_info.chunk_buffer;
			redirect_context->header_info.range_info = context->header_info.range_info;

			redirect_context->header_info.range_info->content_length = 0;	// We must reset this to get the real request length (not the length of the 401/407 request).

			if ( !redirect_context->processed_header )
			{
				redirect_context->header_info.range_info->range_start = 0;
				redirect_context->header_info.range_info->range_end = 0;
				redirect_context->header_info.range_info->content_offset = 0;
				redirect_context->header_info.range_info->file_write_offset = 0;
			}

			redirect_context->header_info.digest_info = context->header_info.digest_info;
			redirect_context->header_info.proxy_digest_info = context->header_info.proxy_digest_info;

			//

			context->header_info.cookie_tree = NULL;
			context->header_info.cookies = NULL;
			context->header_info.chunk_buffer = NULL;
			context->header_info.range_info = NULL;

			context->header_info.digest_info = NULL;
			context->header_info.proxy_digest_info = NULL;

			//

			redirect_context->range_node = context->range_node;

			redirect_context->context_node.data = redirect_context;

			EnterCriticalSection( &context_list_cs );

			DLL_AddNode( &g_context_list, &redirect_context->context_node, 0 );

			LeaveCriticalSection( &context_list_cs );

			if ( context->download_info != NULL )
			{
				EnterCriticalSection( &context->download_info->shared_cs );

				redirect_context->download_info = context->download_info;

				DLL_RemoveNode( &redirect_context->download_info->parts_list, &context->parts_node );

				redirect_context->parts_node.data = redirect_context;
				DLL_AddNode( &redirect_context->download_info->parts_list, &redirect_context->parts_node, -1 );

				LeaveCriticalSection( &context->download_info->shared_cs );

				context->download_info = NULL;
			}

			redirect_context->status = STATUS_CONNECTING;

			if ( !CreateConnection( redirect_context, redirect_context->request_info.host, redirect_context->request_info.port ) )
			{
				redirect_context->status = STATUS_FAILED;

				CleanupConnection( redirect_context );
			}
		//}

		InterlockedIncrement( &context->pending_operations );

		context->overlapped.current_operation = ( context->ssl != NULL ? IO_Shutdown : IO_Close );

		PostQueuedCompletionStatus( g_hIOCP, 0, ( ULONG_PTR )context, ( OVERLAPPED * )&context->overlapped );

		ret = CONTENT_STATUS_NONE;
	}

	return ret;
}

int MakeRangeRequest( SOCKET_CONTEXT *context )
{
	int ret = CONTENT_STATUS_FAILED;

	if ( context != NULL )
	{
		// Create a new connection for the remaining parts.
		if ( context->parts > 1 )
		{
			if ( context->download_info != NULL )
			{
				EnterCriticalSection( &context->download_info->shared_cs );

				context->download_info->processed_header = true;

				LeaveCriticalSection( &context->download_info->shared_cs );
			}

			unsigned long long range_size = context->header_info.range_info->content_length / context->parts;
			unsigned long long range_offset = range_size;

			context->header_info.range_info->range_start = context->header_info.range_info->content_offset;
			context->header_info.range_info->range_end = range_offset;
			context->header_info.range_info->content_offset = 0;

			for ( unsigned char part = 2; part <= context->parts; ++part )
			{
				// Save the request information, the header information (if we got any), and create a new connection.
				SOCKET_CONTEXT *new_context = CreateSocketContext();

				new_context->processed_header = true;

				new_context->part = part;
				new_context->parts = context->parts;

				new_context->got_filename = context->got_filename;	// No need to rename it again.
				new_context->show_file_size_prompt = context->show_file_size_prompt;	// No need to prompt again.

				new_context->request_info.host = GlobalStrDupA( context->request_info.host );
				new_context->request_info.port = context->request_info.port;
				new_context->request_info.resource = GlobalStrDupA( context->request_info.resource );
				new_context->request_info.protocol = context->request_info.protocol;

				new_context->header_info.cookie_tree = CopyCookieTree( context->header_info.cookie_tree );
				new_context->header_info.cookies = GlobalStrDupA( context->header_info.cookies );

				// We can copy the digest info so that we don't have to make any extra requests to 401 and 407 responses.
				if ( context->header_info.digest_info != NULL )
				{
					new_context->header_info.digest_info = ( AUTH_INFO * )GlobalAlloc( GPTR, sizeof( AUTH_INFO ) );

					new_context->header_info.digest_info->algorithm = context->header_info.digest_info->algorithm;
					new_context->header_info.digest_info->auth_type = context->header_info.digest_info->auth_type;
					new_context->header_info.digest_info->qop_type = context->header_info.digest_info->qop_type;

					new_context->header_info.digest_info->domain = GlobalStrDupA( context->header_info.digest_info->domain );
					new_context->header_info.digest_info->nonce = GlobalStrDupA( context->header_info.digest_info->nonce );
					new_context->header_info.digest_info->opaque = GlobalStrDupA( context->header_info.digest_info->opaque );
					new_context->header_info.digest_info->qop = GlobalStrDupA( context->header_info.digest_info->qop );
					new_context->header_info.digest_info->realm = GlobalStrDupA( context->header_info.digest_info->realm );
				}

				if ( context->header_info.proxy_digest_info != NULL )
				{
					new_context->header_info.proxy_digest_info = ( AUTH_INFO * )GlobalAlloc( GPTR, sizeof( AUTH_INFO ) );

					new_context->header_info.proxy_digest_info->algorithm = context->header_info.proxy_digest_info->algorithm;
					new_context->header_info.proxy_digest_info->auth_type = context->header_info.proxy_digest_info->auth_type;
					new_context->header_info.proxy_digest_info->qop_type = context->header_info.proxy_digest_info->qop_type;

					new_context->header_info.proxy_digest_info->domain = GlobalStrDupA( context->header_info.proxy_digest_info->domain );
					new_context->header_info.proxy_digest_info->nonce = GlobalStrDupA( context->header_info.proxy_digest_info->nonce );
					new_context->header_info.proxy_digest_info->opaque = GlobalStrDupA( context->header_info.proxy_digest_info->opaque );
					new_context->header_info.proxy_digest_info->qop = GlobalStrDupA( context->header_info.proxy_digest_info->qop );
					new_context->header_info.proxy_digest_info->realm = GlobalStrDupA( context->header_info.proxy_digest_info->realm );
				}

				RANGE_INFO *ri = ( RANGE_INFO * )GlobalAlloc( GPTR, sizeof( RANGE_INFO ) );

				new_context->header_info.range_info = ri;

				new_context->header_info.range_info->range_start = range_offset + 1;

				if ( new_context->part < context->parts )
				{
					range_offset += range_size;
					new_context->header_info.range_info->range_end = range_offset;
				}
				else	// Make sure we have an accurate range end for the last part.
				{
					new_context->header_info.range_info->range_end = context->header_info.range_info->content_length - 1;
				}

				new_context->header_info.range_info->file_write_offset = new_context->header_info.range_info->range_start;

				//

				new_context->context_node.data = new_context;

				EnterCriticalSection( &context_list_cs );
				
				DLL_AddNode( &g_context_list, &new_context->context_node, 0 );

				LeaveCriticalSection( &context_list_cs );

				// Add to the parts list.
				if ( context->download_info != NULL )
				{
					EnterCriticalSection( &context->download_info->shared_cs );

					new_context->download_info = context->download_info;

					++( new_context->download_info->active_parts );

					DoublyLinkedList *range_node = DLL_CreateNode( ( void * )ri );
					new_context->range_node = range_node;
					DLL_AddNode( &new_context->download_info->range_list, new_context->range_node, -1 );

					new_context->parts_node.data = new_context;
					DLL_AddNode( &new_context->download_info->parts_list, &new_context->parts_node, -1 );

					LeaveCriticalSection( &context->download_info->shared_cs );
				}

				new_context->status = STATUS_CONNECTING;

				if ( !CreateConnection( new_context, new_context->request_info.host, new_context->request_info.port ) )
				{
					new_context->status = STATUS_FAILED;

					CleanupConnection( new_context );
				}
			}
		}

		context->processed_header = true;

		//ret = CONTENT_STATUS_NONE;

		ret = MakeRequest( context, IO_GetContent, false );
	}

	return ret;
}

int MakeRequest( SOCKET_CONTEXT *context, IO_OPERATION next_operation, bool use_connect )
{
	int ret = CONTENT_STATUS_FAILED;

	if ( context != NULL )
	{
		if ( context->header_info.connection == CONNECTION_KEEP_ALIVE )
		{
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

			if ( !context->processed_header )
			{
				context->header_info.range_info->range_start = 0;
				context->header_info.range_info->range_end = 0;
				context->header_info.range_info->content_offset = 0;
				context->header_info.range_info->file_write_offset = 0;
			}

			//

			context->content_status = CONTENT_STATUS_NONE;

			InterlockedIncrement( &context->pending_operations );

			context->wsabuf.buf = context->buffer;
			context->wsabuf.len = BUFFER_SIZE;

			context->overlapped.next_operation = next_operation;

			ConstructRequest( context, use_connect );

			bool sent = false;
			int nRet = 0;
			DWORD dwFlags = 0;

			if ( context->ssl != NULL )
			{
				SSL_WSASend( context, &context->overlapped, &context->wsabuf, sent );
				if ( !sent )
				{
					context->overlapped.current_operation = IO_Shutdown;

					PostQueuedCompletionStatus( g_hIOCP, 0, ( ULONG_PTR )context, ( OVERLAPPED * )&context->overlapped );
				}
			}
			else
			{
				context->overlapped.current_operation = IO_Write;

				nRet = _WSASend( context->socket, &context->wsabuf, 1, NULL, dwFlags, ( OVERLAPPED * )&context->overlapped, NULL );
				if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
				{
					context->overlapped.current_operation = IO_Close;

					PostQueuedCompletionStatus( g_hIOCP, 0, ( ULONG_PTR )context, ( OVERLAPPED * )&context->overlapped );
				}
			}
		}
		else
		{
			SOCKET_CONTEXT *new_context = CreateSocketContext();

			new_context->processed_header = context->processed_header;

			new_context->part = context->part;
			new_context->parts = context->parts;

			new_context->got_filename = context->got_filename;	// No need to rename it again.
			new_context->show_file_size_prompt = context->show_file_size_prompt;	// No need to prompt again.

			//

			new_context->request_info.host = context->request_info.host;
			new_context->request_info.port = context->request_info.port;
			new_context->request_info.resource = context->request_info.resource;
			new_context->request_info.protocol = context->request_info.protocol;

			new_context->header_info.cookie_tree = context->header_info.cookie_tree;
			new_context->header_info.cookies = context->header_info.cookies;
			new_context->header_info.chunk_buffer = context->header_info.chunk_buffer;
			new_context->header_info.range_info = context->header_info.range_info;

			new_context->header_info.range_info->content_length = 0;	// We must reset this to get the real request length (not the length of the 401/407 request).

			if ( !new_context->processed_header )
			{
				new_context->header_info.range_info->range_start = 0;
				new_context->header_info.range_info->range_end = 0;
				new_context->header_info.range_info->content_offset = 0;
				new_context->header_info.range_info->file_write_offset = 0;
			}

			new_context->header_info.digest_info = context->header_info.digest_info;
			new_context->header_info.proxy_digest_info = context->header_info.proxy_digest_info;

			//

			context->request_info.host = NULL;
			context->request_info.resource = NULL;

			context->header_info.cookie_tree = NULL;
			context->header_info.cookies = NULL;
			context->header_info.chunk_buffer = NULL;
			context->header_info.range_info = NULL;

			context->header_info.digest_info = NULL;
			context->header_info.proxy_digest_info = NULL;

			//

			new_context->range_node = context->range_node;

			new_context->context_node.data = new_context;

			EnterCriticalSection( &context_list_cs );

			DLL_AddNode( &g_context_list, &new_context->context_node, 0 );

			LeaveCriticalSection( &context_list_cs );

			// Add to the parts list.
			if ( context->download_info != NULL )
			{
				EnterCriticalSection( &context->download_info->shared_cs );

				new_context->download_info = context->download_info;

				DLL_RemoveNode( &new_context->download_info->parts_list, &context->parts_node );

				new_context->parts_node.data = new_context;
				DLL_AddNode( &new_context->download_info->parts_list, &new_context->parts_node, -1 );

				LeaveCriticalSection( &context->download_info->shared_cs );

				context->download_info = NULL;
			}

			new_context->status = STATUS_CONNECTING;

			if ( !CreateConnection( new_context, new_context->request_info.host, new_context->request_info.port ) )
			{
				new_context->status = STATUS_FAILED;

				CleanupConnection( new_context );
			}

			InterlockedIncrement( &context->pending_operations );

			context->overlapped.current_operation = ( context->ssl != NULL ? IO_Shutdown : IO_Close );

			PostQueuedCompletionStatus( g_hIOCP, 0, ( ULONG_PTR )context, ( OVERLAPPED * )&context->overlapped );
		}

		ret = CONTENT_STATUS_NONE;
	}

	return ret;
}

int MakeResponse( SOCKET_CONTEXT *context )
{
	int ret = CONTENT_STATUS_FAILED;

	if ( context != NULL )
	{
		InterlockedIncrement( &context->pending_operations );

		context->wsabuf.buf = context->buffer;

		bool use_keep_alive = false;

		if ( cfg_use_authentication && ( context->header_info.digest_info == NULL ||
										 context->header_info.digest_info != NULL && context->header_info.digest_info->nc > 0 ) )
		{
			if ( context->header_info.connection == CONNECTION_KEEP_ALIVE )
			{
				context->content_status = CONTENT_STATUS_READ_MORE_HEADER;

				use_keep_alive = true;
			}

			if ( cfg_authentication_type == AUTH_TYPE_DIGEST )
			{
				context->wsabuf.len = __snprintf( context->wsabuf.buf, BUFFER_SIZE,
					"HTTP/1.1 401 Unauthorized\r\n" \
					"Content-Type: text/html\r\n" \
					"WWW-Authenticate: Digest " \
									"realm=\"Authorization Required\", " \
									"algorithm=MD5-sess, " \
									"qop=\"auth\", " \
									"nonce=\"%s\", " \
									"opaque=\"%s\"\r\n" \
					"Content-Length: 110\r\n" \
					"Connection: %s\r\n\r\n" \
					"<!DOCTYPE html><html><head><title>401 Unauthorized</title></head><body><h1>401 Unauthorized</h1></body></html>", g_nonce, g_opaque, ( use_keep_alive ? "keep-alive" : "close" ) );
			}
			else
			{
				context->wsabuf.len = __snprintf( context->wsabuf.buf, BUFFER_SIZE,
					"HTTP/1.1 401 Unauthorized\r\n" \
					"Content-Type: text/html\r\n" \
					"WWW-Authenticate: Basic realm=\"Authorization Required\"\r\n" \
					"Content-Length: 110\r\n" \
					"Connection: %s\r\n\r\n" \
					"<!DOCTYPE html><html><head><title>401 Unauthorized</title></head><body><h1>401 Unauthorized</h1></body></html>", ( use_keep_alive ? "keep-alive" : "close" ) );
			}
		}
		else if ( context->header_info.http_method == METHOD_GET ||
				  context->header_info.http_method == METHOD_HEAD ||
				  context->header_info.http_method == METHOD_POST )
		{
			if ( context->header_info.http_status == 200 )
			{
				context->header_info.http_status = 0;	// Reset.

				context->wsabuf.len = __snprintf( context->wsabuf.buf, BUFFER_SIZE,
					"HTTP/1.1 200 OK\r\n" \
					"Content-Type: text/plain\r\n" \
					"Content-Length: 11\r\n" \
					"Connection: close\r\n\r\n" \
					"DOWNLOADING" );
			}
			else
			{
				context->wsabuf.len = __snprintf( context->wsabuf.buf, BUFFER_SIZE,
					"HTTP/1.1 204 No Content\r\n" \
					"Connection: close\r\n\r\n" );
			}
		}
		else
		{
			context->wsabuf.len = __snprintf( context->wsabuf.buf, BUFFER_SIZE,
				"HTTP/1.1 501 Not Implemented\r\n" \
				"Content-Type: text/html\r\n" \
				"Content-Length: 116\r\n" \
				"Connection: close\r\n\r\n" \
				"<!DOCTYPE html><html><head><title>501 Not Implemented</title></head><body><h1>501 Not Implemented</h1></body></html>" );
		}

		bool sent = false;
		int nRet = 0;
		DWORD dwFlags = 0;

		if ( context->ssl != NULL )
		{
			if ( use_keep_alive )
			{
				context->overlapped.next_operation = IO_GetRequest;
			}
			else
			{
				context->overlapped.next_operation = IO_Shutdown;
			}

			SSL_WSASend( context, &context->overlapped, &context->wsabuf, sent );
			if ( !sent )
			{
				context->overlapped.current_operation = IO_Shutdown;

				PostQueuedCompletionStatus( g_hIOCP, 0, ( ULONG_PTR )context, ( OVERLAPPED * )&context->overlapped );
			}
		}
		else
		{
			context->overlapped.current_operation = IO_Write;

			if ( use_keep_alive )
			{
				context->overlapped.next_operation = IO_GetRequest;
			}
			else
			{
				context->overlapped.next_operation = IO_Close;
			}

			// We do a regular WSASend here since the connection was not encrypted.
			nRet = _WSASend( context->socket, &context->wsabuf, 1, NULL, dwFlags, ( OVERLAPPED * )&context->overlapped, NULL );
			if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
			{
				context->overlapped.current_operation = IO_Close;

				PostQueuedCompletionStatus( g_hIOCP, 0, ( ULONG_PTR )context, ( OVERLAPPED * )&context->overlapped );
			}
		}

		ret = CONTENT_STATUS_NONE;
	}

	return ret;
}

int AllocateFile( SOCKET_CONTEXT *context )
{
	if ( context == NULL )
	{
		return CONTENT_STATUS_FAILED;
	}

	unsigned char file_status = 0;	// 0 = failed, 1 = allocate, 2 = already allocated

	if ( context->download_info != NULL )
	{
		// If not, then block other threads from creating it.
		EnterCriticalSection( &context->download_info->shared_cs );

		context->is_allocated = true;

		// See if we've created a file.
		if ( context->download_info->hFile == INVALID_HANDLE_VALUE )
		{
			// Check again to see if we've created a file (for threads entering this critical section).
			if ( context->download_info->hFile == INVALID_HANDLE_VALUE && context->download_info->status != STATUS_FILE_IO_ERROR )
			{
				wchar_t file_path[ MAX_PATH ];
				_wmemcpy_s( file_path, MAX_PATH, context->download_info->file_path, MAX_PATH );
				if ( context->download_info->filename_offset > 0 )
				{
					file_path[ context->download_info->filename_offset - 1 ] = L'\\';	// Replace the download directory NULL terminator with a directory slash.
				}

				// If the file already exists and has been partially downloaded, then open it to resume downloading.
				if ( GetFileAttributes( file_path ) != INVALID_FILE_ATTRIBUTES && context->download_info->downloaded > 0 )
				{
					// If the file has downloaded data (we're resuming), then open it, otherwise truncate its size to 0.
					context->download_info->hFile = CreateFile( file_path, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL );

					if ( context->download_info->hFile != INVALID_HANDLE_VALUE )
					{
						g_hIOCP = CreateIoCompletionPort( context->download_info->hFile, g_hIOCP, 0, 0 );
						if ( g_hIOCP != NULL )
						{
//							context->overlapped.context = context;

							file_status = 2;
						}
						else
						{
							CloseHandle( context->download_info->hFile );
							context->download_info->hFile = INVALID_HANDLE_VALUE;
						}
					}
				}
				else	// Pre-allocate our file on the disk if it does not exist, or if we're overwriting one that already exists.
				{
					context->download_info->hFile = CreateFile( file_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL );

					if ( context->download_info->hFile != INVALID_HANDLE_VALUE )
					{
						g_hIOCP = CreateIoCompletionPort( context->download_info->hFile, g_hIOCP, 0, 0 );
						if ( g_hIOCP != NULL )
						{
//							context->overlapped.context = context;

							LARGE_INTEGER li;
							li.QuadPart = context->header_info.range_info->content_length;

							SetFilePointerEx( context->download_info->hFile, li, NULL, FILE_BEGIN );
							SetEndOfFile( context->download_info->hFile );

							if ( can_fast_allocate )	// Fast disk allocation if we're an administrator.
							{
								if ( SetFileValidData( context->download_info->hFile, li.QuadPart ) == FALSE )
								{
									file_status = 0;

									CloseHandle( context->download_info->hFile );
									context->download_info->hFile = INVALID_HANDLE_VALUE;
								}
								else
								{
									file_status = 2;	// Start writing to the file immediately.
								}
							}
							else	// Trigger the system to allocate the file on disk. Sloooow.
							{
								file_status = 1;

								context->download_info->status = STATUS_ALLOCATING_FILE;
								context->status = STATUS_ALLOCATING_FILE;

								InterlockedIncrement( &context->pending_operations );

								context->overlapped.current_operation = IO_ResumeGetContent;

								// Adjust the offset back 1.
								if ( li.QuadPart > 0 )
								{
									--li.QuadPart;
								}
								context->overlapped.overlapped.hEvent = NULL;
								context->overlapped.overlapped.Internal = NULL;
								context->overlapped.overlapped.InternalHigh = NULL;
								//context->overlapped.overlapped.Pointer = NULL; // union
								context->overlapped.overlapped.Offset = li.LowPart;
								context->overlapped.overlapped.OffsetHigh = li.HighPart;

								// Write a non-NULL character to the end of the file to zero it out.
								BOOL bRet = WriteFile( context->download_info->hFile, "\x03", 1, NULL, ( OVERLAPPED * )&context->overlapped );
								if ( bRet == FALSE && ( GetLastError() != ERROR_IO_PENDING ) )
								{
									InterlockedDecrement( &context->pending_operations );

									file_status = 0;

									CloseHandle( context->download_info->hFile );
									context->download_info->hFile = INVALID_HANDLE_VALUE;
								}
							}
						}
						else
						{
							CloseHandle( context->download_info->hFile );
							context->download_info->hFile = INVALID_HANDLE_VALUE;
						}
					}
				}

				if ( file_status == 0 )
				{
					context->download_info->status = STATUS_FILE_IO_ERROR;
					context->status = STATUS_FILE_IO_ERROR;
				}
			}
		}
		else
		{
			file_status = 2;
		}

		LeaveCriticalSection( &context->download_info->shared_cs );

		if ( file_status == 0 )
		{
			context->is_allocated = false;

			return CONTENT_STATUS_FAILED;
		}
		else if ( file_status == 1 )
		{
			return CONTENT_STATUS_ALLOCATE_FILE;	// Allocate the file asynchronously.
		}
	}

	return CONTENT_STATUS_NONE;
}

int GetHTTPResponseContent( SOCKET_CONTEXT *context, char *response_buffer, unsigned int response_buffer_length )
{
	if ( context == NULL )
	{
		return CONTENT_STATUS_FAILED;
	}

	if ( context->content_status != CONTENT_STATUS_GET_CONTENT )
	{
		int content_status = CONTENT_STATUS_GET_CONTENT;	// Assume we're now getting the content.

		// If the return status is CONTENT_STATUS_GET_CONTENT, then our end_of_header pointer will have been set.
		if ( context->content_status == CONTENT_STATUS_NONE || context->content_status == CONTENT_STATUS_READ_MORE_HEADER )
		{
			content_status = context->content_status = GetHTTPHeader( context, response_buffer, response_buffer_length );
		}

		// For anything else, we'll need to return immediately.
		if ( content_status != CONTENT_STATUS_GET_CONTENT )
		{
			return content_status;
		}

		// Once we have the file size, allocate our file.
		if ( context->download_info != NULL && !( context->download_info->download_operations & DOWNLOAD_OPERATION_SIMULATE ) )
		{
			// Ignore the prompt of we selected Yes to all.
			if ( g_rename_file_cmb_ret != CMBIDYESALL )
			{
				if ( context->got_filename == 2 )
				{
					context->got_filename = 1;

					// If we select No to all, or it's a remotely initiated download, then close the connection.
					if ( g_rename_file_cmb_ret == CMBIDNOALL || context->download_info->download_operations & DOWNLOAD_OPERATION_OVERRIDE_PROMPTS )
					{
						context->status = STATUS_SKIPPED;

						return CONTENT_STATUS_FAILED;	// Stop downloading the file.
					}
					else	// Otherwise, ask for a prompt.
					{
						context->content_status = CONTENT_STATUS_RENAME_FILE_PROMPT;

						// Add item to prompt queue and continue;
						EnterCriticalSection( &rename_file_prompt_list_cs );

						DoublyLinkedList *di_node = DLL_CreateNode( ( void * )context->download_info );
						DLL_AddNode( &rename_file_prompt_list, di_node, -1 );

						if ( !rename_file_prompt_active )
						{
							rename_file_prompt_active = true;

							HANDLE handle_prompt = ( HANDLE )_CreateThread( NULL, 0, RenameFilePrompt, NULL, 0, NULL );

							// Make sure our thread spawned.
							if ( handle_prompt == NULL )
							{
								DLL_RemoveNode( &rename_file_prompt_list, di_node );
								GlobalFree( di_node );

								rename_file_prompt_active = false;

								LeaveCriticalSection( &rename_file_prompt_list_cs );

								context->status = STATUS_FILE_IO_ERROR;

								return CONTENT_STATUS_FAILED;	// Stop downloading the file.
							}
							else
							{
								CloseHandle( handle_prompt );
							}
						}

						LeaveCriticalSection( &rename_file_prompt_list_cs );

						return CONTENT_STATUS_RENAME_FILE_PROMPT;
					}
				}
			}

			// Ignore the prompt of we selected Yes to all.
			if ( g_file_size_cmb_ret != CMBIDYESALL )
			{
				if ( context->show_file_size_prompt )
				{
					context->show_file_size_prompt = false;	// Ensure it stays false.

					// If we select No to all, or it's a remotely initiated download, then close the connection.
					if ( g_file_size_cmb_ret == CMBIDNOALL || context->download_info->download_operations & DOWNLOAD_OPERATION_OVERRIDE_PROMPTS )
					{
						context->status = STATUS_SKIPPED;

						return CONTENT_STATUS_FAILED;	// Stop downloading the file.
					}
					else	// Otherwise, ask for a prompt.
					{
						context->content_status = CONTENT_STATUS_FILE_SIZE_PROMPT;

						// Add item to prompt queue and continue;
						EnterCriticalSection( &file_size_prompt_list_cs );

						DoublyLinkedList *di_node = DLL_CreateNode( ( void * )context->download_info );
						DLL_AddNode( &file_size_prompt_list, di_node, -1 );

						if ( !file_size_prompt_active )
						{
							file_size_prompt_active = true;

							HANDLE handle_prompt = ( HANDLE )_CreateThread( NULL, 0, FileSizePrompt, NULL, 0, NULL );

							// Make sure our thread spawned.
							if ( handle_prompt == NULL )
							{
								DLL_RemoveNode( &file_size_prompt_list, di_node );
								GlobalFree( di_node );

								file_size_prompt_active = false;

								LeaveCriticalSection( &file_size_prompt_list_cs );

								context->status = STATUS_FILE_IO_ERROR;

								return CONTENT_STATUS_FAILED;	// Stop downloading the file.
							}
							else
							{
								CloseHandle( handle_prompt );
							}
						}

						LeaveCriticalSection( &file_size_prompt_list_cs );

						return CONTENT_STATUS_FILE_SIZE_PROMPT;
					}
				}
			}

			if ( !context->is_allocated )
			{
				// Returns either CONTENT_STATUS_FAILED, CONTENT_STATUS_ALLOCATE_FILE, or CONTENT_STATUS_NONE.
				content_status = context->content_status = AllocateFile( context );

				if ( content_status != CONTENT_STATUS_NONE )
				{
					return content_status;
				}
			}
			else
			{
				EnterCriticalSection( &context->download_info->shared_cs );

				context->download_info->status = STATUS_DOWNLOADING;
				context->status = STATUS_DOWNLOADING;

				LeaveCriticalSection( &context->download_info->shared_cs );
			}
		}

		if ( context->header_info.end_of_header != NULL )
		{
			// Offset our buffer to the content and adjust the new length.
			response_buffer_length -= ( context->header_info.end_of_header - response_buffer );
			response_buffer = context->header_info.end_of_header;

			context->content_status = CONTENT_STATUS_GET_CONTENT;
		}
		else
		{
			return CONTENT_STATUS_FAILED;
		}
	}

	if ( response_buffer_length == 0 )
	{
		return CONTENT_STATUS_READ_MORE_CONTENT;	// Need more content data.
	}

	// Now we need to decode the buffer in case it was a chunked transfer. Boo!!!
	if ( context->header_info.chunked_transfer )
	{
		if ( context->download_info != NULL && !( context->download_info->download_operations & DOWNLOAD_OPERATION_SIMULATE ) )
		{
			if ( context->header_info.chunk_buffer == NULL )
			{
				context->header_info.chunk_buffer = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * BUFFER_SIZE );
			}
		}

		context->write_wsabuf.buf = context->header_info.chunk_buffer;
		context->write_wsabuf.len = 0;

		int status = CONTENT_STATUS_READ_MORE_CONTENT;

		// Offset past the chunk terminating token.
		if ( response_buffer_length >= 2 && response_buffer[ 0 ] == '\r' && response_buffer[ 1 ] == '\n' )
		{
			response_buffer_length -= 2;
			response_buffer += 2;

			context->header_info.got_chunk_start = false;
		}

		while ( response_buffer_length > 0 )
		{
			if ( !context->header_info.got_chunk_start )
			{
				// End of chunk value.
				char *end_chunk = _StrStrA( response_buffer, "\r\n" );
				if ( end_chunk == NULL )
				{
					if ( response_buffer > context->buffer )
					{
						_memmove( context->buffer, response_buffer, response_buffer_length );

						context->wsabuf.buf += response_buffer_length;
						context->wsabuf.len -= response_buffer_length;

						status = CONTENT_STATUS_READ_MORE_CONTENT;	// Read more content data.
						break;
					}
					else	// Buffer is too small.
					{
						status = CONTENT_STATUS_FAILED;
						break;
					}
				}

				// Get the chunk value (including any extensions).
				char chunk[ 32 ];
				_memzero( chunk, 32 );
				_memcpy_s( chunk, 32, response_buffer, ( ( end_chunk - response_buffer ) > 32 ? 32 : ( end_chunk - response_buffer ) ) );
				chunk[ 31 ] = 0; // Sanity

				// Find any chunked extension and ignore it.
				char *find_extension = _StrChrA( chunk, ';' );
				if ( find_extension != NULL )
				{
					*find_extension = NULL;
				}

				end_chunk += 2;

				// Convert the hexidecimal chunk value into an integer. Stupid...
				context->header_info.chunk_length = strtoull( chunk, true );

				if ( context->header_info.chunk_length == 0 )
				{
					// Make sure the chunk terminator looks like "0\r\n\r\n" before we exit. We need to flush all the data if it's not complete.
					if ( ( response_buffer_length - ( end_chunk - response_buffer ) ) < 2 )
					{
						_memmove( context->buffer, response_buffer, response_buffer_length );

						context->wsabuf.buf = context->buffer + response_buffer_length;
						context->wsabuf.len = BUFFER_SIZE - response_buffer_length;

						status = CONTENT_STATUS_READ_MORE_CONTENT;	// Read more content data.
						break;
					}
					else
					{
						context->header_info.got_chunk_terminator = true;

						status = ( !context->processed_header ? CONTENT_STATUS_HANDLE_RESPONSE : CONTENT_STATUS_READ_MORE_CONTENT );	// We're done reading the chunked transfer stream.
						break;
					}
				}

				response_buffer_length -= ( end_chunk - response_buffer );
				response_buffer = end_chunk;

				context->header_info.got_chunk_start = true;
			}
			
			if ( context->header_info.chunk_length > response_buffer_length )	// The chunk is too large, dump our response buffer.
			{
				char *output_buffer = response_buffer;
				unsigned int output_buffer_length = response_buffer_length;

				if ( zlib1_state == ZLIB1_STATE_RUNNING )
				{
					if ( context->header_info.content_encoding == CONTENT_ENCODING_GZIP || context->header_info.content_encoding == CONTENT_ENCODING_DEFLATE )
					{
						unsigned int total_data_length = DecompressStream( context, output_buffer, output_buffer_length );

						if ( context->decompressed_buf != NULL )
						{
							output_buffer = context->decompressed_buf;
							output_buffer_length = total_data_length;
						}
					}
				}

				context->content_offset += response_buffer_length;	// The remainder length of our response. The true amount that was downloaded. Allows us to resume if we stop the download.
				//context->header_info.range_info->content_offset += response_buffer_length;	// The remainder length of our response. The true amount that was downloaded. Allows us to resume if we stop the download.
				context->header_info.chunk_length -= response_buffer_length;				// Exclude the remainder length from the content length.

				if ( context->download_info != NULL )
				{
					if ( !( context->download_info->download_operations & DOWNLOAD_OPERATION_SIMULATE ) )
					{
						_memcpy_s( context->write_wsabuf.buf + context->write_wsabuf.len, BUFFER_SIZE - context->write_wsabuf.len, output_buffer, output_buffer_length );
					}

					context->write_wsabuf.len += output_buffer_length;
				}

				status = CONTENT_STATUS_READ_MORE_CONTENT;
				break;
			}
			else	// We have a complete chunk(s) in our response buffer.
			{
				char *output_buffer = response_buffer;
				unsigned int output_buffer_length = ( unsigned int )context->header_info.chunk_length;

				if ( zlib1_state == ZLIB1_STATE_RUNNING )
				{
					if ( context->header_info.content_encoding == CONTENT_ENCODING_GZIP || context->header_info.content_encoding == CONTENT_ENCODING_DEFLATE )
					{
						unsigned int total_data_length = DecompressStream( context, output_buffer, output_buffer_length );

						if ( context->decompressed_buf != NULL )
						{
							output_buffer = context->decompressed_buf;
							output_buffer_length = total_data_length;
						}
					}
				}

				context->content_offset += context->header_info.chunk_length;	// The true amount that was downloaded. Allows us to resume if we stop the download.
				//context->header_info.range_info->content_offset += context->header_info.chunk_length;	// The true amount that was downloaded. Allows us to resume if we stop the download.

				response_buffer_length -= ( unsigned int )context->header_info.chunk_length;
				response_buffer += ( unsigned int )context->header_info.chunk_length;

				context->header_info.chunk_length = 0;	// Reset the length of the new chunk.

				if ( context->download_info != NULL )
				{
					if ( !( context->download_info->download_operations & DOWNLOAD_OPERATION_SIMULATE ) )
					{
						_memcpy_s( context->write_wsabuf.buf + context->write_wsabuf.len, BUFFER_SIZE - context->write_wsabuf.len, output_buffer, output_buffer_length );
					}

					context->write_wsabuf.len += output_buffer_length;
				}

				// See if we have more chunks.
				if ( response_buffer_length >=2 )
				{
					// Move to the next chunk.
					if ( response_buffer[ 0 ] == '\r' && response_buffer[ 1 ] == '\n' )
					{
						response_buffer_length -= 2;
						response_buffer += 2;

						context->header_info.got_chunk_start = false;	// Get the new chunk length.

						status = CONTENT_STATUS_READ_MORE_CONTENT;
					}
					else	// Bad terminator. Can't continue.
					{
						status = CONTENT_STATUS_FAILED;
						break;
					}
				}
				else	// Incomplete chunk terminator. Request more data to complete the chunk.
				{
					if ( response_buffer_length >= 1 )
					{
						_memmove( context->buffer, response_buffer, response_buffer_length );

						context->wsabuf.buf += response_buffer_length;
						context->wsabuf.len -= response_buffer_length;
					}

					status = CONTENT_STATUS_READ_MORE_CONTENT;
					break;
				}
			}
		}

		// The response buffer length is 0. We need more content.

		if ( context->write_wsabuf.len > 0 )
		{
			if ( context->download_info != NULL )
			{
				if ( !( context->download_info->download_operations & DOWNLOAD_OPERATION_SIMULATE ) )
				{
					unsigned char file_status = 0;	// 0 = failed, 1 = allocate, 2 = already allocated

					if ( context->download_info->hFile != INVALID_HANDLE_VALUE )
					{
						LARGE_INTEGER li;
						li.QuadPart = context->header_info.range_info->file_write_offset;//context->header_info.range_info->range_start + context->header_info.range_info->write_length;

						EnterCriticalSection( &context->download_info->shared_cs );

						InterlockedIncrement( &context->pending_operations );

						context->overlapped.current_operation = IO_WriteFile;

						context->overlapped.overlapped.hEvent = NULL;
						context->overlapped.overlapped.Internal = NULL;
						context->overlapped.overlapped.InternalHigh = NULL;
						//context->overlapped.Pointer = NULL;	// union
						context->overlapped.overlapped.Offset = li.LowPart;
						context->overlapped.overlapped.OffsetHigh = li.HighPart;

//						context->overlapped.context = context;

						file_status = 1;

						context->content_status = status;	// Causes IO_WriteFile to call HandleResponse().

						//context->header_info.range_info->file_write_offset += context->write_wsabuf.len;	// The size of the non-encoded/decoded data that we're writing to the file.

						BOOL bRet = WriteFile( context->download_info->hFile, context->write_wsabuf.buf, context->write_wsabuf.len, NULL, ( OVERLAPPED * )&context->overlapped );
						if ( bRet == FALSE && ( GetLastError() != ERROR_IO_PENDING ) )
						{
							InterlockedDecrement( &context->pending_operations );

							context->download_info->status = STATUS_FILE_IO_ERROR;
							context->status = STATUS_FILE_IO_ERROR;

							file_status = 0;

							context->content_status = CONTENT_STATUS_FAILED;

							context->content_offset = 0;
							//context->header_info.range_info->file_write_offset -= context->write_wsabuf.len;	// The size of the non-encoded/decoded data that we're writing to the file.

							CloseHandle( context->download_info->hFile );
							context->download_info->hFile = INVALID_HANDLE_VALUE;
						}

						LeaveCriticalSection( &context->download_info->shared_cs );

						if ( file_status == 0 )
						{
							return CONTENT_STATUS_FAILED;
						}
						else
						{
							return CONTENT_STATUS_NONE;	// Exits IO_GetContent.
						}
					}
					else	// Shouldn't happen.
					{
						return CONTENT_STATUS_FAILED;
					}
				}
				else	// Simulated download.
				{
					EnterCriticalSection( &context->download_info->shared_cs );
					context->download_info->downloaded += context->write_wsabuf.len;			// The total amount of data (decoded) that was saved/simulated.
					LeaveCriticalSection( &context->download_info->shared_cs );

					EnterCriticalSection( &session_totals_cs );
					session_total_downloaded += context->write_wsabuf.len;
					LeaveCriticalSection( &session_totals_cs );

					context->header_info.range_info->content_offset += context->content_offset;	// The true amount that was downloaded. Allows us to resume if we stop the download.
					context->content_offset = 0;
				}
			}
			else	// Shouldn't happen.
			{
				return CONTENT_STATUS_FAILED;
			}
		}

		if ( ( context->parts == 1 && context->header_info.connection == CONNECTION_KEEP_ALIVE && context->header_info.got_chunk_terminator ) ||
			 ( context->parts > 1 && ( context->header_info.range_info->content_offset >= ( ( context->header_info.range_info->range_end - context->header_info.range_info->range_start ) + 1 ) ) ) )
		{
			return CONTENT_STATUS_FAILED;	// We have no more data, so just close the connection.
		}
		else
		{
			return status;
		}
	}
	else	// Non-chunked transfer
	{
		unsigned char status = 0;

		char *output_buffer = response_buffer;
		unsigned int output_buffer_length = response_buffer_length;

		// Write buffer to file.

		if ( context->download_info != NULL )
		{
			if ( zlib1_state == ZLIB1_STATE_RUNNING )
			{
				if ( context->header_info.content_encoding == CONTENT_ENCODING_GZIP || context->header_info.content_encoding == CONTENT_ENCODING_DEFLATE )
				{
					unsigned int total_data_length = DecompressStream( context, output_buffer, output_buffer_length );

					if ( context->decompressed_buf != NULL )
					{
						output_buffer = context->decompressed_buf;
						output_buffer_length = total_data_length;
					}
				}
			}

			if ( !( context->download_info->download_operations & DOWNLOAD_OPERATION_SIMULATE ) )
			{
				if ( context->download_info->hFile != INVALID_HANDLE_VALUE )
				{
					LARGE_INTEGER li;
					li.QuadPart = context->header_info.range_info->file_write_offset;//context->header_info.range_info->range_start + context->header_info.range_info->content_offset;

					EnterCriticalSection( &context->download_info->shared_cs );

					InterlockedIncrement( &context->pending_operations );

					context->overlapped.current_operation = IO_WriteFile;

					context->write_wsabuf.buf = output_buffer;
					context->write_wsabuf.len = output_buffer_length;

					context->overlapped.overlapped.hEvent = NULL;
					context->overlapped.overlapped.Internal = NULL;
					context->overlapped.overlapped.InternalHigh = NULL;
					//context->overlapped.Pointer = NULL;	// union
					context->overlapped.overlapped.Offset = li.LowPart;
					context->overlapped.overlapped.OffsetHigh = li.HighPart;

//					context->overlapped.context = context;

					status = 1;

					context->content_status = ( !context->processed_header ? CONTENT_STATUS_HANDLE_RESPONSE : CONTENT_STATUS_READ_MORE_CONTENT );

					context->content_offset = response_buffer_length;	// The true amount that was downloaded. Allows us to resume if we stop the download.
					//context->header_info.range_info->content_offset += response_buffer_length;	// The true amount that was downloaded. Allows us to resume if we stop the download.
					//context->header_info.range_info->file_write_offset += output_buffer_length;	// The size of the non-encoded/decoded data that we're writing to the file.

					BOOL bRet = WriteFile( context->download_info->hFile, context->write_wsabuf.buf, context->write_wsabuf.len, NULL, ( OVERLAPPED * )&context->overlapped );
					if ( bRet == FALSE && ( GetLastError() != ERROR_IO_PENDING ) )
					{
						InterlockedDecrement( &context->pending_operations );

						context->download_info->status = STATUS_FILE_IO_ERROR;
						context->status = STATUS_FILE_IO_ERROR;

						status = 0;

						context->content_status = CONTENT_STATUS_FAILED;

						context->content_offset = 0;	// The true amount that was downloaded. Allows us to resume if we stop the download.
						//context->header_info.range_info->content_offset -= response_buffer_length;	// The true amount that was downloaded. Allows us to resume if we stop the download.
						//context->header_info.range_info->file_write_offset -= output_buffer_length;	// The size of the non-encoded/decoded data that we're writing to the file.

						CloseHandle( context->download_info->hFile );
						context->download_info->hFile = INVALID_HANDLE_VALUE;
					}

					LeaveCriticalSection( &context->download_info->shared_cs );

					if ( status == 0 )
					{
						return CONTENT_STATUS_FAILED;
					}
					else
					{
						return CONTENT_STATUS_NONE;	// Exits IO_GetContent.
					}
				}
				else	// Shouldn't happen.
				{
					return CONTENT_STATUS_FAILED;
				}
			}
			else	// Simulated download. Get the decompressed size of the stream.
			{
				EnterCriticalSection( &context->download_info->shared_cs );
				context->download_info->downloaded += output_buffer_length;					// The total amount of data (decoded) that was saved/simulated.
				LeaveCriticalSection( &context->download_info->shared_cs );

				EnterCriticalSection( &session_totals_cs );
				session_total_downloaded += output_buffer_length;
				LeaveCriticalSection( &session_totals_cs );

				context->header_info.range_info->content_offset += response_buffer_length;	// The true amount that was downloaded. Allows us to resume if we stop the download.

				// We need to force the keep-alive connections closed since the server will just keep it open after we've gotten all the data.
				if ( context->header_info.connection == CONNECTION_KEEP_ALIVE &&
				   ( context->header_info.range_info->content_length == 0 ||
				   ( context->header_info.range_info->content_offset >= ( ( context->header_info.range_info->range_end - context->header_info.range_info->range_start ) + 1 ) ) ) )
				{
					return CONTENT_STATUS_FAILED;	// We have no more data, so just close the connection.
				}
				else
				{
					return ( !context->processed_header ? CONTENT_STATUS_HANDLE_RESPONSE : CONTENT_STATUS_READ_MORE_CONTENT );
				}
			}
		}
	}

	return CONTENT_STATUS_FAILED;	// Close the connection.
}

int GetPOSTValue( char *post_data, unsigned int post_data_length, unsigned int &post_data_offset, char **value, unsigned int &value_length )
{
	if ( post_data == NULL )
	{
		return CONTENT_STATUS_FAILED;
	}

	char *data_end = _StrChrA( post_data, '\x1f' );
	if ( data_end != NULL )
	{
		int data_length = data_end - post_data;

		if ( data_length > 0 )
		{
			if ( *value != NULL )
			{
				char *reallocated_value = ( char * )GlobalReAlloc( *value, sizeof( char ) * ( value_length + data_length + 1 ), GMEM_MOVEABLE );
				if ( reallocated_value == NULL )
				{
					return CONTENT_STATUS_FAILED;
				}

				*value = reallocated_value;
			}
			else
			{
				*value = ( char * )GlobalAlloc( GMEM_FIXED, data_length + 1 );
			}

			_memcpy_s( *value + value_length, data_length + 1, post_data, data_length );
			value_length += data_length;
			*( *value + value_length ) = 0;	// Sanity.
		}

		post_data_offset = data_length + 1;

		return CONTENT_STATUS_NONE;
	}
	else
	{
		if ( *value != NULL )
		{
			char *reallocated_value = ( char * )GlobalReAlloc( *value, sizeof( char ) * ( value_length + post_data_length + 1 ), GMEM_MOVEABLE );
			if ( reallocated_value == NULL )
			{
				return CONTENT_STATUS_FAILED;
			}

			*value = reallocated_value;
		}
		else
		{
			*value = ( char * )GlobalAlloc( GMEM_FIXED, post_data_length + 1 );
		}

		_memcpy_s( *value + value_length, post_data_length + 1, post_data, post_data_length );
		value_length += post_data_length;
		*( *value + value_length ) = 0;	// Sanity.
	}

	return CONTENT_STATUS_READ_MORE_CONTENT;
}

int ParsePOSTData( SOCKET_CONTEXT *context, char *post_data, unsigned int post_data_length )
{
	if ( context == NULL || post_data == NULL )
	{
		return CONTENT_STATUS_FAILED;
	}

	int status = CONTENT_STATUS_NONE;

	unsigned int value_length = 0;
	unsigned int post_data_offset = 0;
	unsigned int offset = 0;

	if ( context->post_info == NULL )
	{
		context->post_info = ( POST_INFO * )GlobalAlloc( GPTR, sizeof( POST_INFO ) );
	}

	char **value_buf[ 7 ];
	value_buf[ 0 ] = &context->post_info->urls;
	value_buf[ 1 ] = &context->post_info->username;
	value_buf[ 2 ] = &context->post_info->password;
	value_buf[ 3 ] = &context->post_info->parts;
	value_buf[ 4 ] = &context->post_info->simulate_download;
	value_buf[ 5 ] = &context->post_info->cookies;
	value_buf[ 6 ] = &context->post_info->headers;

	for ( unsigned char i = 0; i < 7; ++i )
	{
		if ( *value_buf[ i ] == NULL )
		{
			value_length = ( unsigned int )context->header_info.chunk_length;

			offset = 0;

			// We'll use chunk_buffer to store any partial values.
			status = GetPOSTValue( post_data + post_data_offset, post_data_length - post_data_offset, offset, &context->header_info.chunk_buffer, value_length );
			if ( status != CONTENT_STATUS_NONE )
			{
				context->header_info.chunk_length = value_length;

				return status;
			}

			if ( value_length > 0 )
			{
				*value_buf[ i ] = context->header_info.chunk_buffer;
			}
			else if ( context->header_info.chunk_buffer != NULL )
			{
				GlobalFree( context->header_info.chunk_buffer );
			}

			// Reset.
			context->header_info.chunk_buffer = NULL;
			context->header_info.chunk_length = value_length = 0;

			post_data_offset += offset;
		}
	}

	return status;
}

bool SanitizePOSTHeaders( char *post_headers, char **sanitized_headers )
{
	if ( post_headers != NULL && sanitized_headers != NULL )
	{
		unsigned int headers_length = lstrlenA( post_headers );

		// Must be at least 2 characters long. "a:" is a valid header name and value.
		if ( headers_length >= 2 )
		{
			char *headers_start = post_headers;

			// Skip newlines that might appear before the field name.
			while ( *headers_start == '\r' || *headers_start == '\n' )
			{
				--headers_length;
				++headers_start;
			}

			// Make sure the header has a colon somewhere in it and not at the very beginning.
			if ( headers_length >= 2 && *headers_start != L':' && _StrChrA( headers_start + 1, L':' ) != NULL )
			{
				bool add_newline = false;

				// Make sure the header ends with a \r\n.
				if ( headers_start[ headers_length - 2 ] != '\r' && headers_start[ headers_length - 1 ] != '\n' )
				{
					add_newline = true;
				}

				// If anything's changed, then create our new header buffer.
				if ( headers_start != post_headers || add_newline )
				{
					*sanitized_headers = ( char * )GlobalAlloc( GMEM_FIXED, headers_length + 1 + ( add_newline ? 2 : 0 ) );
					_memcpy_s( *sanitized_headers, headers_length + 1, headers_start, headers_length );
					if ( add_newline )
					{
						*( *sanitized_headers + headers_length ) = '\r';
						*( *sanitized_headers + headers_length + 1 ) = '\n';
						*( *sanitized_headers + headers_length + 2 ) = 0;	// Sanity.
					}
					else
					{
						*( *sanitized_headers + headers_length ) = 0;	// Sanity.
					}
				}

				return true;
			}
		}
	}

	return false;
}

int GetHTTPRequestContent( SOCKET_CONTEXT *context, char *request_buffer, unsigned int request_buffer_length )
{
	if ( context == NULL )
	{
		return CONTENT_STATUS_FAILED;
	}

	if ( context->content_status != CONTENT_STATUS_GET_CONTENT )
	{
		int content_status = CONTENT_STATUS_GET_CONTENT;	// Assume we're now getting the content.

		// If the return status is CONTENT_STATUS_GET_CONTENT, then our end_of_header pointer will have been set.
		if ( context->content_status == CONTENT_STATUS_NONE || context->content_status == CONTENT_STATUS_READ_MORE_HEADER )
		{
			content_status = context->content_status = ParseHTTPHeader( context, request_buffer, request_buffer_length, true );
		}

		// For anything else, we'll need to return immediately.
		if ( content_status != CONTENT_STATUS_GET_CONTENT )
		{
			return content_status;
		}

		if ( context->header_info.end_of_header != NULL )
		{
			if ( cfg_use_authentication )
			{
				bool valid_authentication = false;

				if ( context->header_info.digest_info != NULL )
				{
					if ( cfg_authentication_type == AUTH_TYPE_BASIC && context->header_info.digest_info->auth_type == AUTH_TYPE_BASIC )
					{
						// We're using nonce to store the client's key.
						if ( context->header_info.digest_info->nonce != NULL )
						{
							int authentication_length = lstrlenA( context->header_info.digest_info->nonce );

							if ( authentication_length == g_encoded_authentication_length && _StrCmpNA( context->header_info.digest_info->nonce, g_encoded_authentication, g_encoded_authentication_length ) == 0 )
							{
								valid_authentication = true;
							}
						}
					}
					else if ( cfg_authentication_type == AUTH_TYPE_DIGEST && context->header_info.digest_info->auth_type == AUTH_TYPE_DIGEST )
					{
						if ( !VerifyDigestAuthorization( g_authentication_username, g_authentication_username_length, g_authentication_password, g_authentication_password_length, g_nonce, g_nonce_length, g_opaque, g_opaque_length, "GET", 3, context->header_info.digest_info ) )
						{
							// Reset our info so we can get new values from the client.
							FreeAuthInfo( &context->header_info.digest_info );
							context->header_info.digest_info = NULL;
						}
						else
						{
							valid_authentication = true;
						}
					}
				}

				if ( !valid_authentication )
				{
					if ( context->header_info.digest_info != NULL )
					{
						++context->header_info.digest_info->nc;	// We're using nc to determine success or failure. 0 = success, > 0 = failure.
					}

					return CONTENT_STATUS_HANDLE_REQUEST;
				}
				else
				{
					if ( context->header_info.digest_info != NULL )
					{
						context->header_info.digest_info->nc = 0;	// Authorization has succeeded.
					}
				}
			}

			if ( context->header_info.http_method == METHOD_POST )
			{
				// Offset our buffer to the content and adjust the new length.
				request_buffer_length -= ( context->header_info.end_of_header - request_buffer );
				request_buffer = context->header_info.end_of_header;

				context->content_status = CONTENT_STATUS_GET_CONTENT;
			}
			else	// Send a response back.
			{
				return CONTENT_STATUS_HANDLE_REQUEST;
			}
		}
		else
		{
			return CONTENT_STATUS_FAILED;
		}
	}

	// We need a content length value.
	if ( context->header_info.range_info->content_length > 0 && context->header_info.range_info->content_offset < context->header_info.range_info->content_length )
	{
		context->header_info.range_info->content_offset += request_buffer_length;

		// Creates context->post_info and fills its data.
		// Returns either CONTENT_STATUS_READ_MORE_CONTENT, CONTENT_STATUS_FAILED, or CONTENT_STATUS_NONE.
		int content_status = ParsePOSTData( context, request_buffer, request_buffer_length );
		if ( content_status != CONTENT_STATUS_NONE )
		{
			context->wsabuf.buf = context->buffer;
			context->wsabuf.len = BUFFER_SIZE;

			return content_status;
		}

		if ( context->post_info != NULL )
		{
			int urls_length = MultiByteToWideChar( CP_UTF8, 0, context->post_info->urls, -1, NULL, 0 );	// Include the NULL terminator.

			// http://a.b + NULL
			if ( urls_length >= 11 )
			{
				wchar_t *urls = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * urls_length );
				MultiByteToWideChar( CP_UTF8, 0, context->post_info->urls, -1, urls, urls_length );

				wchar_t *t_download_directory = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * MAX_PATH );
				_wmemcpy_s( t_download_directory, MAX_PATH, cfg_default_download_directory, g_default_download_directory_length );
				t_download_directory[ g_default_download_directory_length ] = 0;	// Sanity.

				unsigned int parts = 0;
				if ( context->post_info->parts != NULL )
				{
					parts = ( unsigned int )_strtoul( context->post_info->parts, NULL, 10 );
				}

				bool simulate_download = ( context->post_info->simulate_download != NULL && *context->post_info->simulate_download == '1' ? true : false );

				char *headers = NULL;

				// If the headers are valid.
				if ( SanitizePOSTHeaders( context->post_info->headers, &headers ) )
				{
					// It may have been sanitize. Replace the old headers.
					if ( headers != NULL )
					{
						GlobalFree( context->post_info->headers );
						context->post_info->headers = headers;
					}
				}
				else if ( context->post_info->headers != NULL )	// Bad headers.
				{
					GlobalFree( context->post_info->headers );
					context->post_info->headers = NULL;
				}

				context->header_info.http_status = 200;	// Let our MakeResponse() know that we want to send an HTTP 200 back.

				ADD_INFO *ai = ( ADD_INFO * )GlobalAlloc( GPTR, sizeof( ADD_INFO ) );
				ai->download_operations |= DOWNLOAD_OPERATION_SIMULATE;
				if ( parts == 0 )
				{
					ai->parts = cfg_default_download_parts;
				}
				else if ( parts > 100 )
				{
					ai->parts = 100;
				}
				else
				{
					ai->parts = ( unsigned char )parts;
				}
				ai->auth_info.username = context->post_info->username;
				ai->auth_info.password = context->post_info->password;
				ai->ssl_version = cfg_default_ssl_version;
				ai->utf8_cookies = context->post_info->cookies;
				ai->utf8_headers = context->post_info->headers;
				ai->download_operations = DOWNLOAD_OPERATION_OVERRIDE_PROMPTS | ( simulate_download ? DOWNLOAD_OPERATION_SIMULATE : DOWNLOAD_OPERATION_NONE );
				ai->urls = urls;
				ai->download_directory = t_download_directory;

				// These aren't needed.
				context->post_info->username = NULL;
				context->post_info->password = NULL;
				context->post_info->cookies = NULL;
				context->post_info->headers = NULL;

				// ai is freed in AddURL.
				CloseHandle( _CreateThread( NULL, 0, AddURL, ( void * )ai, 0, NULL ) );
			}

			FreePOSTInfo( &context->post_info );
		}
	}

	return CONTENT_STATUS_HANDLE_REQUEST;	// Send a response back.
}
