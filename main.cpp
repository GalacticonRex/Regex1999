#include "re/regex.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char* open_file( const char* fname );
void print_substring( std::ofstream& out, regex_substring& str );

const char* re_file_tmp = 	"^(Cat|Dog|Fish): [abc]{2,5}[A-Z]*$";
const char* re_text_tmp = 	"Monkey: abcabcSEENOEVIL\n"
							"Monkey: cbacbaHEARNOEVIL\n"
							"Monkey: bacbacSPEAKNOEVIL\n"
							"Cat: aaabCATFISH\n"
							"Dog: abcccDOGFISH\n"
							"Monkey: abcabcSEENOEVIL\n"
							"Monkey: cbacbaHEARNOEVIL\n"
							"Monkey: bacbacSPEAKNOEVIL\n"
							"Cat: aCATFISH Cat: bCATFISH\n"
							"Fish: ccccFISHFISH\n"
							"Fish: ccccFISH FISH\n"
							"Elephant: ELEPHANT\n"
							"Cat: aaCATCATCAT\n"
							"Cat: cabcabcab\n";

int main( int argc, char** argv ) {
	const char* re_file;
	const char* re_text;
	
	if ( argc != 3 ) {
		re_file = re_file_tmp;
		re_text = re_text_tmp;
	} else {
		re_file = open_file( argv[1] );
		re_text = open_file( argv[2] );
	}
	
	std::ofstream sout( "output.txt" );
	auto tmp = std::cout.rdbuf();
	std::cout.rdbuf( sout.rdbuf() );
	
	std::cout << re_file << std::endl;
	std::cout << re_text << std::endl;
	
	regex* re = regex_create( re_file );
	
	std::cout << "BLAH" << std::endl;
	
	array_t<regex_substring> output;
	regex_search( re, re_text, output );
	
	std::cout << "BLAH" << std::endl;
	
	std::ofstream out( "matches.txt" );
	for ( uint i=0;i<output.size();i++ )
		print_substring( out, output[i] );
	out.close();
	
	std::cout.rdbuf( tmp );
	sout.close();
	
	return 0;
}

const char* open_file( const char* fname ) {
	FILE* f = fopen( fname, "rb" );
	fseek(f, 0, SEEK_END);
	long fsize = ftell(f);
	fseek(f, 0, SEEK_SET);
	char* str = (char*) malloc( fsize + 1 );
	fread( str, fsize, 1, f );
	fclose( f );
	str[fsize] = '\0';
	return str;
}
void print_substring( std::ofstream& out, regex_substring& str ) {
	if ( str.begin == NULL )
		return;
	const char* a = str.begin;
	const char* b = str.end;
	out << "LN: ";
	out << std::setw(4) << str.line_number;
	out << " | CH: ";
	out << std::setw(6) << str.char_number;
	out << " || \"";
	for ( ;b!=a;++a )
		out << *a;
	out << "\"" << std::endl;
}