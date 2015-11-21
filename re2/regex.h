#ifndef __REGEX_HEADER_H__
#define __REGEX_HEADER_H__

#ifdef __REGEX_EXPORT__
	#define REGEX_FUNCTION(x,y,z) x __declspec(dllexport) y z
	#define REGEX_EXPORT __declspec(dllexport)
#else
	/*#define REGEX_FUNCTION(x,y,z) x __declspec(dllimport) y z
	#define REGEX_EXPORT __declspec(dllimport)*/
	#define REGEX_FUNCTION(x,y,z) x y z
	#define REGEX_EXPORT
#endif

#define REGEX_SYNTAX_ERROR 100

#define REGEX_EXTRA_CLOSE_GROUP 	501
#define REGEX_EXTRA_CLOSE_CHARSET 	502
#define REGEX_CHAIN_HAS_NO_INITIAL 	503
#define REGEX_QUANTIFIER_TO_NONE 	504
#define REGEX_INCORRECT_CHAR_SPAN 	505

typedef char 			int8;
typedef unsigned char 	uint8;
typedef unsigned char 	bool;
typedef short 			uint16;
typedef unsigned short 	int16;
typedef int 			int32;
typedef unsigned int 	uint32;
typedef unsigned int* 	uint32_out;

uint32 next_pow2(uint32 a);

struct stack_t;
struct array_t;
struct charset_t;
struct substr_t;
struct quantifier_t;
struct atom_t;
struct regex_t;

typedef struct stack_t stack;
typedef struct array_t array;
typedef struct charset_t charset;
typedef struct substr_t substr;
typedef struct quantifier_t quantifier;
typedef struct atom_t atom;
typedef struct regex_t regex;

typedef const char* (*recheckfunc)( atom**, const char* );
typedef void (*rekillfunc)( void* );
typedef int (*reprintfunc)( void* );

struct stack_t {
	void** data;
	uint32 size;
	uint32 alloc;
};
struct array_t {
	uint8* data;
	uint32 size;
	uint32 alloc;
};
struct charset_t {
	int32 refcount;
	uint32 data[8];
};

struct substr_t {
	const char* begin;	// string start
	const char* end;	// string end (non-inclusive)
	uint32 line_number; // line number of string in original file
	uint32 char_number; // character number on line in original file
};
struct quantifier_t {
	uint32 lower; 	 // lower satisfied conditions
	int32 upper;	 // upper satisfied conditions
	uint32 working;  // the number of repeats already found
};
struct atom_t {
	regex* parent;		// parent regex struct 
	atom* success;		// on successful match, go to this atom
	atom* failure;		// on unsuccessful match, go to this atom
	quantifier* info;	// information concerning count
	recheckfunc func;	// compare atom against string
	rekillfunc dealloc;	// deallocate the atom
	reprintfunc pself;	// print the atom
	void* data;			// points to additional data structure
};

struct regex_t {
	const char* strstart;	// testing string root
	atom* initial;			// first atom
	atom* current;			// working atom
	stack* track_atom;		// return to these atoms when a match fails
	stack* track_string;	// string location associated with the atom stack
	array* working;			// array of quantifiers (for quick zeroing)
};

/* Use these functions */

	/* create a regular expression object from a regex string */
	REGEX_FUNCTION( regex*, regex_create, ( const char* ) );
	
	/* deallocate a regular expression object */
	REGEX_FUNCTION( int, regex_destroy, ( regex* ) );
	
	/* find the next match for the regex in the target string
	   returns a single substring */
	REGEX_FUNCTION( substr*, regex_match, ( regex*, const char* ) );
	
	/* find all regex matches in target string
	   if two matches overlap, take the initial match
	   the output integer is the number of matches
	   returns a substring array */
	REGEX_FUNCTION( substr*, regex_search, ( regex*, const char*, uint32_out ) );

#endif//__REGEX_HEADER_H__