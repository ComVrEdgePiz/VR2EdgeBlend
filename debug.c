#include "debug.h"

void
printOutputdev(CompOutput * output)
{
    compLogMessage ("edgeblend", CompLogLevelInfo,"outputDev %s (%d %dx%d): %d/%d -> %d/%d",
            output->name, output->id, output->width, output->height,
            output->region.extents.x1,output->region.extents.y1,
            output->region.extents.x2,output->region.extents.y2
            );
}
