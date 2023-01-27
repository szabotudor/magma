pub mod enums;

#[allow(dead_code)]
pub mod mgmath;

#[allow(dead_code)]
#[allow(unused_imports)]
pub mod video;

#[allow(dead_code)]
#[allow(unused_imports)]
pub mod rng;


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

    #[test]
    fn rng_test() {
        let mut p = rng::RNG::new(0, 0);
        p.randomize();
        let mut max = 0.0;
        let mut min = 0.0;

        for _ in 0..10000 {
            let x = p.randf64();
            if x > max {
                max = x;
            }
            else if x < min {
                min = x;
            }
            println!("{}", x);
        }

        println!("max:{}, min:{}", max, min);
    }
}
