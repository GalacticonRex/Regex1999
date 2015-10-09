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
static void stack_set( void* ptr ) {
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

static array* array_create() { 
	array* tmp = (array*) malloc( sizeof(array) );
	tmp->data = NULL;
	tmp->size = 0;
	tmp->alloc = 0;
	return tmp;
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
	ch->data[elem/8] |= ( 0x1 << ( elem % 8 ) );
}
static void charset_add_multiple( charset* ch, int8 elem1, int8 elem2 ) {
	int8 i;
	for ( i=elem1;i<elem2;i++ )
		ch->data[i/8] |= ( 0x1 << ( i % 8 ) );
}
static uint32 charset_check( charset* ch, int8 elem ) {
	return ( ( ch->data[elem/8] & ( 0x1 << ( elem % 8 ) ) ) > 0 );
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
	atom_group_final* end;
};
struct atom_group_final_t {
	atom_group_init* start; // pointer to group start
};
struct atom_start_line_t {
	const char** compare; // dereferenced to the start of the file the
						  // regex is being tested againsts
};

/*------------------------------------------
| Structure Used for Build Data
+-----------------------------------------*/

struct rebuilder_t {
	int no_error;
	stack* groups;
	stack* chain_start;
	stack* chain_current;
};
typedef struct rebuilder_t rebuilder;

static rebuilder* rebuilder_create() {
	rebuilder* re = (rebuilder*) malloc( sizeof(rebuilder) );
	re->no_error = 0;
	re->groups = stack_create();
	re->chain_start = stack_create();
	re->chain_current = stack_create();
	return re;
}

/*------------------------------------------
| Functions to Control Regex Components
+-----------------------------------------*/
static const char* compare_literal( atom* input, const char* str ) {
	if ( (*input->info->working) >= input->info->uppder )
		return str;
	
	atom_literal* lit = (atom_literal*) input->data;
	
	do {
		
		const char* i = lit->data;
		
		for ( ;*i!='\0';++i,++str )
			if ( *i != *str )
				return NULL;
		
		(*input->info->working) ++ ;
		
	} while ( (*input->info->working) < input->info->lower );
	
	return str;
}

static void destroy_literal( void* addr ) {
	free( ((atom_literal*) addr)->data );
	free( (atom_literal*) addr );
}

static const char* compare_group_init( atom* input, const char* str ) {
	atom_group_init* gr = (atom_group_init*) input->data;
	gr->hit = 1;
	return str;
}
static void destroy_group_init( void* addr ) {
	free( (atom_group_init*) addr );
}
static const char* compare_group_final( atom* input, const char* str ) {
	return str;
}
static void destroy_group_final( void* addr ) {
	free( (atom_group_final*) addr );
}

static atom* atom_create( regex* p,
						  recheckfunc c, 
						  void* data,
						  rekillfunc k ) {
	atom* a = (atom*) malloc( sizeof(atom) );
	a->parent = p;
	a->success = NULL;
	a->failure = NULL;
	quantifier* q = (quantifier*) malloc( sizeof(quantifier) );
		q->lower = 1;
		q->upper = 1;
		array_append32( p->working, 0 );
		q->working = (uint32*)p->working->data + p->working->size - sizeof( uint32 );
	a->info = q;
	a->func = c;
	a->dealloc = k;
	a->pself = NULL;
	a->data = data;
	return a;
}
static void atom_destroy( atom* atom ) {
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

static void set_prev( regex* p, rebuilder* b, atom* natom ) {
	if ( p->current == NULL )
		p->initial = natom;
	else if ( stack_size( b->chain_current ) == 0 )
		p->current->success = natom;
	else {
		p->current->failure = natom;
		stack_set( b->chain_current, natom );
	}
	p->current = natom;
}
static void close_literal( regex* p, char* buffer ) {
	if ( *buffer == '\0' )
		return;
	
	atom_literal* lit = (atom_literal*) malloc( sizeof(atom_literal) );
	atom* ret = atom_create( p, compare_literal, lit, destroy_literal );
	
	lit->data = (char*) malloc( sizeof(char) * strlen(buffer) );
	strcpy( lit->data, buffer );
	
	buffer[0] = '\0';
	
	set_prev( p, ret );
}
static void create_group( regex* p, rebuilder* b ) {
	atom_group_init* gr = (atom_group_init*) malloc( sizeof(atom_group_init) );
	gr->hit = false;
	gr->end = NULL;
	atom* ret = atom_create( p, compare_group_init, gr, destroy_group_init );
	
	stack_push( b->groups, gr );
	stack_push( b->chain_start, NULL );
	stack_push( b->chain_current, NULL );
	
	set_prev( p, ret );
}
static void close_group( regex* p, rebuilder* b ) {
	if ( b->groups->size == 0 ) {
		b->no_error = REGEX_EXTRA_CLOSE_GROUP;
		return;
	}
	
	atom_group_final* gr = (atom_group_final*) malloc( sizeof(atom_group_final) );
	atom* ret = atom_create( p, compare_group_final, gr, destroy_group_final );
	
	gr->start = (atom_group_init*) stack_top( b->groups );
	gr->start->end = gr;
	
	atom* st = (atom*) stack_top( b->chain_start );
	atom* cu = (atom*) stack_top( b->chain_current );
	
	while ( st != cu ) {
		st->success = ret;
		st = st->failure;
	}
	cu->success = ret;
	cu = cu->failure;
	
	stack_pop( b->groups );
	stack_pop( b->chain_start );
	stack_pop( b->chain_current );
	
	set_prev( p, ret );
}
static void move_chain( regex* p, rebuilder* b ) {
	if ( p->current == NULL || is_group_init( p->current ) ) {
		b->no_error = REGEX_CHAIN_HAS_NO_INITIAL;
		return;
	}
	
	// a previous chain was already defined
	if ( stack_top( b->chain_start ) != NULL ) {
		
	}
	
	if ( stack_size( b->groups ) == 0 )
		stack_push( b->chain_start, p->initial );
	else
		stack_push( b->chain_start, stack_top( b->groups )->success );
	
	stack_push( b->chain_current, p->current );
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
	char* buffer = (char*) malloc( 80 );
	
	while ( build->no_error == 0 &&  *tmp != '\0' ) {
		if ( is_quantifier( *tmp ) ) {
			
		} else switch ( *tmp ) {
			case '(':
				close_literal( m, buffer );
				create_group( m, build );
				++ tmp;
				break;
			case ')':
				close_literal( m, buffer );
				close_group( m, build );
				++ tmp;
				break;
			case '|':
				close_literal( m, buffer );
				move_chain( m, build );
				++ tmp;
				break;
			/*case '.':
				
			case '^':
				
			case '$':
				
			case '[':
				
			case ']':
			*/	
			default:
				printf( "DEFAULT" );
		}
	}
	
	return NULL;
}