#include <string>
#include <iostream>
#include <climits>
#include "regex.h"

static uint DEBUG_depth;

// for multiple new line characters
// [[ curse you Mircosoft ]]
static bool is_new_line( char n ) {
	return ( n == '\n' );
}

// check if a string literal matches X number of times
const char* regex_literal::check( const char* str ) const {	
	#ifdef __REGEX_DEBUG_STRING__
	std::cout << std::string( DEBUG_depth*4, ' ' ) << "CHECKING LITERAL " << this << " : " << lit << std::endl;
	#endif//__REGEX_DEBUG_STRING__
	
	const char* buf;
	
	for ( int count=0;count<upper||upper==-1;count++ ) {
		
		const char* tmp = lit;
		buf = str;
		
		while ( *tmp != '\0' ) {
			if ( *tmp != *str ) {
				if ( count < lower )
					return NULL;
				else
					return buf;
			}
			str ++ ;
			tmp ++ ;
		}
		
	} return str;;
}

// check if a character set matches X number of times
const char* regex_charset::check( const char* str ) const {
	#ifdef __REGEX_DEBUG_STRING__
	std::cout << std::string( DEBUG_depth*4, ' ' ) << "CHECKING CHAR SET " << this << " (" << lower << ", " << upper << ")" << std::endl;
	#endif//__REGEX_DEBUG_STRING__
	
	for ( int count=0;count<upper||upper==-1;count++ ) {
		
		bool contains = (*cset == *str);
		
		#ifdef __REGEX_DEBUG_STRING__
		std::cout << std::string( DEBUG_depth*4, ' ' ) << "cset contains " << *str << " = " << contains << std::endl;
		#endif//__REGEX_DEBUG_STRING__
		
		if ( ( exclude && contains ) || ( !exclude && !contains ) ) {
			if ( count < lower )
				return NULL;
			else
				return str;
		}
		
		str ++ ;
		
	} return str;
}

// check if a group of atoms matches X number of times
const char* regex_group::check( const char* str ) const {
	
	#ifdef __REGEX_DEBUG_STRING__
	std::cout << std::string( DEBUG_depth*4, ' ' ) << "CHECKING GROUP " << this << std::endl;
	#endif//__REGEX_DEBUG_STRING__
	
	const char* buf;
	
	for ( int count=0;count<upper||upper==-1;count++ ) {
		
		regex_atom* at = initial;
		buf = str;
		
		while ( at != this ) {
			
			DEBUG_depth ++ ;
			const char* tmp = at->check( str );
			
			// if the check fails, go to the atoms
			// fail state without changing the string
			// location
			if ( tmp == NULL )
				at = at->failure;
			else {
				
				#ifdef __REGEX_DEBUG_STRING__
				std::cout << std::string( DEBUG_depth*4, ' ' ) << "GROUP ELEM PASSED " << at->success << " = " << this << std::endl;
				#endif//__REGEX_DEBUG_STRING__
				
				at = at->success;
				str = tmp;
			}
			
			DEBUG_depth -- ;
			// if the group <at> becomes NULL the group
			// has failed to finish
			if ( at == NULL ) {
				if ( count < lower ) {
					
					#ifdef __REGEX_DEBUG_STRING__
					std::cout << std::string( DEBUG_depth*4, ' ' ) << "GROUP " << this << " FULL FAIL" << std::endl;
					#endif//__REGEX_DEBUG_STRING__
					
					return NULL;
				} else {
					
					#ifdef __REGEX_DEBUG_STRING__
					std::cout << std::string( DEBUG_depth*4, ' ' ) << "GROUP " << this << " FULL PASS" << std::endl;
					#endif//__REGEX_DEBUG_STRING__
					
					return buf;
				}
			}
		}
		
	}
	
	#ifdef __REGEX_DEBUG_STRING__
	std::cout << std::string( DEBUG_depth*4, ' ' ) << "GROUP " << this << " PASS" << std::endl;
	#endif//__REGEX_DEBUG_STRING__
	
	return str;
}

// check if the string is the start of a file or follows a new line
const char* regex_start_line::check( const char* str ) const {
	if ( str == *compare || is_new_line( *(str - 1) ) )
		return str;
	
	return NULL;
}

// check if the string is the end of a file or a new line
const char* regex_end_line::check( const char* str ) const {
	if ( str == '\0' || is_new_line( *str ) )
		return str;
	
	return NULL;
}

// CREATE A REGULAR EXPRESSION STRUCTURE
static re_char_set<char>* standard_charset() {
	static re_char_set<char> standard;
	if ( standard.size() == 0 )
		standard.include( ' ', '~' );
	return &standard;
}

static bool is_quantifier( char n ) {
	return  ( n == '{' || 
			  n == '?' || 
			  n == '*' || 
			  n == '+' );
}

static int convertval( const char* a, const char* b ) {
	if ( b < a ) 
		return 0;
	uint len = b - a;
	char buffer[len+1];
	for ( uint i=0;i<len;i++ )
		buffer[i] = *(a+i);
	buffer[len] = '\0';
	return atoi( buffer );
}

// ZeroMem basic structures
static void zero_atom( regex_atom* a ) {
	a->success = NULL;
	a->failure = NULL;
}
static void zero_quan( regex_quantifier* q ) {
	zero_atom( q );
	q->lower = 1;
	q->upper = 1;
}

static bool check_quantifier( const char*& str, regex_quantifier* quan );
static void set_prev( regex_quantifier* natom, regex_quantifier*& prev, regex_group* group );
static void close_literal( std::string& buffer, regex_quantifier*& prev, regex_group* group );

regex* regex_create( const char* str ) {
	regex* m = new regex();
	m->initial = NULL;
	zero_atom( m );
	m->lower = 1;
	m->upper = 1;
	
	#ifdef __REGEX_DEBUG_STRING__
	std::cout << "BEGIN REGEX MAKE " << m << std::endl;
	#endif//__REGEX_DEBUG_STRING__
	
	const char* tmp = str;
	
	// start of the or chain
	stack_t<regex_quantifier*> chaininit;
	
	// first and last element in the last '|...|' section
	stack_t<regex_quantifier*> chainnow;
	
	stack_t<regex_group*> groups;
	stack_t<regex_quantifier*> prev;
	
	std::string buffer;
	
	groups.push( m );
	prev.push( NULL );
	chaininit.push( NULL );
	chainnow.push( NULL );
	
	try {
		while ( *tmp != '\0' ) {
			if ( is_quantifier( *tmp ) ) {
				
				close_literal( buffer, prev[0], groups[0] );
				check_quantifier( tmp, prev[0] );
				
			} else if ( *tmp == '.' ) {
				
				close_literal( buffer, prev[0], groups[0] );
				regex_charset* n = new regex_charset();
				zero_quan( n );
				n->cset = standard_charset();
				
				set_prev( n, prev[0], groups[0] );
				
				++ tmp;
				
			} else if ( *tmp == '^' ) {
				
				close_literal( buffer, prev[0], groups[0] );
				
				regex_start_line* n = new regex_start_line();
				zero_quan( n );
				n->compare = &(m->start_of_file);
				
				set_prev( n, prev[0], groups[0] );
				
				++ tmp;
				
			} else if ( *tmp == '$' ) {
				
				close_literal( buffer, prev[0], groups[0] );
				
				regex_end_line* n = new regex_end_line();
				zero_quan( n );
				
				set_prev( n, prev[0], groups[0] );
				
				++ tmp;
				
			} else if ( *tmp == '(' ) {
				
				close_literal( buffer, prev[0], groups[0] );
				
				regex_group* n = new regex_group();
				zero_quan( n );
				
				set_prev( n, prev[0], groups[0] );
				
				groups.push( n );
				prev.push( NULL );
				chaininit.push( NULL );
				chainnow.push( NULL );
				
				++ tmp;
				
			} else if ( *tmp == ')' ) {
				if ( groups.size() == 1 )
					throw tmp;
				
				close_literal( buffer, prev[0], groups[0] );
				
				if ( chaininit[0] != NULL ) {
					prev[0]->success = chainnow[0];
					
					groups.pop();
					prev.pop();
				}
				
				regex_group* endscope = groups[0];
				
				groups.pop();
				prev.pop();
				chaininit.pop();
				chainnow.pop();
				
				prev[0] = endscope;
				
				++ tmp;
				
			} else if ( *tmp == '|' ) {
				
				close_literal( buffer, prev[0], groups[0] );
				
				#ifdef __REGEX_DEBUG_STRING__
				std::cout << "register OR symbol" << std::endl;
				#endif//__REGEX_DEBUG_STRING__
				
				if ( prev[0] == NULL )
					throw tmp;
				
				regex_group* nscope = new regex_group();
				zero_quan( nscope );
				
				#ifdef __REGEX_DEBUG_STRING__
				std::cout << " -- create right scope " << nscope << std::endl;
				#endif//__REGEX_DEBUG_STRING__
				
				// if the this is the first OR bar,
				// convert all preceding elements into
				// a single group
				if ( chaininit[0] == NULL ) {
					
					regex_group* ngroup = new regex_group();
					zero_quan( ngroup );
					
					#ifdef __REGEX_DEBUG_STRING__
					std::cout << " -- create left scope " << ngroup << std::endl;
					#endif//__REGEX_DEBUG_STRING__
					
					ngroup->initial = groups[0]->initial;
					
					#ifdef __REGEX_DEBUG_STRING__
					std::cout << " -- assign " << groups[0]->initial << " to " << ngroup << std::endl;
					#endif//__REGEX_DEBUG_STRING__
					
					
					groups[0]->initial = ngroup;
					
					#ifdef __REGEX_DEBUG_STRING__
					std::cout << " -- assign " << ngroup << " to " << groups[0] << std::endl;
					#endif//__REGEX_DEBUG_STRING__
					
					
					chaininit[0] = ngroup;
					ngroup->failure = nscope;
					
					ngroup->success = groups[0];
					prev[0]->success = ngroup;
					
					groups.push( nscope );
					prev.push( NULL );
					
				} else {
					
					prev[0]->success = chainnow[0];
					chainnow[0]->failure = nscope;
					
					groups[0] = nscope;
					prev[0] = NULL;
					
				}
				
				#ifdef __REGEX_DEBUG_STRING__
				std::cout << "SIZE: " << groups.size() << std::endl;
				#endif//__REGEX_DEBUG_STRING__
				
				
				nscope->success = groups[1];
				chainnow[0] = nscope;
				
				++ tmp;
				
			} else if ( *tmp == '[' ) {
				
				close_literal( buffer, prev[0], groups[0] );
				
				regex_charset* chs = new regex_charset();
				zero_quan( chs );
				
				++ tmp;
				
				if ( *tmp == '^' ) {
					chs->exclude = true;
					++ tmp;
				}
				
				re_char_set<char>* rchs = new re_char_set<char>();
				
				if ( *tmp == '-' ) {
					rchs->include( '-' );
					++ tmp;
				}
				
				bool span = false;
				char previous = '\0';
				while ( *tmp != ']' ) {
					
					#ifdef __REGEX_DEBUG_STRING__
					std::cout << (int) previous << " : " << previous << std::endl;
					#endif//__REGEX_DEBUG_STRING__
					
					if ( span ) {
						if ( previous == '\0' )
							throw tmp;
						rchs->include( previous, *tmp );
						
						#ifdef __REGEX_DEBUG_STRING__
						std::cout << "include " << previous << " : " << *tmp << std::endl;
						#endif//__REGEX_DEBUG_STRING__
						
						span = false;
						previous = '\0';
					} else {
						if ( *tmp == '-' ) {
							span = true;
						} else if ( *tmp == '\\' ) {
							
							// ...
							
						} else {
							if ( previous != '\0' )
								rchs->include( previous );
							previous = *tmp;
						}
					}
					++ tmp;
				}
				
				#ifdef __REGEX_DEBUG_STRING__
				std::cout << (int) previous << " : " << previous << std::endl;
				#endif//__REGEX_DEBUG_STRING__
				
				if ( previous != '\0' ) {
					if ( span )
						throw tmp;
					rchs->include( previous );
				}
				
				chs->cset = rchs;
				set_prev( chs, prev[0], groups[0] );
				
		 		++ tmp;
				
			} else if ( *tmp == '.' ) {
				
				regex_charset* chs = new regex_charset();
				zero_quan( chs );
				chs->cset = standard_charset();
				set_prev( chs, prev[0], groups[0] );
				++ tmp;
				
			} else if ( *tmp == ']' ) {
				
				throw tmp;
				
			} else {
				
				#ifdef __REGEX_DEBUG_STRING__
				std::cout << "grab char " << *tmp << std::endl;
				#endif//__REGEX_DEBUG_STRING__
				
				buffer += *tmp;
				++ tmp;
			}
		}
		
		close_literal( buffer, prev[0], groups[0] );
		
		if ( chaininit[0] != NULL ) {
			
			#ifdef __REGEX_DEBUG_STRING__
			std::cout << "close with " << groups[0] << std::endl;
			#endif//__REGEX_DEBUG_STRING__
			
			groups[0]->success = m;
			groups.pop();
			prev.pop();
			
		} else
			prev[0]->success = m;
		
	} catch ( const char* err ) {
		const char* tmp = str;
		std::cout << "RegEx Error!!! (" << err - tmp << ")" << std::endl;
		std::cout << tmp << std::endl;
		for ( ;tmp != err;tmp++ )
			std::cout << " ";
		std::cout << "^" << std::endl;
		free( m );
		m = NULL;
	}
	return m;
}

static void set_prev( regex_quantifier* natom,  	// incoming atom
					  regex_quantifier*& prev,  	// the previous atom
					  regex_group* group ){ // the current group
	if ( prev == NULL ) {
		#ifdef __REGEX_DEBUG_STRING__
		std::cout << "set " << group << "->initial to " << natom << std::endl;
		#endif//__REGEX_DEBUG_STRING__
		group->initial = natom;
	} else {
		#ifdef __REGEX_DEBUG_STRING__
		std::cout << "set " << prev << "->success to " << natom << std::endl;
		#endif//__REGEX_DEBUG_STRING__
		prev->success = natom;
	}
	prev = natom;
}
static void close_literal( std::string& buffer,
						   regex_quantifier*& prev,
						   regex_group* group ) {
	if ( buffer.size() == 0 )
		return;
	
	regex_literal* lit = new regex_literal();
	zero_quan( lit );
	char* tmpbuf = new char[buffer.size()+1];
	
	uint i;
	for ( i=0;i<buffer.size();i++ )
		tmpbuf[i] = buffer[i];
	tmpbuf[i] = '\0';
	
	#ifdef __REGEX_DEBUG_STRING__
	std::cout << "BUFFER: " << tmpbuf << " to " << lit << std::endl;
	#endif//__REGEX_DEBUG_STRING__
	
	lit->lit = tmpbuf;
	
	buffer.clear();
	
	set_prev( lit, prev, group );
}
static bool check_quantifier( const char*& str, regex_quantifier* quan ) {
	if ( *str == '{' ) {
		
		if ( quan == NULL )
			throw str;
		
		int lower, upper;
		const char* tmp = str ++ ;
		
		for ( ;*tmp!=',';++tmp );
		
		lower = convertval( str, tmp );
		str = ++tmp;
		
		for ( ;*tmp!='}';++tmp );
		
		( tmp == str ) ? upper = -1 :
			upper = convertval( str, tmp );
		str = ++tmp;
		
		quan->lower = lower;
		quan->upper = upper;
		
		#ifdef __REGEX_DEBUG_STRING__
		std::cout << "{...} for " << quan << " is " << lower << " : " << upper << std::endl;
		#endif//__REGEX_DEBUG_STRING__
		
		return true;
		
	} else if ( *str == '?' ) {
		
		if ( quan == NULL )
			throw str;
		
		quan->lower = 0;
		quan->upper = 1;
		str ++ ;
		return true;
		
	} else if ( *str == '*' ) {
		
		if ( quan == NULL )
			throw str;
		
		quan->lower = 0;
		quan->upper = -1;
		str ++ ;
		return true;
		
	} else if ( *str == '+' ) {
		
		if ( quan == NULL )
			throw str;
		
		quan->lower = 1;
		quan->upper = -1;
		str ++ ;
		return true;
		
	} return false;
}

void regex_match( regex* re, const char* str, regex_substring& output ) {
	output.begin = NULL;
	output.end = NULL;
	output.line_number = 1;
	output.char_number = 1;
	
	re->start_of_file = str;
	
	for ( ;*str!='\0';++str ) {
		DEBUG_depth = 0;
		const char* tmp;
		tmp = re->check( str );
		if ( tmp != NULL && tmp != str ) {
			output.begin = str;
			output.end = tmp;
			break;
		}
		if ( *str == '\n' ) {
			output.line_number ++ ;
			output.char_number = 1;
		} else {
			output.char_number ++ ;
			if ( output.char_number == UINT_MAX ) {
				output.line_number ++ ;
				output.char_number = 1;
			}
		}
	}
}
void regex_search( regex* re, const char* str, array_t<regex_substring>& output ) {
	regex_substring buffer;
	regex_match( re, str, buffer );
	
	uint lnnum = buffer.line_number;
	while ( buffer.begin != NULL ) {
		output.append( buffer );
		str = buffer.end;
		regex_match( re, str, buffer );
		lnnum = buffer.line_number + lnnum - 1;
		buffer.line_number = lnnum;
	}
}