pub struct Vecb<const S: usize, T> {
    data: [T; S]
}


impl<const S: usize, T> Vecb<S, T>
where T: Default + From<f64> + Into<f64> + std::ops::Mul<Output = T> + std::ops::AddAssign + Copy,
    Self: std::ops::Sub<Output = Self> + std::ops::Div<Output = Self> + std::ops::DivAssign + From<T> {
    pub fn dot(&self, v: &Self) -> T {
        let mut res: T = T::default();
        for i in 0..S {
            res += self[i] * v[i];
        }
        res
    }

    pub fn len(&self) -> T {
        f64::sqrt(self.clone().dot(self).into()).into()
    }

    pub fn distance_to(&self, v: &Self) -> T {
        (*v - *self).len()
    }

    pub fn normalize(&mut self) -> &mut Self {
        let len = Vecb::<S, T>::from(self.len());
        *self /= len;
        self
    }

    pub fn normalized(&self) -> Self {
        *self / Vecb::<S, T>::from(self.len())
    }
}


impl<const S: usize, T> std::ops::Index<usize> for Vecb<S, T> {
    type Output = T;

    fn index(&self, index: usize) -> &Self::Output {
        &self.data[index]
    }
}


impl<const S: usize, T> std::ops::IndexMut<usize> for Vecb<S, T> {
    fn index_mut(&mut self, index: usize) -> &mut Self::Output {
        &mut self.data[index]
    }
}


impl<T> Vecb<2, T> {
    pub fn x(&mut self) -> &mut T { &mut self.data[0] }
    pub fn y(&mut self) -> &mut T { &mut self.data[1] }
}
impl<T> Vecb<3, T> {
    pub fn x(&mut self) -> &mut T { &mut self.data[0] }
    pub fn y(&mut self) -> &mut T { &mut self.data[1] }
    pub fn z(&mut self) -> &mut T { &mut self.data[2] }
}
impl<T> Vecb<4, T> {
    pub fn x(&mut self) -> &mut T { &mut self.data[0] }
    pub fn y(&mut self) -> &mut T { &mut self.data[1] }
    pub fn z(&mut self) -> &mut T { &mut self.data[2] }
    pub fn w(&mut self) -> &mut T { &mut self.data[3] }
}


impl<const S: usize, T> std::ops::Add for Vecb<S, T>
where T: std::ops::AddAssign + Copy{
    type Output = Self;

    fn add(self, rhs: Self) -> Self::Output {
        let mut res = Vecb { data: self.data };
        for i in 0..S {
            res.data[i] += rhs.data[i];
        }
        res
    }
}
impl<const S: usize, T> std::ops::Sub for Vecb<S, T>
where T: std::ops::SubAssign + Copy{
    type Output = Self;

    fn sub(self, rhs: Self) -> Self::Output {
        let mut res = Vecb { data: self.data };
        for i in 0..S {
            res.data[i] -= rhs.data[i];
        }
        res
    }
}
impl<const S: usize, T> std::ops::Mul for Vecb<S, T>
where T: std::ops::MulAssign + Copy{
    type Output = Self;

    fn mul(self, rhs: Self) -> Self::Output {
        let mut res = Vecb { data: self.data };
        for i in 0..S {
            res.data[i] *= rhs.data[i];
        }
        res
    }
}
impl<const S: usize, T> std::ops::Div for Vecb<S, T>
where T: std::ops::DivAssign + Copy{
    type Output = Self;

    fn div(self, rhs: Self) -> Self::Output {
        let mut res = Vecb { data: self.data };
        for i in 0..S {
            res.data[i] /= rhs.data[i];
        }
        res
    }
}


impl<const S: usize, T> std::ops::AddAssign for Vecb<S, T>
where T: Into<Self>,
    Self: std::ops::Add<Output = Self> + Clone {
    fn add_assign(&mut self, rhs: Self) {
        (*self) = self.clone() + rhs.into();
    }
}
impl<const S: usize, T> std::ops::SubAssign for Vecb<S, T>
where T: Into<Self>,
    Self: std::ops::Sub<Output = Self> + Clone {
    fn sub_assign(&mut self, rhs: Self) {
        (*self) = self.clone() - rhs.into();
    }
}
impl<const S: usize, T> std::ops::MulAssign for Vecb<S, T>
where T: Into<Self>,
    Self: std::ops::Mul<Output = Self> + Clone {
    fn mul_assign(&mut self, rhs: Self) {
        (*self) = self.clone() * rhs.into();
    }
}
impl<const S: usize, T> std::ops::DivAssign for Vecb<S, T>
where T: Into<Self>,
    Self: std::ops::Div<Output = Self> + Clone {
    fn div_assign(&mut self, rhs: Self) {
        (*self) = self.clone() / rhs.into();
    }
}


impl<const S: usize, T> From<[T; S]> for Vecb<S, T> {
    fn from(value: [T; S]) -> Self {
        Vecb { data: value }
    }
}


impl<const S: usize, T: Copy> From<T> for Vecb<S, T> {
    fn from(value: T) -> Self {
        Vecb { data: [value; S] }
    }
}


impl<const S: usize, T: Clone> Clone for Vecb<S, T> {
    fn clone(&self) -> Self {
        Self { data: self.data.clone() }
    }
}
impl<const S: usize, T: Copy> Copy for Vecb<S, T> { }


impl<const S: usize, T: std::fmt::Display> std::fmt::Display for Vecb<S, T> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{{").unwrap();
        for i in 0..S-1 {
            write!(f, "{}, ", self.data[i]).unwrap();
        }
        write!(f, "{}}}", self.data[S - 1])
    }
}


#[macro_export]
macro_rules! Vector {
    (count $x:expr) => {
        1
    };

    (count $x:expr, $($y:expr),*) => {
        1 + Vector!(count $($y),*)
    };

    (init $d:expr, $i:expr, $x:expr) => {
        $d[$i] = $x;
    };

    (init $d:expr, $i:expr, $x:expr, $($y:expr),*) => {
        $d[$i] = $x;
        Vector!(init $d, $i + 1, $($y),*);
    };

    ($x:expr, $($y:expr),*) => {{
        use crate::mgmath::*;
        let mut data = [$x; Vector!(count $x, $($y),*)];
        Vector!(init data, 1, $($y),*);
        Vecb::from(data)
    }};
}

pub(crate) use Vector;

pub type Vec2f32 = Vecb<2, f32>;
pub type Vec3f32 = Vecb<3, f32>;
pub type Vec4f32 = Vecb<4, f32>;
pub type Vec2f64 = Vecb<2, f64>;
pub type Vec3f64 = Vecb<3, f64>;
pub type Vec4f64 = Vecb<4, f64>;

pub type Vec2u8 = Vecb<2, u8>;
pub type Vec3u8 = Vecb<3, u8>;
pub type Vec4u8 = Vecb<4, u8>;
pub type Vec2i8 = Vecb<2, i8>;
pub type Vec3i8 = Vecb<3, i8>;
pub type Vec4i8 = Vecb<4, i8>;

pub type Vec2u16 = Vecb<2, u16>;
pub type Vec3u16 = Vecb<3, u16>;
pub type Vec4u16 = Vecb<4, u16>;
pub type Vec2i16 = Vecb<2, i16>;
pub type Vec3i16 = Vecb<3, i16>;
pub type Vec4i16 = Vecb<4, i16>;

pub type Vec2u32 = Vecb<2, u32>;
pub type Vec3u32 = Vecb<3, u32>;
pub type Vec4u32 = Vecb<4, u32>;
pub type Vec2i32 = Vecb<2, i32>;
pub type Vec3i32 = Vecb<3, i32>;
pub type Vec4i32 = Vecb<4, i32>;

pub type Vec2u64 = Vecb<2, u64>;
pub type Vec3u64 = Vecb<3, u64>;
pub type Vec4u64 = Vecb<4, u64>;
pub type Vec2i64 = Vecb<2, i64>;
pub type Vec3i64 = Vecb<3, i64>;
pub type Vec4i64 = Vecb<4, i64>;
