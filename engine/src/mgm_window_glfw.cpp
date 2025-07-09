#include "glad/glad.h"

#include "GLFW/glfw3.h"

#include "mgm_assert.hpp"
#include "mgm_string.hpp"
#include "mgm_threads.hpp"
#include "mgm_window.hpp"


namespace mgm {
    struct MgmWindow::Data {
        GLFWwindow* window{};

        static inline MMutex mutex{};
        static inline bool init = false;

        static void glfw_framebuffer_size_callback(GLFWwindow* window, int width, int height) {
        }
    };

    void MgmWindow::set_option_private(Option option, bool value) {
        data->mutex.lock();

        switch (option) {
            case Option::FULLSCREEN: {
                break;
            }
            case Option::ALLOW_RESIZE: {
                break;
            }
            case Option::VSYNC: {
                glfwMakeContextCurrent(data->window);
                glfwSwapInterval(value);
                glfwMakeContextCurrent(nullptr);
                break;
            }
            default: break;
        }

        data->mutex.unlock();
    }

    MgmWindow::MgmWindow(const vec2u32& size, const MString& title) {
        data = new Data{};

        data->mutex.lock();

        if (!data->init) {
            MAssert{"Failed to initialize GLFW"}(glfwInit() == GLFW_TRUE);
        }

#ifdef MAGMA_BACKEND_OpenGL
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif
        data->window = glfwCreateWindow(size.x, size.y, title.c_str(), nullptr, nullptr);

        MAssert{"Failed to create GLFW window"}(data->window != nullptr);

#ifdef MAGMA_BACKEND_OpenGL
        glfwMakeContextCurrent(data->window);
        MAssert{"Failed to load glad gl loader"}(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress));
        glfwMakeContextCurrent(nullptr);
#endif

        glfwSetFramebufferSizeCallback(data->window, &Data::glfw_framebuffer_size_callback);
        glfwShowWindow(data->window);

        data->mutex.unlock();
    }

    bool MgmWindow::should_close() const {
        return glfwWindowShouldClose(data->window);
    }

    void MgmWindow::update() {
        glfwMakeContextCurrent(data->window);

        if (glfwWindowShouldClose(data->window))
            glfwSetWindowShouldClose(data->window, false);

        glfwPollEvents();
        glfwSwapBuffers(data->window);

        glfwMakeContextCurrent(nullptr);
    }

    MgmWindow::~MgmWindow() {
        data->mutex.lock();

        glfwDestroyWindow(data->window);

        glfwTerminate();

        data->mutex.unlock();

        delete data;
    }
} // namespace mgm
