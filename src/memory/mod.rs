use std::alloc::{alloc, dealloc, realloc, Layout};


pub struct Block<T> {
    data: *const T,
    len: usize,
    iter_pos: usize
}


impl<T> Block<T> {
    /// Allocate a new UNINITIALIZED block of memory on the heap\
    /// 
    /// `len` -> The number of elements of type `T` to fit into the block\
    /// If `len` is `0`, this function will return an empty block
    pub fn new(len: usize) -> Result<Self, String> {
        if len == 0 {
            Ok(Block {
                data: 0 as *const T,
                len: 0,
                iter_pos: 0
            })
        }
        else {
            let ptr = unsafe { alloc(Layout::array::<T>(len).unwrap()) as *const T };
            if ptr == (0 as *const T) {
                Err("Allocation Fail".to_string())
            }
            else {
                Ok(Block {
                    data: ptr,
                    len,
                    iter_pos: 0
                })
            }
        }
    }

    /// Allocate a new block of memory on the heap, initialized with `0`\
    /// 
    /// `len` -> The number of elements of type `T` to fit into the block\
    /// If `len` is `0`, this function will return an empty block
    pub fn new_zeroed(len: usize) -> Result<Self, String> {
        let mut res = Block::<T>::new(len).unwrap();
        res.init_zeroed();
        Ok(res)
    }

    /// Reallocate the memory for the block with a new size\
    /// Size `0` will result in the block being destroyed and all data discarded\
    /// Any data after the element with the index of `new_len` will be discarded is `new_len` is lower that `len`
    /// 
    /// `new_len` -> The new length for the block
    pub fn realloc(&mut self, new_len: usize) {
        if new_len == 0 {
            self.destroy();
        }
        else {
            unsafe {
                self.data = realloc(
                    self.data as *mut u8,
                    Layout::array::<T>(self.len).unwrap(),
                    new_len
                ) as *const T;
            }
        }
    }

    /// Destroy the block and discard its data
    pub fn destroy(&mut self) {
        unsafe {
            dealloc(
                self.data as *mut u8,
                Layout::array::<T>(self.len).unwrap()
            );
        }
        self.data = 0 as *const T;
        self.len = 0;
    }

    /// Returns the length of the block (number of elements of type `T`)
    pub fn len(&mut self) -> usize {
        self.len
    }

    /// Returns the size of the block in bytes
    pub fn size(&mut self) -> usize {
        self.len * std::mem::size_of::<T>()
    }

    /// Sets all bytes in the block to `0`
    pub fn init_zeroed(&mut self) {
        let ptr = self.data as *mut u8;
        for i in 0..self.len*std::mem::size_of::<T>() {
            unsafe { *ptr.add(i) = 0 };
        }
    }

    /// Copy the data in this block into another block\
    /// The number of elements that will be copied will be the lowest of `self.len` and `to.len`
    /// 
    /// `to` -> The block to copy the data to
    pub fn copy_to(&mut self, to: &mut Self) {
        unsafe {
            self.data.copy_to(
                to.data as *mut T,
                self.len.min(to.len)
            );
        }
    }

    pub fn ptr(&mut self) -> *const T {
        self.data
    }
}


impl<T> std::ops::Index<usize> for Block<T> {
    type Output = T;

    fn index(&self, index: usize) -> &Self::Output {
        assert!(index < self.len, "Index not within range");
        unsafe { &*self.data.add(index) }
    }
}


impl<T> std::ops::IndexMut<usize> for Block<T> {
    fn index_mut(&mut self, index: usize) -> &mut Self::Output {
        assert!(index < self.len, "Index not within range");
        unsafe { &mut *(self.data.add(index) as *mut T) }
    }
}


impl<T> Drop for Block<T> {
    fn drop(&mut self) {
        self.destroy();
    }
}


impl<T: Copy> Iterator for Block<T> {
    type Item = T;

    fn next(&mut self) -> Option<Self::Item> {
        if self.iter_pos < self.len {
            Some(self[self.iter_pos])
        }
        else {
            None
        }
    }
}
