#include "platform_config.h"

#define TFT_LINEARINT  1
#define LINEARINT_HACK 1

//#define FLIP_SCREEN    1
//#define TFT_DEBUG      1
#if defined(__IMXRT1052__) || defined(__IMXRT1062__)    
#define TFT_STATICFB   1
#endif
