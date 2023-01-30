use crate::mgmath::{self, Vec2u32};
use super::super::enums::*;

mod opengl;
pub mod vertex_array_info;
use vertex_array_info::*;


pub struct VideoBackend {
    backend_type: VideoBackendType,

    pub(in crate::video) sdl_context: sdl2::Sdl,
    pub(in crate::video) sdl_video_subsystem: sdl2::VideoSubsystem,
    pub(in crate::video) sdl_window: sdl2::video::Window,

    opengl_context: Option<sdl2::video::GLContext>,
    pub opengl_clear_buffer_mask: u32,
    opengl_meshes: Vec<opengl::OpenGLMesh>
}


impl VideoBackend {
    pub fn new(res: Vec2u32, title: &str, backend_type: VideoBackendType) -> Self {
        let sdl_context = sdl2::init().unwrap();
        let sdl_video_subsystem = sdl_context.video().unwrap();
        let sdl_window = Self::create_window(&sdl_video_subsystem, res, title, backend_type);

        let mut vb = Self {
            backend_type,
            sdl_context,
            sdl_video_subsystem,
            sdl_window,
            opengl_context: None,
            opengl_clear_buffer_mask: 0u32,
            opengl_meshes: Vec::new()
        };
        match backend_type {
            VideoBackendType::OpenGL => {
                vb.create_opengl();
            }
        }
        vb
    }
    
    pub fn create_window(sdl_video_subsystem: &sdl2::VideoSubsystem,
    res: mgmath::Vec2u32,
    title: &str,
    backend: VideoBackendType) -> sdl2::video::Window {
        match backend {
            VideoBackendType::OpenGL => {
                Self::create_opengl_window(sdl_video_subsystem, res, title)
            }
        }
    }

    pub fn set_clear_color(&mut self, color: mgmath::Vec4f32) {
        match self.backend_type {
            VideoBackendType::OpenGL => {
                self.opengl_set_clear_color(color);
            }
        }
    }

    pub fn clear(&mut self) {
        match self.backend_type {
            VideoBackendType::OpenGL => {
                self.opengl_clear();
            }
        }
    }

    pub fn swap(&mut self) {
        match self.backend_type {
            VideoBackendType::OpenGL => {
                self.opengl_swap();
            }
        }
    }

    pub fn new_mesh(&mut self) -> MeshID {
        match self.backend_type {
            VideoBackendType::OpenGL => {
                self.opengl_init_new_mesh()
            }
        }
    }

    pub fn mesh_data(&mut self, mesh: MeshID, data: VertexArrayCreateInfo) {
        match self.backend_type {
            VideoBackendType::OpenGL => {
                self.opengl_mesh_data(mesh, data);
            }
        }
    }
}


pub type MeshID = isize;
