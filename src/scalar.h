#ifndef SCALAR_HPP
#define SCALAR_HPP

#define MIN(A, B) ({\
    __auto_type A_ = (A);\
    __auto_type B_ = (B);\
    A_ < B_ ? A_ : B_;\
})

#define MAX(A, B) ({\
    __auto_type A_ = (A);\
    __auto_type B_ = (B);\
    A_ > B_ ? A_ : B_;\
})

#define ABS(A) ({\
    __auto_type A_ = (A);\
    A_ < 0 ? -A_ : A_;\
})

#endif
