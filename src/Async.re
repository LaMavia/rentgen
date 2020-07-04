open Sync;
open Helpers;
open Js.Promise;
type async('a) = Js.Promise.t('a);
type next_async('a) = async(next('a));
type gen_async('a) = {
  next: unit => next_async('a),
  return: 'a => next_async('a),
  throw: Js.Exn.t => next_async('a),
};

[@bs.send] external next_async: gen_async('a) => next_async('a) = "next";
[@bs.send]
external return_async: (gen_async('a), 'a) => next_async('a) = "return";
[@bs.send]
external throw_async: (gen_async('a), Js.Exn.t) => next_async('a) = "throw";

external from_async: 'a => gen_async('b) = "%identity";

/**
 * Converts a sync generator into an async one.
 * ```reason
 * inf(1, a => a + 1)
 * ->from_sync
 * ->take_async(100)
 * ->consume_async(Js.Console.log)
 * ```
 */
let from_sync: gen('a) => gen_async('a) =
  gen => {
    next: () => gen->next->resolve,
    return: a => {value: Some(a), done_: true} |> resolve,
    throw: _ => {value: None, done_: true} |> resolve,
  };

/**
 * Transforms an async generator using the function given.
 * ```reason
 * puppy_stream
 * -> map_async(pet)
 * // => <async("doggo_1"), async("doggo_2"), ...>
 * ```
 */
let map_async: (gen_async('a), 'a => 'b) => gen_async('b) =
  (gen, f) => {
    let aux: unit => next_async('b) =
      () =>
        gen->next_async
        >> (
          v =>
            switch (v.done_, v.value) {
            | (false, Some(v)) => {value: Some(f(v)), done_: false}
            | _ => v
            }
        );

    {
      next: aux,
      return: a => {value: Some(a), done_: true} |> resolve,
      throw: _ => {value: None, done_: true} |> resolve,
    };
  };

/**
 * Folds an async generetor into a single promise
 * ```reason
 * open_file("data/numbers.txt")
 * ->map_async(Js.Float.fromString)
 * ->foldl_async((+.), 0.)
 * // => async(21)
 * ```
 */
let foldl_async: (gen_async('a), ('b, 'a) => 'b, 'b) => async('b) =
  (gen, f, a0) => {
    let rec aux = (acc, nx) =>
      switch (nx.done_, nx.value) {
      | (false, Some(v)) => gen |> next_async |> then_(aux(f(acc, v)))
      | _ => resolve(acc)
      };
    gen |> next_async |> then_(aux(a0));
  };

/**
 * Folds an async generator into a single promise, taking the first value as `a0`.
 * ```reason
 * range(~to_=3, ~from=1, ())
 * ->map(float_of_int)
 * ->from_sync
 * ->foldl1_async((a, b) => a /. b)
 * // => async(1/6)
 * ```
 */
let foldl1_async: (gen_async('a), ('b, 'a) => 'b) => async('b) =
  (gen, f) => {
    let rec aux = (acc, nx) =>
      switch (nx.done_, nx.value) {
      | (false, Some(v)) => gen |> next_async |> then_(aux(f(acc, v)))
      | _ => resolve(acc)
      };
    gen
    |> next_async
    |> then_(nx =>
         switch (nx) {
         | {done_: false, value: Some(a0)} =>
           gen |> next_async |> then_(aux(a0))
         | _ => raise(Js.Exn.raiseError("empty generator in foldl1_async"))
         }
       );
  };

/**
 * Consumes an async stream, and returns an empty promise
 * ```reason
 * from_async(Deno.iter(file, ()))
 * ->consume_async(Js.Console.log)
 * // (prints the file)
 * // => async(unit)
 * ```
 */
let consume_async: (gen_async('a), 'a => 'b) => async(unit) =
  (gen, f) => {
    let rec aux = nx =>
      switch (nx.done_, nx.value) {
      | (false, Some(v)) =>
        f(v);
        gen |> next_async |> then_(aux);
      | _ => resolve()
      };
    gen |> next_async |> then_(aux);
  };

/**
 * Takes the first n elements from the async generator, and returns a new one of the length n
 * ```reason
 * infinite_puppies
 * ->take_async(5)
 * // => async<"puppy1", "puppy2", ... , "puppy5">
 * ```
 */
let take_async = (gen, n) => {
  let i = ref(0);

  let aux = () =>
    gen->next_async
    >> (
      nx =>
        switch (nx.done_, i^) {
        | (false, j) when j < n =>
          i := j + 1;
          nx;
        | _ => {value: None, done_: true}
        }
    );

  {
    next: aux,
    return: a => {value: Some(a), done_: true} |> resolve,
    throw: _ => {value: None, done_: true} |> resolve,
  };
};

/**
 * Takes elements of the generator as long as the condition given is met. Returns a new async generator. 
 */
let take_while_async = (gen, f) => {
  let aux = () =>
    gen->next_async
    >> (
      nx =>
        switch (nx.done_, nx.value) {
        | (false, Some(v)) when f(v) => nx
        | _ => {value: None, done_: true}
        }
    );

  {
    next: aux,
    return: a => {value: Some(a), done_: true} |> resolve,
    throw: _ => {value: None, done_: true} |> resolve,
  };
};

/**
 * Returns an async generator of the items which meet the condition given
 */
let keep_async: (gen_async('a), 'a => bool) => gen_async('a) =
  (gen, f) => {
    let aux: next('a) => next_async('a) =
      p => {
        let rec loop = v =>
          switch (v.done_, v.value) {
          | (false, Some(v)) =>
            if (f(v)) {
              Some(v) |> resolve;
            } else {
              gen->next_async |> then_(loop);
            }
          | (false, None) => gen->next_async |> then_(loop)
          | _ => None |> resolve
          };

        loop(p)
        >> (
          p =>
            switch (p) {
            | Some(v) => {value: Some(v), done_: false}
            | _ => {value: None, done_: true}
            }
        );
      };

    // Return a generator //
    {
      next: () => gen->next_async |> then_(aux),
      return: a => {value: Some(a), done_: true} |> resolve,
      throw: _ => {value: None, done_: true} |> resolve,
    };
  };