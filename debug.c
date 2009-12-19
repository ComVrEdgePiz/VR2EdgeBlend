#include "debug.h"

/**
 * Prints some data about the Output to the log
 *
 * @PARAM CompOutput    *output - Compiz Output
 */
void
printOutputdev(CompOutput *output)
{
    compLogMessage ("edgeblend", CompLogLevelInfo,"outputDev %s (%d %dx%d): %d/%d -> %d/%d",
            output->name, output->id, output->width, output->height,
            output->region.extents.x1,output->region.extents.y1,
            output->region.extents.x2,output->region.extents.y2
            );
}
