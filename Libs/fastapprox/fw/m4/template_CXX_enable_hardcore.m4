AC_DEFUN([FW_TEMPLATE_CXX_ENABLE_HARDCORE],
[
  AC_REQUIRE([FW_TEMPLATE_C_ENABLE_HARDCORE])

  if test "x[$]FW_ENABLE_HARDCORE" = "x1"
    then
      HARDCORECPPFLAGS=`echo "$HARDCORECPPFLAGS" | perl -pe 's%-Wmissing-prototypes%%g;'`
    fi
])
