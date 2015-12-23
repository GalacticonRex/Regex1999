#ifndef __QKS_HEADER_H__
#define __QKS_HEADER_H__

#ifdef __QKS_EXEC__
	#define QKS_FUNCTION(x,y,z) x y z
	#define QKS_EXPORT
#else
	#ifdef __QKS_EXPORT__
		#define QKS_FUNCTION(x,y,z) x __declspec(dllexport) y z
		#define QKS_EXPORT __declspec(dllexport)
	#else
		#define QKS_FUNCTION(x,y,z) x __declspec(dllimport) y z
		#define QKS_EXPORT __declspec(dllimport)
	#endif
#endif

#include "regex.h"

struct QKSMatch {
	QKSDef* 
};
struct QKSDef {
	const char* name;
	
};
struct QKSNode {
	regex* func;
	QKSNode* 
	QKSNode* 
};
struct QKSTable {
	
};

QKS_FUNCTION( void, qks_create_from_file, ( const char* fname ) );
QKS_FUNCTION( void, qks_create_from_text, ( const char* text ) );

#endif//__QKS_HEADER_H__