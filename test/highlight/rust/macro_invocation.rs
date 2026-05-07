macro_rules! make_vec {
    ($($x:expr),* $(,)?) => { vec![$($x),*] };
}

let values = make_vec!(1, 2, 3);
