pub mod enums;

#[allow(dead_code)]
pub mod mgmath;

#[allow(dead_code)]
#[allow(unused_imports)]
pub mod video;


#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn opengl() {
        let mut window = video::MgmWindow::new(Vector!(800, 600), Some("Window Title"));
        window.set_clear_color(Vector!(0.1, 0.2, 0.3, 1.0));
        while window.is_open() {
            window.clear();
            window.update();
        }
    }
}
