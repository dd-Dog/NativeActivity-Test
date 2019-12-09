project(Native-Activity)

cmake_minimum_required(VERSION 3.4.1)
#引入native_app_glue头文件
include_directories(D:/Android/Sdk/ndk-bundle/sources/android/native_app_glue)

add_library(native-activity SHARED main.c)

find_library(log-lib log)
find_library(OPENGLES3_LIBRARY GLESv3 "OpenGL ES v3.0 library")
find_library(EGL_LIBRARY EGL "EGL 1.4 library")
find_library(android-lib android)

#编译为静态库
add_library(app_glue STATIC
        D:/Android/Sdk/ndk-bundle/sources/android/native_app_glue/android_native_app_glue.c)

target_link_libraries(native-activity
        app_glue #链接静态库native_app_glue
        ${log-lib} #链接log库
        ${android-lib} #链接android库
        ${OPENGLES3_LIBRARY} #链接OpenGLES库
        ${EGL_LIBRARY} #链接EGL库
        )