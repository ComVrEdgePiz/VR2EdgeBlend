/* 
 * File:   parser.h
 * Author: flatline
 *
 * Created on December 15, 2009, 7:07 PM
 */

#ifndef _PARSER_H
#define	_PARSER_H
#include <compiz-core.h>
#include <libxml2/libxml/parser.h>
#include <libxml2/libxml/tree.h>


#ifdef	__cplusplus
extern "C" {
#endif

int load_config(char * file);



#ifdef	__cplusplus
}
#endif

#endif	/* _PARSER_H */

