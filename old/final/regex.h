#ifndef __REGEX_HEADER_H__
#define __REGEX_HEADER_H__

#ifdef __REGEX_EXEC__
	#define REGEX_FUNCTION(x,y,z) x y z
	#define REGEX_EXPORT
#else
	#ifdef __REGEX_EXPORT__
		#define REGEX_FUNCTION(x,y,z) x __declspec(dllexport) y z
		#define REGEX_EXPORT __declspec(dllexport)
	#else
		#define REGEX_FUNCTION(x,y,z) x __declspec(dllimport) y z
		#define REGEX_EXPORT __declspec(dllimport)
	#endif
#endif

#define REGEX_SYNTAX_ERROR 			100
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

struct array_t;
struct charset_t;
struct substr_t;
struct quantifier_t;
struct atom_t;
struct regex_t;

typedef struct array_t array;
typedef struct charset_t charset;
typedef struct substr_t substr;
typedef struct atom_t atom;
typedef struct regex_t regex;

typedef int32 (*recheckfunc)( void*, const char* );
typedef void (*rekillfunc)( void* );

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
struct atom_t {
	regex* parent;		// parent regex struct 
	
	atom* next;			// select one of these atoms to advance to
	
	uint32 lower;		// minimum number of matches
	int32 upper;		// maximum number of matches
	
	void* data;			// points to additional data structure
	rekillfunc dealloc;	// deallocate the atom data
};

struct atom_literal_t;
struct atom_charset_t;
struct atom_group_init_t;
struct atom_group_final_t;
struct atom_line_limits_t;

typedef struct atom_literal_t atom_literal;
typedef struct atom_charset_t atom_charset;
typedef struct atom_group_init_t atom_group_init;
typedef struct atom_group_final_t atom_group_final;
typedef struct atom_line_limits_t atom_line_limits;

struct atom_literal_t {
	char* data; // literal for comparison
};
struct atom_charset_t {
	bool exclude; // set true if the charset is to be excluded rather than included
	charset* checkset; // collection of characters
};
// group init treats assigned quantifier value as current choice index
struct atom_group_init_t {
	array* choices;
	uint32 current;
};

// signal an iteration to the initial group node
// (the group should not skip if the lower limit is satisfied)
#define ATOM_GROUP_FLAG_ITERATION 	0x80000000

// signal that a group iteration failed, and that
// the inner nodes should be re-evaluated
#define ATOM_GROUP_FLAG_FAILED		0x40000000

struct atom_group_final_t {
	array* working;
	atom* start; // pointer to group start
	uint32 iteration; // iteration value; top bit respresents a flag
};

#define ATOM_LINE_START 1
#define ATOM_LINE_END 	2

struct atom_line_limits_t {
	const char** strstart;
	uint32 type;
};

struct regex_t {
	const char* strstart;	// testing string root
	
	atom* initial;			// first atom
	atom* current;			// working atom
	
	array* track_atom;		// return to these atoms when a match fails
	array* track_string;	// string location associated with the atom stack
	array* track_quan;		// quantifier amount
};

/* Use these functions */

	/* create a regular expression object from a regex string */
	REGEX_FUNCTION( regex*, regex_create, ( const char* ) );
	
	/* sets the starting location for the string */
	REGEX_FUNCTION( void, regex_assign, ( regex*, const char* ) );
	
	/* find the next match for the regex in the target string
	   returns a single substring */
	REGEX_FUNCTION( substr*, regex_match, ( regex*, const char* ) );
	
	/* find all regex matches in target string
	   if two matches overlap, take the initial match
	   the output integer is the number of matches
	   returns a substring array */
	REGEX_FUNCTION( substr**, regex_search, ( regex*, const char*, uint32_out ) );
	
	/* deallocate a regular expression object */
	REGEX_FUNCTION( int, regex_destroy, ( regex* ) );
	
#ifdef __DEBUG__
	REGEX_FUNCTION( void, regex_display, ( regex* ) );
#endif

#endif//__REGEX_HEADER_H__