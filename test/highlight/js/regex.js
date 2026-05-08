if (x + /abc/.test("abc")) {
  console.log("matched");
}

const additive = value + /foo/.exec(text);
const subtractive = value - /bar/.exec(text);
const unary = +/baz/.test(text);
const division = value / divisor / 2;
const afterReturn = (() => { return /ret/.test(text); })();
