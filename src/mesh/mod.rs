use super::video::*;
use crate::video::backends::vertex_array_info::*;
extern crate alloc;


pub struct Mesh<'a> {
    backend: &'a mut backends::VideoBackend,
    mesh_id: backends::MeshID
}


impl<'a> Mesh<'a> {
    /// Create a new empty mesh compatible with the given backend\
    /// 
    /// `backend` -> The mesh will retain a pointer to the backend\
    /// If this goes out of scope, drawing the mesh will fail, and the mesh must be created again
    pub fn new(backend: &'a mut backends::VideoBackend) -> Self {
        Mesh {
            backend,
            mesh_id: -1
        }
    }

    /// Manually load vertices into the mesh\
    /// 
    /// `vertex_array_components_create_info` -> Info about how to send vertex info to be drawn
    pub fn manual_load_vertices(&mut self, vertex_array_components_create_info: VertexArrayCreateInfo) {
        if self.mesh_id == -1 {
            self.mesh_id = self.backend.new_mesh();
        }
        (*self.backend).mesh_data(self.mesh_id, vertex_array_components_create_info);
    }
}
