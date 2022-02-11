AC_DEFUN([FW_TEMPLATE_CXX_ENABLE_COVERAGE],
[
  AC_REQUIRE([FW_TEMPLATE_C_ENABLE_COVERAGE])

  if test "x[$]FW_ENABLE_COVERAGE" = "x1"
    then
      CXXFLAGS="`echo \"[$]CXXFLAGS\" | perl -pe 's/-O\d+//g;'` -fprofile-arcs -ftest-coverage"
    fi
])
