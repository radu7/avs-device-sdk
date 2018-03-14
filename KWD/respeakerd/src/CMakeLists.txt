add_definitions("-DACSDK_LOG_MODULE=RespeakerdKeyWordDetector")
add_library(RESPEAKERD SHARED
    RespeakerdKeyWordDetector.cpp)

target_include_directories(RESPEAKERD PUBLIC
	"${DBUS_INCLUDE_DIRS}"
	"${KWD_SOURCE_DIR}/include"
	"${RESPEAKERD_SOURCE_DIR}/include")

target_link_libraries(RESPEAKERD KWD AVSCommon "${DBUS_LIBRARIES}")

# install target
asdk_install()