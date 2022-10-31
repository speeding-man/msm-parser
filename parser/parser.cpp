#include <Windows.h>
#include <string>
#include <regex>
#include <vector>
#include <fstream>
#include <filesystem>

std::vector <std::string> parse_files { };
std::fstream read_file { };
std::ofstream out_file { };
std::wstring current_dir { };
std::string search_string { };
std::string ymir_work_path_name { };

void Print ( const char *format, ... )
{
	va_list args;
	va_start ( args, format );
	SetConsoleTextAttribute ( GetStdHandle ( STD_OUTPUT_HANDLE ), 0xC );
	vprintf ( format, args );
	SetConsoleTextAttribute ( GetStdHandle ( STD_OUTPUT_HANDLE ), 0xF );
	va_end ( args );
}

void Print ( const wchar_t *format, ... )
{
	va_list args;
	va_start ( args, format );
	SetConsoleTextAttribute ( GetStdHandle ( STD_OUTPUT_HANDLE ), 0xC );
	vwprintf ( format, args );
	SetConsoleTextAttribute ( GetStdHandle ( STD_OUTPUT_HANDLE ), 0xF );
	va_end ( args );
}

void ReplaceString ( std::string &string, const std::string &search, const std::string &replace )
{
	for ( size_t pos = 0; ; pos += replace.length ( ) )
	{
		pos = string.find ( search, pos );
		if ( pos == std::string::npos )
			break;

		string.erase ( pos, search.length ( ) );
		string.insert ( pos, replace );
	}
}

int main ( )
{
	FILE *file = nullptr;
	bool overwrite = false;
	bool file_exists = std::filesystem::exists ( "list.txt" );

	AllocConsole ( );
	SetConsoleTitleW ( L"Parser" );

	errno_t err = freopen_s ( &file, "CONOUT$", "w", stdout );

	current_dir = GetCommandLineW ( );

	if ( current_dir.front ( ) == 0x22 )
	{
		current_dir.erase ( current_dir.begin ( ) );
		current_dir.pop_back ( );
	}

	if ( current_dir.empty ( ) )
		return 0;

	while ( true )
	{
		current_dir.pop_back ( );

		if ( current_dir.back ( ) == 0x5c || current_dir.back ( ) == 0x2f )
			break;

		if ( current_dir.empty ( ) )
			return 0;
	}

	Print ( L"Parse directory: %ws\n", current_dir.c_str ( ) );

	if ( file_exists && MessageBoxW ( nullptr, L"Overwrite list.txt?", L"", MB_YESNO ) == IDYES )
		overwrite = true;

	for ( const auto &currfile : std::filesystem::directory_iterator ( current_dir.c_str ( ) ) )
		if ( strcmp ( currfile.path ( ).filename ( ).string ( ).c_str ( ), "list.txt" ) )
			parse_files.push_back ( currfile.path ( ).filename ( ).string ( ) );

	std::smatch _smatch;
	std::regex _regex ( "\x22\x5b\x5e\x22\x5c\x72\x5c\x6e\x5d\x2a\x22" );

	out_file.open ( "list.txt", overwrite ? std::ios_base::trunc : std::ios_base::app );

	for ( auto &file : parse_files )
	{
		if ( !out_file.is_open ( ) )
			out_file.open ( "list.txt", std::ios_base::app );

		read_file.open ( file.c_str ( ) );

		if ( !read_file.good ( ) )
		{
			read_file.close ( );
			out_file.close ( );
			continue;
		}

		while ( std::getline ( read_file, search_string ) )
		{
			while ( std::regex_search ( search_string, _smatch, _regex ) )
			{
				for ( auto &match : _smatch )
				{
					std::string sanitized_string = match.str ( );

					if ( sanitized_string.front ( ) != 0x22 )
						continue;

					sanitized_string.erase ( sanitized_string.begin ( ) );
					sanitized_string.pop_back ( );

					if ( sanitized_string.empty ( ) )
						continue;

					std::transform ( sanitized_string.begin ( ), sanitized_string.end ( ), sanitized_string.begin ( ), [ ] ( unsigned char c ) { return std::tolower ( c ); } );
					ReplaceString ( sanitized_string, "\\", "/" );

					if ( sanitized_string.front ( ) == 0x64 )
					{
						if ( sanitized_string.back ( ) != 0x2f )
						{
							out_file << ymir_work_path_name + sanitized_string + "\n";
							continue;
						}

						ymir_work_path_name = sanitized_string;
					}

					if ( sanitized_string == ymir_work_path_name )
						continue;

					out_file << ymir_work_path_name + sanitized_string + "\n";
				}
				search_string = _smatch.suffix ( ).str ( );
			}
		}

		ymir_work_path_name.clear ( );

		if ( out_file.is_open ( ) )
			out_file.close ( );

		if ( read_file.is_open ( ) )
			read_file.close ( );

		Print ( "%s\n", file.c_str ( ) );
	}

	if ( out_file.is_open ( ) )
		out_file.close ( );

	if ( file_exists )
		Sleep ( 2000 );
	else
		MessageBoxW ( nullptr, L"Done", L"", 0 );

	fclose ( file );

	FreeConsole ( );

	return 1;
}
