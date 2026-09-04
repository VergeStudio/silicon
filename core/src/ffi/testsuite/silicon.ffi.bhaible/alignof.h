#ifndef _ALIGNOF_H
#define _ALIGNOF_H

#include <stddef.h>

#if defined __cplusplus
template<class type>
struct alignof_helper {
    char __slot1;
    type __slot2;
};
#    define alignof_slot(type) offsetof(alignof_helper<type>, __slot2)
#else
#    define alignof_slot(type) offsetof(struct { char __slot1; type __slot2; }, __slot2)
#endif

#if defined __GNUC__ || defined __IBM__ALIGNOF__
#    define alignof_type __alignof__
#else
#    define alignof_type alignof_slot
#endif

#endif 
