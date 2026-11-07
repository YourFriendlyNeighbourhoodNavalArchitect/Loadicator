# loadicatorSetWarnings(<target>): strict warnings on every supported compiler.
function(loadicatorSetWarnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
        if(LOADICATOR_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion
            -Wnon-virtual-dtor -Wold-style-cast -Wcast-align -Woverloaded-virtual
            -Wdouble-promotion -Wimplicit-fallthrough
            # Not -Wnull-dereference: at -O2/-O3 GCC 13 reports it inside libstdc++'s <streambuf>.
            # Designated initialisers may omit members that have default initialisers.
            -Wno-missing-field-initializers)
        if(LOADICATOR_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()
