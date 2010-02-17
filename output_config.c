#include "output_config.h"
#include <string.h>

static EdgeblendOutputConfig *
allocate_outputconfig(void)
{
    EdgeblendOutputConfig *config = NULL;

    config = (EdgeblendOutputConfig *) calloc(1, sizeof(EdgeblendOutputConfig));

    return config;
}

static EdgeblendOutputScreen *
allocate_screenconfig(int i)
{
    EdgeblendOutputScreen *s = NULL;

    s = (EdgeblendOutputScreen *) calloc(i, sizeof(EdgeblendOutputScreen));

    return s;
}

static char*
allocate_screenpath(int i)
{
    char *s = NULL;

    s = (char *) calloc(i, sizeof(char));

    return s;
}

static void
initialize_screens(EdgeblendOutputScreen* s, int m) {
  int i; 
  for(i=0;i<m;i++) {
    s[i].imagepath = NULL;
    s[i].left.a = s[i].left.b = s[i].left.c = 0.0;
    s[i].top.a = s[i].top.b = s[i].top.c = 0.0;
    s[i].right.a = s[i].right.b = s[i].right.c = 0.0;
    s[i].bottom.a = s[i].bottom.b = s[i].bottom.c = 0.0;
  }
}

static void
free_outputconfig(EdgeblendOutputConfig *config)
{
    if(config->screens){
      int screens = config->grid.cols*config->grid.rows;
      int i;
      for(i=0;i < screens; i++)
        if(config->screens[i].imagepath)
          free(config->screens[i].imagepath);
      free(config->screens);
    }
    free(config);
}

static void
parse_screen_config(xmlNode* subsubsection,EdgeblendOutputBlendFunc* b) {
  if (strcmp((char*)subsubsection->name, "a") == 0) {
      b->a = atof((char*)subsubsection->children->content);
  } else if (strcmp((char*)subsubsection->name, "b") == 0) {
      b->b = atof((char*)subsubsection->children->content);
  } else if (strcmp((char*)subsubsection->name, "c") == 0) {
      b->c = atof((char*)subsubsection->children->content);
  } 
}

static Bool
parse_outputconfig(xmlNode *root, EdgeblendOutputConfig *config)
{
    Bool configCheck    = TRUE;
    xmlNode *section    = NULL;
    xmlNode *option     = NULL;
    xmlNode *subsection = NULL;
    xmlNode *subsubsection = NULL;
    
    if (3 <= xmlChildElementCount(root)) {
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
                } else if (strcmp((char*)section->name, "screens") == 0) {
                  int s = config->grid.cols * config->grid.rows;
                  config->screens = allocate_screenconfig(s);
                  initialize_screens(config->screens,s);
                  int i;
                  for (i = 0, subsection = option->children; i<s; subsection = subsection->next, i++) {
                    if (!subsection)
                      compLogMessage ("edgeblend::output_config->parse_outputconfig", CompLogLevelError, "not enough screen definitions for grid");
                    if (strcmp((char*)option->name, "screen") == 0) {
                        if (strcmp((char*)subsection->name, "imagemask") == 0) {
                            char* s = allocate_screenpath(strlen((char*)option->children->content));
                            s = (char*)option->children->content;
                            config->screens[i].imagepath = s;
                        } else {
                          for (subsubsection = subsection->children; subsubsection; subsubsection = subsubsection->next) {
                            if (strcmp((char*)subsection->name, "left") == 0) {
                              parse_screen_config(subsubsection, &(config->screens[i].left));
                            } else if (strcmp((char*)subsection->name, "top") == 0) {
                              parse_screen_config(subsubsection, &(config->screens[i].top));
                            } else if (strcmp((char*)subsection->name, "right") == 0) {
                              parse_screen_config(subsubsection, &(config->screens[i].right));
                            } else if (strcmp((char*)subsection->name, "bottom") == 0) {
                              parse_screen_config(subsubsection, &(config->screens[i].bottom));
                            }
                          }
                        } //option check
                    } //subsection check
                  }
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
