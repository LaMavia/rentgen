# Re-Gen
A ReasonML generator library, designed for Deno and Node.
## Goals
1. Uses an open api, which allows for the usage of fully in-reason generators, as well as external ones (i.e. Deno.iter).
2. Handling of sync and async generators.
3. Written fully in Reason.

## Examples
### Fizz-buzz
```reason
open Sync;

let () = inf(0, a => a + 1)
->map(a =>
   if (a mod 15 == 0) {
     "fizzbuzz";
   } else if (a mod 3 == 0) {
     "fizz";
   } else if (a mod 5 == 0) {
     "buzz";
   } else {
     Js.Int.toString(a);
   }
 )
->consume(Js.Console.log);
```