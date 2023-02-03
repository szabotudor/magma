use super::video::*;
use crate::video::backends::vertex_array_info::*;
extern crate alloc;


pub struct Mesh {
    backend: *mut backends::VideoBackend,
    mesh_id: backends::MeshID,
    shader: *mut crate::shader::Shader
}


impl Mesh {
    /// Create a new empty mesh compatible with the given backend\
    /// 
    /// `backend` -> Reference to the backend to use
    pub fn new(backend: *const backends::VideoBackend, shader: *const crate::shader::Shader) -> Self {
        Mesh {
            backend: backend as *mut backends::VideoBackend,
            mesh_id: -1,
            shader: shader as *mut crate::shader::Shader
        }
    }

    /// Manually load vertices into the mesh\
    /// 
    /// `vertex_array_components_create_info` -> Info about how to send vertex info to be drawn
    pub fn manual_load_vertices(&mut self, vertex_array_components_create_info: VertexArrayCreateInfo) {
        unsafe {
            if self.mesh_id == -1 {
                self.mesh_id = (*self.backend).new_mesh();
            }
            (*self.backend).mesh_data(self.mesh_id, vertex_array_components_create_info);
        }
    }

    pub fn draw(&mut self) {
        unsafe {
            (*self.shader).make_current();
            (*self.backend).draw_mesh(self.mesh_id);
        }
    }
}
