open Sync;
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
 * Converts an sync generator into an async one.
 * ```reason
 * inf(1, a => a + 1)
 * ->from_sync
 * ->take_async(100)
 * ->consume_async(Js.Console.log)
 * ```
 */
let from_sync: gen('a) => gen_async('a) =
  gen =>
    Js.Promise.{
      next: () => gen->next->resolve,
      return: a => {value: Some(a), done_: true} |> resolve,
      throw: _ => {value: None, done_: true} |> resolve,
    };

/**
 * Transforms an async generator into a new one.
 * ```reason
 * puppy_stream
 * -> map_async(pet)
 * // => <async("doggo_1"), async("doggo_2"), ...>
 * ```
 */
let map_async: (gen_async('a), 'a => 'b) => gen_async('b) =
  (gen, f) => {
    open Js.Promise;
    let aux: unit => next_async('b) =
      () =>
        gen->next_async
        |> then_(v =>
             (
               switch (v.done_, v.value) {
               | (false, Some(v)) => {value: Some(f(v)), done_: false}
               | _ => {value: None, done_: true}
               }
             )
             |> resolve
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
    open Js.Promise;

    let rec aux = (acc, nx) =>
      switch (nx.done_, nx.value) {
      | (false, Some(v)) => gen |> next_async |> then_(aux(f(acc, v)))
      | _ => resolve(acc)
      };
    gen |> next_async |> then_(aux(a0));
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
    open Js.Promise;

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
 * Takes n elements from the async generator, and returns a new one of length n
 * ```reason
 *
 * ```
 */
let take_async = (gen, n) => {
  open Js.Promise;
  let i = ref(0);

  let aux = () =>
    gen->next_async
    |> then_(nx =>
         (
           switch (nx.done_, i^) {
           | (false, j) when j < n =>
             i := j + 1;
             {value: nx.value, done_: false};
           | _ => {value: None, done_: true}
           }
         )
         |> resolve
       );

  {
    next: aux,
    return: a => {value: Some(a), done_: true} |> resolve,
    throw: _ => {value: None, done_: true} |> resolve,
  };
};