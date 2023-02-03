pub mod memory;
pub mod mgmath;
pub mod video;
pub mod shader;
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
        let vertex_shader = "#version 420 core
            layout (location = 0) in vec3 aPos;
            
            void main()
            {
                gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
            }\0";
        let fragment_shader = "#version 420 core
            out vec4 FragColor;
            
            void main()
            {
                FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
            }\0";

        let vertices = [
            -0.5f32, -0.5, 0.0,
            0.5, -0.5, 0.0,
            0.0, 0.5, 0.0
        ];

        let mut window = video::MgmWindow::new(
            Vector!(800, 600),
            "OpenGL Test",
            video::backends::VideoBackendType::OpenGL
        );
        window.set_clear_color(Vector!(0.18, 0.19, 0.22, 1.0));

        let mut shader = shader::Shader::new(&window.backend);
        shader.add_source(vertex_shader, shader::ShaderType::VERTEX);
        shader.add_source(fragment_shader, shader::ShaderType::FRAGMENT);
        shader.link();

        let mut mesh = mesh::Mesh::new(&window.backend, &shader);
        let vac_info = video::backends::vertex_array_info::VertexArrayComponentCreateInfo {
            name: "vertices".to_string(),
            num_elem: 3
        };
        let mut va_info = video::backends::vertex_array_info::VertexArrayCreateInfo::new(
            vertices.as_ptr(),
            vertices.len()
        );
        va_info.add_component_info(vac_info);
        mesh.manual_load_vertices(va_info);

        while window.is_open() {
            window.clear();
            mesh.draw();
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
