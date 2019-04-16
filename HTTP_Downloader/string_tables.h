/*
	HTTP Downloader can download files through HTTP and HTTPS connections.
	Copyright (C) 2015-2019 Eric Kutcher

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

#ifndef _STRING_TABLES_H
#define _STRING_TABLES_H

// Values with ":", "&", or "-" are replaced with _
// Values with "..." are replaced with ___ 
// Values with # are replaced with NUM
// Keep the defines case sensitive.
// Try to keep them in order.

#define MONTH_STRING_TABLE_SIZE					12
#define DAY_STRING_TABLE_SIZE					7
#define DOWNLOAD_STRING_TABLE_SIZE				14
#define MENU_STRING_TABLE_SIZE					66

#define OPTIONS_STRING_TABLE_SIZE				7
#define OPTIONS_ADVANCED_STRING_TABLE_SIZE		19
#define OPTIONS_APPEARANCE_STRING_TABLE_SIZE	18
#define OPTIONS_CONNECTION_STRING_TABLE_SIZE	26
#define OPTIONS_GENERAL_STRING_TABLE_SIZE		10
#define OPTIONS_PROXY_STRING_TABLE_SIZE			9

#define CMESSAGEBOX_STRING_TABLE_SIZE			7

#define ADD_URLS_STRING_TABLE_SIZE				22
#define SEARCH_STRING_TABLE_SIZE				8
#define COMMON_STRING_TABLE_SIZE				43
#define COMMON_MESSAGE_STRING_TABLE_SIZE		21

#define TOTAL_LOCALE_STRINGS	( MONTH_STRING_TABLE_SIZE + \
								  DAY_STRING_TABLE_SIZE + \
								  DOWNLOAD_STRING_TABLE_SIZE + \
								  MENU_STRING_TABLE_SIZE + \
								  OPTIONS_STRING_TABLE_SIZE + \
								  OPTIONS_ADVANCED_STRING_TABLE_SIZE + \
								  OPTIONS_APPEARANCE_STRING_TABLE_SIZE + \
								  OPTIONS_CONNECTION_STRING_TABLE_SIZE + \
								  OPTIONS_GENERAL_STRING_TABLE_SIZE + \
								  OPTIONS_PROXY_STRING_TABLE_SIZE + \
								  CMESSAGEBOX_STRING_TABLE_SIZE + \
								  ADD_URLS_STRING_TABLE_SIZE + \
								  SEARCH_STRING_TABLE_SIZE + \
								  COMMON_STRING_TABLE_SIZE + \
								  COMMON_MESSAGE_STRING_TABLE_SIZE )

#define MONTH_STRING_TABLE_OFFSET				0
#define DAY_STRING_TABLE_OFFSET					( MONTH_STRING_TABLE_OFFSET + MONTH_STRING_TABLE_SIZE )
#define DOWNLOAD_STRING_TABLE_OFFSET			( DAY_STRING_TABLE_OFFSET + DAY_STRING_TABLE_SIZE )
#define MENU_STRING_TABLE_OFFSET				( DOWNLOAD_STRING_TABLE_OFFSET + DOWNLOAD_STRING_TABLE_SIZE )
#define OPTIONS_STRING_TABLE_OFFSET				( MENU_STRING_TABLE_OFFSET + MENU_STRING_TABLE_SIZE )
#define OPTIONS_ADVANCED_STRING_TABLE_OFFSET	( OPTIONS_STRING_TABLE_OFFSET + OPTIONS_STRING_TABLE_SIZE )
#define OPTIONS_APPEARANCE_STRING_TABLE_OFFSET	( OPTIONS_ADVANCED_STRING_TABLE_OFFSET + OPTIONS_ADVANCED_STRING_TABLE_SIZE )
#define OPTIONS_CONNECTION_STRING_TABLE_OFFSET	( OPTIONS_APPEARANCE_STRING_TABLE_OFFSET + OPTIONS_APPEARANCE_STRING_TABLE_SIZE )
#define OPTIONS_GENERAL_STRING_TABLE_OFFSET		( OPTIONS_CONNECTION_STRING_TABLE_OFFSET + OPTIONS_CONNECTION_STRING_TABLE_SIZE )
#define OPTIONS_PROXY_STRING_TABLE_OFFSET		( OPTIONS_GENERAL_STRING_TABLE_OFFSET + OPTIONS_GENERAL_STRING_TABLE_SIZE )
#define CMESSAGEBOX_STRING_TABLE_OFFSET			( OPTIONS_PROXY_STRING_TABLE_OFFSET + OPTIONS_PROXY_STRING_TABLE_SIZE )
#define ADD_URLS_STRING_TABLE_OFFSET			( CMESSAGEBOX_STRING_TABLE_OFFSET + CMESSAGEBOX_STRING_TABLE_SIZE )
#define SEARCH_STRING_TABLE_OFFSET				( ADD_URLS_STRING_TABLE_OFFSET + ADD_URLS_STRING_TABLE_SIZE )
#define COMMON_STRING_TABLE_OFFSET				( SEARCH_STRING_TABLE_OFFSET + SEARCH_STRING_TABLE_SIZE )
#define COMMON_MESSAGE_STRING_TABLE_OFFSET		( COMMON_STRING_TABLE_OFFSET + COMMON_STRING_TABLE_SIZE )

struct STRING_TABLE_DATA
{
	wchar_t *value;
	unsigned short length;
};

extern bool g_use_dynamic_locale;	// Did we get the strings from a file? We'll need to free the memory when we're done.

extern STRING_TABLE_DATA g_locale_table[ TOTAL_LOCALE_STRINGS ];

extern STRING_TABLE_DATA month_string_table[];
extern STRING_TABLE_DATA day_string_table[];
extern STRING_TABLE_DATA download_string_table[];
extern STRING_TABLE_DATA menu_string_table[];
extern STRING_TABLE_DATA options_string_table[];
extern STRING_TABLE_DATA options_advanced_string_table[];
extern STRING_TABLE_DATA options_appearance_string_table[];
extern STRING_TABLE_DATA options_connection_string_table[];
extern STRING_TABLE_DATA options_general_string_table[];
extern STRING_TABLE_DATA options_proxy_string_table[];
extern STRING_TABLE_DATA cmessagebox_string_table[];
extern STRING_TABLE_DATA add_urls_string_table[];
extern STRING_TABLE_DATA search_string_table[];
extern STRING_TABLE_DATA common_string_table[];
extern STRING_TABLE_DATA common_message_string_table[];

void InitializeLocaleValues();
void UninitializeLocaleValues();

// Month
#define ST_V_January									g_locale_table[ 0 ].value
#define ST_V_February									g_locale_table[ 1 ].value
#define ST_V_March										g_locale_table[ 2 ].value
#define ST_V_April										g_locale_table[ 3 ].value
#define ST_V_May										g_locale_table[ 4 ].value
#define ST_V_June										g_locale_table[ 5 ].value
#define ST_V_July										g_locale_table[ 6 ].value
#define ST_V_August										g_locale_table[ 7 ].value
#define ST_V_September									g_locale_table[ 8 ].value
#define ST_V_October									g_locale_table[ 9 ].value
#define ST_V_November									g_locale_table[ 10 ].value
#define ST_V_December									g_locale_table[ 11 ].value

// Day
#define ST_V_Sunday										g_locale_table[ 12 ].value
#define ST_V_Monday										g_locale_table[ 13 ].value
#define ST_V_Tuesday									g_locale_table[ 14 ].value
#define ST_V_Wednesday									g_locale_table[ 15 ].value
#define ST_V_Thursday									g_locale_table[ 16 ].value
#define ST_V_Friday										g_locale_table[ 17 ].value
#define ST_V_Saturday									g_locale_table[ 18 ].value
/*
// Download
#define ST_V_NUM										g_locale_table[ 19 ].value
#define ST_V_Active_Parts								g_locale_table[ 20 ].value
#define ST_V_Date_and_Time_Added						g_locale_table[ 21 ].value
#define ST_V_Download_Directory							g_locale_table[ 22 ].value
#define ST_V_Download_Speed								g_locale_table[ 23 ].value
#define ST_V_Downloaded									g_locale_table[ 24 ].value
#define ST_V_File_Size									g_locale_table[ 25 ].value
#define ST_V_File_Type									g_locale_table[ 26 ].value
#define ST_V_Filename									g_locale_table[ 27 ].value
#define ST_V_Progress									g_locale_table[ 28 ].value
#define ST_V_SSL___TLS_Version							g_locale_table[ 29 ].value
#define ST_V_Time_Elapsed								g_locale_table[ 30 ].value
#define ST_V_Time_Remaining								g_locale_table[ 31 ].value
#define ST_V_URL										g_locale_table[ 32 ].value
*/
// Menu
#define ST_V_NUM										g_locale_table[ 33 ].value
#define ST_V__About										g_locale_table[ 34 ].value
#define ST_V_Active_Parts								g_locale_table[ 35 ].value
#define ST_V__Add_URL_s____								g_locale_table[ 36 ].value
#define ST_V_Add_URL_s____								g_locale_table[ 37 ].value
#define ST_V_Always_on_Top								g_locale_table[ 38 ].value
#define ST_V__Column_Headers							g_locale_table[ 39 ].value
#define ST_V__Copy_URL_s_								g_locale_table[ 40 ].value
#define ST_V_Copy_URL_s_								g_locale_table[ 41 ].value
#define ST_V_Date_and_Time_Added						g_locale_table[ 42 ].value
#define ST_V__Delete_									g_locale_table[ 43 ].value
#define ST_V_Delete										g_locale_table[ 44 ].value
#define ST_V_Download_Directory							g_locale_table[ 45 ].value
#define ST_V_Download_Speed								g_locale_table[ 46 ].value
#define ST_V_Downloaded									g_locale_table[ 47 ].value
#define ST_V__Edit										g_locale_table[ 48 ].value
#define ST_V_E_xit										g_locale_table[ 49 ].value
#define ST_V_Exit										g_locale_table[ 50 ].value
#define ST_V__Export_Download_History___				g_locale_table[ 51 ].value
#define ST_V__File										g_locale_table[ 52 ].value
#define ST_V_File_Size									g_locale_table[ 53 ].value
#define ST_V_File_Type									g_locale_table[ 54 ].value
#define ST_V_Filename									g_locale_table[ 55 ].value
#define ST_V__Help										g_locale_table[ 56 ].value
#define ST_V_HTTP_Downloader__Home_Page					g_locale_table[ 57 ].value
#define ST_V__Import_Download_History___				g_locale_table[ 58 ].value
#define ST_V_Move_Down									g_locale_table[ 59 ].value
#define ST_V_Move_to_Bottom								g_locale_table[ 60 ].value
#define ST_V_Move_to_Top								g_locale_table[ 61 ].value
#define ST_V_Move_Up									g_locale_table[ 62 ].value
#define ST_V_Open_Directory								g_locale_table[ 63 ].value
#define ST_V_Open_File									g_locale_table[ 64 ].value
#define ST_V_Open_Download_List							g_locale_table[ 65 ].value
#define ST_V__Options____								g_locale_table[ 66 ].value
#define ST_V_Options___									g_locale_table[ 67 ].value
#define ST_V__Pause										g_locale_table[ 68 ].value
#define ST_V_Pause										g_locale_table[ 69 ].value
#define ST_V_Pause_Active								g_locale_table[ 70 ].value
#define ST_V_Progress									g_locale_table[ 71 ].value
#define ST_V_Queue										g_locale_table[ 72 ].value
#define ST_V__Remove_									g_locale_table[ 73 ].value
#define ST_V_Remove										g_locale_table[ 74 ].value
#define ST_V_Remove_and_Delete_							g_locale_table[ 75 ].value
#define ST_V_Remove_and_Delete							g_locale_table[ 76 ].value
#define ST_V_Remove_Completed							g_locale_table[ 77 ].value
#define ST_V_Rename_									g_locale_table[ 78 ].value
#define ST_V_Rename										g_locale_table[ 79 ].value
#define ST_V_Restart									g_locale_table[ 80 ].value
#define ST_V__Save_Download_History___					g_locale_table[ 81 ].value
#define ST_V__Search____								g_locale_table[ 82 ].value
#define ST_V__Select_All_								g_locale_table[ 83 ].value
#define ST_V_Select_All									g_locale_table[ 84 ].value
#define ST_V_SSL___TLS_Version							g_locale_table[ 85 ].value
#define ST_V_St_art										g_locale_table[ 86 ].value
#define ST_V_Start										g_locale_table[ 87 ].value
#define ST_V__Status_Bar								g_locale_table[ 88 ].value
#define ST_V_St_op										g_locale_table[ 89 ].value
#define ST_V_Stop										g_locale_table[ 90 ].value
#define ST_V_Stop_All									g_locale_table[ 91 ].value
#define ST_V_Time_Elapsed								g_locale_table[ 92 ].value
#define ST_V_Time_Remaining								g_locale_table[ 93 ].value
#define ST_V__Toolbar									g_locale_table[ 94 ].value
#define ST_V__Tools										g_locale_table[ 95 ].value
#define ST_V_Update_Download___							g_locale_table[ 96 ].value
#define ST_V_URL										g_locale_table[ 97 ].value
#define ST_V__View										g_locale_table[ 98 ].value

// Options
#define ST_V_Advanced									g_locale_table[ 99 ].value
#define ST_V_Appearance									g_locale_table[ 100 ].value
#define ST_V_Apply										g_locale_table[ 101 ].value
#define ST_V_Connection									g_locale_table[ 102 ].value
#define ST_V_General									g_locale_table[ 103 ].value
#define ST_V_OK											g_locale_table[ 104 ].value
#define ST_V_Proxy										g_locale_table[ 105 ].value

// Options Advanced
#define ST_V_Allow_only_one_instance					g_locale_table[ 106 ].value
#define ST_V_Continue_Download							g_locale_table[ 107 ].value
#define ST_V_Default_download_directory_				g_locale_table[ 108 ].value
#define ST_V_Display_Prompt								g_locale_table[ 109 ].value
#define ST_V_Download_drag_and_drop_immediately			g_locale_table[ 110 ].value
#define ST_V_Enable_download_history					g_locale_table[ 111 ].value
#define ST_V_Enable_quick_file_allocation				g_locale_table[ 112 ].value
#define ST_V_Overwrite_File								g_locale_table[ 113 ].value
#define ST_V_Prevent_system_standby						g_locale_table[ 114 ].value
#define ST_V_Rename_File								g_locale_table[ 115 ].value
#define ST_V_Restart_Download							g_locale_table[ 116 ].value
#define ST_V_Resume_previously_downloading				g_locale_table[ 117 ].value
#define ST_V_Set_date_and_time_of_file					g_locale_table[ 118 ].value
#define ST_V_Skip_Download								g_locale_table[ 119 ].value
#define ST_V_Thread_pool_count_							g_locale_table[ 120 ].value
#define ST_V_Use_temporary_download_directory_			g_locale_table[ 121 ].value
#define ST_V_When_a_file_already_exists_				g_locale_table[ 122 ].value
#define ST_V_When_a_file_has_been_modified_				g_locale_table[ 123 ].value
#define ST_V_When_a_file_is_greater_than_or_equal_to_	g_locale_table[ 124 ].value

// Options Appearance
#define ST_V_Background_Color							g_locale_table[ 125 ].value
#define ST_V_Background_Font_Color						g_locale_table[ 126 ].value
#define ST_V_Border_Color								g_locale_table[ 127 ].value
#define ST_V_Download_list_								g_locale_table[ 128 ].value
#define ST_V_Even_Row_Background_Color					g_locale_table[ 129 ].value
#define ST_V_Even_Row_Font								g_locale_table[ 130 ].value
#define ST_V_Even_Row_Font_Color						g_locale_table[ 131 ].value
#define ST_V_Even_Row_Highlight_Color					g_locale_table[ 132 ].value
#define ST_V_Even_Row_Highlight_Font_Color				g_locale_table[ 133 ].value
#define ST_V_Odd_Row_Background_Color					g_locale_table[ 134 ].value
#define ST_V_Odd_Row_Font								g_locale_table[ 135 ].value
#define ST_V_Odd_Row_Font_Color							g_locale_table[ 136 ].value
#define ST_V_Odd_Row_Highlight_Color					g_locale_table[ 137 ].value
#define ST_V_Odd_Row_Highlight_Font_Color				g_locale_table[ 138 ].value
#define ST_V_Progress_Color								g_locale_table[ 139 ].value
#define ST_V_Progress_bar_								g_locale_table[ 140 ].value
#define ST_V_Progress_Font_Color						g_locale_table[ 141 ].value
#define ST_V_Show_gridlines_in_download_list			g_locale_table[ 142 ].value

// Options Connection
#define ST_V__											g_locale_table[ 143 ].value
#define ST_V_Active_download_limit_						g_locale_table[ 144 ].value
#define ST_V_Basic_Authentication						g_locale_table[ 145 ].value
#define ST_V_Certificate_file_							g_locale_table[ 146 ].value
#define ST_V_Default_download_parts_					g_locale_table[ 147 ].value
#define ST_V_Default_SSL___TLS_version_					g_locale_table[ 148 ].value
#define ST_V_Digest_Authentication						g_locale_table[ 149 ].value
#define ST_V_Enable_server_								g_locale_table[ 150 ].value
#define ST_V_Enable_SSL___TLS_							g_locale_table[ 151 ].value
#define ST_V_Hostname___IPv6_address_					g_locale_table[ 152 ].value
#define ST_V_IPv4_address_								g_locale_table[ 153 ].value
#define ST_V_Key_file_									g_locale_table[ 154 ].value
#define ST_V_Load_PKCS_NUM12_File						g_locale_table[ 155 ].value
#define ST_V_Load_Private_Key_File						g_locale_table[ 156 ].value
#define ST_V_Load_X_509_Certificate_File				g_locale_table[ 157 ].value
#define ST_V_Maximum_redirects_							g_locale_table[ 158 ].value
#define ST_V_PKCS_NUM12_								g_locale_table[ 159 ].value
#define ST_V_PKCS_NUM12_file_							g_locale_table[ 160 ].value
#define ST_V_PKCS_NUM12_password_						g_locale_table[ 161 ].value
#define ST_V_Port_										g_locale_table[ 162 ].value
#define ST_V_Public___Private_key_pair_					g_locale_table[ 163 ].value
#define ST_V_Require_authentication_					g_locale_table[ 164 ].value
#define ST_V_Retry_incomplete_downloads_				g_locale_table[ 165 ].value
#define ST_V_Retry_incomplete_parts_					g_locale_table[ 166 ].value
#define ST_V_Server_SSL___TLS_version_					g_locale_table[ 167 ].value
#define ST_V_Timeout__seconds__							g_locale_table[ 168 ].value

// Options General
#define ST_V_Always_on_top								g_locale_table[ 169 ].value
#define ST_V_Close_to_System_Tray						g_locale_table[ 170 ].value
#define ST_V_Enable_System_Tray_icon_					g_locale_table[ 171 ].value
#define ST_V_Enable_URL_drop_window_					g_locale_table[ 172 ].value
#define ST_V_Load_Download_Finish_Sound_File			g_locale_table[ 173 ].value
#define ST_V_Minimize_to_System_Tray					g_locale_table[ 174 ].value
#define ST_V_Play_sound_when_downloads_finish_			g_locale_table[ 175 ].value
#define ST_V_Show_notification_when_downloads_finish	g_locale_table[ 176 ].value
#define ST_V_Show_progress_bar							g_locale_table[ 177 ].value
#define ST_V_Transparency_								g_locale_table[ 178 ].value

// Options Proxy
#define ST_V_Allow_proxy_to_resolve_domain_names		g_locale_table[ 179 ].value
#define ST_V_Allow_proxy_to_resolve_domain_names_v4a	g_locale_table[ 180 ].value
#define ST_V_Hostname_									g_locale_table[ 181 ].value
#define ST_V_SOCKS_v4									g_locale_table[ 182 ].value
#define ST_V_SOCKS_v5									g_locale_table[ 183 ].value
#define ST_V_Use_Authentication_						g_locale_table[ 184 ].value
#define ST_V_Use_HTTP_proxy_							g_locale_table[ 185 ].value
#define ST_V_Use_HTTPS_proxy_							g_locale_table[ 186 ].value
#define ST_V_Use_SOCKS_proxy_							g_locale_table[ 187 ].value

// CMessageBox
#define ST_V_Continue									g_locale_table[ 188 ].value
#define ST_V_No											g_locale_table[ 189 ].value
#define ST_V_Overwrite									g_locale_table[ 190 ].value
#define ST_V_Remember_choice							g_locale_table[ 191 ].value
#define ST_V_Skip										g_locale_table[ 192 ].value
#define ST_V_Skip_remaining_messages					g_locale_table[ 193 ].value
#define ST_V_Yes										g_locale_table[ 194 ].value

// Add URL(s)
#define ST_V_Advanced_AB								g_locale_table[ 195 ].value
#define ST_V_Advanced_BB								g_locale_table[ 196 ].value
#define	ST_V_Authentication								g_locale_table[ 197 ].value
#define ST_V_Cookies									g_locale_table[ 198 ].value
#define ST_V_Cookies_									g_locale_table[ 199 ].value
#define ST_V_Custom										g_locale_table[ 200 ].value
#define ST_V_Download									g_locale_table[ 201 ].value
#define ST_V_Download_directory_						g_locale_table[ 202 ].value
#define ST_V_Download_parts_							g_locale_table[ 203 ].value
#define ST_V_Headers									g_locale_table[ 204 ].value
#define ST_V_Headers_									g_locale_table[ 205 ].value
#define ST_V_Images										g_locale_table[ 206 ].value
#define ST_V_Music										g_locale_table[ 207 ].value
#define ST_V_Password_									g_locale_table[ 208 ].value
#define ST_V_POST_Data									g_locale_table[ 209 ].value
#define ST_V_RegEx_filter_								g_locale_table[ 210 ].value
#define ST_V_Send_POST_Data_							g_locale_table[ 211 ].value
#define ST_V_Simulate_download							g_locale_table[ 212 ].value
#define ST_V_SSL___TLS_version_							g_locale_table[ 213 ].value
#define ST_V_URL_s__									g_locale_table[ 214 ].value
#define ST_V_Username_									g_locale_table[ 215 ].value
#define ST_V_Videos										g_locale_table[ 216 ].value

// Search
#define ST_V_Match_case									g_locale_table[ 217 ].value
#define ST_V_Match_whole_word							g_locale_table[ 218 ].value
#define ST_V_Regular_expression							g_locale_table[ 219 ].value
#define ST_V_Search										g_locale_table[ 220 ].value
#define ST_V_Search_All									g_locale_table[ 221 ].value
#define ST_V_Search_for_								g_locale_table[ 222 ].value
#define ST_V_Search_Next								g_locale_table[ 223 ].value
#define ST_V_Search_Type								g_locale_table[ 224 ].value

// Common
#define ST_V_BTN___										g_locale_table[ 225 ].value
#define ST_V__Simulated_								g_locale_table[ 226 ].value
#define ST_V_Add_URL_s_									g_locale_table[ 227 ].value
#define ST_V_Added										g_locale_table[ 228 ].value
#define ST_V_Allocating_File							g_locale_table[ 229 ].value
#define ST_V_Authorization_Required						g_locale_table[ 230 ].value
#define ST_V_Cancel										g_locale_table[ 231 ].value
#define ST_V_Completed									g_locale_table[ 232 ].value
#define ST_V_Connecting									g_locale_table[ 233 ].value
#define ST_V_Download_speed_							g_locale_table[ 234 ].value
#define ST_V_Download_speed__0_B_s						g_locale_table[ 235 ].value
#define ST_V_Download_speed__0_00_GB_s					g_locale_table[ 236 ].value
#define ST_V_Download_speed__0_00_KB_s					g_locale_table[ 237 ].value
#define ST_V_Download_speed__0_00_MB_s					g_locale_table[ 238 ].value
#define ST_V_Downloading								g_locale_table[ 239 ].value
#define ST_V_Downloads_Have_Finished					g_locale_table[ 240 ].value
#define ST_V_Export_Download_History					g_locale_table[ 241 ].value
#define ST_V_Failed										g_locale_table[ 242 ].value
#define ST_V_File_IO_Error								g_locale_table[ 243 ].value
#define ST_V_Import_Download_History					g_locale_table[ 244 ].value
#define ST_V_Moving_File								g_locale_table[ 245 ].value
#define ST_V_Options									g_locale_table[ 246 ].value
#define ST_V_Paused										g_locale_table[ 247 ].value
#define ST_V_Proxy_Authentication_Required				g_locale_table[ 248 ].value
#define ST_V_Queued										g_locale_table[ 249 ].value
#define ST_V_Restarting									g_locale_table[ 250 ].value
#define ST_V_Save_Download_History						g_locale_table[ 251 ].value
#define ST_V_Skipped									g_locale_table[ 252 ].value
#define ST_V_Stopped									g_locale_table[ 253 ].value
#define ST_V_SSL_2_0									g_locale_table[ 254 ].value
#define ST_V_SSL_3_0									g_locale_table[ 255 ].value
#define ST_V_Timed_Out									g_locale_table[ 256 ].value
#define ST_V_TLS_1_0									g_locale_table[ 257 ].value
#define ST_V_TLS_1_1									g_locale_table[ 258 ].value
#define ST_V_TLS_1_2									g_locale_table[ 259 ].value
#define ST_V_Total_downloaded_							g_locale_table[ 260 ].value
#define ST_V_Total_downloaded__0_B						g_locale_table[ 261 ].value
#define ST_V_Total_downloaded__0_00_GB					g_locale_table[ 262 ].value
#define ST_V_Total_downloaded__0_00_KB					g_locale_table[ 263 ].value
#define ST_V_Total_downloaded__0_00_MB					g_locale_table[ 264 ].value
#define ST_V_URL_										g_locale_table[ 265 ].value
#define ST_V_Update										g_locale_table[ 266 ].value
#define ST_V_Update_Download							g_locale_table[ 267 ].value

// Common Messages
#define ST_V_A_restart_is_required						g_locale_table[ 268 ].value
#define ST_V_A_restart_is_required_allocation			g_locale_table[ 269 ].value
#define ST_V_A_restart_is_required_threads				g_locale_table[ 270 ].value
#define ST_V_PROMPT_delete_selected_files				g_locale_table[ 271 ].value
#define ST_V_PROMPT_remove_completed_entries			g_locale_table[ 272 ].value
#define ST_V_PROMPT_remove_and_delete_selected_entries	g_locale_table[ 273 ].value
#define ST_V_PROMPT_remove_selected_entries				g_locale_table[ 274 ].value
#define ST_V_PROMPT_restart_selected_entries			g_locale_table[ 275 ].value
#define ST_V_One_or_more_files_are_in_use				g_locale_table[ 276 ].value
#define ST_V_One_or_more_files_were_not_found			g_locale_table[ 277 ].value
#define ST_V_Select_the_default_download_directory		g_locale_table[ 278 ].value
#define ST_V_Select_the_download_directory				g_locale_table[ 279 ].value
#define ST_V_Select_the_temporary_download_directory	g_locale_table[ 280 ].value
#define ST_V_The_download_will_be_resumed				g_locale_table[ 281 ].value
#define ST_V_File_is_in_use_cannot_delete				g_locale_table[ 282 ].value
#define ST_V_File_is_in_use_cannot_rename				g_locale_table[ 283 ].value
#define ST_V_File_format_is_incorrect					g_locale_table[ 284 ].value
#define ST_V_PROMPT_The_specified_file_was_not_found	g_locale_table[ 285 ].value
#define ST_V_The_specified_path_was_not_found			g_locale_table[ 286 ].value
#define ST_V_There_is_already_a_file					g_locale_table[ 287 ].value
#define ST_V_You_must_supply_download_directory			g_locale_table[ 288 ].value

//

// Month
#define ST_L_January									g_locale_table[ 0 ].length
#define ST_L_February									g_locale_table[ 1 ].length
#define ST_L_March										g_locale_table[ 2 ].length
#define ST_L_April										g_locale_table[ 3 ].length
#define ST_L_May										g_locale_table[ 4 ].length
#define ST_L_June										g_locale_table[ 5 ].length
#define ST_L_July										g_locale_table[ 6 ].length
#define ST_L_August										g_locale_table[ 7 ].length
#define ST_L_September									g_locale_table[ 8 ].length
#define ST_L_October									g_locale_table[ 9 ].length
#define ST_L_November									g_locale_table[ 10 ].length
#define ST_L_December									g_locale_table[ 11 ].length

// Day
#define ST_L_Sunday										g_locale_table[ 12 ].length
#define ST_L_Monday										g_locale_table[ 13 ].length
#define ST_L_Tuesday									g_locale_table[ 14 ].length
#define ST_L_Wednesday									g_locale_table[ 15 ].length
#define ST_L_Thursday									g_locale_table[ 16 ].length
#define ST_L_Friday										g_locale_table[ 17 ].length
#define ST_L_Saturday									g_locale_table[ 18 ].length
/*
// Download
#define ST_L_NUM										g_locale_table[ 19 ].length
#define ST_L_Active_Parts								g_locale_table[ 20 ].length
#define ST_L_Date_and_Time_Added						g_locale_table[ 21 ].length
#define ST_L_Download_Directory							g_locale_table[ 22 ].length
#define ST_L_Download_Speed								g_locale_table[ 23 ].length
#define ST_L_Downloaded									g_locale_table[ 24 ].length
#define ST_L_File_Size									g_locale_table[ 25 ].length
#define ST_L_File_Type									g_locale_table[ 26 ].length
#define ST_L_Filename									g_locale_table[ 27 ].length
#define ST_L_Progress									g_locale_table[ 28 ].length
#define ST_L_SSL___TLS_Version							g_locale_table[ 29 ].length
#define ST_L_Time_Elapsed								g_locale_table[ 30 ].length
#define ST_L_Time_Remaining								g_locale_table[ 31 ].length
#define ST_L_URL										g_locale_table[ 32 ].length
*/
// Menu
#define ST_L_NUM										g_locale_table[ 33 ].length
#define ST_L__About										g_locale_table[ 34 ].length
#define ST_L_Active_Parts								g_locale_table[ 35 ].length
#define ST_L__Add_URL_s____								g_locale_table[ 36 ].length
#define ST_L_Add_URL_s____								g_locale_table[ 37 ].length
#define ST_L_Always_on_Top								g_locale_table[ 38 ].length
#define ST_L__Column_Headers							g_locale_table[ 39 ].length
#define ST_L__Copy_URL_s_								g_locale_table[ 40 ].length
#define ST_L_Copy_URL_s_								g_locale_table[ 41 ].length
#define ST_L_Date_and_Time_Added						g_locale_table[ 42 ].length
#define ST_L__Delete_									g_locale_table[ 43 ].length
#define ST_L_Delete										g_locale_table[ 44 ].length
#define ST_L_Download_Directory							g_locale_table[ 45 ].length
#define ST_L_Download_Speed								g_locale_table[ 46 ].length
#define ST_L_Downloaded									g_locale_table[ 47 ].length
#define ST_L__Edit										g_locale_table[ 48 ].length
#define ST_L_E_xit										g_locale_table[ 49 ].length
#define ST_L_Exit										g_locale_table[ 50 ].length
#define ST_L__Export_Download_History___				g_locale_table[ 51 ].length
#define ST_L__File										g_locale_table[ 52 ].length
#define ST_L_File_Size									g_locale_table[ 53 ].length
#define ST_L_File_Type									g_locale_table[ 54 ].length
#define ST_L_Filename									g_locale_table[ 55 ].length
#define ST_L__Help										g_locale_table[ 56 ].length
#define ST_L_HTTP_Downloader__Home_Page					g_locale_table[ 57 ].length
#define ST_L__Import_Download_History___				g_locale_table[ 58 ].length
#define ST_L_Move_Down									g_locale_table[ 59 ].length
#define ST_L_Move_to_Bottom								g_locale_table[ 60 ].length
#define ST_L_Move_to_Top								g_locale_table[ 61 ].length
#define ST_L_Move_Up									g_locale_table[ 62 ].length
#define ST_L_Open_Directory								g_locale_table[ 63 ].length
#define ST_L_Open_File									g_locale_table[ 64 ].length
#define ST_L_Open_Download_List							g_locale_table[ 65 ].length
#define ST_L__Options____								g_locale_table[ 66 ].length
#define ST_L_Options___									g_locale_table[ 67 ].length
#define ST_L__Pause										g_locale_table[ 68 ].length
#define ST_L_Pause										g_locale_table[ 69 ].length
#define ST_L_Pause_Active								g_locale_table[ 70 ].length
#define ST_L_Progress									g_locale_table[ 71 ].length
#define ST_L_Queue										g_locale_table[ 72 ].length
#define ST_L__Remove_									g_locale_table[ 73 ].length
#define ST_L_Remove										g_locale_table[ 74 ].length
#define ST_L_Remove_and_Delete_							g_locale_table[ 75 ].length
#define ST_L_Remove_and_Delete							g_locale_table[ 76 ].length
#define ST_L_Remove_Completed							g_locale_table[ 77 ].length
#define ST_L_Rename_									g_locale_table[ 78 ].length
#define ST_L_Rename										g_locale_table[ 79 ].length
#define ST_L_Restart									g_locale_table[ 80 ].length
#define ST_L__Save_Download_History___					g_locale_table[ 81 ].length
#define ST_L__Search____								g_locale_table[ 82 ].length
#define ST_L__Select_All_								g_locale_table[ 83 ].length
#define ST_L_Select_All									g_locale_table[ 84 ].length
#define ST_L_SSL___TLS_Version							g_locale_table[ 85 ].length
#define ST_L_St_art										g_locale_table[ 86 ].length
#define ST_L_Start										g_locale_table[ 87 ].length
#define ST_L__Status_Bar								g_locale_table[ 88 ].length
#define ST_L_St_op										g_locale_table[ 89 ].length
#define ST_L_Stop										g_locale_table[ 90 ].length
#define ST_L_Stop_All									g_locale_table[ 91 ].length
#define ST_L_Time_Elapsed								g_locale_table[ 92 ].length
#define ST_L_Time_Remaining								g_locale_table[ 93 ].length
#define ST_L__Toolbar									g_locale_table[ 94 ].length
#define ST_L__Tools										g_locale_table[ 95 ].length
#define ST_L_Update_Download___							g_locale_table[ 96 ].length
#define ST_L_URL										g_locale_table[ 97 ].length
#define ST_L__View										g_locale_table[ 98 ].length

// Options
#define ST_L_Advanced									g_locale_table[ 99 ].length
#define ST_L_Appearance									g_locale_table[ 100 ].length
#define ST_L_Apply										g_locale_table[ 101 ].length
#define ST_L_Connection									g_locale_table[ 102 ].length
#define ST_L_General									g_locale_table[ 103 ].length
#define ST_L_OK											g_locale_table[ 104 ].length
#define ST_L_Proxy										g_locale_table[ 105 ].length

// Options Advanced
#define ST_L_Allow_only_one_instance					g_locale_table[ 106 ].length
#define ST_L_Continue_Download							g_locale_table[ 107 ].length
#define ST_L_Default_download_directory_				g_locale_table[ 108 ].length
#define ST_L_Display_Prompt								g_locale_table[ 109 ].length
#define ST_L_Download_drag_and_drop_immediately			g_locale_table[ 110 ].length
#define ST_L_Enable_download_history					g_locale_table[ 111 ].length
#define ST_L_Enable_quick_file_allocation				g_locale_table[ 112 ].length
#define ST_L_Overwrite_File								g_locale_table[ 113 ].length
#define ST_L_Prevent_system_standby						g_locale_table[ 114 ].length
#define ST_L_Rename_File								g_locale_table[ 115 ].length
#define ST_L_Restart_Download							g_locale_table[ 116 ].length
#define ST_L_Resume_previously_downloading				g_locale_table[ 117 ].length
#define ST_L_Set_date_and_time_of_file					g_locale_table[ 118 ].length
#define ST_L_Skip_Download								g_locale_table[ 119 ].length
#define ST_L_Thread_pool_count_							g_locale_table[ 120 ].length
#define ST_L_Use_temporary_download_directory_			g_locale_table[ 121 ].length
#define ST_L_When_a_file_already_exists_				g_locale_table[ 122 ].length
#define ST_L_When_a_file_has_been_modified_				g_locale_table[ 123 ].length
#define ST_L_When_a_file_is_greater_than_or_equal_to_	g_locale_table[ 124 ].length

// Options Appearance
#define ST_L_Background_Color							g_locale_table[ 125 ].length
#define ST_L_Background_Font_Color						g_locale_table[ 126 ].length
#define ST_L_Border_Color								g_locale_table[ 127 ].length
#define ST_L_Download_list_								g_locale_table[ 128 ].length
#define ST_L_Even_Row_Background_Color					g_locale_table[ 129 ].length
#define ST_L_Even_Row_Font								g_locale_table[ 130 ].length
#define ST_L_Even_Row_Font_Color						g_locale_table[ 131 ].length
#define ST_L_Even_Row_Highlight_Color					g_locale_table[ 132 ].length
#define ST_L_Even_Row_Highlight_Font_Color				g_locale_table[ 133 ].length
#define ST_L_Odd_Row_Background_Color					g_locale_table[ 134 ].length
#define ST_L_Odd_Row_Font								g_locale_table[ 135 ].length
#define ST_L_Odd_Row_Font_Color							g_locale_table[ 136 ].length
#define ST_L_Odd_Row_Highlight_Color					g_locale_table[ 137 ].length
#define ST_L_Odd_Row_Highlight_Font_Color				g_locale_table[ 138 ].length
#define ST_L_Progress_Color								g_locale_table[ 139 ].length
#define ST_L_Progress_bar_								g_locale_table[ 140 ].length
#define ST_L_Progress_Font_Color						g_locale_table[ 141 ].length
#define ST_L_Show_gridlines_in_download_list			g_locale_table[ 142 ].length

// Options Connection
#define ST_L__											g_locale_table[ 143 ].length
#define ST_L_Active_download_limit_						g_locale_table[ 144 ].length
#define ST_L_Basic_Authentication						g_locale_table[ 145 ].length
#define ST_L_Certificate_file_							g_locale_table[ 146 ].length
#define ST_L_Default_download_parts_					g_locale_table[ 147 ].length
#define ST_L_Default_SSL___TLS_version_					g_locale_table[ 148 ].length
#define ST_L_Digest_Authentication						g_locale_table[ 149 ].length
#define ST_L_Enable_server_								g_locale_table[ 150 ].length
#define ST_L_Enable_SSL___TLS_							g_locale_table[ 151 ].length
#define ST_L_Hostname___IPv6_address_					g_locale_table[ 152 ].length
#define ST_L_IPv4_address_								g_locale_table[ 153 ].length
#define ST_L_Key_file_									g_locale_table[ 154 ].length
#define ST_L_Load_PKCS_NUM12_File						g_locale_table[ 155 ].length
#define ST_L_Load_Private_Key_File						g_locale_table[ 156 ].length
#define ST_L_Load_X_509_Certificate_File				g_locale_table[ 157 ].length
#define ST_L_Maximum_redirects_							g_locale_table[ 158 ].length
#define ST_L_PKCS_NUM12_								g_locale_table[ 159 ].length
#define ST_L_PKCS_NUM12_file_							g_locale_table[ 160 ].length
#define ST_L_PKCS_NUM12_password_						g_locale_table[ 161 ].length
#define ST_L_Port_										g_locale_table[ 162 ].length
#define ST_L_Public___Private_key_pair_					g_locale_table[ 163 ].length
#define ST_L_Require_authentication_					g_locale_table[ 164 ].length
#define ST_L_Retry_incomplete_downloads_				g_locale_table[ 165 ].length
#define ST_L_Retry_incomplete_parts_					g_locale_table[ 166 ].length
#define ST_L_Server_SSL___TLS_version_					g_locale_table[ 167 ].length
#define ST_L_Timeout__seconds__							g_locale_table[ 168 ].length

// Options General
#define ST_L_Always_on_top								g_locale_table[ 169 ].length
#define ST_L_Close_to_System_Tray						g_locale_table[ 170 ].length
#define ST_L_Enable_System_Tray_icon_					g_locale_table[ 171 ].length
#define ST_L_Enable_URL_drop_window_					g_locale_table[ 172 ].length
#define ST_L_Load_Download_Finish_Sound_File			g_locale_table[ 173 ].length
#define ST_L_Minimize_to_System_Tray					g_locale_table[ 174 ].length
#define ST_L_Play_sound_when_downloads_finish_			g_locale_table[ 175 ].length
#define ST_L_Show_notification_when_downloads_finish	g_locale_table[ 176 ].length
#define ST_L_Show_progress_bar							g_locale_table[ 177 ].length
#define ST_L_Transparency_								g_locale_table[ 178 ].length

// Options Proxy
#define ST_L_Allow_proxy_to_resolve_domain_names		g_locale_table[ 179 ].length
#define ST_L_Allow_proxy_to_resolve_domain_names_v4a	g_locale_table[ 180 ].length
#define ST_L_Hostname_									g_locale_table[ 181 ].length
#define ST_L_SOCKS_v4									g_locale_table[ 182 ].length
#define ST_L_SOCKS_v5									g_locale_table[ 183 ].length
#define ST_L_Use_Authentication_						g_locale_table[ 184 ].length
#define ST_L_Use_HTTP_proxy_							g_locale_table[ 185 ].length
#define ST_L_Use_HTTPS_proxy_							g_locale_table[ 186 ].length
#define ST_L_Use_SOCKS_proxy_							g_locale_table[ 187 ].length

// CMessageBox
#define ST_L_Continue									g_locale_table[ 188 ].length
#define ST_L_No											g_locale_table[ 189 ].length
#define ST_L_Overwrite									g_locale_table[ 190 ].length
#define ST_L_Remember_choice							g_locale_table[ 191 ].length
#define ST_L_Skip										g_locale_table[ 192 ].length
#define ST_L_Skip_remaining_messages					g_locale_table[ 193 ].length
#define ST_L_Yes										g_locale_table[ 194 ].length

// Add URL(s)
#define ST_L_Advanced_AB								g_locale_table[ 195 ].length
#define ST_L_Advanced_BB								g_locale_table[ 196 ].length
#define	ST_L_Authentication								g_locale_table[ 197 ].length
#define ST_L_Cookies									g_locale_table[ 198 ].length
#define ST_L_Cookies_									g_locale_table[ 199 ].length
#define ST_L_Custom										g_locale_table[ 200 ].length
#define ST_L_Download									g_locale_table[ 201 ].length
#define ST_L_Download_directory_						g_locale_table[ 202 ].length
#define ST_L_Download_parts_							g_locale_table[ 203 ].length
#define ST_L_Headers									g_locale_table[ 204 ].length
#define ST_L_Headers_									g_locale_table[ 205 ].length
#define ST_L_Images										g_locale_table[ 206 ].length
#define ST_L_Music										g_locale_table[ 207 ].length
#define ST_L_Password_									g_locale_table[ 208 ].length
#define ST_L_POST_Data									g_locale_table[ 209 ].length
#define ST_L_RegEx_filter_								g_locale_table[ 210 ].length
#define ST_L_Send_POST_Data_							g_locale_table[ 211 ].length
#define ST_L_Simulate_download							g_locale_table[ 212 ].length
#define ST_L_SSL___TLS_version_							g_locale_table[ 213 ].length
#define ST_L_URL_s__									g_locale_table[ 214 ].length
#define ST_L_Username_									g_locale_table[ 215 ].length
#define ST_L_Videos										g_locale_table[ 216 ].length

// Search
#define ST_L_Match_case									g_locale_table[ 217 ].length
#define ST_L_Match_whole_word							g_locale_table[ 218 ].length
#define ST_L_Regular_expression							g_locale_table[ 219 ].length
#define ST_L_Search										g_locale_table[ 220 ].length
#define ST_L_Search_All									g_locale_table[ 221 ].length
#define ST_L_Search_for_								g_locale_table[ 222 ].length
#define ST_L_Search_Next								g_locale_table[ 223 ].length
#define ST_L_Search_Type								g_locale_table[ 224 ].length

// Common
#define ST_L_BTN___										g_locale_table[ 225 ].length
#define ST_L__Simulated_								g_locale_table[ 226 ].length
#define ST_L_Add_URL_s_									g_locale_table[ 227 ].length
#define ST_L_Added										g_locale_table[ 228 ].length
#define ST_L_Allocating_File							g_locale_table[ 229 ].length
#define ST_L_Authorization_Required						g_locale_table[ 230 ].length
#define ST_L_Cancel										g_locale_table[ 231 ].length
#define ST_L_Completed									g_locale_table[ 232 ].length
#define ST_L_Connecting									g_locale_table[ 233 ].length
#define ST_L_Download_speed_							g_locale_table[ 234 ].length
#define ST_L_Download_speed__0_B_s						g_locale_table[ 235 ].length
#define ST_L_Download_speed__0_00_GB_s					g_locale_table[ 236 ].length
#define ST_L_Download_speed__0_00_KB_s					g_locale_table[ 237 ].length
#define ST_L_Download_speed__0_00_MB_s					g_locale_table[ 238 ].length
#define ST_L_Downloading								g_locale_table[ 239 ].length
#define ST_L_Downloads_Have_Finished					g_locale_table[ 240 ].length
#define ST_L_Export_Download_History					g_locale_table[ 241 ].length
#define ST_L_Failed										g_locale_table[ 242 ].length
#define ST_L_File_IO_Error								g_locale_table[ 243 ].length
#define ST_L_Import_Download_History					g_locale_table[ 244 ].length
#define ST_L_Moving_File								g_locale_table[ 245 ].length
#define ST_L_Options									g_locale_table[ 246 ].length
#define ST_L_Paused										g_locale_table[ 247 ].length
#define ST_L_Proxy_Authentication_Required				g_locale_table[ 248 ].length
#define ST_L_Queued										g_locale_table[ 249 ].length
#define ST_L_Restarting									g_locale_table[ 250 ].length
#define ST_L_Save_Download_History						g_locale_table[ 251 ].length
#define ST_L_Skipped									g_locale_table[ 252 ].length
#define ST_L_Stopped									g_locale_table[ 253 ].length
#define ST_L_SSL_2_0									g_locale_table[ 254 ].length
#define ST_L_SSL_3_0									g_locale_table[ 255 ].length
#define ST_L_Timed_Out									g_locale_table[ 256 ].length
#define ST_L_TLS_1_0									g_locale_table[ 257 ].length
#define ST_L_TLS_1_1									g_locale_table[ 258 ].length
#define ST_L_TLS_1_2									g_locale_table[ 259 ].length
#define ST_L_Total_downloaded_							g_locale_table[ 260 ].length
#define ST_L_Total_downloaded__0_B						g_locale_table[ 261 ].length
#define ST_L_Total_downloaded__0_00_GB					g_locale_table[ 262 ].length
#define ST_L_Total_downloaded__0_00_KB					g_locale_table[ 263 ].length
#define ST_L_Total_downloaded__0_00_MB					g_locale_table[ 264 ].length
#define ST_L_URL_										g_locale_table[ 265 ].length
#define ST_L_Update										g_locale_table[ 266 ].length
#define ST_L_Update_Download							g_locale_table[ 267 ].length

// Common Messages
#define ST_L_A_restart_is_required						g_locale_table[ 268 ].length
#define ST_L_A_restart_is_required_allocation			g_locale_table[ 269 ].length
#define ST_L_A_restart_is_required_threads				g_locale_table[ 270 ].length
#define ST_L_PROMPT_delete_selected_files				g_locale_table[ 271 ].length
#define ST_L_PROMPT_remove_completed_entries			g_locale_table[ 272 ].length
#define ST_L_PROMPT_remove_and_delete_selected_entries	g_locale_table[ 273 ].length
#define ST_L_PROMPT_remove_selected_entries				g_locale_table[ 274 ].length
#define ST_L_PROMPT_restart_selected_entries			g_locale_table[ 275 ].length
#define ST_L_One_or_more_files_are_in_use				g_locale_table[ 276 ].length
#define ST_L_One_or_more_files_were_not_found			g_locale_table[ 277 ].length
#define ST_L_Select_the_default_download_directory		g_locale_table[ 278 ].length
#define ST_L_Select_the_download_directory				g_locale_table[ 279 ].length
#define ST_L_Select_the_temporary_download_directory	g_locale_table[ 280 ].length
#define ST_L_The_download_will_be_resumed				g_locale_table[ 281 ].length
#define ST_L_File_is_in_use_cannot_delete				g_locale_table[ 282 ].length
#define ST_L_File_is_in_use_cannot_rename				g_locale_table[ 283 ].length
#define ST_L_File_format_is_incorrect					g_locale_table[ 284 ].length
#define ST_L_PROMPT_The_specified_file_was_not_found	g_locale_table[ 285 ].length
#define ST_L_The_specified_path_was_not_found			g_locale_table[ 286 ].length
#define ST_L_There_is_already_a_file					g_locale_table[ 287 ].length
#define ST_L_You_must_supply_download_directory			g_locale_table[ 288 ].length

#endif
