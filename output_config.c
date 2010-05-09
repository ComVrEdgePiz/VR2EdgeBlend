/*
 * File:   output_config.c
 * Author: Markus Knofe, Alexander Treptow
 *
 * Created on May 05, 2010, 2:43 PM
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
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
allocate_screenspath(int i)
{
    char *s = NULL;

    s = (char *) calloc(i, sizeof(char));

    return s;
}

static void
initialize_screens(EdgeblendOutputScreen* s, int m) {
  int i; 
  for(i=0;i<m;i++) {
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
    int screensFound = 0;
    int screensAvalible = 0;
    
    if (2 <= xmlChildElementCount(root)) {
        //parse Grid
        for (section = root->children; section; section = section->next) {
            if (strcmp((char *)section->name, "grid") == 0) {
                for (option = section->children; option; option = option->next) {
                    if (FALSE) {
                        //....
                    } else if (strcmp((char*)option->name, "rows") == 0) {
                        config->grid.rows  = atoi((char*)option->children->content);
                    } else if (strcmp((char*)option->name, "cols") == 0) {
                        config->grid.cols  = atoi((char*)option->children->content);
                    } else if (strcmp((char*)option->name, "blend") == 0) {
                        config->grid.blend = atoi((char*)option->children->content);
                    } else if (strcmp((char*)option->name, "cell") == 0) {
                        for (subsection = option->children; subsection; subsection = subsection->next) {
                            if (FALSE) {
                                //....
                            } else if (strcmp((char*)subsection->name, "height") == 0) {
                                config->cell.height = atoi((char*)subsection->children->content);
                            } else if (strcmp((char*)subsection->name, "width") == 0) {
                                config->cell.width  = atoi((char*)subsection->children->content);
                            } //option check
                        }
                    }
                }
            }
        }

        screensAvalible = config->grid.cols * config->grid.rows;
        config->screens = allocate_screenconfig(screensAvalible);
        config->imagepath = 0;
        initialize_screens(config->screens, screensAvalible);
        
        //parse Screens
        for (section = root->children; section; section = section->next) {
            if (strcmp((char *)section->name, "screens") == 0) {
                for (option = section->children; option; option = option->next) {
                    if (strcmp((char*)option->name, "screen") == 0 && screensFound<screensAvalible) {
                        
                        for (subsection = option->children; subsection; subsection = subsection->next) {                        

                            if (strcmp((char*)subsection->name, "left") == 0) {
                                for (subsubsection = subsection->children; subsubsection; subsubsection = subsubsection->next) {
                                  parse_screen_config(subsubsection, &(config->screens[screensFound].left));
                                }
                            } else if (strcmp((char*)subsection->name, "top") == 0) {
                                for (subsubsection = subsection->children; subsubsection; subsubsection = subsubsection->next) {
                                  parse_screen_config(subsubsection, &(config->screens[screensFound].top));
                                }
                            } else if (strcmp((char*)subsection->name, "right") == 0) {
                                for (subsubsection = subsection->children; subsubsection; subsubsection = subsubsection->next) {
                                  parse_screen_config(subsubsection, &(config->screens[screensFound].right));
                                }
                            } else if (strcmp((char*)subsection->name, "bottom") == 0) {
                                for (subsubsection = subsection->children; subsubsection; subsubsection = subsubsection->next) {
                                  parse_screen_config(subsubsection, &(config->screens[screensFound].bottom));
                                }
                            }
                            
                        }
                        screensFound++;
                    }
                }
            } else if (strcmp((char *)section->name, "image") == 0) {
                config->imagepath = allocate_screenspath(strlen((char*)section->children->content));
                memcpy(config->imagepath, (void*)section->children->content, strlen((char*)section->children->content));
                screensFound = screensAvalible;
            }
        }
        configCheck =  (config->grid.blend  > 0)
                    && (config->grid.rows   > 0)
                    && (config->grid.cols   > 0)
                    && (config->cell.height > 0)
                    && (config->cell.width  > 0)
                    && (config->cell.height > config->grid.blend)
                    && (config->cell.width  > config->grid.blend)
                    && ( (config->grid.rows + config->grid.cols) > 2)
                    && (screensFound == screensAvalible); //due to for no +1 for foundscreens

        compLogMessage ("edgeblend::output_config->load_outputconfig",
                        CompLogLevelInfo,
                        "Cell: %d/%d  Grid: %dx%d (%d) screencfgs: %d/%d",
                        config->cell.width, config->cell.height,
                        config->grid.rows, config->grid.cols, config->grid.blend,
                        screensFound, screensAvalible);
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
