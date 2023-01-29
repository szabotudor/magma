pub mod enums;

pub mod memory;

#[allow(dead_code)]
pub mod mgmath;

#[allow(dead_code)]
#[allow(unused_imports)]
pub mod video;

pub mod mesh;

#[allow(dead_code)]
#[allow(unused_imports)]
pub mod rng;


#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn memory_test() {
        let mut b = memory::Block::<f32>::new(16).unwrap();
        b[10] = 16.4;
        println!("{}", b[10]);
    }

    #[test]
    fn vectors() {
        let mut v: mgmath::Vec2f64 = Vector!(1.0, 1.0);
        println!("Normalized without assign: {}", v.normalized());
        println!("After: {}\n", v);

        println!("Normalized with assign: {}", v.normalize());
        println!("After {}", v);
    }

    #[test]
    fn opengl() {
        let mut window = video::MgmWindow::new(
            Vector!(800, 600),
            "OpenGL Test",
            enums::VideoBackendType::OpenGL
        );
        window.set_clear_color(Vector!(0.18, 0.19, 0.22, 1.0));
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
