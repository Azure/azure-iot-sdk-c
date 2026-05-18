#include "macro_utils/macro_utils.h"

#define MODEL_PROPERTY(type, name) type, name
#define MODEL_ACTION(...)
#define MODEL_METHOD(...)
#define MODEL_DESIRED_PROPERTY(type, name, ...)
#define MODEL_REPORTED_PROPERTY(type, name, ...)
#define MODEL_IN_MODEL(type, name)
#define WITH_DATA(type, name) MODEL_PROPERTY(type, name)

#define CREATE_DESIRED_PROPERTY_CALLBACK(...) CREATE_DESIRED_PROPERTY_CALLBACK_##__VA_ARGS__
#define CREATE_DESIRED_PROPERTY_CALLBACK_MODEL_PROPERTY(...)

MU_FOR_EACH_1(CREATE_DESIRED_PROPERTY_CALLBACK, WITH_DATA(double, this_is_double))
