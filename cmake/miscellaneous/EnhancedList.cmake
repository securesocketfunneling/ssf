#
# Define some enhancement to list object
#
if(__H_ENHANCED_LIST_INCLUDED)
  return()
endif()
set(__H_ENHANCED_LIST_INCLUDED TRUE)

# ==============================================================================
# ======================= D e f i n e   f u n c t i o n s ======================
# ==============================================================================

# ------------------------------------------------------------------------------
# list_join(
#   <list_name>    # Name of the list
#   <separator>    # String to use as separator for each list items
#   <destination>  # Name of the variable that will host the resulting string
# )
#
# This function joins all items of a list into a string. Each items stored in
# the destination string will be separated with a given string separator.
macro(list_join list_name separator string_name)
  string(REGEX REPLACE
    "([^\\\\]|^);"
    "\\1${separator}"
    ${string_name}
    "${${list_name}}"
  )
endmacro()
