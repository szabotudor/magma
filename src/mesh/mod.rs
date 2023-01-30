use super::video::*;
use crate::video::backends::vertex_array_info::*;
extern crate alloc;


pub struct Mesh {
    backend: *mut backends::VideoBackend,
    backend_mesh_id: backends::MeshID
}


impl Mesh {
    /// Create a new empty mesh compatible with the given backend\
    /// 
    /// `backend` -> The mesh will retain a pointer to the backend\
    /// If this goes out of scope, drawing the mesh will fail, and the mesh must be created again
    pub fn new(backend: *mut backends::VideoBackend) -> Self {
        Mesh {
            backend,
            backend_mesh_id: 0
        }
    }

    pub unsafe fn manual_load_vertices(&mut self, vertex_array_components_create_info: VertexArrayCreateInfo) {
        self.backend_mesh_id = (*self.backend).new_mesh();
        (*self.backend).mesh_data(self.backend_mesh_id, vertex_array_components_create_info);
    }
}
