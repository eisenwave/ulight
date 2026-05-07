#define type_name(x) _Generic((x), \
    int: "int", \
    double: "double", \
    default: "other")

const char* t = type_name(1.5);
