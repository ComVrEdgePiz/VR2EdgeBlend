#include "parser.h"
/**
 * Loads the XML-config-file
 *
 * @PARAM char *file - Path to the config-file
 */
int
load_config(char *file)
{
    xmlDoc  *doc          = NULL;
    xmlNode *root_element = NULL;
    /*parse the file and get the DOM */

    compLogMessage ("edgeblend.edge::load_config", CompLogLevelInfo, "parsing file %s...",file);
    doc = xmlReadFile(file, NULL, XML_PARSE_NOCDATA);


    if (doc == NULL) {
        compLogMessage ("edgeblend.edge::load_config", CompLogLevelError, "could not parse %s",file);
    } else {
        /*Get the root element node */
        root_element = xmlDocGetRootElement(doc);
        compLogMessage ("edgeblend.edge::load_config", CompLogLevelInfo, "%s parsed, root_element: %s", file, root_element->name);

        //do even more with the root_elem

        /*free the document */
        xmlFreeDoc(doc);
    }
    /*
     * Free the global variables that may
     * have been allocated by the parser.
     */
    xmlCleanupParser();
    return 1;
}
