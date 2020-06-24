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

let map_async: (gen_async('a), 'a => 'b) => gen_async('b) =
  (gen, f) => {
    open Js.Promise;
    let p = ref(gen->next_async);
    let aux: unit => next_async('b) =
      () =>
        p^
        |> then_(v =>
             (
               switch (v.done_, v.value) {
               | (false, Some(v)) =>
                 p := gen->next_async;
                 {value: Some(f(v)), done_: false};
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

let consume_async: (gen_async('a), 'a => 'b) => async(unit) =
  (gen, f) => {
    open Js.Promise;
    let p = ref(gen->next_async);

    let rec aux = () =>
      p^
      |> then_(nx =>
           switch (nx.done_, nx.value) {
           | (false, Some(v)) =>
             f(v);
             p := gen->next_async;
             aux();
           | (false, None) =>
             p := gen->next_async;
             aux();
           | (true, _) => resolve()
           }
         );
    aux();
  };