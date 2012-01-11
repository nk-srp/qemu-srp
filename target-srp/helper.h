#include "def-helper.h"

//dxw begin
DEF_HELPER_2(add_cc, i32, i32, i32)
DEF_HELPER_2(sub_cc, i32, i32, i32)

DEF_HELPER_1(ror_cc, i32, i32)
DEF_HELPER_1(rol_cc, i32, i32)
DEF_HELPER_1(rcr_cc, i32, i32)
DEF_HELPER_1(rcl_cc, i32, i32)
//dxw end

DEF_HELPER_1(exception, void, i32)

#include "def-helper.h"
