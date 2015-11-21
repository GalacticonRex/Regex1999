#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "regex.h"

char* open_file( const char* fname ) {
	FILE *stream = fopen(fname, "rb");
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

void display_re( regex* re ) {
	atom* h = re->initial;
	int i = 0;
	while ( i < 30 && h != NULL ) {
		printf ("%p", h );
		fflush( NULL );
		atom* old = h;
		h = old->failure;
		if ( h == NULL ) {
			h = old->success;
			if ( h != NULL ) {
				printf( " --SUCCESS-->\n" );
			}
		} else {
			printf( " --FAIL-->\n" );
		}
		++ i;
	}
	printf( "\n" );
	fflush( NULL );
}

int main( int argc, char** argv ) {
	char* f = open_file( argv[1] );
	
	regex* re = regex_create( f );
	
	if ( re != NULL ) {
		printf( "\n\n\n" );
		//const char* str = "HelloGoodFlower,BushandTree.GoodHelloFlowerIsBushHelloooooooFlowerTreeBushHello";
		const char* str = "HelloooooooAABBCCABCHello";
		substr* loc = regex_match( re, str );
		
		printf( "\n\nMATCHED TO:\n" );
		for ( ;loc->begin!=loc->end;++loc->begin )
			printf( "%c", *loc->begin );
		printf( "\n" );
		
		free( loc );
	}
	
	free( f );
	
	return EXIT_SUCCESS;
}