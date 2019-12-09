//BEGIN_INCLUDE(all)
/* Java Native Interface */
#include <jni.h>
/* 错误报告机制 */
#include <errno.h>

/* EGL */
#include <EGL/egl.h>
/* OpenGL ES 1.x */
#include <GLES3/gl3.h>

/* 接收和处理传感器事件 */
#include <android/sensor.h>
/* Android logging API */
#include <android/log.h>

/* android-ndk-r5b/sources/android/native_app_glue 静态库头文件 */
#include <android_native_app_glue.h>
#include <malloc.h>

#define LOGI(...) \
((void)__android_log_print( ANDROID_LOG_INFO, "native-activity", __VA_ARGS__ ))

#define LOGW(...) \
((void)__android_log_print( ANDROID_LOG_WARN, "native-activity", __VA_ARGS__ ))

/**
 * Our saved state data.
 * 我们已保存的状态数据。
 */
struct saved_state {
    float angle; /* RGB 中的绿色值 */
    int32_t x;     /* X 坐标 */
    int32_t y;     /* Y 坐标 */
};

/**
 * Shared state for our app.
 * 应用程序共享状态的结构体
 */
struct engine {
    /* android_native_app_glue.h 中定义的本地应用程序粘合剂模块用数据结构 */
    struct android_app *app;

    /* sensor.h 中定义的传感器管理器 */
    ASensorManager *sensorManager;
    /* 加速度传感器 */
    const ASensor *accelerometerSensor;
    /* 已与一个循环器关联起来的传感器事件队列 */
    ASensorEventQueue *sensorEventQueue;

    /* 非零为可以绘制动画 */
    int animating;

    /* 显示器句柄 */
    EGLDisplay display;
    /* 系统窗口或 frame buffer 句柄 */
    EGLSurface surface;
    /* OpenGL ES 图形上下文 */
    EGLContext context;

    /* 系统窗口的宽度(像素) */
    int32_t width;
    /* 系统窗口的宽度(像素) */
    int32_t height;

    /* 我们已保存的状态数据 */
    struct saved_state state;
};

/**
 * Initialize an EGL context for the current display.
 * 为当前显示器初始化一个 EGL 上下文。
 */
static int engine_init_display(struct engine *engine) {
    LOGI("engine_init_display");
    /* initialize OpenGL ES and EGL
     * 初始化 OpenGL ES 和 EGL
     */

    /*
     * Here specify the attributes of the desired configuration.
     * 在这里具体指定想要的配置的属性。
     *
     * Below, we select an EGLConfig with at least 8 bits per color
     * component compatible with on-screen windows.
     * 在下面，我们选择一个至少 8 位色的 EGLConfig 与屏幕上的窗口一致。
     * 注：通常以 ID, Value 依次存放，对于个别标识性的属性可以只有 ID 没有 Value 。
     */
    const EGLint attribs[] = {EGL_SURFACE_TYPE, EGL_WINDOW_BIT, /* 系统窗口类型 */
                              EGL_BLUE_SIZE, 8,              /* 蓝色位数 */
                              EGL_GREEN_SIZE, 8,              /* 绿色位数 */
                              EGL_RED_SIZE, 8,              /* 红色位数 */
                              EGL_NONE};

    /* 系统窗口的宽度(像素) */
    EGLint w;
    /* 系统窗口的高度(像素) */
    EGLint h;
    /* 未使用的变量 */
    EGLint dummy;
    /* 像素格式ID - RGBA/RGBX/RGB565 */
    EGLint format;
    /* 系统中 Surface 的 EGL 配置 的总个数 */
    EGLint numConfigs;
    /* Surface 的 EGL 配置 */
    EGLConfig config;
    /* 系统窗口句柄 */
    EGLSurface surface;
    /* OpenGL ES 图形上下文 */
    EGLContext context;

    /* 1.返回一个显示器连接 - 是一个关联系统物理屏幕的通用数据类型。 */
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY); /* 得到系统默认的 */
    /* 原型：EGLDisplay eglGetDisplay ( NativeDisplayType display );
     * 　　　display 参数是本地系统显示器类型，取值为本地显示器 ID 值。
     * 返回：如果系统中没有一个可用的本地显示器 ID 值与 display 参数匹配，
     * 　　　函数将返回 EGL_NO_DISPLAY ，而没有任何 Error 状态被设置。
     */

    /* 2. EGL 在使用前需要初始化，因此每个显示器句柄(EGLDisplay)在使用前都需要初始化。*/
    eglInitialize(display, /* 有效的显示器句柄 */
                  0,       /* 返回主版本号 - 不关心可设为 NULL 值或零(0) */
                  0);     /* 返回次版本号 - 不关心可设为 NULL 值或零(0) */
    /* 原型：EGLBoolean eglInitialize( EGLDisplay dpy,
     *                                EGLint*    major,
     *                                EGLint*    minor );
     * 　　　EGLint 为 int 数据类型。
     * 返回：EGLBOOlean 取值：EGL_TRUE = 1, EGL_FALSE = 0 。
     */

    /* Here, the application chooses the configuration it desires.
     * 在这里，应用程序决定它要求的配置。
     *
     * In this sample, we have a very simplified selection process,
     * where we pick the first EGLConfig that matches our criteria.
     * 在这个示例中，我们有一个非常精简的选择处理，
     * 我们选择第一个 EGLConfig 适应我们的标准。
     */
    /* 定义一个希望从系统获得的配置，它将返回一个最接近你的需求的配置 */
    eglChooseConfig(display,       /* 有效的显示器句柄 */
                    attribs,       /* 以 EGL_NONE 结束的参数数组 */
                    &config,       /* Surface 的 EGL 配置 */
                    1,             /* Surface 的 EGL 配置个数 */
                    &numConfigs); /* 系统中 Surface 的 EGL 配置 的总个数 */
    /* 原型：EGLboolean eglChooseConfig( EGLDisplay    dpy,
     *                                  const EGLint* attr_list,
     *                                  EGLConfig*    config,
     *                                  EGLint        config_size,
     *                                  EGLint*       num_config );
     */

    /* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig
     * that is guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
     * EGL_NATIVE_VISUAL_ID 是一个 EGLConfig 的属性，
     * 保证被 ANativeWindow_setBuffersGeometry 函数认可。
     *
     * As soon as we picked a EGLConfig,
     * we can safely reconfigure the ANativeWindow buffers to match,
     * using EGL_NATIVE_VISUAL_ID.
     * 我们已经挑选了一个 EGLConfig ，
     * 使用 EGL_NATIVE_VISUAL_ID 我们可以安全地重新配置相应的 ANativeWindow 缓冲区了。
     */
    /* 查询 EGLConfig 的指定属性值 */
    eglGetConfigAttrib(display,              /* 有效的显示器句柄 */
                       config,               /* 有效的 Surface 的 EGL 配置 */
                       EGL_NATIVE_VISUAL_ID, /* 与操作系统通讯的可视 ID 句柄 */
                       &format);
    /* 原型：EGLBoolean eglGetConfigAttrib( EGLDisplay display,
     *                                     EGLConfig  config,
     *                                     EGLint     attribute,
     *                                     EGLint*    value );
     */

    /* 改变窗口表面像素格式和大小
     * 参见：native_window.h */
    ANativeWindow_setBuffersGeometry(engine->app->window, /* ANativeWindow */
                                     0,                   /* 宽度(像素) */
                                     0,                   /* 高度(像素) */
                                     format);            /* 像素格式 */

    /* 创建一个可实际显示的系统窗口句柄，实际上就是一个 FrameBuffer */
    surface = eglCreateWindowSurface(display, /* 有效的显示器句柄 */
                                     config,  /* 有效的 Surface 的 EGL 配置 */
                                     engine->app->window, /* ANativeWindow */
                                     NULL); /* 属性列表可以是空，使用默认值 */
    /* 原型：EGLSurface eglCreateWindowSurface( EGLDisplay          display,
     *                                         EGLConfig           config,
     *                                         EGLNatvieWindowType window,
     *                                         const EGLint*       attribList );
     */

    /* 创建一个 OpenGL ES 图形上下文 */
    context = eglCreateContext(display, /* 有效的显示器句柄 */
                               config,  /* 有效的 Surface 的 EGL 配置 */
                               NULL,    /* 不和其他 EGLContext 分享资源 */
                               NULL);  /* 属性列表可以是空，使用默认值 */
    /* 原型：EGLContext eglCreateContext( EGLDisplay    display,
     *                                   EGLConfig     config,
     *                                   EGLContext    shareContext,
     *                                   const EGLint* attribList );
     */

    /* 激活 OpenGL ES 图形上下文
     * 在 OpenGL ES ，一次只能有一个 context 生效。
     */
    if (EGL_FALSE == eglMakeCurrent(display,     /* 有效的显示器句柄 */
                                    surface,     /* 上面创建的系统窗口 */
                                    surface,     /* 上面创建的系统窗口 */
                                    context))  /* OpenGL ES 图形上下文 */
    {
        LOGW("Unable to eglMakeCurrent");

        return -1;
    }
    /* 原型：EGLBoolean eglMakeCurrent( EGLDisplay display,
     * 　　　                           EGLSurface draw,
     * 　　　                           EGLSurface read,
     * 　　　                           EGLContext context );
     */

    eglQuerySurface(display,    /* 有效的显示器句柄 */
                    surface,    /* 上面创建的系统窗口 */
                    EGL_WIDTH,  /* 返回系统窗口的宽度(像素) */
                    &w);
    /* 原型：EGLBoolean eglQuerySurface( EGLDisplay display,
     * 　　　EGLSurface surface,
     * 　　　EGLint     attribute,
     *      EGLint*    value );
     */

    eglQuerySurface(display,
                    surface,
                    EGL_HEIGHT, /* 返回系统窗口的高度(像素) */
                    &h);
    LOGI("检测到屏幕宽高，width=%d,height=%d", w, h);

    engine->display = display;
    engine->context = context;
    engine->surface = surface;
    engine->width = w;
    engine->height = h;

    engine->state.angle = 0;

    /* Initialize GL state.
     * 初始化 GL 状态。GL_PERSPECTIVE_CORRECTION_HINT
     */
    glHint(GL_PROGRAM_BINARY_RETRIEVABLE_HINT, /* 指定颜色和纹理坐标的插值质量 */
           GL_FASTEST);                   /* 使用速度最快的模式 */

    /* 开启服务端 GL 功能 */
    glEnable(GL_CULL_FACE); /* 开启多边形表面剔除功能 */
    /* 原型：void glEnable( GLenum cap ); */

    /* 禁用服务端 GL 功能 */
    glDisable(GL_DEPTH_TEST); /* 禁用深度测试 */

    return 0;
}

/**
 * Just the current frame in the display.
 * 显示的当前帧。
 */
static void engine_draw_frame(struct engine *engine) {
    LOGI("engine_draw_frame");
    /* 显示器句柄为空 */
    if (engine->display == NULL) {
        /* No display.
         * 不显示。
         */
        return;
    }

    /* Just fill the screen with a color.
     * 只是用一种颜色添充屏幕。
     */
    glClearColor(((float) engine->state.x) / engine->width,
                 engine->state.angle,
                 ((float) engine->state.y) / engine->height,
                 1);
    /* 原型：void glClearColor( GLclampf red,
     * 　　　                   GLclampf green,
     * 　　　                   GLclampf blue,
     * 　　　                   GLclampf alpha );
     */

    /* 要清除颜色缓冲 */
    glClear(GL_COLOR_BUFFER_BIT);
    /* 原型：void glClear( GLbitfield mask ); */

    /* 发送 EGL 系统窗口绘图缓冲区到本地窗口 */
    eglSwapBuffers(engine->display,
                   engine->surface);
    /* 原型：EGLBoolean eglSwapBuffers( EGLDisplay display,
     * 　　　                           EGLSurface surface );
     */
}

/**
 * Tear down the EGL context currently associated with the display.
 * 拆掉与显示器关联的当前 EGL 上下文。
 */
static void engine_term_display(struct engine *engine) {
    LOGI("engine_term_display");
    if (engine->display != EGL_NO_DISPLAY) {
        /* 解绑 OpenGL ES 图形上下文 与 可实际显示的系统窗口句柄 */
        eglMakeCurrent(engine->display,
                       EGL_NO_SURFACE,
                       EGL_NO_SURFACE,
                       EGL_NO_CONTEXT);

        if (engine->context != EGL_NO_CONTEXT) {
            /* 销毁 OpenGL ES 图形上下文 */
            eglDestroyContext(engine->display,
                              engine->context);
        }

        if (engine->surface != EGL_NO_SURFACE) {
            /* 销毁 可实际显示的系统窗口句柄 */
            eglDestroySurface(engine->display,
                              engine->surface);
        }

        /* 终止一个显示器的连接 */
        eglTerminate(engine->display);
    }

    /* 禁止动画绘制 */
    engine->animating = 0;

    /* 以防上面的判断出错，修改记录 */
    engine->display = EGL_NO_DISPLAY;

    engine->context = EGL_NO_CONTEXT;

    engine->surface = EGL_NO_SURFACE;
}

/**
 * Process the next input event.
 * 处理下一个输入事件。
 */
static int32_t engine_handle_input(struct android_app *app, AInputEvent *event) {
    LOGI("engine_handle_input");
    struct engine *engine = (struct engine *) app->userData;

    /* 得到的输入事件类型为触摸屏事件 */
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        /* 启用动画绘制 */
        engine->animating = 1;

        /* 得到第一个触摸点的当前 x 坐标 */
        engine->state.x = AMotionEvent_getX(event, 0);

        /* 得到第一个触摸点的当前 y 坐标 */
        engine->state.y = AMotionEvent_getY(event, 0);

        return 1;
    }

    return 0;
}

/**
 * Process the next main command.
 * 处理下一个主线程命令消息。
 */
static void engine_handle_cmd(struct android_app *app, int32_t cmd) {

    struct engine *engine = (struct engine *) app->userData;

    switch (cmd) {
        case APP_CMD_SAVE_STATE:
            LOGI("engine_handle_cmd,cmd=APP_CMD_SAVE_STATE");
            /* The system has asked us to save our current state.
             * 系统告诉我们去保存我们的当前状态。
             *
             * Do so.
             * 如下所做。
             */
            engine->app->savedState = malloc(sizeof(struct saved_state));

            *((struct saved_state *) engine->app->savedState) = engine->state;

            engine->app->savedStateSize = sizeof(struct saved_state);

            break;

        case APP_CMD_INIT_WINDOW:
            LOGI("engine_handle_cmd,cmd=APP_CMD_INIT_WINDOW");
            /* The window is being shown, get it ready.
             * 窗口是正在显示，准备得到它。
             */
            if (engine->app->window != NULL) {
                engine_init_display(engine);
                engine_draw_frame(engine);
            }

            break;

        case APP_CMD_TERM_WINDOW:
            LOGI("engine_handle_cmd,cmd=APP_CMD_TERM_WINDOW");
            /* The window is being hidden or closed, clean it up.
             * 窗口是正在隐藏或关闭，清理它。
             */
            engine_term_display(engine);
            break;

        case APP_CMD_GAINED_FOCUS:
            LOGI("engine_handle_cmd,cmd=APP_CMD_GAINED_FOCUS");
            /* When our app gains focus, we start monitoring the accelerometer.
             * 当我们的应用程序得到焦点时，我们开始监测加速器。
             */
            if (engine->accelerometerSensor != NULL) {
                /* 启用指定的传感器 */
                ASensorEventQueue_enableSensor(engine->sensorEventQueue,
                                               engine->accelerometerSensor);

                /* We'd like to get 60 events per second (in us).
                 * 我们将喜欢每秒得到六十个事件。
                 */
                ASensorEventQueue_setEventRate(engine->sensorEventQueue,
                                               engine->accelerometerSensor,
                                               (1000L / 60) * 1000);
            }

            break;

        case APP_CMD_LOST_FOCUS:
            LOGI("engine_handle_cmd,cmd=APP_CMD_LOST_FOCUS");
            /* When our app loses focus, we stop monitoring the accelerometer.
             * 当我们的应用程序丢失焦点时，我们停止监测加速器。
             *
             * This is to avoid consuming battery while not being used.
             * 在不使用时期避免消耗电池。
             */
            if (engine->accelerometerSensor != NULL) {
                /* 禁用指定的传感器 */
                ASensorEventQueue_disableSensor(engine->sensorEventQueue,
                                                engine->accelerometerSensor);
            }

            /* Also stop animating.
             * 同样禁止动画绘制。
             */
            engine->animating = 0;

            engine_draw_frame(engine);

            break;
    }
}


/**
 * This is the main entry point of a native application
 * that is using android_native_app_glue.
 * 这是使用了 android_native_app_glue 粘合剂模块的一个本地应用程序的主入口点。
 *
 * It runs in its own thread,
 * with its own event loop for receiving input events and doing other things.
 * 它运行在它自己的线程中，用它自己的事件循环来接收输入事件和做其它事情。
 */
void android_main(struct android_app *state) {
    LOGI("android_main,start Activity...");
    struct engine engine;

    /* Make sure glue isn't stripped.
     * 确认glue模块未剥离。
     * 注：在 android_native_app_glue.c 文件中这是一个空函数。
     */
    app_dummy();

    //重置engine结构体为0
    memset(&engine, 0, sizeof(engine));

    /* 在应用程序各部分之间共享的数据 */
    state->userData = &engine;

    /* 指向一个处理应用程序命令的函数,这是很重要的一个函数 */
    state->onAppCmd = engine_handle_cmd;

    /* 指向一个处理输入事件的函数 */
    state->onInputEvent = engine_handle_input;

    engine.app = state;

    /* Prepare to monitor accelerometer.
     * 准备监测加速器。
     */
    engine.sensorManager = ASensorManager_getInstance(); /* 传感器管理器为单例模式 */

    /* 得到默认的加速度传感器 */
    engine.accelerometerSensor = ASensorManager_getDefaultSensor(engine.sensorManager,
                                                                 ASENSOR_TYPE_ACCELEROMETER);

    engine.sensorEventQueue = ASensorManager_createEventQueue(engine.sensorManager,
                                                              state->looper,
                                                              LOOPER_ID_USER,
                                                              NULL,
                                                              NULL);

    if (state->savedState != NULL) {
        /* We are starting with a previous saved state;
         * 我们使用一个之前保存状态启动。
         *
         * restore from it.
         * 用它来恢复。
         */
        engine.state = *(struct saved_state *) state->savedState;
    }

    /* loop waiting for stuff to do.
     * 循环等待事件
     */
    while (1) {
        /* Read all pending events.
         * 读取全部待解决事件。
         */
        int ident;

        int events;

        struct android_poll_source *source;

        /* If not animating, we will block forever waiting for events.
         * 如果不是动画绘制中，我们将永远阻塞等待事件。
         *
         * If animating, we loop until all events are read,
         * then continue to draw the next frame of animation.
         * 如果动画绘制中，我们循环直到全部事件被读取，然后继续去画下一个动画帧。
         */
        /* (0) - 立即返回，(-1) - 无限等待 */
        while ((ident = ALooper_pollAll(engine.animating ? 0 : -1,
                                        NULL, /* 不返回发生事件的文件描述符 */
                /* 在 android_native_app_glue.c 文件的 android_app_entry 函数体中
                 * 调用 ALooper_addFd 函数时设置为 ALOOPER_EVENT_INPUT - 文件描述符读操作有效 */
                                        &events,
                                        (void **) &source)) >= 0) {
            /* Process this event.
             * 处理这个事件。
             */
            if (source != NULL) {
                /* 处理应用程序主线程的命令
                 * 在 android_native_app_glue.c 文件的 android_app_entry 函数体中
                 * 设置指向 process_cmd 或 process_input 函数。
                 */
                source->process(state, source);
            }

            /* If a sensor has data, process it now.
             * 如果一个传感器有数据，马上处理它。
             */
            if (ident == LOOPER_ID_USER) {
                if (engine.accelerometerSensor != NULL) {
                    ASensorEvent event;

                    while (ASensorEventQueue_getEvents(engine.sensorEventQueue, &event, 1) > 0) {
                        LOGI("accelerometer: x=%f y=%f z=%f",
                             event.acceleration.x, event.acceleration.y,
                             event.acceleration.z);
                    }
                }
            }

            /* Check if we are exiting.
             * 如果我们是正在退出，停止循环并返回。
             */
            if (state->destroyRequested != 0) {
                engine_term_display(&engine);
                return;
            }
        }

        if (engine.animating) {
            /* Done with events; draw next animation frame.
             * 事件完成；画下一个动画帧。
             */
            engine.state.angle += .01f; /* 0.1 递增变化 */

            if (engine.state.angle > 1) {
                engine.state.angle = 0;
            }

            /* Drawing is throttled to the screen update rate,
             * so there is no need to do timing here.
             * 绘制是被屏幕更新率调节，所以这里是不需要做计时的。
             */
            engine_draw_frame(&engine);
        }
    }
}

//END_INCLUDE(all)
