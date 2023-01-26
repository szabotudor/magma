use crate::mgmath;
use super::super::enums::*;

mod opengl;


pub struct VideoBackend {
    bakcend_type: VideoBackendType,

    opengl_context: Option<sdl2::video::GLContext>,
    pub opengl_clear_buffer_mask: u32
}


impl VideoBackend {
    pub fn set_clear_color(&mut self, color: mgmath::Vec4f32) {
        match self.bakcend_type {
            VideoBackendType::OpenGL => {
                self.opengl_set_clear_color(color);
            }
        }
    }

    pub fn clear(&mut self) {
        match self.bakcend_type {
            VideoBackendType::OpenGL => {
                self.opengl_clear();
            }
        }
    }

    pub fn swap(&mut self, window: &sdl2::video::Window) {
        match self.bakcend_type {
            VideoBackendType::OpenGL => {
                self.opengl_swap(window);
            }
        }
    }
}
