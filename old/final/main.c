#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "regex.h"

char* open_file( const char* fname ) {
	FILE *stream = fopen(fname, "rb");
	if ( stream == NULL )
		return NULL;
	char *contents;	

	fseek(stream, 0L, SEEK_END);
	int fileSize = ftell(stream);
	fseek(stream, 0L, SEEK_SET);

	contents = (char*) malloc(fileSize+1);

	size_t size = fread( contents, 1, fileSize, stream );
	contents[size] = '\0';

	fclose(stream);
	return contents;
}

int main( int argc, char** argv ) {
	
	char buffer[4096];
	
	char* f;
	char* data;
	uint32 len;
	
	if ( argc == 1 ) {
		
		printf( "Enter some text: " );
		scanf( "%s", buffer );
		
		len = strlen( buffer );
		data = (char*) malloc( len );
		strcpy( data, buffer );
		
		printf( "Enter an expression: " );
		scanf( "%s", buffer );
		
		len = strlen( buffer );
		f = (char*) malloc( len );
		strcpy( f, buffer );
		
	} else if ( argc == 2 ) {
		
		printf( "Enter some text: %s\n", argv[1] );
		
		printf( "Enter an expression: " );
		scanf( "%s", buffer );
		
		len = strlen( buffer );
		f = (char*) malloc( len );
		strcpy( f, buffer );
		
		data = open_file( argv[1] );
		
		if ( data == NULL )
			return EXIT_FAILURE;
		
	} else if ( argc == 3 ) {
		
		printf( "Enter some text: %s\n", argv[2] );
		printf( "Enter an expression: %s\n", argv[1] );
		
		f = open_file( argv[1] );
		data = open_file( argv[2] );
		
		if ( f == NULL || data == NULL )
			return EXIT_FAILURE;
		
	} else {
		return EXIT_FAILURE;
	}
	
	const char* str = data;
	int32 error;
	
	regex* re = regex_create( f, &error );
	
	if ( re != NULL ) {
		
	} else {
		printf( "Regex could not be created: '" );
		switch( error ) {
			
		}
		printf( "'\n" );
	}
	
	free( f );
	free( data );
	
	return EXIT_SUCCESS;
}