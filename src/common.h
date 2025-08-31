#pragma once

#include "volk.h"
#include <iostream>
#include <sstream>
#include <string>


#define VK(call) {\
auto __result = (call);\
if (__result != VK_SUCCESS) {\
    std::stringstream ss;\
    ss << "Vk error in " << __FILE__ << " at line " << __LINE__ << " with error " << __result << std::endl;\
    throw std::runtime_error(ss.str());\
}\
}\
