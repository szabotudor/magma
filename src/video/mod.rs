/// Module to manage and automate different video backends
/// - OpenGL
/// - Vulkan (todo)
pub mod backends;

use super::mgmath::*;


#[allow(dead_code)]
pub struct MgmWindow {
    resolution: Vec2u32,
    title: String,
    is_open: bool,

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
    pub fn new(res: Vec2u32, title: &str, backend_type: backends::VideoBackendType) -> Self {
        let backend = backends::VideoBackend::new(res, title, backend_type);

        MgmWindow {
            resolution: res,
            title: title.to_string(),
            is_open: true,
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
        let mut events = self.backend.sdl_context.event_pump().unwrap();
        for event in events.poll_iter() {
            match event {
                sdl2::event::Event::Quit { .. } => {
                    self.is_open = false;
                },
                _ => { }
            }
        }
        self.backend.swap();
    }

    /// Close the window
    pub fn close(&mut self) {
        self.is_open = false;
    }
}
