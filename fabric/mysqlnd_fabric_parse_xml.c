/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2013 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Andrey Hristov <andrey@php.net>                              |
  |         Ulf Wendel <uw@php.net>                                      |
  |         Johannes Schlueter <johannes@php.net>                        |
  +----------------------------------------------------------------------+
*/


#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml2/libxml/tree.h>

#include "zend.h"
#include "zend_alloc.h"
#include "fabric/mysqlnd_fabric.h"

static xmlXPathObjectPtr mysqlnd_fabric_find_value_nodes(xmlDocPtr doc)
{
	xmlXPathObjectPtr retval;
    xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);
    if(xpathCtx == NULL) {
        xmlFreeDoc(doc); 
        return;
    }
    
    retval = xmlXPathEvalExpression("//params/param/value/array/data/value[3]/array/data/value", xpathCtx);
	xmlXPathFreeContext(xpathCtx); 
	
	return retval;
}

static char *myslqnd_fabric_get_actual_value(char *xpath, xmlXPathContextPtr xpathCtx)
{
	char *retval;
	xmlXPathObjectPtr xpathObj = xpathObj = xmlXPathEvalExpression(xpath, xpathCtx);
	
	if (xpathObj == NULL) {
		return NULL;
	}
	
	retval = xpathObj->nodesetval->nodeTab[0]->children->content;
	
	xmlXPathFreeObject(xpathObj);
	
	return retval;
}

static int mysqlnd_fabric_fill_server_from_value(xmlNodePtr node, mysqlnd_fabric_server *server)
{
	xmlXPathContextPtr xpathCtx = xmlXPathNewContext(node);
	char *tmp;
	
	if (xpathCtx == NULL) {
		return 1;
	}
	
	tmp = myslqnd_fabric_get_actual_value("//array/data/value[1]/string", xpathCtx);
	if (!tmp) {
		xmlXPathFreeContext(xpathCtx);
		return 1;
	}
	
	server->uuid = estrdup(tmp);

	tmp = myslqnd_fabric_get_actual_value("//array/data/value[2]/string", xpathCtx);
	if (!tmp) {
		xmlXPathFreeContext(xpathCtx);
		return 1;
	}
	
	server->hostname = estrdup(tmp);
	
	tmp = strchr(server->hostname, ':');
	*tmp = '\0';
	server->port = atoi(&tmp[1]);

	tmp = myslqnd_fabric_get_actual_value("//array/data/value[3]/boolean", xpathCtx);
	if (!tmp) {
		xmlXPathFreeContext(xpathCtx);
		return 1;
	}

	switch (tmp[0]) {
	case '0': server->master = 0; break;
	case '1': server->master = 1; break;
	default:
		xmlXPathFreeContext(xpathCtx);
		return 1;
	}

	xmlXPathFreeContext(xpathCtx);
	
	return 0;
}

mysqlnd_fabric_server *mysqlnd_fabric_parse_xml(char *xmlstr, int xmlstr_len)
{
	mysqlnd_fabric_server *retval;
    xmlDocPtr doc;
    xmlXPathObjectPtr xpathObj1;
	int i;

	LIBXML_TEST_VERSION
	doc = xmlParseMemory(xmlstr, xmlstr_len);

	if (doc == NULL) {
		return NULL;
    }
	
	xpathObj1 = mysqlnd_fabric_find_value_nodes(doc);
	if (!xpathObj1) {
		xmlFreeDoc(doc);
		return NULL;
	}
	
	if (!xpathObj1->nodesetval) {
		/* Verbose debug info in /methodresponse/params/param/value/array/data/value[2]/array/data/value[3]/struct/member/value/string */
		xmlXPathFreeObject(xpathObj1);
		xmlFreeDoc(doc);
		return NULL;
	}
	
	retval = safe_emalloc(xpathObj1->nodesetval->nodeNr+1, sizeof(mysqlnd_fabric_server), 0);
	for (i = 0; i < xpathObj1->nodesetval->nodeNr; i++) {
		if (mysqlnd_fabric_fill_server_from_value(xpathObj1->nodesetval->nodeTab[i], &retval[i])) {
			xmlXPathFreeObject(xpathObj1);
			xmlFreeDoc(doc);
			return NULL;
		}
	}
	
	retval[i].hostname = NULL;
	retval[i].port = 0;
	

    xmlXPathFreeObject(xpathObj1);
    xmlFreeDoc(doc); 
	
	return retval;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
