

fn main() {
  let mut local_var = [0u8; 32];
  let hello = b"Hello";
  local_var[..hello.len()].copy_from_slice(hello);
  println!("Mem: {}", String::from_utf8_lossy(&local_var[..hello.len()]));
}
