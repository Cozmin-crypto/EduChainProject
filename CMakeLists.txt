cmake_minimum_required(VERSION 3.20)

project(EduChain LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# SQLite este distribuit in proiect ca amalgamare C si a fost compilat anterior
# cu MSVC in modul C (/TC). Restul proiectului ramane exclusiv C++17.
enable_language(C)

add_library(SQLiteAmalgamation STATIC
    external/sqlite3.c
)

target_include_directories(SQLiteAmalgamation
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/external"
)

if(MSVC)
    target_compile_options(SQLiteAmalgamation PRIVATE /W0)
    target_compile_definitions(SQLiteAmalgamation PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

add_library(EduChainCore STATIC
    include/Administrator.h
    include/AutentificareService.h
    include/Catalog.h
    include/Chestionar.h
    include/ClientEdu.h
    include/ConectorBazaDate.h
    include/Curs.h
    include/CursRepository.h
    include/CursService.h
    include/Evaluare.h
    include/EvaluareRepository.h
    include/EvaluareService.h
    include/ExamenFinal.h
    include/ExceptieEdu.h
    include/INodRetea.h
    include/IUtilizator.h
    include/Jurnalizare.h
    include/Lectie.h
    include/LectieRepository.h
    include/LectieService.h
    include/LectieText.h
    include/LectieVideo.h
    include/ManagerSocket.h
    include/Personal.h
    include/Profesor.h
    include/ProtocolEdu.h
    include/ReguliAccesService.h
    include/ServerEdu.h
    include/SesiuneClient.h
    include/Student.h
    include/UtilizatorBaza.h
    include/UtilizatorRepository.h
    src/Administrator.cpp
    src/AutentificareService.cpp
    src/Chestionar.cpp
    src/ClientEdu.cpp
    src/ConectorBazaDate.cpp
    src/Curs.cpp
    src/CursRepository.cpp
    src/CursService.cpp
    src/Evaluare.cpp
    src/EvaluareRepository.cpp
    src/EvaluareService.cpp
    src/ExamenFinal.cpp
    src/ExceptieEdu.cpp
    src/INodRetea.cpp
    src/IUtilizator.cpp
    src/Jurnalizare.cpp
    src/Lectie.cpp
    src/LectieRepository.cpp
    src/LectieService.cpp
    src/LectieText.cpp
    src/LectieVideo.cpp
    src/ManagerSocket.cpp
    src/Personal.cpp
    src/Profesor.cpp
    src/ProtocolEdu.cpp
    src/ReguliAccesService.cpp
    src/ServerEdu.cpp
    src/Student.cpp
    src/UtilizatorBaza.cpp
    src/UtilizatorRepository.cpp
)

target_include_directories(EduChainCore
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/external"
)

target_link_libraries(EduChainCore
    PUBLIC
        SQLiteAmalgamation
        Ws2_32
)

if(MSVC)
    target_compile_options(EduChainCore PRIVATE /W4 /permissive-)
endif()

function(educhain_add_test target source)
    add_executable(${target} ${source})
    target_link_libraries(${target} PRIVATE EduChainCore)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    endif()
    add_test(NAME ${target} COMMAND ${target})
    set_tests_properties(${target} PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
endfunction()

enable_testing()

educhain_add_test(TestComunicatieClientServer
    tests/TestComunicatieClientServer.cpp)
educhain_add_test(TestIntegrareReteaService
    tests/TestIntegrareReteaService.cpp)
educhain_add_test(TestIntegrareService
    tests/TestIntegrareService.cpp)
educhain_add_test(TestIntegrarePersistenta
    tests/TestIntegrarePersistenta.cpp)
educhain_add_test(TestIntegrareReteaLectii
    tests/TestIntegrareReteaLectii.cpp)
educhain_add_test(TestIntegrareReteaEvaluari
    tests/TestIntegrareReteaEvaluari.cpp)
