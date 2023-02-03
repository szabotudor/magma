use crate::mgmath::{self, Vec2u32};

mod opengl;
pub mod vertex_array_info;
use vertex_array_info::*;


pub enum VideoBackendType {
    OpenGL
}
impl Clone for VideoBackendType {
    fn clone(&self) -> Self {
        match self {
            Self::OpenGL => Self::OpenGL,
        }
    }
}
impl Copy for VideoBackendType { }


pub struct VideoBackend {
    backend_type: VideoBackendType,

    pub(in crate::video) sdl_context: sdl2::Sdl,
    pub(in crate::video) sdl_video_subsystem: sdl2::VideoSubsystem,
    pub(in crate::video) sdl_window: sdl2::video::Window,

    pub opengl: opengl::VideoBackendOpenGL
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
            opengl: opengl::VideoBackendOpenGL::new()
        };
        match backend_type {
            VideoBackendType::OpenGL => {
                vb.opengl.init(&vb.sdl_video_subsystem, &vb.sdl_window);
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
                opengl::VideoBackendOpenGL::create_window(sdl_video_subsystem, res, title)
            }
        }
    }

    pub fn set_clear_color(&mut self, color: mgmath::Vec4f32) {
        match self.backend_type {
            VideoBackendType::OpenGL => {
                self.opengl.set_clear_color(color);
            }
        }
    }

    pub fn clear(&mut self) {
        match self.backend_type {
            VideoBackendType::OpenGL => {
                self.opengl.clear();
            }
        }
    }

    pub fn swap(&mut self) {
        match self.backend_type {
            VideoBackendType::OpenGL => {
                self.sdl_window.gl_swap_window();
            }
        }
    }

    pub fn new_mesh(&mut self) -> MeshID {
        match self.backend_type {
            VideoBackendType::OpenGL => {
                self.opengl.new_mesh()
            }
        }
    }

    pub fn mesh_data(&mut self, mesh: MeshID, data: VertexArrayCreateInfo) {
        match self.backend_type {
            VideoBackendType::OpenGL => {
                self.opengl.mesh_data(mesh, data);
            }
        }
    }

    pub fn new_shader(&mut self) -> ShaderID {
        match self.backend_type {
            VideoBackendType::OpenGL => {
                self.opengl.new_shader()
            }
        }
    }

    pub fn shader_add_source(&mut self, shader_id: ShaderID, shader_type: crate::shader::ShaderType, source: &str) {
        match self.backend_type {
            VideoBackendType::OpenGL => {
                self.opengl.shader_add_source(shader_id, shader_type, source);
            }
        }
    }
}


pub type MeshID = isize;
pub type ShaderID = isize;
