use std::os::raw::c_void;

use super::*;


impl VideoBackend {
    pub fn create_opengl_window(sdl_video_subsystem: &sdl2::VideoSubsystem, res: mgmath::Vec2u32, title: &str) -> sdl2::video::Window {
        sdl_video_subsystem.window(title, res[0], res[1]).opengl().build().unwrap()
    }

    pub fn create_opengl(sdl_video_subsystem: &sdl2::VideoSubsystem, window: &sdl2::video::Window) -> Self {
        gl::load_with(|s| sdl_video_subsystem.gl_get_proc_address(s) as *const _);
        let gl_context = window.gl_create_context().unwrap();
        window.gl_make_current(&gl_context).unwrap();
        VideoBackend {
            bakcend_type: VideoBackendType::OpenGL,
            opengl_context: Some(window.gl_create_context().unwrap()),
            opengl_clear_buffer_mask: gl::COLOR_BUFFER_BIT | gl::DEPTH_BUFFER_BIT
        }
    }

    pub(in crate::video::backends) fn opengl_set_clear_color(&mut self, color: mgmath::Vec4f32) {
        unsafe {
            gl::ClearColor(
                color[0],
                color[1],
                color[2],
                color[3]
            );
        }
    }

    pub(in crate::video::backends) fn opengl_clear(&mut self) {
        unsafe {
            gl::Clear(self.opengl_clear_buffer_mask);
        }
    }

    pub(in crate::video::backends) fn opengl_swap(&mut self, window: &sdl2::video::Window) {
        window.gl_swap_window();
    }
}
