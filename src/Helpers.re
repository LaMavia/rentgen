let ($) = (f, g, x) => f(g(x));
let (>>) = (p, f) => Js.Promise.(then_(resolve $ f, p));