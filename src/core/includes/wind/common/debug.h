#ifndef WIND_COMMON_DEBUG_H
#define WIND_COMMON_DEBUG_H


#include <iostream>

#define LOG(x) std::cerr << "[LOG][" << __FILE__ << ":" << __LINE__ << "] " << #x << " = " << x << std::endl

#endif