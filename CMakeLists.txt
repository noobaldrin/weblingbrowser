cmake_minimum_required(VERSION 3.30)

project(webling)

set(CMAKE_CXX_STANDARD 17)

find_program(GLIB_COMPILE_RESOURCES NAMES glib-compile-resources REQUIRED)
set(GRESOURCE_XML ${CMAKE_CURRENT_SOURCE_DIR}/res/resource.gresource.xml)
set(GRESOURCEBIN ${CMAKE_CURRENT_SOURCE_DIR}/res/resource.gresource)

add_custom_command(
    OUTPUT  ${GRESOURCEBIN}
    COMMAND ${GLIB_COMPILE_RESOURCES}
            --target=${GRESOURCEBIN}
            --sourcedir=${CMAKE_CURRENT_SOURCE_DIR}
                        ${GRESOURCE_XML}
   DEPENDS ${GRESOURCE_XML}
        ${CMAKE_CURRENT_SOURCE_DIR}/res/style.css
        ${CMAKE_CURRENT_SOURCE_DIR}/res/introspection.xml
   COMMENT "Compiling Resource GResource"
)

find_program(GLIB_COMPILE_SCHEMAS NAMES glib-compile-schemas REQUIRED)
set(GSCHEMA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/res)
set(GSCHEMA_XML ${GSCHEMA_DIR}/settings.gschema.xml)
set(GSCHEMA_COMPILED ${GSCHEMA_DIR}/gschemas.compiled)

add_custom_command(
    OUTPUT ${GSCHEMA_COMPILED}
    COMMAND ${GLIB_COMPILE_SCHEMAS} --strict --targetdir=${GSCHEMA_DIR} ${GSCHEMA_DIR}
    DEPENDS ${GSCHEMA_XML}
    COMMENT "Compiling GSettings schema"
)

add_custom_target(resources_schema ALL
    DEPENDS ${GRESOURCEBIN} ${GSCHEMA_COMPILED}
)

find_package(PkgConfig)
pkg_check_modules(GTKMM REQUIRED gtkmm-4.0)
pkg_check_modules(WEBKIT REQUIRED webkitgtk-6.0)
pkg_check_modules(SOUP REQUIRED libsoup-3.0)
pkg_check_modules(GUMBO REQUIRED gumbo)
pkg_check_modules(PSL REQUIRED libpsl)
pkg_check_modules(GLYCIN-GTK4 REQUIRED glycin-gtk4-2)
find_package(SQLiteCpp REQUIRED)

add_executable(webling main.cpp
                       Webling.cpp
                       BrowserWindow.cpp
                       Settings.cpp
                       FontSettings.cpp
                       UrlDropDown.cpp
                       DBContext.cpp)

add_dependencies(webling resources_schema)

target_link_libraries(webling
        PRIVATE ${GTKMM_LIBRARIES}
                ${WEBKIT_LIBRARIES}
                ${SOUP_LIBRARIES}
                ${GUMBO_LIBRARIES}
                ${PSL_LIBRARIES}
                ${GLYCIN-GTK4_LIBRARIES}
                SQLiteCpp)

target_include_directories(webling
        PRIVATE ${GTKMM_INCLUDE_DIRS}
                ${WEBKIT_INCLUDE_DIRS}
                ${SOUP_INCLUDE_DIRS}
                ${GLYCIN-GTK4_INCLUDE_DIRS})


set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/.local" CACHE PATH "Install path prefix" FORCE)
install(TARGETS webling RUNTIME DESTINATION bin)
install(FILES ${GRESOURCEBIN} DESTINATION share/webling/resources/)
install(FILES ${GSCHEMA_COMPILED} DESTINATION share/webling/schemas/)
