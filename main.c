#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "regex.h"

char* open_file( const char*, int32* );
void regex_display( regex* re );

int main( int argc, char** argv ) {
	
	char buffer[4096];
	
	char* f;
	char* data;
	int32 flen;
	int32 datalen;
	
	if ( argc == 1 ) {
		
		printf( "Enter some text: " );
		scanf( "%s", buffer );
		
		datalen = strlen( buffer );
		data = (char*) malloc( datalen+1 );
		strcpy( data, buffer );
		
		printf( "Enter an expression: " );
		scanf( "%s", buffer );
		
		flen = strlen( buffer );
		f = (char*) malloc( flen+1 );
		strcpy( f, buffer );
		
	} else if ( argc == 2 ) {
		
		printf( "Enter some text: %s\n", argv[1] );
		
		printf( "Enter an expression: " );
		scanf( "%s", buffer );
		
		flen = strlen( buffer );
		f = (char*) malloc( flen+1 );
		strcpy( f, buffer );
		
		data = open_file( argv[1], &datalen );
		
		if ( data == NULL )
			return EXIT_FAILURE;
		
	} else if ( argc == 3 ) {
		
		printf( "Enter some text: %s\n", argv[2] );
		printf( "Enter an expression: %s\n", argv[1] );
		
		f = open_file( argv[1], &flen );
		data = open_file( argv[2], &datalen );
		
		if ( f == NULL || data == NULL )
			return EXIT_FAILURE;
		
	} else {
		return EXIT_FAILURE;
	}
	
	char buf1 = '\0', 
		 buf2 = '\0';
	
	if ( flen > 1024 ) {
		buf1 = f[800];
		f[800] = '\0';
	} if ( datalen > 1024 ) {
		buf2 = data[800];
		data[800] = '\0';
	}
	
	printf( "\n----- Text/Expression\n\n" );
	printf( "\n======\n" );
	printf( "\n\n----- Compiled RegEx\n\n" );
	
	if ( flen > 1024 )
		f[800] = buf1;
	if ( datalen > 1024 )
		data[800] = buf2;
	
	re_error_t error;
	regex* re = regex_create( f, &error );
	
	printf( "%p\n", re );
	
	if ( re != NULL ) {
		
		FILE* output = fopen( "coutput", "w" );
		
		regex_display( re );
		
		uint32 len;
		clock_t t1, t2;
		t1 = clock();
		substr** array = regex_search( re, data, &len );
		t2 = clock();
		
		printf( "\n----- Search Results (%d ms)\nFound %d %s\n", (int32)(t2-t1), len, (len==1)?"Match":"Matches" );
		
		uint32 i;
		for ( i=0;i<len;++i ) {
			substr* sub = array[i];
			fprintf( output, "%3d >>> ", i+1 );
			for ( ;sub->begin!=sub->end;++sub->begin )
				fprintf( output, "%c", *sub->begin );
			fprintf( output, "\n" );
			free( sub );
		}
		free( array );
		
		regex_destroy( re );
		
		fclose( output );
		
	} else {
		printf( ">>> %s\n    ", f );
		uint32 i = 0;
		for ( ;i<error.position;++i )
			printf( " " );
		printf( "^\n" );
		printf( "Compile Error: " );
		switch( error.error ) {
			case REGEX_SYNTAX_ERROR:
				printf("invalid syntax" );
				break;
			case REGEX_EXTRA_CLOSE_GROUP:
				printf("invalid ')'" );
				break;
			case REGEX_EXTRA_CLOSE_CHARSET:
				printf("invalid ']'" );
				break;
			case REGEX_CHAIN_HAS_NO_INITIAL:
				printf("invalid '|'" );
				break;
			case REGEX_QUANTIFIER_TO_NONE:
				printf("quantifier has no associated atom" );
				break;
			case REGEX_INCORRECT_CHAR_SPAN:
				printf("character set incorrectly defined" );
				break;
			default:
				printf("unknown error" );
		}
		printf( "\n" );
	}
	
	free( f );
	free( data );
	
	return EXIT_SUCCESS;
}

char* open_file( const char* fname, int32* size ) {
	FILE *stream = fopen(fname, "rb");
	if ( stream == NULL ) {
		(*size) = 0;
		return NULL;
	}
	char *contents;	
	
	fseek(stream, 0L, SEEK_END);
	(*size) = ftell(stream);
	fseek(stream, 0L, SEEK_SET);

	contents = (char*) malloc(*size+1);

	size_t lsize = fread( contents, 1, *size, stream );
	contents[lsize] = '\0';

	fclose(stream);
	return contents;
}