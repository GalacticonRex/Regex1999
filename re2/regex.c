#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "regex.h"

/*------------------------------------------
| Functions to Control Data Structures
+-----------------------------------------*/
static inline uint32 max32( uint32 a, uint32 b ) {
	return ( a > b ) ? (a) : (b);
}

static stack* stack_create() {
	stack* tmp = (stack*) malloc( sizeof(stack) );
	tmp->data = NULL;
	tmp->size = 0;
	tmp->alloc = 0;
	return tmp;
}
static void stack_clear( stack* st ) {
	st->size = 0;
}
static void stack_set( stack* st, void* ptr ) {
	if ( st->size == 0 )
		return;
	st->data[st->size-1] = ptr;
}
static void* stack_top( stack* st ) {
	if ( st->size == 0 )
		return NULL;
	return st->data[st->size-1];
}
static void stack_push( stack* st, void* ptr ) {
	if ( st->alloc <= st->size + 1 ) {
		uint32 tmp = max32( 4, st->alloc * 2 );
		st->data = (void**) realloc( st->data, tmp * sizeof( void* ) );
		if ( st->data == NULL ) {
			printf( "could not re-allocate stack to %d elements", tmp );
			exit( -18 );
		}
		st->alloc = tmp;
	}
	st->data[st->size] = ptr;
	st->size ++ ;
}
static void* stack_pop( stack* st ) {
	if ( st->size == 0 )
		return NULL;
	void* tmp = stack_top( st );
	st->size -- ;
	return tmp;
}
static int stack_size( stack* st ) {
	return st->size;
}

static array* array_create() { 
	array* tmp = (array*) malloc( sizeof(array) );
	tmp->data = NULL;
	tmp->size = 0;
	tmp->alloc = 0;
	return tmp;
}
static void array_clear8( array* arr, uint8 val ) {
	uint32 i;
	for ( i=0;i<arr->size;++i )
		arr->data[i] = val;
}
static void array_clear32( array* arr, uint32 val ) {
	uint32 i;
	uint32* data = (uint32*) arr->data;
	uint32 len = arr->size / sizeof( uint32 );
	for ( i=0;i<len;++i )
		data[i] = val;
}
static uint8 array_get8( array* arr, uint32 index ) {
	if ( index >= arr->size )
		return 0;
	return arr->data[index];
}
static uint32 array_get32( array* arr, uint32 index ) {
	uint32 r_id = (index+1) * sizeof( uint32 );
	if ( r_id > arr->size )
		return 0;
	return ((uint32*)arr->data)[index];
}
static void array_realloc( array* arr, uint32 size ) {
	if ( arr->alloc <= arr->size + size ) {
		uint32 tmp = max32( 16, arr->alloc * 2 );
		arr->data = (uint8*) realloc( arr->data, tmp );
		if ( arr->data == NULL ) {
			printf( "could not re-allocate array to %d bytes", tmp );
			exit( -17 );
		}
		arr->alloc = tmp;
	}
}
static void array_append8( array* arr, uint8 object ) {
	array_realloc( arr, 1 );
	arr->data[arr->size] = object;
	arr->size ++ ;
}
static void array_append32( array* arr, uint32 object ) {
	array_realloc( arr, sizeof( uint32 ) );
	uint32 offset = arr->size % sizeof( uint32 );
	((uint32*)(arr->data+offset))[arr->size/sizeof(uint32)] = object;
	arr->size += sizeof( uint32 );
}


static charset* charset_create() {
	charset* tmp = (charset*) malloc( sizeof(charset) );
	memset( tmp->data, 0, sizeof(charset) );
	return tmp;
}
static void charset_add( charset* ch, int8 elem ) {
	ch->data[elem/32] |= ( (uint32)(1) << ( elem % 32 ) );
}
static void charset_add_multiple( charset* ch, int8 elem1, int8 elem2 ) {
	int8 i;
	for ( i=elem1;i<elem2;i++ )
		ch->data[i/32] |= ( (uint32)(1) << ( i % 32 ) );
}
static uint32 charset_check( charset* ch, int8 elem ) {
	uint32 big = elem/32;
	uint32 small = elem % 32;
	uint32 d = ch->data[big];
	uint32 off = ( (uint32)(1) << small );
	return ( ( d & off ) > 0 );
}

static charset* charset_any() {
	static charset self;
	if ( self.data[0] == 0 ) {
		self.refcount = 0;
		uint32 i;
		for ( i=0;i<8;i++ )
			self.data[i] = 0xFFFFFFFF;
	}
	return &self;
}

/*------------------------------------------
| Atom Extenders
+-----------------------------------------*/

struct atom_literal_t;
struct atom_charset_t;
struct atom_group_init_t;
struct atom_group_final_t;
struct atom_start_line_t;

typedef struct atom_literal_t atom_literal;
typedef struct atom_charset_t atom_charset;
typedef struct atom_group_init_t atom_group_init;
typedef struct atom_group_final_t atom_group_final;
typedef struct atom_start_line_t atom_start_line;

struct atom_literal_t {
	char* data; // literal for comparison
};
struct atom_charset_t {
	bool exclude; // set true if the charset is to be excluded rather than included
	charset* checkset; // collection of characters
};
struct atom_group_init_t {
	bool hit; // has the group been entered
	atom* end;
};
struct atom_group_final_t {
	uint32 scandex;
	atom* start; // pointer to group start
};
struct atom_start_line_t {
	const char** compare; // dereferenced to the start of the file the
						  // regex is being tested againsts
};

/*------------------------------------------
| Structure Used for Build Data
+-----------------------------------------*/

struct rebuilder_t {
	int error;		// error value
	stack* root;	// group origin (atom*)
	stack* subroot;	// chain origin (atom*)
};
typedef struct rebuilder_t rebuilder;

static rebuilder* rebuilder_create() {
	rebuilder* re = (rebuilder*) malloc( sizeof(rebuilder) );
	re->error = 0;
	re->root = stack_create();
	re->subroot = stack_create();
	return re;
}

/*------------------------------------------
| Functions to Control Regex Components
+-----------------------------------------*/
static const char* compare_literal( atom** reference, const char* str ) {
	atom* input = *reference;
	
	uint32* working = (uint32*)(input->parent->working->data + input->info->working);
	if ( input->info->upper != -1 && *working >= input->info->upper ) {
		(*reference) = input->failure;
		return str;
	}
	
	atom_literal* lit = (atom_literal*) input->data;
	
	const char* istr = str;
	do {
		
		const char* i = lit->data;
		printf( "Test this string: '%s'\n", i );
		
		for ( ;*i!='\0';++i,++str ) {
			printf( "Check %c vs %c\n", *i, *str );
			if ( *i != *str ) {
				(*reference) = input->failure;
				return istr;
			}
		}
		
		++ *working;
		
	} while ( *working < input->info->lower );
	
	if ( input->info->upper == -1 || *working < input->info->upper ) {
		stack_push( input->parent->track_atom, (void*)input );
		stack_push( input->parent->track_string, (void*)str );
	}
	
	(*reference) = input->success;
	return str;
}
static void destroy_literal( void* addr ) {
	free( ((atom_literal*) addr)->data );
	free( (atom_literal*) addr );
}
static int print_literal( void* addr ) {
	printf( "lit:'%s'", ((atom_literal*)addr)->data );
	return 0;
}


static const char* compare_charset( atom** reference, const char* str ) {
	atom* input = *reference;
	
	uint32* working = (uint32*)(input->parent->working->data + input->info->working);
	if ( input->info->upper != -1 && *working >= input->info->upper ) {
		(*reference) = input->failure;
		return str;
	}
	
	atom_charset* data = (atom_charset*) input->data;
	charset* cset = data->checkset;
	
	do {
		
		printf( "Test this charset %c\n", *str );
		uint32 found = charset_check( cset, *str );
		if ( (data->exclude && found) || (!(data->exclude)&&!found) ) {
			(*reference) = input->failure;
			return str;
		}
		
		++ str;
		++ *working;
		
	} while ( *working < input->info->lower );
	
	if ( input->info->upper == -1 || *working < input->info->upper ) {
		stack_push( input->parent->track_atom, (void*)input );
		stack_push( input->parent->track_string, (void*)str );
	}
	
	printf( "Charset matched!\n" );
	(*reference) = input->success;
	return str;
}
static void destroy_charset( void* addr ) {
	atom_charset* data = (atom_charset*) addr;
	charset* cset = data->checkset;
	free( data );
	if ( -- cset->refcount <= 0 )
		free( cset );
}
static int print_charset( void* addr ) {
	charset* cset = ((atom_charset*) addr)->checkset;
	printf( "cset:'%x %x %x %x %x %x %x %x'(%d)", cset->data[0],
												  cset->data[1],
												  cset->data[2],
												  cset->data[3],
												  cset->data[4],
												  cset->data[5],
												  cset->data[6],
												  cset->data[7],
												  cset->refcount );
	return 0;
}


static const char* compare_group_init( atom** reference, const char* str ) {
	atom* input = *reference;
	atom_group_init* gr = (atom_group_init*) input->data;
	gr->hit = 1;
	
	uint32* working = (uint32*)(input->parent->working->data + input->info->working);
	if ( input->info->upper == -1 || *working < input->info->upper )
		(*reference) = input->success;
	else
		(*reference) = gr->end;
	
	return str;
}
static void destroy_group_init( void* addr ) {
	free( (atom_group_init*) addr );
}
static int print_group_init( void* addr ) {
	printf( "group:initial" );
	return 0;
}


static const char* compare_group_final( atom** reference, const char* str ) {
	atom* input = *reference;
	
	atom_group_final* gr = (atom_group_final*) input->data;
	atom* init = gr->start;
	atom_group_init* begin = (atom_group_init*) init->data;
	
	uint32* working = (uint32*)(input->parent->working->data + input->info->working);
	if ( begin->hit ) {
		++ (*working);
		begin->hit = 0;
	}
	
	if ( *working < input->info->lower ) {
		regex* parent = input->parent;
		
		uint32 start = input->info->working / sizeof( uint32 );
		uint32 end = gr->scandex / sizeof( uint32 );
		
		for ( ;start<end;++start )
			((uint32*)parent->working->data)[start] = 0;
		
		(*reference) = init;
	} else {
		if ( *working == -1 || *working < input->info->upper ) {
			stack_push( input->parent->track_atom, (void*)init );
			stack_push( input->parent->track_string, (void*)str );
		}
		(*reference) = input->success;
	}
	
	return str;
}
static void destroy_group_final( void* addr ) {
	free( (atom_group_final*) addr );
}
static int print_group_final( void* addr ) {
	printf( "group:final" );
	return 0;
}






static atom* atom_create( regex* p,
						  recheckfunc c, 
						  void* data,
						  rekillfunc k,
						  reprintfunc r,
						  quantifier* quan ) {
	atom* a = (atom*) malloc( sizeof(atom) );
	a->parent = p;
	a->success = NULL;
	a->failure = NULL;
	if ( quan == NULL && data != NULL ) {
		quantifier* q = (quantifier*) malloc( sizeof(quantifier) );
			q->lower = 1;
			q->upper = 1;
			q->working = p->working->size;
			printf( "Working: %d\n", q->working / sizeof( uint32 ) );
			array_append32( p->working, 0 );
		a->info = q;
	} else
		a->info = quan;
	a->func = c;
	a->dealloc = k;
	a->pself = r;
	a->data = data;
	return a;
}
static void atom_destroy( atom* atom ) {
	if ( atom->dealloc != NULL )
		atom->dealloc( atom->data );
	free( atom->info );
	free( atom );
}
static bool is_literal( atom* a ) {
	return ( a->func == compare_literal );
}
static bool is_group_init( atom* a ) {
	return ( a->func == compare_group_init );
}
static bool is_group_final( atom* a ) {
	return ( a->func == compare_group_final );
}
static bool is_quantifier( char n ) {
	return  ( n == '{' || 
			  n == '?' || 
			  n == '*' || 
			  n == '+' );
}

// Link the current atom to a new incoming atom
static void link_to_prev( regex* re, atom* natom ) {
	if ( re->initial == NULL )
		re->initial = natom;
	
	if ( re->current == NULL )
		re->current = natom;
	else
		re->current->success = natom;
	
	re->current = natom;
}

// Iterates backwards through the subroots to build a failure OR chain
static void link_options( regex* re, rebuilder* build, atom* final ) {
	atom* root = stack_top( build->root );
	atom* endsub = stack_top( build->subroot );
	while ( endsub != root ) {
		atom* tmp = endsub->success;
		
		stack_pop( build->subroot );
		atom* beginsub = stack_top( build->subroot );
		atom* iterate = beginsub->success;
		printf( "    Link %p to %p\n", iterate, endsub );
		
		while ( iterate != tmp ) {
			printf( "    %p fails to %p\n", iterate, tmp );
			iterate->failure = tmp;
			iterate = iterate->success;
		}
		
		endsub->success = final;
		endsub = beginsub;
	}
}


static void close_regex( regex* re ) {
	atom* empty = atom_create( re, NULL, NULL, NULL, NULL, NULL );
	link_to_prev( re, empty );
	printf( "%p Close Regex\n", empty );
}
// Terminate a literal and create an atom for it
static void close_literal( regex* re, const char* buffer, uint32_out size ) {
	if ( *size == 0 )
		return;
	
	atom_literal* lit = (atom_literal*) malloc( sizeof(atom_literal) );
	lit->data = (char*) malloc( sizeof(char) * (*size+1) );
	memcpy( lit->data, buffer, *size );
	lit->data[*size] = '\0';
	
	atom* ret = atom_create( re, 
							 compare_literal, 
							 lit, 
							 destroy_literal, 
							 print_literal, 
							 NULL );
	
	*size = 0;
	
	link_to_prev( re, ret );
	
	printf( "%p = '%s'\n", ret, lit->data );
}

// Create an open group node
static void create_group( regex* re, rebuilder* build ) {
	atom_group_init* gr = (atom_group_init*) malloc( sizeof(atom_group_init) );
	gr->hit = 0;
	gr->end = NULL;
	atom* ret = atom_create( re,
							 compare_group_init, 
							 gr, 
							 destroy_group_init, 
							 print_group_init,
							 NULL );
	
	// assign new root
	stack_push( build->root, ret );
	stack_push( build->subroot, ret );
	
	link_to_prev( re, ret );
	
	printf( "%p Group\n", ret );
	printf( "    With Subroot %p\n", ret );
}

// Create a close group node and bind it to the open group node
static void close_group( regex* re, rebuilder* build ) {
	if ( build->root->size == 0 ) {
		build->error = REGEX_EXTRA_CLOSE_GROUP;
		return;
	}
	
	atom_group_final* gr = (atom_group_final*) malloc( sizeof(atom_group_final) );
	gr->start = (atom*) stack_top( build->root );
	gr->scandex = re->working->size;
	atom* ret = atom_create( re,
							 compare_group_final, 
							 gr, 
							 destroy_group_final, 
							 print_group_final,
							 gr->start->info );
	
	((atom_group_init*)(gr->start->data))->end = ret;
	
	link_to_prev( re, ret );
	
	link_options( re, build, ret );
	stack_pop( build->root );
	
	printf( "%p Close %p\n", ret, gr->start );
}
static void move_chain( regex* re, rebuilder* build ) {
	if ( re->current == NULL || is_group_init( re->current ) ) {
		build->error = REGEX_CHAIN_HAS_NO_INITIAL;
		return;
	}
	
	printf( "    With Subroot %p\n", re->current );
	
	stack_push( build->subroot, re->current );
}
static const char* create_charset( regex* re, rebuilder* build, const char* str ) {
	if ( *str == '.' ) {
		charset* cset = charset_any();
		++ cset->refcount;
		
		atom_charset* data = (atom_charset*) malloc( sizeof( atom_charset ) );
		data->exclude = 0;
		data->checkset = cset;
		
		atom* ret = atom_create( re,
								 compare_charset,
								 data,
								 destroy_charset,
								 print_charset,
								 NULL );
		link_to_prev( re, ret );
		return ++ str;
	} else {
		charset* cset = charset_create();
		cset->refcount = 1;
		
		atom_charset* data = (atom_charset*) malloc( sizeof( atom_charset ) );
		data->checkset = cset;
		
		if ( *str == '^' ) {
			data->exclude = 1;
			++ str;
		} else
			data->exclude = 0;
		
		char last2 = '\0';
		char last = *str;
		++ str;
		for ( ;*str!='\0'&&*str!=']';++str ) {
			if ( last == '\0' )
				continue;
			if ( last == '-' ) {
				if ( last2 == '\0' || *str == '-' || *str < last2 ) {
					build->error = REGEX_INCORRECT_CHAR_SPAN;
					return str;
				}
				charset_add_multiple( cset, last2, *str );
				last2 = '\0';
				last = '\0';
			} else {
				charset_add( cset, last );
				last2 = last;
				last = *str;
			}
		}
		
		if ( last != '\0' )
			charset_add( cset, last );
		
		if ( *str==']' )
			++ str;
		
		atom* ret = atom_create( re,
								 compare_charset,
								 data,
								 destroy_charset,
								 print_charset,
								 NULL );
		link_to_prev( re, ret );
		
		return str;
	}
}


void assign_quantifier( regex* re, rebuilder* build, int low, int high ) {
	if ( re->current == NULL ) {
		build->error = REGEX_QUANTIFIER_TO_NONE;
		return;
	}
	
	re->current->info->lower = low;
	re->current->info->upper = high;
	
	printf( "    Assign Quantifier {%d,%d} to %p\n", low, high, re->current );
}
const char* assign_quan_span( regex* re, rebuilder* build, const char* init ) {
	if ( *init != '{' )
		return init;
	++ init;
	
	char buffer[80];
	
	// clear out whitespace
	for ( ;*init==' '||*init=='\t';++init );
	
	char* iter = buffer;
	for ( ;*init!=' '&&*init!='\t'&&*init!=','&&*init!='}';++init ) {
		if ( *init < '0' || *init > '9' ) {
			build->error = REGEX_SYNTAX_ERROR;
			return init;
		}
		*iter = *init;
		++ iter;
	}
	*iter = '\0';
	int low = atoi(buffer);
	int high = low;
	
	for ( ;*init==' '||*init=='\t';++init );
	
	if ( *init == ',' ) {
	
		++ init;
		for ( ;*init==' '||*init=='\t';++init );
		
		iter = buffer;
		for ( ;*init!=' '&&*init!='\t'&&*init!=','&&*init!='}';++init ) {
			if ( *init < '0' || *init > '9' ) {
				build->error = REGEX_SYNTAX_ERROR;
				return init;
			}
			*iter = *init;
			++ iter;
		}
		*iter = '\0';
		high = atoi(buffer);
		
		for ( ;*init==' '||*init=='\t';++init );
		
		if ( *init != '}' ) {
			build->error = REGEX_SYNTAX_ERROR;
			return init;
		}
		
	} else if ( *init != '}' ) {
		build->error = REGEX_SYNTAX_ERROR;
		return init;
	}
	
	assign_quantifier( re, build, low, high );
	
	return init;
}

/* --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- 
   --- MAIN REGEX FUNCTIONS -----------------------------------------------
    --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- */
regex* regex_create( const char* cc ) {
	regex* m = (regex*) malloc( sizeof(regex) );
	
	m->strstart = NULL;
	m->initial = NULL;
	m->current = NULL;
	
	m->track_atom = stack_create();
	m->track_string = stack_create();
	m->working = array_create();
	
	rebuilder* build = rebuilder_create();
	
	const char* tmp = cc;
	const char* buffer = tmp;
	uint32 size = 0;
	
	while ( build->error == 0 &&  *tmp != '\0' ) {
		if ( is_quantifier( *tmp ) ) {
			if ( size > 0 ) {
				-- size;
				const char* next = buffer + size;
				close_literal( m, buffer, &size );
				size = 1;
				close_literal( m, next, &size );
			}
			
			switch ( *tmp ) {
				case '*':
					assign_quantifier( m, build, 0, -1 );
					break;
				case '?':
					assign_quantifier( m, build, 0, 1 );
					break;
				case '+':
					assign_quantifier( m, build, 1, -1 );
					break;
				case '{':
					tmp = assign_quan_span( m, build, tmp );
					break;
			}
			
			++ tmp;
			buffer = tmp;
		} else switch ( *tmp ) {
			case '(':
				close_literal( m, buffer, &size );
				create_group( m, build );
				++ tmp;
				buffer = tmp;
				break;
			case ')':
				close_literal( m, buffer, &size );
				close_group( m, build );
				++ tmp;
				buffer = tmp;
				break;
			case '|':
				close_literal( m, buffer, &size );
				move_chain( m, build );
				++ tmp;
				buffer = tmp;
				break;
			case '[':
				++ tmp;
			case '.':
				close_literal( m, buffer, &size );
				tmp = create_charset( m, build, tmp );
				buffer = tmp;
				break;
			case ']':
				build->error = REGEX_EXTRA_CLOSE_CHARSET;
				break;
			/*
			case '^':
			case '$':
			*/	
			default:
				++ size;
				++ tmp;
		}
	}
	
	if ( build->error != 0 ) {
		printf( "ERROR %d!\n", build->error );
		return NULL;
	} else {
		close_literal( m, buffer, &size );
		close_regex( m );
	}
	
	return m;
}

static atom* regex_reset( regex* re, const char* val ) {
	re->strstart = val;
	array_clear32( re->working, 0 );
	stack_clear( re->track_atom );
	stack_clear( re->track_string );
	return re->initial;
}
substr* regex_match( regex* re, const char* str ) {
	atom* this = regex_reset( re, str );
	
	printf( "Begin search with %p\n", this );
	printf( "Array %p to %p\n", re->working->data, re->working->data + re->working->size );
	
	const char* istr = str;
	uint32 iter = 0;
	while ( *str != '\0' && iter < 80 && this->data != NULL ) {
		atom* old = this;
		const char* out = this->func( &this, str );
		if ( this == NULL ) { // match failed
			if ( stack_size( re->track_atom ) != 0 ) {
				
				atom* topatom = (atom*) stack_top( re->track_atom );
				const char* topstr = (const char*) stack_top( re->track_string );
				
				uint32 begin = topatom->info->working / sizeof( uint32 ) + 1;
				uint32 end = old->info->working / sizeof( uint32 );
				
				printf( "RESETTING TO TOP OF STACK %p AND %c (CLEARED %d TO %d)\n", topatom, *topstr, begin, end );
				
				for ( ;begin<=end;++begin )
					((uint32*)re->working->data)[begin] = 0;
				
				this = topatom;
				str = topstr;
				
			} else {
				
				printf( "Match fail for %p; RESETTING\n", this );
				++ istr;
				this = regex_reset( re, istr );
				str = istr;
				
			}
		} else {
			str = out;
		}
		fflush( NULL );
		++ iter;
	}
	if ( istr == '\0' ) {
		printf( "COULD NOT FIND!" );
		return NULL;
	} else {
		printf( "MATCH FOUND!" );
		substr* sub = (substr*) malloc( sizeof( substr ) );
		sub->begin = istr;
		sub->end = str;
		return sub;
	}
}

int regex_destroy( regex* re ) {
	return 0;
}