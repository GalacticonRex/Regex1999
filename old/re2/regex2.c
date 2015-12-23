#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "regex2.h"

/*------------------------------------------
| Functions to Control Data Structures
+-----------------------------------------*/
static inline uint32 max32( uint32 a, uint32 b ) {
	return ( a > b ) ? (a) : (b);
}

static array* array_create() { 
	array* tmp = (array*) malloc( sizeof(array) );
	tmp->data = NULL;
	tmp->size = 0;
	tmp->alloc = 0;
	return tmp;
}
static void array_free( array* arr ) {
	free( arr->data );
	free( arr );
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
static void* array_getptr( array* arr, uint32 index ) {
	uint32 r_id = (index+1) * sizeof( void* );
	if ( r_id > arr->size )
		return 0;
	return ((void**)arr->data)[index];
}

static uint32 array_size32( array* arr ) {
	return arr->size / sizeof(uint32);
}
static uint32 array_sizeptr( array* arr ) {
	return arr->size / sizeof(void*);
}

static uint8* array_top8( array* arr, uint32 index ) {
	if ( index >= arr->size )
		return 0;
	return &(arr->data[arr->size-index-1]);
}
static uint32* array_top32( array* arr, uint32 index ) {
	uint32 r_id = (index+1) * sizeof( uint32 );
	if ( r_id > arr->size )
		return 0;
	return &(((uint32*)arr->data)[arr->size/sizeof(uint32)-index-1]);
}
static void** array_topptr( array* arr, uint32 index ) {
	uint32 r_id = (index+1) * sizeof( void* );
	if ( r_id > arr->size )
		return 0;
	return &(((void**)arr->data)[arr->size/sizeof(void*)-index-1]);
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
	++ arr->size;
}
static void array_append32( array* arr, uint32 object ) {
	array_realloc( arr, sizeof( uint32 ) );
	uint32 offset = arr->size % sizeof( uint32 );
	((uint32*)(arr->data+offset))[arr->size/sizeof(uint32)] = object;
	arr->size += sizeof( uint32 );
}
static void array_appendptr( array* arr, void* object ) {
	array_realloc( arr, sizeof( void* ) );
	uint32 offset = arr->size % sizeof( void* );
	((void**)(arr->data+offset))[arr->size/sizeof(void*)] = object;
	arr->size += sizeof( void* );
}

static void array_push8( array* arr, uint8 object ) {
	array_append8( arr, object );
}
static void array_push32( array* arr, uint32 object ) {
	array_append32( arr, object );
}
static void array_pushptr( array* arr, void* object ) {
	array_appendptr( arr, object );
}
static void array_pop8( array* arr ) {
	-- arr->size;
}
static void array_pop32( array* arr ) {
	arr->size -= sizeof( uint32 );
}
static void array_popptr( array* arr ) {
	arr->size -= sizeof( void* );
}
static void array_print( array* arr ) {
	printf( "{ " );
	uint32 i;
	for ( i=0;i<arr->size;++i ) {
		printf( "%02x ", arr->data[i] );
	} printf( "}\n" );
	fflush( NULL );
}
static void array_printptr( array* arr ) {
	printf( "{ " );
	uint32 i;
	for ( i=0;i<arr->size/sizeof(void*);++i ) {
		printf( "%p ", ((void**)arr->data)[i] );
	} printf( "}\n" );
	fflush( NULL );
}
static void array_printint( array* arr ) {
	printf( "{ " );
	uint32 i;
	for ( i=0;i<arr->size/sizeof(uint32);++i ) {
		printf( "%d ", *((uint32**)arr->data)[i] );
	} printf( "}\n" );
	fflush( NULL );
}
static void array_printstr( array* arr ) {
	printf( "{ " );
	uint32 i;
	for ( i=0;i<arr->size/sizeof(const char*);++i ) {
		printf( "%s ", ((const char**)arr->data)[i] );
	} printf( "}\n" );
	fflush( NULL );
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
	for ( i=elem1;i<=elem2;++i )
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
		self.refcount = 1;
		uint32 i;
		for ( i=0;i<8;i++ )
			self.data[i] = 0xFFFFFFFF;
	}
	return &self;
}

/*------------------------------------------
| Atom Extenders
+-----------------------------------------*/

static void destroy_literal( void* addr ) {
	free( ((atom_literal*) addr)->data );
	free( (atom_literal*) addr );
}
static void destroy_charset( void* addr ) {
	atom_charset* data = (atom_charset*) addr;
	charset* cset = data->checkset;
	free( data );
	if ( cset->refcount == 0 )
		free( cset );
}
static void destroy_group_init( void* addr ) {
	atom_group_init* a = (atom_group_init*) addr;
	array_free( a->choices );
	free( a );
}
static void destroy_group_final( void* addr ) {
	atom_group_final* a = (atom_group_final*) addr;
	array_free( a->working );
	free( a );
}

static void array_printatoms( array* arr ) {
	printf( "{ " );
	uint32 i;
	for ( i=0;i<arr->size/sizeof(atom*);++i ) {
		atom* a = ((atom**)arr->data)[i];
		if ( a->dealloc == destroy_literal ) {
			printf( "LITERAL  " );
		} else if ( a->dealloc == destroy_charset ) {
			printf( "CHARSET  " );
		} else if ( a->dealloc == destroy_group_init ) {
			printf( "START    " );
		} else if ( a->dealloc == destroy_group_final ) {
			printf( "FINISH   " );
		}
	} printf( "}\n" );
	fflush( NULL );
}

/*------------------------------------------
| Structure Used for Build Data
+-----------------------------------------*/

struct rebuilder_t {
	int error;		// error value
	array* root;	// group origin (atom*)
	array* subroot;	// chain origin (atom*)
};
typedef struct rebuilder_t rebuilder;

static rebuilder* rebuilder_create() {
	rebuilder* re = (rebuilder*) malloc( sizeof(rebuilder) );
	re->error = 0;
	re->root = array_create();
	re->subroot = array_create();
	return re;
}

/*------------------------------------------
| Functions to Control Regex Components
+-----------------------------------------*/

static int32 litcompare( void* lit, const char* str ) {
	const char* i = ((atom_literal*)lit)->data;
	for ( ;*i!='\0';++i,++str )
		if ( *i != *str )
			return -1;
	return (int32)(i-((atom_literal*)lit)->data);
}
static int32 charcompare( void* a, const char* str ) {
	charset* cset = ((atom_charset*)a)->checkset;
	uint32 found = charset_check( cset, *str );
	if ( (((atom_charset*)a)->exclude && found) || (!(((atom_charset*)a)->exclude)&&!found) )
		return -1;
	return 1;
}

static atom* atom_create( regex* p,
						  void* data,
						  rekillfunc k ) {
	atom* a = (atom*) malloc( sizeof(atom) );
	a->parent = p;
	a->next = NULL;
	a->lower = 1;
	a->upper = 1;
	a->dealloc = k;
	a->data = data;
	return a;
}
static void atom_destroy( atom* atom ) {
	if ( atom->dealloc != NULL )
		atom->dealloc( atom->data );
	free( atom );
}
static bool is_literal( atom* a ) {
	return ( a->dealloc == destroy_literal );
}
static bool is_group_init( atom* a ) {
	return ( a->dealloc == destroy_group_init );
}
static bool is_group_final( atom* a ) {
	return ( a->dealloc == destroy_group_final );
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
		re->current->next = natom;
	
	re->current = natom;
}

// Iterates backwards through the subroots to build a failure OR chain
static void link_options( regex* re, rebuilder* build, atom* final ) {
	atom* root = (atom*) *array_topptr( build->root, 0 );
	atom_group_init* init = (atom_group_init*) root->data;	
	atom* endsub;
	uint32 i;
	
	array* tmp = array_create();
	do {
		endsub = (atom*) *array_topptr( build->subroot, 0 );
		array_popptr( build->subroot );
		array_appendptr( tmp, endsub->next );
		endsub->next = final;
	} while ( endsub != root );
	
	for ( i=array_sizeptr( tmp );i>0;--i ) {
		void* g = array_getptr( tmp, i-1 );
		array_appendptr( init->choices, g );
	}
	
	array_free( tmp );
}

// Terminate a literal and create an atom for it
static void close_literal( regex* re, const char* buffer, uint32_out size ) {
	if ( *size == 0 )
		return;
	
	atom_literal* lit = (atom_literal*) malloc( sizeof(atom_literal) );
	lit->data = (char*) malloc( sizeof(char) * (*size+1) );
	memcpy( lit->data, buffer, *size );
	lit->data[*size] = '\0';
	
	atom* ret = atom_create( re, lit, destroy_literal );
	
	*size = 0;
	
	link_to_prev( re, ret );
}

// Create an open group node
static void create_group( regex* re, rebuilder* build ) {
	atom_group_init* gr = (atom_group_init*) malloc( sizeof(atom_group_init) );
		gr->choices = array_create();
		
	atom* ret = atom_create( re, gr, destroy_group_init );
	
	// assign new root
	array_pushptr( build->root, ret );
	array_pushptr( build->subroot, ret );
	
	link_to_prev( re, ret );
}

// Create a close group node and bind it to the open group node
static void close_group( regex* re, rebuilder* build ) {
	if ( build->root->size == 0 ) {
		build->error = REGEX_EXTRA_CLOSE_GROUP;
		return;
	}
	
	atom_group_final* gr = (atom_group_final*) malloc( sizeof(atom_group_final) );
		gr->working = array_create();
		gr->start = (atom*) *array_topptr( build->root, 0 );
		gr->iteration = 0;
	atom* ret = atom_create( re, gr, destroy_group_final );
	
	link_options( re, build, ret );
	array_popptr( build->root );
	
	// forced link_to_prev
	gr->start->next = ret;
	link_to_prev( re, ret );
}
static void move_chain( regex* re, rebuilder* build ) {
	if ( re->current == NULL || is_group_init( re->current ) ) {
		build->error = REGEX_CHAIN_HAS_NO_INITIAL;
		return;
	}
	
	array_pushptr( build->subroot, re->current );
}
static const char* create_charset( regex* re, rebuilder* build, const char* str ) {
	if ( *str == '.' ) {
		charset* cset = charset_any();
		++ cset->refcount;
		
		atom_charset* data = (atom_charset*) malloc( sizeof( atom_charset ) );
		data->exclude = 0;
		data->checkset = cset;
		
		atom* ret = atom_create( re, data, destroy_charset );
		link_to_prev( re, ret );
		return ++ str;
	} else {
		charset* cset = charset_create();
		cset->refcount = 0;
		
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
			if ( last == '-' ) {
				if ( last2 == '\0' || *str == '-' || *str < last2 ) {
					build->error = REGEX_INCORRECT_CHAR_SPAN;
					return str;
				}
				charset_add_multiple( cset, last2, *str );
				last2 = '\0';
				last = '\0';
			} else {
				if ( last != '\0' )
					charset_add( cset, last );
				last2 = last;
				last = *str;
			}
		}
		
		if ( last != '\0' )
			charset_add( cset, last );
		
		if ( *str==']' )
			++ str;
		
		atom* ret = atom_create( re, data, destroy_charset );
		
		link_to_prev( re, ret );
		
		return str;
	}
}


void assign_quantifier( regex* re, rebuilder* build, int low, int high ) {
	if ( re->current == NULL ) {
		build->error = REGEX_QUANTIFIER_TO_NONE;
		return;
	}
	
	re->current->lower = low;
	re->current->upper = high;
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
	
	m->current_scope = array_create();
	m->track_atom = array_create();
	m->track_string = array_create();
	m->track_quan = array_create();
	
	rebuilder* build = rebuilder_create();
	
	const char* tmp = cc;
	const char* buffer = tmp;
	uint32 size = 0;
	
	create_group( m, build );
	
	while ( build->error == 0 && *tmp != '\0' ) {
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
		close_group( m, build );
	}
	
	return m;
}

static void regex_display_group( uint32 amt, atom* gr ) {
	uint32 i;
	atom* j;
	uint32 k;
	uint32 sz;
	
	fflush( NULL );
	
	atom_group_init* init = (atom_group_init*) gr->data;
	for ( k=0;k<amt;++k ) printf( "	" );
	printf( "-------------\n" );
fflush( NULL );
	sz = array_sizeptr( init->choices );
	for ( i=0;i<sz;++i ) {
		j = array_getptr( init->choices, i );
		while ( j != gr->next ) {
			if ( j->dealloc == destroy_literal ) {
				for ( k=0;k<amt;++k ) printf( "	" );
				printf( "%p -> LITERAL {%d, %d}", j, j->lower, j->upper );
			fflush( NULL );
			} else if ( j->dealloc == destroy_charset ) {
				for ( k=0;k<amt;++k ) printf( "	" );
				printf( "%p -> CHARSET {%d, %d}", j, j->lower, j->upper );
			fflush( NULL );
			} else if ( j->dealloc == destroy_group_init ) {
				for ( k=0;k<amt;++k ) printf( "	" );
				printf( "%p -> OPEN GROUP\n", j );
			fflush( NULL );
				regex_display_group( amt+1, j );
				fflush( NULL );
			} else if ( j->dealloc == destroy_group_final ){
				for ( k=0;k<amt;++k ) printf( "	" );
				printf( "%p -> CLOSE GROUP {%d, %d}", j, j->lower, j->upper );
			fflush( NULL );
			} else {
				for ( k=0;k<amt;++k ) printf( "	" );
				printf( "%p -> ???", j );
			fflush( NULL );
			}
			printf( "\n" );
			j = j->next;
		}
		for ( k=0;k<amt;++k ) printf( "	" );
		printf( "-------------" );
		if ( i+1 != sz ) printf("\n");
	}
	
}
void regex_display( regex* re ) {
	printf( "%p -> INITIAL\n", re->initial );
	regex_display_group( 0, re->initial );
	printf("\n%p -> FINAL\n", re->initial->next );
}

static atom* regex_reset( regex* re, const char* val ) {
	re->strstart = val;
	
	re->current_scope->size = 0;
	
	re->track_atom->size = 0;
	re->track_string->size = 0;
	re->track_quan->size = 0;
	
	return re->initial;
}
static const char* regex_undo( regex* re, atom** ref ) {
	array_popptr( re->track_atom );
	array_popptr( re->track_string );
	array_popptr( re->track_quan );
	
	(*ref) = (atom*) *array_topptr( re->track_atom, 0 );
	const char* result = (const char*) *array_topptr( re->track_string, 0 );
	
	return result;
}
static void regex_advance( regex* re, atom* ref, const char* str ) {
	array_pushptr( re->track_atom, ref );
	array_pushptr( re->track_string, (void*) str );
	array_push32( re->track_quan, 0 );
}

substr* regex_match( regex* re, const char* str ) {
	// stacks in the regex are setup by the caller
	
	atom* this = regex_reset( re, str );
	atom* last = NULL;
	uint32* lastui = NULL;
	
	const char* istr = str;
	uint32 returning = 0;
	uint32 iter = 0;
	
	regex_advance( re, this, str );
	
	while ( *str != '\0' && this != re->initial->next ) {
		
#ifdef __DEBUG__
		printf( "\n-- Iteration ( %d ) --\n", iter );
		printf( "==> '%s'\n", str );
		array_printatoms( re->track_atom );
		array_printptr( re->track_quan );
#endif
		
		if ( this->dealloc == destroy_group_init ) {
			
			atom* final = this->next;
			atom_group_init* in = (atom_group_init*) this->data;
			atom_group_final* fn = (atom_group_final*) final->data;
			
			uint32* current = array_top32( re->track_quan, 0 );
			
			// if the control flow is being passed back to this node
			// after a failed match
			if ( returning ) {
				
				if ( in->current >= array_sizeptr( in->choices ) ) {
					// if there are no other options for this group,
					// go back to the previous atom
					str = regex_undo( re, &this );
					fn->iteration |= ATOM_GROUP_FLAG_FAILED;
					
					// inform the termination node that this iteration
					// is no longer active
					array_popptr( fn->working );
				}
				
				else {
					
					// advance to the next available atom in
					// the possibility list
					atom* next = array_getptr( in->choices, in->current );
					regex_advance( re, next, str );
					this = next;
					++ in->current;
				}
				
			}
			
			// if this node is being seen for the first time
			else {
				
				// inform the termination node that a new iteration
				// of this group is active
				array_pushptr( fn->working, current );
				
				// if the current value equals or exceeds the lower bound
				// don't be greedy!
				if ( *current >= final->lower && !(fn->iteration&ATOM_GROUP_FLAG_ITERATION) )
					this = this->next;
				
				else {
					in->current = 0;
					fn->iteration &= ~ATOM_GROUP_FLAG_ITERATION;
					atom* next = array_getptr( in->choices, in->current );
					regex_advance( re, next, str );
					this = next;
					++ in->current;
				}
				
			}
			
		} else if ( this->dealloc == destroy_group_final ) {
			
			atom_group_final* fn = (atom_group_final*) this->data;
			
			// initial node quantifier value
			uint32* working = (uint32*) *array_topptr( fn->working, 0 );
			
			// termination node quantifier value
			uint32* current = array_top32( re->track_quan, 0 );
			
			// backward control flow
			if ( returning ) {
				
				// if the upper limit has not been reached,
				// do another iteration
				if ( this->upper == -1 || *current + 1 < this->upper ) {
					if ( fn->iteration & ATOM_GROUP_FLAG_FAILED ) {
						fn->iteration &= ~ATOM_GROUP_FLAG_FAILED;
						str = regex_undo( re, &this );
					} else {
						returning = 0;
						fn->iteration |= ATOM_GROUP_FLAG_ITERATION;
						regex_advance( re, fn->start, str );
						(*array_top32( re->track_quan, 0 )) = *current + 1;
						this = fn->start;
					}
				}
				
				// if the upper limit has been reached,
				// attempt to expand components within the group
				else {
					str = regex_undo( re, &this );
				}
				
			} 
			
			// forward control flow
			else {
				
				// make the termination quanitifier the same as
				// the initial quantifier
				(*current) = (*working);
				
				// need more iterations
				if ( this->upper == -1 || *current + 1 < this->lower ) {
					fn->iteration |= ATOM_GROUP_FLAG_ITERATION;
					regex_advance( re, fn->start, str );
					(*array_top32( re->track_quan, 0 )) = *current + 1;
					this = fn->start;
				}
				
				// group is satisfied, move on
				else {
					regex_advance( re, this->next, str );
					this = this->next;
				}
				
			}
			
		} else {
			
			uint32* current = array_top32( re->track_quan, 0 );
			
			last = this;
			lastui = current;
			
			// cannot process this atom because 
			// it has already filled it's maximum
			// number of repeats
			if ( this->upper != -1 && this->upper <= *current ) {
				
				// if returning, continue to return
				if ( returning )
					str = regex_undo( re, &this );
				
				// if advancing, continue to advance
				// !!! ERROR !!!
				else {
					regex_advance( re, this->next, str );
					this = this->next;
				}
				
			} 
			
			else {
				
				// decide which check function to use
				recheckfunc op;
				if ( this->dealloc == destroy_literal )
					op = litcompare;
				else if ( this->dealloc == destroy_charset )
					op = charcompare;
				
				if ( returning ) {
					
					// when returning, only increment by one iteration
					int32 len = op( this->data, str );
					
					// failed to match
					if ( len == -1 )
						str = regex_undo( re, &this );
					
					// match was successful
					else {
						// unset the returning flag
						returning = 0;
						str += len;
						++ (*current);
					}
					
				} else {
					
					while ( *current < this->lower ) {
						int32 len = op( this->data, str );
						
						// failed to match
						if ( len == -1 ) {
							// go back to the last successful element
							str = regex_undo( re, &this );
							returning = 1;
							break;
						}
						
						// match was successful
						else {
							str += len;
							++ (*current);
						}
					}
					
				}
				
				if ( returning == 0 ) {
					// once the check is complete, set the return string
					// to the completed position
					const char** pos = (const char**) array_topptr( re->track_string, 0 );
					(*pos) = str;
					regex_advance( re, this->next, str );
					this = this->next;
				}
				
			}
			
		}
		
		++ iter;
		if ( array_sizeptr( re->track_atom ) <= 1 )
			break;
	}
	
	if ( last != NULL ) {
		
		recheckfunc op;
		if ( last->dealloc == destroy_literal )
			op = litcompare;
		else if ( last->dealloc == destroy_charset )
			op = charcompare;
		
		while ( last->upper == -1 || *lastui < last->upper ) {
			int32 len = op( last->data, str );
			if ( len == -1 )
				break;
			str += len;
			++ (*lastui);
		}
		
	}
	
#ifdef __DEBUG__
	printf( "\n--- RESULTS ----\n" );
	printf( "==> '%s'\n", str );
	array_printatoms( re->track_atom );
	array_printptr( re->track_quan );
	printf( "\n" );
#endif
	
	if ( this == re->initial->next ) {
		substr* ret = (substr*) malloc( sizeof( substr ) );
			ret->begin = istr;
			ret->end = str;
			ret->line_number = 0;
			ret->char_number = 0;
		return ret;
	}
	
	return NULL;
}

int regex_destroy( regex* re ) {
	return 0;
}