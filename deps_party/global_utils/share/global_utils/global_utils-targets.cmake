
add_library(global_utils::global_utils INTERFACE IMPORTED)

set_target_properties(global_utils::global_utils PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${GlobalUtils_Root}/include"
)