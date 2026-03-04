# ANTLR4 C++ code generation from .g4 grammar.
# Requires: Java, and either ANTLR4_JAR or we download antlr-4.13.1-complete.jar.
# Usage: antlr4cpp_generate(GrammarName grammar/ConditionExpr.g4
#                           OUTPUT_DIR ${CMAKE_BINARY_DIR}/generated
#                           VISITOR)
#        Then add ${ANTLR4CPP_Generated_SOURCES} and include ${ANTLR4CPP_Generated_INCLUDE_DIR}

set(ANTLR4_VERSION "4.13.1")
set(ANTLR4_JAR_NAME "antlr-${ANTLR4_VERSION}-complete.jar")

if(NOT ANTLR4_JAR)
    set(ANTLR4_JAR_DOWNLOAD "${CMAKE_BINARY_DIR}/${ANTLR4_JAR_NAME}")
    if(NOT EXISTS "${ANTLR4_JAR_DOWNLOAD}")
        message(STATUS "Downloading ${ANTLR4_JAR_NAME}...")
        # Try Maven Central (reliable) then GitHub releases
        file(DOWNLOAD
            "https://repo1.maven.org/maven2/org/antlr/antlr4/${ANTLR4_VERSION}/antlr4-${ANTLR4_VERSION}-complete.jar"
            "${ANTLR4_JAR_DOWNLOAD}"
            TLS_VERIFY ON
            STATUS _dl_status
        )
        list(GET _dl_status 0 _dl_code)
        if(NOT _dl_code EQUAL 0)
            list(GET _dl_status 1 _dl_msg)
            message(FATAL_ERROR "Failed to download ANTLR jar: ${_dl_msg}. Set -DANTLR4_JAR=/path/to/antlr-${ANTLR4_VERSION}-complete.jar")
        endif()
    endif()
    set(ANTLR4_JAR "${ANTLR4_JAR_DOWNLOAD}")
endif()

if(NOT EXISTS "${ANTLR4_JAR}")
    message(FATAL_ERROR "ANTLR4_JAR not found: ${ANTLR4_JAR}. Set -DANTLR4_JAR=/path/to/antlr-4.x-complete.jar")
endif()

find_package(Java COMPONENTS Runtime REQUIRED)

function(antlr4cpp_generate GRAMMAR_NAME GRAMMAR_FILE)
    cmake_parse_arguments(ARG "VISITOR;LISTENER" "" "OUTPUT_DIR" ${ARGN})
    if(NOT ARG_OUTPUT_DIR)
        set(ARG_OUTPUT_DIR ${CMAKE_BINARY_DIR}/antlr4_generated)
    endif()
    file(MAKE_DIRECTORY ${ARG_OUTPUT_DIR})

    get_filename_component(GRAMMAR_ABS ${GRAMMAR_FILE} ABSOLUTE)
    if(NOT EXISTS ${GRAMMAR_ABS})
        message(FATAL_ERROR "Grammar file not found: ${GRAMMAR_ABS}")
    endif()

    set(GEN_FLAGS -Dlanguage=Cpp -no-listener -visitor)
    if(ARG_LISTENER)
        set(GEN_FLAGS -Dlanguage=Cpp -listener -visitor)
    endif()

    set(GEN_OUTPUT ${ARG_OUTPUT_DIR})
    add_custom_command(
        OUTPUT
            ${GEN_OUTPUT}/${GRAMMAR_NAME}Lexer.cpp
            ${GEN_OUTPUT}/${GRAMMAR_NAME}Lexer.h
            ${GEN_OUTPUT}/${GRAMMAR_NAME}Parser.cpp
            ${GEN_OUTPUT}/${GRAMMAR_NAME}Parser.h
            ${GEN_OUTPUT}/${GRAMMAR_NAME}BaseVisitor.cpp
            ${GEN_OUTPUT}/${GRAMMAR_NAME}BaseVisitor.h
            ${GEN_OUTPUT}/${GRAMMAR_NAME}Visitor.h
        COMMAND ${Java_JAVA_EXECUTABLE} -jar "${ANTLR4_JAR}"
            ${GEN_FLAGS}
            -o "${GEN_OUTPUT}"
            "${GRAMMAR_ABS}"
        DEPENDS ${GRAMMAR_ABS}
        COMMENT "Generating ANTLR C++ parser for ${GRAMMAR_NAME}"
        VERBATIM
    )

    set(ANTLR4CPP_${GRAMMAR_NAME}_SOURCES
        ${GEN_OUTPUT}/${GRAMMAR_NAME}Lexer.cpp
        ${GEN_OUTPUT}/${GRAMMAR_NAME}Parser.cpp
        ${GEN_OUTPUT}/${GRAMMAR_NAME}BaseVisitor.cpp
        PARENT_SCOPE
    )
    set(ANTLR4CPP_${GRAMMAR_NAME}_INCLUDE_DIR ${GEN_OUTPUT} PARENT_SCOPE)
    set(ANTLR4CPP_${GRAMMAR_NAME}_OUTPUT_DIR ${GEN_OUTPUT} PARENT_SCOPE)
endfunction()
