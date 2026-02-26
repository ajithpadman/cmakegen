add_library(generic_uart STATIC uart_generic.c)
target_include_directories(generic_uart PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
