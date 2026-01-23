# CMakeLists.txt for BitchX 1.3 - Modern Cross-Platform Build
# Supports Windows 11 (VS2022), Linux, macOS, and FreeBSD
cmake_minimum_required(VERSION 3.16)

project(BitchX
    VERSION 1.3.0
    DESCRIPTION "BitchX IRC Client"
    LANGUAGES C
)

# Require Windows 10/11 SDK (10.0.22000.0 or later for Win11 features)
if(WIN32)
    set(CMAKE_SYSTEM_VERSION "10.0" CACHE STRING "Windows SDK version")
endif()

# Options
option(BITCHX_WITH_SSL "Enable SSL/TLS support" ON)
option(BITCHX_WITH_GUI "Build Windows GUI version" ON)
option(BITCHX_WITH_PLUGINS "Enable plugin/DLL support" ON)
option(BITCHX_WITH_IDENTD "Enable built-in IdentD server" ON)
option(BITCHX_WITH_SOUND "Enable sound support (requires GUI)" OFF)

# C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Generate defs.h configuration header
include(CheckIncludeFile)
include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckSymbolExists)
include(CheckTypeSize)

# Platform checks
check_include_file("sys/socket.h" HAVE_SYS_SOCKET_H)
check_include_file("sys/time.h" HAVE_SYS_TIME_H)
check_include_file("sys/fcntl.h" HAVE_SYS_FCNTL_H)
check_include_file("fcntl.h" HAVE_FCNTL_H)
check_include_file("sys/file.h" HAVE_SYS_FILE_H)
check_include_file("netinet/in.h" HAVE_NETINET_IN_H)
check_include_file("netdb.h" HAVE_NETDB_H)
check_include_file("sys/un.h" HAVE_SYS_UN_H)
check_include_file("sys/filio.h" HAVE_SYS_FILIO_H)
check_include_file("regex.h" HAVE_REGEX_H)
check_include_file("dirent.h" HAVE_DIRENT_H)
check_include_file("arpa/inet.h" HAVE_ARPA_INET_H)
check_include_file("arpa/nameser.h" HAVE_ARPA_NAMESER_H)
check_include_file("resolv.h" HAVE_RESOLV_H)

check_function_exists(snprintf HAVE_SNPRINTF)
check_function_exists(vsnprintf HAVE_VSNPRINTF)
check_function_exists(setsid HAVE_SETSID)
check_function_exists(getpgid HAVE_GETPGID)
check_function_exists(gettimeofday HAVE_GETTIMEOFDAY)
check_function_exists(scandir HAVE_SCANDIR)
check_function_exists(strtoul HAVE_STRTOUL)
check_function_exists(inet_aton HAVE_INET_ATON)
check_function_exists(uname HAVE_UNAME)
check_function_exists(setenv HAVE_SETENV)

check_type_size("unsigned long long" UNSIGNED_LONG_LONG)

# Core source files (the IRC engine)
set(BITCHX_CORE_SOURCES
    source/alias.c
    source/alist.c
    source/array.c
    source/art.c
    source/banlist.c
    source/bot_link.c
    source/cdcc.c
    source/cdns.c
    source/chelp.c
    source/commands.c
    source/commands2.c
    source/compat.c
    source/cset.c
    source/ctcp.c
    source/dcc.c
    source/debug.c
    source/encrypt.c
    source/exec.c
    source/expr.c
    source/expr2.c
    source/files.c
    source/flood.c
    source/fset.c
    source/functions.c
    source/funny.c
    source/glob.c
    source/hash.c
    source/hebrew.c
    source/help.c
    source/history.c
    source/hook.c
    source/if.c
    source/ignore.c
    source/input.c
    source/irc.c
    source/ircaux.c
    source/ircsig.c
    source/keys.c
    source/lastlog.c
    source/list.c
    source/log.c
    source/mail.c
    source/misc.c
    source/modules.c
    source/names.c
    source/network.c
    source/newio.c
    source/notice.c
    source/notify.c
    source/numbers.c
    source/output.c
    source/parse.c
    source/queue.c
    source/readlog.c
    source/reg.c
    source/screen.c
    source/server.c
    source/stack.c
    source/status.c
    source/struct.c
    source/tcl_public.c
    source/term.c
    source/timer.c
    source/translat.c
    source/user.c
    source/userlist.c
    source/vars.c
    source/who.c
    source/whowas.c
    source/window.c
    source/words.c
)

# Windows-specific configuration
if(WIN32)
    # Define platform macros
    add_definitions(
        -DWINNT
        -DGUI
        -D_CRT_SECURE_NO_WARNINGS
        -D_WINSOCK_DEPRECATED_NO_WARNINGS
        -DWIN32_LEAN_AND_MEAN
        -D_WIN32_WINNT=0x0A00
        -DNTDDI_VERSION=0x0A00000B
        -DUNICODE
        -D_UNICODE
    )

    if(BITCHX_WITH_GUI)
        # Windows GUI build
        set(BITCHX_GUI_SOURCES source/winbitchx.c)

        add_executable(WinBitchX WIN32
            ${BITCHX_CORE_SOURCES}
            ${BITCHX_GUI_SOURCES}
            win32/bitchx.rc
        )

        target_include_directories(WinBitchX PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_BINARY_DIR}/include
        )

        target_link_libraries(WinBitchX PRIVATE
            ws2_32
            user32
            gdi32
            shell32
            advapi32
            comctl32
            ole32
            iphlpapi
            dwmapi
            uxtheme
            shcore
        )

        if(BITCHX_WITH_SOUND)
            target_compile_definitions(WinBitchX PRIVATE SOUND)
            target_link_libraries(WinBitchX PRIVATE winmm)
        endif()

        # Enable High-DPI awareness via linker
        set_target_properties(WinBitchX PROPERTIES
            VS_DPI_AWARE "PerMonitor"
            WIN32_EXECUTABLE TRUE
        )

        # Set Windows 11 manifest
        set_target_properties(WinBitchX PROPERTIES
            LINK_FLAGS "/MANIFEST:EMBED /MANIFESTINPUT:${CMAKE_CURRENT_SOURCE_DIR}/win32/bitchx.manifest"
        )

        # MSVC-specific flags
        if(MSVC)
            target_compile_options(WinBitchX PRIVATE
                /W3
                /utf-8
                /permissive-
            )
            # Use static runtime for easier distribution
            set_property(TARGET WinBitchX PROPERTY
                MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
            )
        endif()
    endif()

    # Console build (for Windows Terminal / PowerShell use)
    add_executable(BitchX
        ${BITCHX_CORE_SOURCES}
    )

    target_include_directories(BitchX PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}/include
    )

    target_compile_definitions(BitchX PRIVATE
        BITCHX_CONSOLE
    )

    target_link_libraries(BitchX PRIVATE
        ws2_32
        advapi32
        shell32
        iphlpapi
    )

    if(MSVC)
        target_compile_options(BitchX PRIVATE
            /W3
            /utf-8
            /permissive-
        )
        set_property(TARGET BitchX PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
        )
    endif()

    # SSL support
    if(BITCHX_WITH_SSL)
        find_package(OpenSSL)
        if(OpenSSL_FOUND)
            if(TARGET WinBitchX)
                target_compile_definitions(WinBitchX PRIVATE HAVE_SSL)
                target_link_libraries(WinBitchX PRIVATE OpenSSL::SSL OpenSSL::Crypto)
            endif()
            target_compile_definitions(BitchX PRIVATE HAVE_SSL)
            target_link_libraries(BitchX PRIVATE OpenSSL::SSL OpenSSL::Crypto)
        else()
            message(WARNING "OpenSSL not found. Building without SSL support. Install via vcpkg: vcpkg install openssl")
        endif()
    endif()

    # IdentD support
    if(BITCHX_WITH_IDENTD)
        if(TARGET WinBitchX)
            target_compile_definitions(WinBitchX PRIVATE WANT_IDENTD)
        endif()
        target_compile_definitions(BitchX PRIVATE WANT_IDENTD)
    endif()

else()
    # =========================================
    # Unix / Linux / macOS / FreeBSD build
    # =========================================

    # Additional options for Unix
    option(BITCHX_WITH_IPV6 "Enable IPv6 support" ON)
    option(BITCHX_WITH_PLUGINS "Enable plugin/shared library support" ON)

    # Find required packages using pkg-config
    find_package(PkgConfig)

    # Find ncurses (required for terminal UI)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(NCURSES ncursesw)
        if(NOT NCURSES_FOUND)
            pkg_check_modules(NCURSES ncurses)
        endif()
    endif()
    if(NOT NCURSES_FOUND)
        # Fallback: try find_package or manual detection
        find_package(Curses)
        if(CURSES_FOUND)
            set(NCURSES_LIBRARIES ${CURSES_LIBRARIES})
            set(NCURSES_INCLUDE_DIRS ${CURSES_INCLUDE_DIRS})
        else()
            message(FATAL_ERROR "ncurses/ncursesw not found. Install: apt install libncurses-dev (Debian/Ubuntu) or yum install ncurses-devel (RHEL/Fedora)")
        endif()
    endif()

    # Find OpenSSL
    if(BITCHX_WITH_SSL)
        if(PKG_CONFIG_FOUND)
            pkg_check_modules(OPENSSL openssl)
        endif()
        if(NOT OPENSSL_FOUND)
            find_package(OpenSSL)
            if(OpenSSL_FOUND)
                set(OPENSSL_LIBRARIES OpenSSL::SSL OpenSSL::Crypto)
                set(OPENSSL_INCLUDE_DIRS ${OPENSSL_INCLUDE_DIR})
            endif()
        endif()
        if(NOT OPENSSL_FOUND AND NOT OpenSSL_FOUND)
            message(WARNING "OpenSSL not found. Building without SSL/TLS support.")
            set(BITCHX_WITH_SSL OFF)
        endif()
    endif()

    # Find resolver library
    include(CheckLibraryExists)
    check_library_exists(resolv res_query "" HAVE_LIBRESOLV)

    # Check for dlopen (for plugins)
    check_library_exists(dl dlopen "" HAVE_LIBDL)

    # Check for socket library (Solaris/illumos)
    check_library_exists(socket connect "" HAVE_LIBSOCKET)
    check_library_exists(nsl gethostbyname "" HAVE_LIBNSL)

    # Check for crypt
    check_library_exists(crypt crypt "" HAVE_LIBCRYPT)

    # Check for getaddrinfo (IPv6 / modern networking)
    check_function_exists(getaddrinfo HAVE_GETADDRINFO)

    # Platform-specific definitions
    add_definitions(-D_GNU_SOURCE)

    if(BITCHX_WITH_IPV6)
        add_definitions(-DIPV6)
    endif()

    # Compiler warnings
    add_compile_options(
        -Wall
        -Wno-unused-variable
        -Wno-unused-but-set-variable
        -Wno-pointer-sign
        -fno-strict-aliasing
    )

    # Build the main BitchX executable
    add_executable(BitchX ${BITCHX_CORE_SOURCES})

    target_include_directories(BitchX PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}/include
        ${NCURSES_INCLUDE_DIRS}
    )

    # Link libraries
    target_link_libraries(BitchX PRIVATE ${NCURSES_LIBRARIES})

    if(HAVE_LIBRESOLV)
        target_link_libraries(BitchX PRIVATE resolv)
    endif()
    if(HAVE_LIBDL)
        target_link_libraries(BitchX PRIVATE dl)
    endif()
    if(HAVE_LIBSOCKET)
        target_link_libraries(BitchX PRIVATE socket)
    endif()
    if(HAVE_LIBNSL)
        target_link_libraries(BitchX PRIVATE nsl)
    endif()
    if(HAVE_LIBCRYPT)
        target_link_libraries(BitchX PRIVATE crypt)
        target_compile_definitions(BitchX PRIVATE HAVE_CRYPT)
    endif()

    # Math library
    target_link_libraries(BitchX PRIVATE m)

    # SSL support
    if(BITCHX_WITH_SSL)
        target_compile_definitions(BitchX PRIVATE HAVE_SSL)
        if(OPENSSL_INCLUDE_DIRS)
            target_include_directories(BitchX PRIVATE ${OPENSSL_INCLUDE_DIRS})
        endif()
        target_link_libraries(BitchX PRIVATE ${OPENSSL_LIBRARIES})
    endif()

    # Plugin support (shared library loading)
    if(BITCHX_WITH_PLUGINS AND HAVE_LIBDL)
        target_compile_definitions(BitchX PRIVATE WANT_DLL)
    endif()

    # Build wserv helper
    add_executable(wserv source/wserv.c)
    target_include_directories(wserv PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}/include
        ${NCURSES_INCLUDE_DIRS}
    )
    target_compile_definitions(wserv PRIVATE WTERM_C)
    target_link_libraries(wserv PRIVATE ${NCURSES_LIBRARIES})
    if(HAVE_LIBRESOLV)
        target_link_libraries(wserv PRIVATE resolv)
    endif()
    if(HAVE_LIBSOCKET)
        target_link_libraries(wserv PRIVATE socket)
    endif()
    if(HAVE_LIBNSL)
        target_link_libraries(wserv PRIVATE nsl)
    endif()

    # Build scr-bx helper
    add_executable(scr-bx source/scr-bx.c)
    target_include_directories(scr-bx PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}/include
        ${NCURSES_INCLUDE_DIRS}
    )
    target_compile_definitions(scr-bx PRIVATE STERM_C)
    target_link_libraries(scr-bx PRIVATE ${NCURSES_LIBRARIES})

    # macOS-specific settings
    if(APPLE)
        target_compile_definitions(BitchX PRIVATE BIND_8_COMPAT)
    endif()

    # Installation rules (Unix FHS)
    include(GNUInstallDirs)
    set(BITCHX_LIBDIR "${CMAKE_INSTALL_LIBDIR}/bx" CACHE STRING "BitchX library directory")

    install(TARGETS BitchX DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(TARGETS wserv DESTINATION ${BITCHX_LIBDIR})
    install(TARGETS scr-bx DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(DIRECTORY script/ DESTINATION ${BITCHX_LIBDIR}/script)
    install(DIRECTORY translation/ DESTINATION ${BITCHX_LIBDIR}/translation)
    install(DIRECTORY bitchx-docs/ DESTINATION ${BITCHX_LIBDIR}/help)

endif()

# Installation rules (Windows-specific)
if(WIN32)
    install(TARGETS BitchX DESTINATION bin)
    if(TARGET WinBitchX)
        install(TARGETS WinBitchX DESTINATION bin)
    endif()
    install(DIRECTORY script/ DESTINATION share/BitchX/script)
    install(DIRECTORY translation/ DESTINATION share/BitchX/translation)
    install(DIRECTORY bitchx-docs/ DESTINATION share/BitchX/help)
endif()

# Summary
message(STATUS "")
message(STATUS "BitchX ${PROJECT_VERSION} build configuration:")
message(STATUS "  Platform:     ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "  C Compiler:   ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "  SSL/TLS:      ${BITCHX_WITH_SSL}")
if(NOT WIN32)
    message(STATUS "  IPv6:         ${BITCHX_WITH_IPV6}")
    message(STATUS "  Plugins:      ${BITCHX_WITH_PLUGINS}")
endif()
if(WIN32)
    message(STATUS "  GUI:          ${BITCHX_WITH_GUI}")
endif()
message(STATUS "")
