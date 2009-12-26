#include "output_config.h"
#include <string.h>

static EdgeblendOutputConfig *
allocate_outputconfig(void)
{
    EdgeblendOutputConfig *config = NULL;

    config = (EdgeblendOutputConfig *) calloc(1, sizeof(EdgeblendOutputConfig));

    return config;
}

static void
free_outputconfig(EdgeblendOutputConfig *config)
{
    free(config);
}

static Bool
parse_outputconfig(xmlNode *root, EdgeblendOutputConfig *config)
{
    Bool configCheck    = TRUE;
    xmlNode *section    = NULL;
    xmlNode *option     = NULL;
    
    if (2 <= xmlChildElementCount(root)) {
        for (section = root->children; section; section = section->next) {
            for (option = section->children; option; option = option->next) {

                if (strcmp((char *)section->name, "grid") == 0) {
                    if (FALSE) {
                        //....
                    } else if (strcmp((char*)option->name, "rows") == 0) {
                        config->grid.rows  = atoi((char*)option->children->content);
                    } else if (strcmp((char*)option->name, "cols") == 0) {
                        config->grid.cols  = atoi((char*)option->children->content);
                    } else if (strcmp((char*)option->name, "blend") == 0) {
                        config->grid.blend = atoi((char*)option->children->content);
                    }
                } else if (strcmp((char*)section->name, "cell") == 0) {
                    if (FALSE) {
                        //....
                    } else if (strcmp((char*)option->name, "height") == 0) {
                        config->cell.height = atoi((char*)option->children->content);
                    } else if (strcmp((char*)option->name, "width") == 0) {
                        config->cell.width  = atoi((char*)option->children->content);
                    } //option check
                } // sectioncheck
            } //option
        } //section

        configCheck =  (config->grid.blend  > 0)
                    && (config->grid.rows   > 0)
                    && (config->grid.cols   > 0)
                    && (config->cell.height > 0)
                    && (config->cell.width  > 0)
                    && (config->cell.height > config->grid.blend)
                    && (config->cell.width  > config->grid.blend)
                    && ( (config->grid.rows + config->grid.cols) > 2);
               
        compLogMessage ("edgeblend::output_config->load_outputconfig",
                        CompLogLevelInfo,
                        "Cell: %d/%d  Grid: %dx%d (%d)",
                        config->cell.width, config->cell.height,
                        config->grid.rows, config->grid.cols, config->grid.blend);
        return configCheck;
    }
    return FALSE;
}


EdgeblendOutputConfig *
load_outputconfig(char * filepath, CompScreen *screen) {
    xmlDoc                  *doc            = NULL;
    xmlNode                 *root_element   = NULL;
    EdgeblendOutputConfig   *config         = NULL;

    /*parse the file and get the DOM */
    compLogMessage ("edgeblend::output_config->load_outputconfig", CompLogLevelInfo, "parsing file \"%s\"",filepath);
    doc = xmlReadFile(filepath, NULL, XML_PARSE_NOCDATA);

    if (doc == NULL) {
        compLogMessage ("edgeblend::output_config->load_outputconfig", CompLogLevelError, "could not parse \"%s\"",filepath);
    } else {
          /*Get the root element node */
        root_element = xmlDocGetRootElement(doc);
        compLogMessage ("edgeblend::output_config->load_outputconfig", CompLogLevelInfo, "\"%s\" parsed, root_element: \"%s\"", filepath, root_element->name);

        if (strcmp((char*)root_element->name, "output")) {
            compLogMessage ("edgeblend::output_config->load_outputconfig", CompLogLevelInfo, "expected root-element \"output\"");
        } else {
            config = allocate_outputconfig();
            if (config) {
                if ( !parse_outputconfig(root_element, config)) {
                    free_outputconfig(config);
                    config = NULL;
                }
            }
        }
        /*free the document */
        xmlFreeDoc(doc);
    }
    /*
     * Free the global variables that may
     * have been allocated by the parser.
     */
    xmlCleanupParser();

    return config;
}
