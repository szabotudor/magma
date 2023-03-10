use super::*;


pub(in crate::video::backends) struct OpenGLMesh {
    vao: gl::types::GLuint,
    vbo: gl::types::GLuint,
    ebo: gl::types::GLuint,
    num_elem: i32,
    use_ebo: bool,
    draw_usage: DrawUsage,
    //draw_mode: DrawMode
}
impl OpenGLMesh {
    pub fn new() -> Self {
        let (mut vao, mut vbo, mut ebo) = (0u32, 0u32, 0u32);

        unsafe {
            gl::GenVertexArrays(1, &mut vao);
            gl::BindVertexArray(vao);
            gl::GenBuffers(1, &mut vbo);
            gl::GenBuffers(1, &mut ebo);
        }

        OpenGLMesh {
            vao,
            vbo,
            ebo,
            num_elem: 0,
            use_ebo: false,
            draw_usage: DrawUsage::STATIC,
            //draw_mode: DrawMode::FILL
        }
    }
}
impl BackendMesh for OpenGLMesh {
    fn data(&mut self, data: VertexArrayCreateInfo) {
        let draw_usage = match self.draw_usage {
            DrawUsage::STATIC => gl::STATIC_DRAW,
            DrawUsage::DYNAMIC => gl::DYNAMIC_DRAW,
            DrawUsage::STREAM => gl::STREAM_DRAW
        };
        unsafe {
            gl::BindVertexArray(self.vao);
            gl::BindBuffer(gl::ARRAY_BUFFER, self.vbo);
            gl::BufferData(
                gl::ARRAY_BUFFER,
                (data.num_verts * std::mem::size_of::<f32>()) as isize,
                data.vert_data.as_ptr() as *const _,
                draw_usage
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

            if data.elem_data.has_data() {
                gl::BindBuffer(gl::ELEMENT_ARRAY_BUFFER, self.ebo);
                gl::BufferData(
                    gl::ELEMENT_ARRAY_BUFFER,
                    (data.num_elem * std::mem::size_of::<u32>()) as isize,
                    data.elem_data.as_ptr() as *const _,
                    draw_usage
                );
                self.num_elem = data.num_elem as i32;
                self.use_ebo = true;
            }
            else {
                self.num_elem = (data.num_verts / data.stride_size) as i32;
            }
        }
    }

    fn draw(&mut self) {
        unsafe {
            if self.use_ebo {
                gl::BindBuffer(gl::ELEMENT_ARRAY_BUFFER, self.ebo);
                gl::DrawElements(gl::TRIANGLES, self.num_elem, gl::UNSIGNED_INT, 0 as *const _);
            }
            else {
                gl::BindVertexArray(self.vao);
                gl::DrawArrays(gl::TRIANGLES, 0, self.num_elem);
            }
        }
    }
}


pub(in crate::video::backends) struct OpenGLShader {
    shaders_uninit: Vec<u32>,
    program: u32
}
impl OpenGLShader {
    pub fn new() -> Self{
        OpenGLShader {
            shaders_uninit: Vec::new(),
            program: unsafe { gl::CreateProgram() }
        }
    }
}
impl BackendShader for OpenGLShader {
    fn add_source(&mut self, source: &str, shader_type: crate::shader::ShaderType) {
        unsafe {
            gl::UseProgram(self.program);
            let shader = gl::CreateShader(shader_type as u32);
            gl::ShaderSource(shader, 1, &(source as *const str as *const i8), 0 as *const i32);
            gl::CompileShader(shader);
            self.shaders_uninit.push(shader);

            let info = crate::memory::Block::<i8>::new(512).unwrap();
            gl::GetShaderInfoLog(shader, 512, 0 as *mut i32, info.as_ptr() as *mut i8);
            println!("{}", std::ffi::CStr::from_ptr(info.as_ptr()).to_str().unwrap());
        }
    }

    fn link(&mut self) {
        unsafe {
            for s in &self.shaders_uninit {
                gl::AttachShader(self.program, *s);
            }
            gl::LinkProgram(self.program);

            let info = crate::memory::Block::<i8>::new(512).unwrap();
            gl::GetProgramInfoLog(self.program, 512, 0 as *mut i32, info.as_ptr() as *mut i8);
            println!("{}", std::ffi::CStr::from_ptr(info.as_ptr()).to_str().unwrap());

            for s in &self.shaders_uninit {
                gl::DeleteShader(*s);
            }
            self.shaders_uninit.clear();
        }
    }

    fn make_current(&mut self) {
        unsafe {
            gl::UseProgram(self.program);
        }
    }
}


pub struct VideoBackendOpenGL {
    pub(in crate::video::backends) context: Option<sdl2::video::GLContext>,
    pub clear_buffer_mask: u32,
}
impl VideoBackendOpenGL {
    pub(in crate::video::backends) fn new() -> Self {
        VideoBackendOpenGL {
            context: None,
            clear_buffer_mask: 0u32
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
}
