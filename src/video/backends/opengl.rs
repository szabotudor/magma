use super::*;


pub(in crate::video::backends) struct OpenGLMesh {
    vao: gl::types::GLuint,
    vbo: gl::types::GLuint,
    ebo: gl::types::GLuint,
    num_elem: i32,
    use_ebo: bool
}

pub(in crate::video::backends) struct OpenGLShader {
    shaders_uninit: Vec<u32>,
    program: u32
}


pub struct VideoBackendOpenGL {
    pub(in crate::video::backends) context: Option<sdl2::video::GLContext>,
    pub clear_buffer_mask: u32,
    pub(in crate::video::backends) meshes: Vec<opengl::OpenGLMesh>,
    pub(in crate::video::backends) shaders: Vec<opengl::OpenGLShader>
}


impl VideoBackendOpenGL {
    pub(in crate::video::backends) fn new() -> Self {
        VideoBackendOpenGL {
            context: None,
            clear_buffer_mask: 0u32,
            meshes: Vec::new(),
            shaders: Vec::new()
        }
    }

    pub(in crate::video::backends) fn create_window(sdl_video_subsystem: &sdl2::VideoSubsystem, res: mgmath::Vec2u32, title: &str) -> sdl2::video::Window {
        sdl_video_subsystem.gl_attr().set_context_version(4, 6);
        sdl_video_subsystem.window(title, res[0], res[1]).opengl().build().unwrap()
    }

    pub(in crate::video::backends) fn init(&mut self, sdl_video_subsystem: &sdl2::VideoSubsystem, sdl_window: &sdl2::video::Window) {
        gl::load_with(|s| sdl_video_subsystem.gl_get_proc_address(s) as *const _);
        self.context = Some(sdl_window.gl_create_context().unwrap());
        sdl_window.gl_make_current(self.context.as_ref().unwrap()).unwrap();
        self.clear_buffer_mask = gl::COLOR_BUFFER_BIT | gl::DEPTH_BUFFER_BIT;
    }

    pub(in crate::video::backends) fn set_clear_color(&mut self, color: mgmath::Vec4f32) {
        unsafe {
            gl::ClearColor(
                color[0],
                color[1],
                color[2],
                color[3]
            );
        }
    }

    pub(in crate::video::backends) fn clear(&mut self) {
        unsafe {
            gl::Clear(self.clear_buffer_mask);
        }
    }

    pub(in crate::video::backends) fn new_mesh(&mut self) -> MeshID {
        let (mut vao, mut vbo, mut ebo) = (0u32, 0u32, 0u32);

        unsafe {
            gl::GenVertexArrays(1, &mut vao);
            gl::BindVertexArray(vao);
            gl::GenBuffers(1, &mut vbo);
            gl::GenBuffers(1, &mut ebo);
        }

        self.meshes.push(OpenGLMesh {
            vao,
            vbo,
            ebo,
            num_elem: 0,
            use_ebo: false
        });
        (self.meshes.len() - 1) as isize
    }

    pub(in crate::video::backends) fn mesh_data(&mut self, mesh: MeshID, data: VertexArrayCreateInfo) {
        let id = mesh as usize;
        let (vao, vbo, _ebo) = (
            self.meshes[id].vao,
            self.meshes[id].vbo,
            self.meshes[id].ebo
        );
        self.meshes[id].use_ebo = false;
        unsafe {
            gl::BindVertexArray(vao);
            gl::BindBuffer(gl::ARRAY_BUFFER, vbo);
            gl::BufferData(
                gl::ARRAY_BUFFER,
                (data.num_verts * std::mem::size_of::<f32>()) as isize,
                data.vert_data as *const _,
                gl::STATIC_DRAW
            );

            let mut start_pointer = 0usize;
            for c in 0..data.components.len() {
                gl::VertexAttribPointer(
                    c as u32,
                    data.components[c].num_elem as i32,
                    gl::FLOAT,
                    gl::FALSE,
                    (data.stride_size * std::mem::size_of::<f32>()) as i32,
                    start_pointer as *const _
                );
                gl::EnableVertexAttribArray(c as u32);
                start_pointer += data.components[c].num_elem * std::mem::size_of::<f32>();
            }
        }
        self.meshes[id].num_elem = (data.num_verts / data.stride_size) as i32;
    }

    pub(in crate::video::backends) fn draw_mesh(&mut self, mesh: MeshID) {
        let id = mesh as usize;
        unsafe {
            gl::BindVertexArray(self.meshes[id].vao);
            gl::DrawArrays(gl::TRIANGLES, 0, self.meshes[id].num_elem);
        }
    }

    pub(in crate::video::backends) fn new_shader(&mut self) -> ShaderID {
        self.shaders.push(OpenGLShader {
            shaders_uninit: Vec::new(),
            program: unsafe { gl::CreateProgram() }
        });
        (self.shaders.len() - 1) as isize
    }

    pub(in crate::video::backends) fn shader_add_source(&mut self, shader_id: ShaderID, shader_type: crate::shader::ShaderType, source: &str) {
        let id = shader_id as usize;
        unsafe {
            gl::UseProgram(self.shaders[id].program);
            let shader = gl::CreateShader(shader_type as u32);
            gl::ShaderSource(shader, 1, &(source as *const str as *const i8), 0 as *const i32);
            gl::CompileShader(shader);
            self.shaders[id].shaders_uninit.push(shader);

            let mut info = crate::memory::Block::<i8>::new(512).unwrap();
            gl::GetShaderInfoLog(shader, 512, 0 as *mut i32, info.ptr() as *mut i8);
            println!("{}", std::ffi::CStr::from_ptr(info.ptr()).to_str().unwrap());
        }
    }

    pub(in crate::video::backends) fn link_shader(&mut self, shader_id: ShaderID) {
        let id = shader_id as usize;
        unsafe {
            for s in &self.shaders[id].shaders_uninit {
                gl::AttachShader(self.shaders[id].program, *s);
            }
            gl::LinkProgram(self.shaders[id].program);
            
            let mut info = crate::memory::Block::<i8>::new(512).unwrap();
            gl::GetProgramInfoLog(self.shaders[id].program, 512, 0 as *mut i32, info.ptr() as *mut i8);
            println!("{}", std::ffi::CStr::from_ptr(info.ptr()).to_str().unwrap());
        }
    }

    pub(in crate::video::backends) fn make_shader_current(&mut self, shader_id: ShaderID) {
        unsafe {
            gl::UseProgram(self.shaders[shader_id as usize].program);
        }
    }
}
