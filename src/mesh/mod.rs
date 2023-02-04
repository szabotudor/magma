use super::video::*;
use crate::video::backends::vertex_array_info::*;
extern crate alloc;


pub struct Mesh {
    mesh: Box<dyn backends::BackendMesh>
}


impl Mesh {
    /// Create a new empty mesh compatible with the given backend\
    /// 
    /// `backend` -> Reference to the backend to use
    pub fn new(backend: &mut backends::VideoBackend) -> Self {
        Mesh {
            mesh: backend.new_mesh()
        }
    }

    /// Manually load vertices into the mesh\
    /// 
    /// `vertex_array_components_create_info` -> Info about how to send vertex info to be drawn
    pub fn manual_load_vertices(&mut self, vertex_array_components_create_info: VertexArrayCreateInfo) {
        self.mesh.data(vertex_array_components_create_info);
    }

    pub fn draw(&mut self, shader: &mut crate::shader::Shader) {
        shader.make_current();
        self.mesh.draw();
    }
}
