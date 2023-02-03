use crate::video::backends;


pub enum ShaderType {
    VERTEX = gl::VERTEX_SHADER as isize,
    FRAGMENT = gl::FRAGMENT_SHADER as isize
}


pub struct Shader {
    backend: *mut backends::VideoBackend,
    shader_id: backends::ShaderID,
    linked: bool
}


impl Shader {
    /// Create a new shader compatible with the given backend\
    /// The shader source must be compatible with the selected backend (GLES for OpenGL, SPIR-V for Vulkant etc)\
    /// 
    /// `backend` -> Reference to the backend to use
    pub fn new(backend: *const backends::VideoBackend) -> Self {
        Shader {
            backend: backend as *mut backends::VideoBackend,
            shader_id: -1,
            linked: false
        }
    }

    /// Add a new source string to the shader (multiple can be added before linking)\
    /// 
    /// `source` -> The source for the shader\
    /// `shader_type` -> The type of the shader (ShaderType::VERTEX or ShaderType::FRAGMENT)
    pub fn add_source(&mut self, source: &str, shader_type: ShaderType) {
        unsafe {
            if self.shader_id == -1 {
                self.shader_id = (*self.backend).new_shader();
            }
            (*self.backend).shader_add_source(self.shader_id, shader_type, source);
        }
    }

    /// Link the shader, so it can be used
    pub fn link(&mut self) {
        unsafe { (*self.backend).link_shader(self.shader_id) };
        self.linked = true;
    }

    /// Set this shader as the shader to be used for drawing
    pub fn make_current(&mut self) {
        if !self.linked {
            self.link();
        }
        unsafe { (*self.backend).make_shader_current(self.shader_id) };
    }
}
