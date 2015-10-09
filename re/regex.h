#ifndef __REGULAR_EXPRESSIONS_H__
#define __REGULAR_EXPRESSIONS_H__

#include <algorithm>
#include <limits.h>

#ifdef __REGEX_EXPORT__
	#define REGEX_FUNCTION(x,y,z) x __declspec(dllexport) y z
	#define REGEX_EXPORT __declspec(dllexport)
#else
	#define REGEX_FUNCTION(x,y,z) x __declspec(dllimport) y z
	#define REGEX_EXPORT __declspec(dllimport)
#endif

typedef unsigned uint;
uint next_pow2(uint a) {
	uint bitsz = 0, vec = 0;
	while ( a != 0 ) {
		vec += a % 2;
		a >>= 1;
		bitsz ++ ;
	} return ( 0x1 << ( bitsz - ( vec == 1 ) ) );
}

// STACK
template<typename __T>
struct stack_t {
	private:
		
		uint _alloc, _size;
		__T* _data;
		
		void __memcpy( void* a, void* b, uint size ) {
			for ( uint i=0;i<size;i++ )
				((char*)a)[i] = ((char*)b)[i];
		}
		void __realloc( uint x ) {
			if ( x > _alloc ) {
				while ( _alloc < x )
					_alloc = std::max( 1u, _alloc * 2 );
				__T* tmp = new __T[_alloc];
				if ( _data != NULL && _size != 0 )
					__memcpy( tmp, _data, sizeof( __T ) * (_size+1) );
				delete[] _data;
				_data = tmp;
			}
		}
		
	public:
		
		stack_t() : _alloc( 0 ), _size( 0 ), _data( NULL ) { }
		stack_t( const __T& item) : _alloc( 1 ), _size( 1 ), _data( new __T[_alloc] ) {
			_data[0] = item;
		}
		stack_t( const stack_t<__T>& copy )
			: _alloc( copy._alloc ), _size( copy._size ), _data( new __T[_alloc] ) {
			for ( uint i=0;i<_size;i++ )
				_data[i] = copy._data[i];
		}
		~stack_t() { delete[] _data; }
		stack_t& operator=( const stack_t& st ) {
			_alloc = st._alloc;
			_size = st._size;
			_data = new __T[_alloc];
			for ( uint i=0;i<_size;i++ )
				_data[i] = st._data[i];
		}
		
		void clear() { _size = 0; }
		
		void push() {
			__realloc( _size+1 );
			_data[_size] = __T();
			_size ++ ;
		}
		void push( const __T& i ) {
			__realloc( _size+1 );
			_data[_size] = i;
			_size ++ ;
		}
		
		void pop() {
			if ( _size == 0 )
				return;
			_size -- ;
		}
		void pop( __T& output ) {
			if ( _size == 0 )
				return;
			_size -- ;
			output = _data[_size];
		}
		
		__T& operator[] ( uint x ) {
			if ( x >= _size )
				return *_data;
			return _data[_size-x-1];
		}
		const __T& operator[] ( uint x ) const {
			if ( x >= _size )
				return *_data;
			return _data[_size-x-1];
		}
		
		uint size() const { return _size; }
		bool empty() const { return ( _size == 0 ); }
};

// ARRAY
template<typename T>
struct array_t
{
	private:
		
	uint _sz, _alloc;
	T* _data;
	
	void __realloc( uint x ) {
		if ( x > _alloc ) {
			while ( _alloc < x )
				_alloc = std::max( 1u, _alloc * 2 );
			T* tmp = new T[_alloc];
			for ( uint i=0;i<_sz;i++ )
				tmp[i] = _data[i];
			delete[] _data;
			_data = tmp;
		}
	}
	
	public:
	
	array_t() : _sz( 0 ), _alloc( 0 ), _data( NULL ) { }
	array_t(const array_t<T>& copy)
		: _sz( copy._sz ), _alloc( copy._alloc ), _data( new T[_alloc] ) {
		for ( uint i=0;i<_sz;i++ )
			_data[i] = copy._data[i];
	}
	const array_t<T>& operator= (const array_t<T>& copy) {
		delete[] _data;
		
		_sz = copy._sz;
		_alloc = copy._alloc;
		_data = new T[_alloc];
		for ( uint i=0;i<_sz;i++ )
			_data[i] = copy._data[i];
		return *this;
	}
	
	array_t( uint x ) : _sz( 0 ), 
						_alloc( next_pow2(x) ), 
						_data( new T[_alloc] ) { }
	~array_t() { delete[] _data; }
	
	T* data() { return _data; }
	const T* data() const { return _data; }
	
	T& operator[] (uint x) { return _data[x]; }
	const T& operator[] (uint x) const { return _data[x]; }
	
	void append( const T& x ) {
		__realloc( _sz + 1 );
		_data[_sz] = x;
		_sz ++ ;
	}
	
	uint size() const { return _sz; }
};

// CHARSET
template<typename T>
struct re_char_set {
	protected:
		
		struct __vec {
			T x, y;
			__vec() : x(0), y(0) { ; }
			__vec( T a, T b ) : x(a), y(b) { ; }
		};
		array_t<__vec> data;
		
	public:
		
		void include( T t ) {
			data.append( __vec(t,t) );
		}
		void include( T a, T b ) {
			data.append( __vec(a,b) );
		}
		
		bool check( T t ) const {
			for ( uint i=0;i<data.size();i++ )
				if ( data[i].x <= t && t <= data[i].y )
					return true;
			return false;
		}
		bool check( const T* str ) const {
			while ( *str != '\0' ) {
				if ( !check( *str ) )
					return false;
				str ++ ;
			} return true;
		}
		
		uint size() const {
			return data.size();
		}
};

template<typename T>
bool operator== ( T ch, const re_char_set<T>& chs ) {
	return chs.check( ch );
}
template<typename T>
bool operator== ( const re_char_set<T>& chs, T ch ) {
	return chs.check( ch );
}

template<typename T>
bool operator== ( const T* ch, const re_char_set<T>& chs ) {
	return chs.check( ch );
}
template<typename T>
bool operator== ( const re_char_set<T>& chs, const T* ch ) {
	return chs.check( ch );
}

/*
Meta Characters
A- . 	 - single character (ANY character)
A- [] 	 - matches a single character within the brackets --
A- [^]	 - matches a single character NOT within the brackets --
A- ^	 - start of the line / start of the string --
A- $	 - end of the line / end of the string --
I- *	 - match preceding element zero or more times --
I- {m,n} - match from m instances to n instances (leave out either for unbounded) --
I- ?	 - match preceding element zero or one times --
I- +	 - match preceding element one or more times --
G- ()	 - capture group --
G- |	 - choose one or the other (e.g. cat|dog chooses "cat" or "dog") --
*/

struct REGEX_EXPORT regex_substring {
	const char* begin;
	const char* end;
	uint line_number;
	uint char_number;
};

struct REGEX_EXPORT regex;

struct REGEX_EXPORT regex_atom {
	regex* parent;
	// linked list redirection
	regex_atom* success; // if the atom matches, go here
	regex_atom* failure; // if the atom does not match, go here
	
	// virtual function to check success or failure to match
	// -- 	a success will return the pointer of the string where
	// 		the check ended, allowing further checking
	// --	a failure will return a NULL pointer
	virtual const char* check( const char* ) const = 0;
	regex_atom() : success( NULL ), failure( NULL ) { ; }
};

// quantify an atom
// --	must exist in the string <lower> number of
// 		times or more, and <upper> number of times
// 		or fewer
// --	if <upper> is -1 there is no upper limit
struct REGEX_EXPORT regex_quantifier : regex_atom {
	int lower;
	int upper;
	uint* current;
};

// quantifier types
// --	match a literal string
struct REGEX_EXPORT regex_literal : regex_quantifier {
	const char* lit;
	const char* check( const char* ) const;	
};
// --	match a character in a set
struct REGEX_EXPORT regex_charset : regex_quantifier {
	bool exclude;
	re_char_set<char>* cset;
	const char* check( const char* ) const;	
};
// --	group atoms together
struct REGEX_EXPORT regex_group_init : regex_quantifier {
	bool hit;
	const char* check( const char* ) const;
};
struct REGEX_EXPORT regex_group_finish : regex_atom {
	regex_group_init* start;
	const char* check( const char* ) const;	
};

// special characters
// --	finds the start of a line or file
struct REGEX_EXPORT regex_start_line : regex_quantifier {
	const char** compare;
	const char* check( const char* ) const;
};
// --	finds the end of a line or file
struct REGEX_EXPORT regex_end_line : regex_quantifier {
	const char* check( const char* ) const;	
};

struct regex {
	regex_quantifier* initial;
	stack_t<regex_quantifier*> open;
	array_t<uint> repeats;
};

REGEX_FUNCTION( regex*, regex_create, ( const char* ) );
REGEX_FUNCTION( regex_substring, regex_match, ( regex*, const char* ) );
REGEX_FUNCTION( void, regex_search, ( regex*, const char*, array_t<regex_substring>& ) );

#endif//__REGULAR_EXPRESSIONS_H__