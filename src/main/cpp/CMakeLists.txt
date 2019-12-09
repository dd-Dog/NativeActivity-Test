project(Native-Activity)

cmake_minimum_required(VERSION 3.4.1)

include_directories(D:/Android/Sdk/ndk-bundle/sources/android/native_app_glue)

add_library(native-activity SHARED main.c)

find_library(log-lib log)
find_library(OPENGLES3_LIBRARY GLESv3 "OpenGL ES v3.0 library")
find_library(EGL_LIBRARY EGL "EGL 1.4 library")
find_library(android-lib android)


add_library(app_glue STATIC
        D:/Android/Sdk/ndk-bundle/sources/android/native_app_glue/android_native_app_glue.c)

target_link_libraries(native-activity
        app_glue
        ${log-lib}
        ${android-lib}
        ${OPENGLES3_LIBRARY}
        ${EGL_LIBRARY}
        )