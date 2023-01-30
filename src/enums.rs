pub enum VideoBackendType {
    OpenGL
}


impl Clone for VideoBackendType {
    fn clone(&self) -> Self {
        match self {
            Self::OpenGL => Self::OpenGL,
        }
    }
}


impl Copy for VideoBackendType { }
