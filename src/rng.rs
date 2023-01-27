use sdl2::TimerSubsystem;

pub struct RNG {
    state: u64,
    seed: u64
}


impl RNG {
    /// Create a new random number generator\
    /// 
    /// `initial_state` -> The state of the rng\
    /// `initial_seed` -> The seed for the rng
    pub fn new(initial_state: u64, initial_seed: u64) -> Self {
        let mut r = RNG { state: initial_state, seed: initial_seed };
        if initial_state == 0 || initial_seed == 0 { r.randomize_state(); }
        r
    }

    fn now() -> u64 {
        std::time::SystemTime::now()
            .duration_since(std::time::SystemTime::UNIX_EPOCH)
            .unwrap()
            .as_millis() as u64
    }

    pub fn randomize_state(&mut self) -> &mut Self {
        self.state = Self::now();
        self
    }

    pub fn randomize_seed(&mut self) -> &mut Self {
        self.seed = Self::now();
        self
    }

    pub fn randomize(&mut self) -> &mut Self {
        self.randomize_state();
        self.randomize_seed();
        self
    }

    pub fn set_seed(&mut self, seed: u64) -> &mut Self {
        self.seed = seed;
        self
    }

    pub fn reset_state(&mut self) -> &mut Self {
        self.state = 0;
        self
    }

    pub fn randu64(&mut self) -> u64 {
        let mut s = self.seed ^ self.state;
        s ^= s << 13;
        s ^= s >> 17;
        s ^= s << 5;
        self.state = s;
        self.state
    }

    pub fn randi64(&mut self) -> i64 { self.randu64() as i64 }
    pub fn randuf64(&mut self) -> f64 { self.randu64() as f64 * 5.4210108624275222E-20 }
    pub fn randf64(&mut self) -> f64 { self.randi64() as f64 * 1.0842021724855044E-19 }


    pub fn randu32(&mut self) -> u32 { self.randu64() as u32 }
    pub fn randi32(&mut self) -> i32 { self.randu32() as i32 }
    pub fn randuf32(&mut self) -> f32 { self.randu32() as f32 * 2.32830644E-10 }
    pub fn randf32(&mut self) -> f32 { self.randi32() as f32 * 4.65661287E-10 }

    pub fn randu16(&mut self) -> u16 { self.randu64() as u16 }
    pub fn randi16(&mut self) -> i16 { self.randu64() as i16 }

    pub fn randu8(&mut self) -> u8 { self.randu64() as u8 }
    pub fn randi8(&mut self) -> i8 { self.randu64() as i8 }

    pub fn randb(&mut self) -> bool { self.randi64() >= 0 }
}
