typedef struct _EgdeblendOutputCell {
    int height;
    int width;
} EdgeblendOutputCell;

typedef struct _EdgeblendOutputGrid {
    int rows;
    int cols;
    int blend;
} EdgeblendOutputGrid;

typedef struct _EdgeblendOutputBlendFunc {
    double a,b,c; //a*x^2 + b*x + c
} EdgeblendOutputBlendFunc;

typedef struct _EdgeblendOutputScreen {
    EdgeblendOutputBlendFunc left;
    EdgeblendOutputBlendFunc top;
    EdgeblendOutputBlendFunc right;
    EdgeblendOutputBlendFunc bottom;
} EdgeblendOutputScreen;

typedef struct _EdgeblendOutputConfig {
    EdgeblendOutputGrid      grid;
    EdgeblendOutputCell      cell;
    EdgeblendOutputScreen*   screens;
    char* imagepath;
} EdgeblendOutputConfig;