extern crate cmake;

use cmake::Config;
use std::env;

fn main()
{
    // Same for C++ - to build the static libary from libfoo++ subdirectory,
    // but we don't need to consume its result.
    let dst = Config::new("c++_harness").build();

    // Now - emitting some cargo commands to build and link the lib.
    // This turns to be common to both our libs, so we do it once.
    println!("cargo:rustc-link-search=native={}", dst.display());

    println!("cargo:rustc-link-lib=static=analyze_rhythm");

    // C++ is bit more complicated, since platform specifics come to play
    let target  = env::var("TARGET").unwrap();
    if target.contains("apple")
    {
        println!("cargo:rustc-link-lib=dylib=c++");
    }
    else if target.contains("linux")
    {
        println!("cargo:rustc-link-lib=dylib=stdc++");
    }
    else
    {
        unimplemented!();
    }
}