// use hound;
// use std::env;

#[repr(C)]
pub struct AudioAnalyzeRhythmDetector {
    _data: [u8; 0],
    _marker:
        core::marker::PhantomData<(*mut u8, core::marker::PhantomPinned)>,
}

extern "C" {
    #[link(name="analyze_rhythm", kind="static")]
    pub fn create_rhythm_detector() -> *mut AudioAnalyzeRhythmDetector;
    pub fn delete_rhythm_detector(arg: *mut AudioAnalyzeRhythmDetector);
    fn testcall_cpp(v: f32);

}


fn main() {
    // let args: Vec<String> = env::args().collect();
    // let infile = &args[1];

    // let mut reader = hound::WavReader::open(infile).unwrap();
    // let samples = reader.samples::<i16>().for_each(|x| println!("{}", x.unwrap())   );

    unsafe {
        let detector = create_rhythm_detector();
        delete_rhythm_detector(detector);
    };
}
