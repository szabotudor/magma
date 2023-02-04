use crate::video::backends;


pub enum ShaderType {
    VERTEX = gl::VERTEX_SHADER as isize,
    FRAGMENT = gl::FRAGMENT_SHADER as isize
}


pub struct Shader {
    shader: Box<dyn backends::BackendShader>,
    linked: bool
}


impl Shader {
    /// Create a new shader compatible with the given backend\
    /// The shader source must be compatible with the selected backend (GLES for OpenGL, SPIR-V for Vulkant etc)\
    /// 
    /// `backend` -> Reference to the backend to use
    pub fn new(backend: &mut backends::VideoBackend) -> Self {
        Shader {
            shader: backend.new_shader(),
            linked: false
        }
    }

    /// Add a new source string to the shader (multiple can be added before linking)\
    /// 
    /// `source` -> The source for the shader\
    /// `shader_type` -> The type of the shader (ShaderType::VERTEX or ShaderType::FRAGMENT)
    pub fn add_source(&mut self, source: &str, shader_type: ShaderType) {
        self.shader.add_source(source, shader_type);
    }

    /// Link the shader, so it can be used
    pub fn link(&mut self) {
        self.shader.link();
        self.linked = true;
    }

    /// Set this shader as the shader to be used for drawing
    pub fn make_current(&mut self) {
        if !self.linked {
            self.link();
        }
        self.shader.make_current();
    }
}
