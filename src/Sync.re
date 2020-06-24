type next('a) = {
  value: option('a),
  [@bs.as "done"]
  done_: bool,
};
type gen('a) = {
  next: unit => next('a),
  return: 'a => next('a),
  throw: Js.Exn.t => next('a),
};

[@bs.send] external next: gen('a) => next('a) = "next";
[@bs.send] external return: (gen('a), 'a) => next('a) = "return";
[@bs.send] external throw: (gen('a), Js.Exn.t) => next('a) = "throw";

external from: 'a => gen('b) = "%identity";
/**
 * Creates an int range.
 * ```reason
 * range(~to_=10)->foldl((+), 0)
 * // => 55
 * ```
 */
let range = (~to_, ~from=0, ~step=1, ()) => {
  let v = ref(from - step);
  let aux: unit => next('b) =
    () => {
      v := v^ + step;
      if (v^ > to_) {
        {value: None, done_: true};
      } else {
        {value: Some(v^), done_: false};
      };
    };

  // Return a generator // 55
  {
    next: aux,
    return: a => {value: Some(a), done_: true},
    throw: _ => {value: None, done_: true},
  };
};

/**
 * Converts a list to a generator.
 * ```reason
 * [1,2,3]->from_list->foldl((+), 0)
 * // => 6
 * ```
 */
let from_list: list('a) => gen('a) =
  ls => {
    let i = ref(0);
    let n = List.length(ls);

    let aux = () =>
      if (i^ < n) {
        let v = List.nth(ls, i^);
        i := i^ + 1;
        {value: Some(v), done_: false};
      } else {
        {value: None, done_: true};
      };

    {
      next: aux,
      return: a => {value: Some(a), done_: true},
      throw: _ => {value: None, done_: true},
    };
  };

/**
 * Creates an infinite generator from an initial value and a succesor function.
 * ```reason
 * inf(0, a => a + 1) // => <0, 1, 2, 3, ...>
 * ```
 */
let inf = (a0, f) => {
  let v = ref(a0);

  let aux = () => {
    let v' = v^;
    v := f(v^);
    {value: Some(v'), done_: false};
  };

  {
    next: aux,
    return: a => {value: Some(a), done_: true},
    throw: _ => {value: None, done_: true},
  };
};

/**
 * Transforms the values of a generator.
 * ```reason
 * range(~to_=5, ()) -> map(a => a * 2)
 * // => <0, 2, 4, 6, 8, 10>
 * ```
 */
let map: (gen('a), 'a => 'b) => gen('b) =
  (gen, f) => {
    let v = ref(gen->next);
    let aux: unit => next('b) =
      () =>
        switch (v^.done_, v^.value) {
        | (false, Some(a)) =>
          v := gen->next;
          {value: Some(f(a)), done_: false};
        | _ => {value: None, done_: true}
        };

    // Return a generator //
    {
      next: aux,
      return: a => {value: Some(a), done_: true},
      throw: _ => {value: None, done_: true},
    };
  };

/**
 * Returns only the values of the generator which match the function.
 * ```reason
 * range(~to_=10, ()) -> keep(a => a mod 2 == 0)
 * => <0, 2, 4, 6, 8, 10>
 * ```
 */
let keep: (gen('a), 'a => bool) => gen('a) =
  (gen, f) => {
    let aux = () => {
      let rec loop = v =>
        switch (v.done_, v.value) {
        | (false, Some(v)) =>
          if (f(v)) {
            Some(v);
          } else {
            loop(gen->next);
          }
        | (false, None) => loop(gen->next)
        | _ => None
        };

      switch (loop(gen->next)) {
      | Some(v) => {value: Some(v), done_: false}
      | _ => {value: None, done_: true}
      };
    };

    // Return a generator //
    {
      next: aux,
      return: a => {value: Some(a), done_: true},
      throw: _ => {value: None, done_: true},
    };
  };

/**
 * Consumes a generator with a function.
 * ```reason
 * // prints an infinite generator
 * inf(0, a => a + 1) -> consume(Js.Console.log)
 * // 0
 * // 1
 * // 2 ...
 * // => unit
 * ```
 */
let consume: (gen('a), 'a => 'b) => unit =
  (gen, f) => {
    let rec aux = nx =>
      switch (nx.done_, nx.value) {
      | (false, Some(v)) =>
        f(v);
        aux(gen->next);
      | (false, None) => aux(gen->next)
      | (true, _) => ()
      };
    aux(gen->next);
  };

/**
 * Folds a generator into a single value.
 * ```reason
 * [1,2,3]->from_list->foldl((+), 0)
 * // => 6
 * ```
 */
let foldl: (gen('a), ('b, 'a) => 'b, 'b) => 'b =
  (gen, f, a0) => {
    let rec aux = (nx, acc) =>
      switch (nx.done_, nx.value) {
      | (false, Some(v)) => aux(gen->next, f(acc, v))
      | (false, None) => aux(gen->next, acc)
      | (true, _) => acc
      };
    aux(gen->next, a0);
  };

/**
 * Takes n values of the generator or until it ends.
 * ```reason
 * inf(0, a => a + 1) -> take(5)
 * // => <0, 1, 2, 3, 4>
 * ```
 */
let take = (gen, n) => {
  let i = ref(0);

  let aux = () => {
    let nx = gen->next;
    switch (nx.done_, i^) {
    | (false, j) when j < n =>
      i := j + 1;
      {value: nx.value, done_: false};
    | _ => {value: None, done_: true}
    };
  };

  {
    next: aux,
    return: a => {value: Some(a), done_: true},
    throw: _ => {value: None, done_: true},
  };
};

/**
 * Takes the values of the generator as long as the function returns `true`.
 * ```reason
 * inf(0, a => a*2 + 1)->take_while(a => a < 15)
 * // => <0, 1, 3, 7>
 * ```
 */
let take_while = (gen, f) => {
  let aux = () => {
    let nx = gen->next;
    switch (nx.done_, nx.value) {
    | (false, Some(v)) when f(v) => {value: Some(v), done_: false}
    | _ => {value: None, done_: true}
    };
  };

  {
    next: aux,
    return: a => {value: Some(a), done_: true},
    throw: _ => {value: None, done_: true},
  };
};