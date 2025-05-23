#ifndef _MACRO_UTILS_H
#define _MACRO_UTILS_H

#define X_EXPAND_CNT(...)		 +1
#define X_EXPAND_ENUM(NAME)		 NAME,
#define X_EXPAND_STRINGIFY(NAME) #NAME,

#define MS_TO_FREQ(ms) (1000 / (ms))
#define MS_TO_US(ms)   ((ms)*1000)

#endif // _MACRO_UTILS_H