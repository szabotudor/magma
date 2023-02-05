pub struct VertexArrayComponentCreateInfo {
    pub num_elem: usize,
    pub name: String
}


pub struct VertexArrayCreateInfo {
    pub(in crate::video::backends) vert_data: crate::memory::Block<f32>,
    pub(in crate::video::backends) num_verts: usize,
    pub(in crate::video::backends) components: Vec<VertexArrayComponentCreateInfo>,
    pub(in crate::video::backends) stride_size: usize,

    pub(in crate::video::backends) elem_data: crate::memory::Block<u32>,
    pub(in crate::video::backends) num_elem: usize
}


impl VertexArrayCreateInfo {
    /// Create a new vertex array info struct\
    /// 
    /// `vert_data` -> Pointer to vertex data\
    /// `num_verts` -> Total number of floats in the vert_data pointer (`vert_data.len()` if `vert_data` is an array)\
    /// `element_data` -> Pointer to element array (use `0` to ignore EBO)\
    /// `num_elements` -> Number of elements in the element array (use `0` if unused)
    pub fn new(vert_data: *const f32, num_verts: usize, element_data: *const u32, num_elements: usize) -> Self {
        VertexArrayCreateInfo {
            vert_data: crate::memory::Block::from_ptr(vert_data, num_verts).unwrap(),
            num_verts,
            components: Vec::new(),
            stride_size: 0,
            elem_data: crate::memory::Block::from_ptr(element_data, num_elements).unwrap(),
            num_elem: num_elements
        }
    }

    /// Add a component to the vertex array info
    pub fn add_component_info(&mut self, comp: VertexArrayComponentCreateInfo) {
        self.stride_size += comp.num_elem;
        self.components.append(&mut vec![comp]);
    }
}


impl Drop for VertexArrayCreateInfo {
    fn drop(&mut self) {
        self.vert_data.destroy();
        self.elem_data.destroy();
    }
}
