pub struct VertexArrayComponentCreateInfo {
    pub num_elem: usize,
    pub name: String
}


pub struct VertexArrayCreateInfo {
    pub(in crate::video::backends) vert_data: *const f32,
    pub(in crate::video::backends) num_verts: usize,
    pub(in crate::video::backends) components: Vec<VertexArrayComponentCreateInfo>,
    pub(in crate::video::backends) stride_size: usize
}


impl VertexArrayCreateInfo {
    /// Create a new vertex array info struct\
    /// 
    /// `vert_data` -> Pointer to vertex data\
    /// `num_verts` -> Total number of elements in the vert_data pointer
    pub fn new(vert_data: *const f32, num_verts: usize) -> Self {
        VertexArrayCreateInfo {
            vert_data,
            num_verts,
            components: Vec::new(),
            stride_size: 0
        }
    }

    pub fn add_component_info(&mut self, comp: VertexArrayComponentCreateInfo) {
        self.stride_size += comp.num_elem;
        self.components.append(&mut vec![comp]);
    }
}