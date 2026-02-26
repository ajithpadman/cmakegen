add_library(stm32_uart STATIC uart_stm32.c)
target_include_directories(stm32_uart PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
