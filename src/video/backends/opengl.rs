use std::os::raw::c_void;

use super::*;


pub(in crate::video::backends) struct OpenGLMesh {
    vao: gl::types::GLuint,
    vbo: gl::types::GLuint,
    ebo: gl::types::GLuint
}


impl VideoBackend {
    pub fn create_opengl_window(sdl_video_subsystem: &sdl2::VideoSubsystem, res: mgmath::Vec2u32, title: &str) -> sdl2::video::Window {
        sdl_video_subsystem.window(title, res[0], res[1]).opengl().build().unwrap()
    }

    pub fn create_opengl(sdl_video_subsystem: &sdl2::VideoSubsystem, window: &sdl2::video::Window) -> Self {
        gl::load_with(|s| sdl_video_subsystem.gl_get_proc_address(s) as *const _);
        let gl_context = window.gl_create_context().unwrap();
        window.gl_make_current(&gl_context).unwrap();
        VideoBackend {
            backend_type: VideoBackendType::OpenGL,
            opengl_context: Some(window.gl_create_context().unwrap()),
            opengl_clear_buffer_mask: gl::COLOR_BUFFER_BIT | gl::DEPTH_BUFFER_BIT,
            opengl_meshes: Vec::new()
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

    pub(in crate::video::backends) fn opengl_init_new_mesh(&mut self) -> MeshID {
        let (mut vao, mut vbo, mut ebo) = (0u32, 0u32, 0u32);

        unsafe {
            gl::GenVertexArrays(1, &mut vao);
            gl::BindVertexArray(vao);
            gl::GenBuffers(1, &mut vbo);
            gl::GenBuffers(1, &mut ebo);
        }

        self.opengl_meshes.append(&mut vec![OpenGLMesh{
            vao: vao,
            vbo: vbo,
            ebo: ebo
        }]);
        (self.opengl_meshes.len() - 1) as isize
    }

    pub(in crate::video::backends) fn opengl_mesh_data(&mut self, mesh: MeshID, data: crate::video::VertexArrayCreateInfo) {
        let (vao, vbo, _ebo) = (
            self.opengl_meshes[mesh as usize].vao,
            self.opengl_meshes[mesh as usize].vbo,
            self.opengl_meshes[mesh as usize].ebo
        );
        unsafe {
            gl::BindVertexArray(vao);
            gl::BindBuffer(gl::VERTEX_ARRAY, vbo);
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
                start_pointer += data.components[c].num_elem * std::mem::size_of::<f32>();
            }
        }
    }
}
