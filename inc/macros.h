#ifndef TERMIX_COMMON_INCLUDE_MACROS_H
#define TERMIX_COMMON_INCLUDE_MACROS_H

/*
 * macro for defining structs and etc. with Termix prefix and typedefs
 *
 * must be paired when using
 */
#define _tmix_typedef(_type, _name)         _type _tmix##_name
#define _tmix_typedef_end(_type, _name)     typedef _type _tmix##_name tmix##_name

#endif /* TERMIX_COMMON_INCLUDE_MACROS_H */
