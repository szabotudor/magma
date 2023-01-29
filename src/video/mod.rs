/// Module to manage and automate different video backends
/// - OpenGL
/// - Vulkan (todo)
pub mod backends;

use self::backends::VideoBackend;
use super::mgmath::*;


pub struct VertexArrayComponentCreateInfo {
    pub num_elem: usize,
    pub name: String
}


pub struct VertexArrayCreateInfo {
    vert_data: *const f32,
    num_verts: usize,
    components: Vec<VertexArrayComponentCreateInfo>,
    stride_size: usize
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




pub struct MgmWindow {
    resolution: Vec2u32,
    title: String,
    is_open: bool,
    
    sdl_context: sdl2::Sdl,
    sdl_video_subsystem: sdl2::VideoSubsystem,
    sdl_window: sdl2::video::Window,

    pub backend: backends::VideoBackend,

    clear_color: Vec4f32
}


impl MgmWindow {
    /// Create a new MgmWindow\
    /// 
    /// `res` -> Window size and resolution\
    /// `title` -> Window titles
    /// 
    /// Important
    /// =
    /// - Default backend set to opengl
    /// - Whatever backend set, you must use apropriate shaders. (OpenGL backend only accepts OpenGL shaders)
    /// - 
    pub fn new(res: Vec2u32, title: &str, backend: crate::enums::VideoBackendType) -> Self {
        let sdl_context = sdl2::init().unwrap();
        let sdl_video_subsystem = sdl_context.video().unwrap();
        let sdl_window = backends::VideoBackend::create_window(&sdl_video_subsystem, res, title, backend);

        let backend = backends::VideoBackend::create_opengl(&sdl_video_subsystem, &sdl_window);

        MgmWindow {
            resolution: res,
            title: title.to_string(),
            is_open: true,
            sdl_context,
            sdl_video_subsystem,
            sdl_window,
            backend,
            clear_color: Vector!(0.0, 0.0, 0.0, 1.0)
        }
    }

    /// Set the color to clear the screen to\
    /// Clearing the screen can be done with `MgmWindow::clear()`\
    /// Default color is black - `Vector!(0.0, 0.0, 0.0, 1.0)`\
    /// 
    /// `color` -> the color to clear the screen to
    pub fn set_clear_color(&mut self, color: Vec4f32) {
        self.clear_color = color;
        self.backend.set_clear_color(color);
    }

    /// Clear the screen to the selected color\
    /// Set the color to clear the screen to using `set_clear_color()`
    pub fn clear(&mut self) {
        self.backend.clear();
    }

    /// Return true if the window is open\
    /// Window can be closed using:
    /// - `Alt + F4`
    /// - The X button on the window (not the X key on the keyboard)
    /// - Using the `close()` function
    pub fn is_open(&mut self) -> bool {
        self.is_open
    }

    /// Poll all events and process them\
    /// Swap buffers (this can also be done on the backend)
    pub fn update(&mut self) {
        let mut events = self.sdl_context.event_pump().unwrap();
        for event in events.poll_iter() {
            match event {
                sdl2::event::Event::Quit { .. } => {
                    self.is_open = false;
                },
                _ => { }
            }
        }
        self.backend.swap(&self.sdl_window);
    }

    /// Close the window
    pub fn close(&mut self) {
        self.is_open = false;
    }
}
