CMAKE_MINIMUM_REQUIRED(VERSION 3.15)
PROJECT(PICSL_C3D)

# Enable languages
ENABLE_LANGUAGE(CXX)

# Specify the C++ standard
SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

# Avoid -fPIC errors
SET(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Option, whether to fetch external dependencies or to find locally
set(FETCH_DEPENDENCIES OFF CACHE BOOL "Fetch dependencies (ITK and Convert3D)?")
set(DEPENDENCIES_ONLY OFF CACHE BOOL "Only build dependencies, skip main build")

# If requested, fetch the dependencies
IF(FETCH_DEPENDENCIES)

    # Dependency: ITK
    include(FetchContent)
    set(Module_MorphologicalContourInterpolation ON CACHE BOOL "" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        ITK
        GIT_REPOSITORY https://github.com/InsightSoftwareConsortium/ITK.git
        GIT_TAG        v5.2.1
    )
    FetchContent_MakeAvailable(ITK)
    SET(ITK_DIR ${itk_BINARY_DIR})
    FIND_PACKAGE(ITK CONFIG REQUIRED)
    INCLUDE(${ITK_USE_FILE})

    # Dependency: Convert3D
    FetchContent_Declare(
        Convert3D
        GIT_REPOSITORY https://github.com/pyushkevich/c3d.git
        GIT_TAG        master
    )
    FetchContent_MakeAvailable(Convert3D)
    INCLUDE_DIRECTORIES(${convert3d_SOURCE_DIR}/api)
    
ELSE()

    # Find ITK and C3D
    FIND_PACKAGE(ITK REQUIRED)
    INCLUDE(${ITK_USE_FILE})

    FIND_PACKAGE(Convert3D REQUIRED)

ENDIF()

# Python bindings
IF(NOT DEPENDENCIES_ONLY)
    SET(PYBIND11_FINDPYTHON ON)
    ADD_SUBDIRECTORY(external/pybind11)
    INCLUDE_DIRECTORIES(external/pybind11)
    # INCLUDE_DIRECTORIES(external/c3d/api)
    pybind11_add_module(picsl_c3d src/ConvertPythonBindings.cxx)
    TARGET_INCLUDE_DIRECTORIES(picsl_c3d PRIVATE ${Convert3D_INCLUDE_DIRS})
    TARGET_LINK_LIBRARIES(picsl_c3d PUBLIC ${ITK_LIBRARIES} cnd_api)
    install(TARGETS picsl_c3d DESTINATION .)
ENDIF()