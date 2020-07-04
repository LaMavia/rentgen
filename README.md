# Rentgen
A ReasonML generator library
## Goals
1. Uses an open api, which allows for the usage of fully in-reason generators, as well as external ones.
2. Handling of sync and async generators.
3. Written fully in Reason.

## Documentation
Most of the documentation, together with examples, can be found inline
### Sync
---
### type **next('a)**
   > Represents the next value yielded by a generator
### type **gen('a)**
   > Represents the api of a generator
### **next** : [gen('a)](#type-gena) => [next('a)](#type-nexta)
  > Increments the generator given, and returns its [next](#type-nexta)) value.

### **return** : ([gen('a)](#type-gena), 'a) => [next('a)](#type-nexta)
  > Forces the generator to return the 2nd argument and stop.
### **throw** : ([gen('a)](#type-gena), Js.Exn.t) => [next('a)](#type-nexta)
  > Throws the error in the generator and forces it to stop.

### **from** : 'a => [gen('b)](#type-gena)
  > Creates a generator from an external source, which implements [the javascript generator api](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Generator).

### **range** : (~to_: int, ~from=0 : int, ~step=1 : int) => [gen(int)](#type-gena)
  > Creates a generator returning numbers from the range `<from; to_>` separated by the distance of `step`.
  Creates an int range.
  ```reason
  range(~to_=10)->foldl((+), 0)
  // => 55
  ```

### **from_list** : list('a) => [gen('a)](#type-gena)
  > Converts a list to a generator.
  ```reason
  [1,2,3]->from_list->foldl((+), 0)
  // => 6
  ```

### **inf** : 'a => ('a => 'a) => [gen('a)](#type-gena)
  > Creates an infinite generator from an initial value and a successor function.
  ```reason
  inf(0, a => a + 1) // => <0, 1, 2, 3, ...>
  ```

### **map** : ([gen('a)](#type-gena), ('a => 'b)) => [gen('b)](#type-gena)
  > Transforms the values of a generator.
  ```reason
  range(~to_=5, ()) -> map(a => a * 2)
  // => <0, 2, 4, 6, 8, 10>
  ```

### **keep** : ([gen('a)](#type-gena), ('a => bool)) => [gen('a)](#type-gena)
  > Returns only the values of the generator which match the function.
  ```reason
  range(~to_=10, ()) -> keep(a => a mod 2 == 0)
  // => <0, 2, 4, 6, 8, 10>
  ```

### **consume** : ([gen('a)](#type-gena), 'a => 'b) => unit
  > Consumes a generator with a function.
  ```reason
  // prints an infinite generator
  inf(0, a => a + 1) -> consume(Js.Console.log)
  // 0
  // 1
  // 2 ...
  // => unit
  ```

### **foldl** : ([gen('a)](#type-gena), (('b, 'a) => 'b), 'b) => 'b
  > Folds a generator into a single value.
  ```reason
  [1,2,3]->from_list->foldl((+), 0)
  // => 6
  ```

### **foldl1** : ([gen('a)](#type-gena), ('b, 'a) => 'b) => 'b
  > Folds a generator into a single value, taking the first value as the initial value.
  ```reason
  range(~to_=3, ~from=1,())
  ->map(float_of_int)
  ->foldl1((a, b) => a /. b)
  // => 1/6
  ```

### **take** : ([gen('a)](#type-gena), int) => [gen('a)](#type-gena)
  > Takes n values of the generator or until it ends.
  ```reason
  inf(0, a => a + 1) -> take(5)
  // => <0, 1, 2, 3, 4>
  ```

### **take_while** : ([gen('a)](#type-gena), ('a => bool)) => [gen('a)](#type-gena)
  > Takes the values of the generator as long as the function returns `true`.
  ```reason
  inf(0, a => a*2 + 1)->take_while(a => a < 15)
  // => <0, 1, 3, 7>
  ```

### Async
---
### type **async('a)** = Js.Promise.t('a)
### type **next_async('a)** = async(next('a))
### type **gen_async('a)**

### **next_async** : [gen_async('a)](#type-gen_asynca) => [next_async('a)](#type-next_asynca--asyncnexta)
  > Returns the next value of an async generator

### **return_async** : ([gen_async('a)](#type-gen_asynca), 'a) => [next_async('a)](#type-next_asynca--asyncnexta)
  > Returns the value given (as a promise), and forces the generator to stop

### **throw_async** : ([gen_async('a)](#type-gen_asynca), Js.Exn.t) => [next_async('a)](#type-next_asynca--asyncnexta)
  > Throw the given error in the generator and forces it to stop

### **from_async** : 'a => [gen_async('b)](#type-gen_asynca)
  > Creates a generator from an external source, which implements [the async iterator api](https://github.com/tc39/proposal-async-iteration).

### **from_sync** : [gen('a)](#type-gena) => [gen_async('a)](#type-gen_asynca)
  > Converts a sync generator into an async one.
  ```reason
  inf(1, a => a + 1)
  ->from_sync
  ->take_async(100)
  ->consume_async(Js.Console.log)
  ```

### **map_async** : ([gen_async('a)](#type-gen_asynca), ('a => 'b)) => [gen_async('b)](#type-gen_asynca)
  > Transforms an async generator using the function given.
  ```reason
  puppy_stream
  -> map_async(pet)
  // => <async("doggo_1"), async("doggo_2"), ...>
  ```

### **foldl_async** : ([gen_async('a)](#type-gen_asynca), (('b, 'a) => 'b), 'b) => [async('b)](#type-asynca--jspromiseta)
  > Folds an async generetor into a single promise
  ```reason
  open_file("data/numbers.txt")
  ->map_async(Js.Float.fromString)
  ->foldl_async((+.), 0.)
  // => async(21)
  ```
### **foldl1_async** : ([gen_async('a)](#type-gen_asynca), (('b, 'a) => 'b)) => [async('b)]
  > Folds an async generator into a single promise, taking the first value as the initial value.
  ```reason
  range(~to_=3, ~from=1, ())
  ->map(float_of_int)
  ->from_sync
  ->foldl1_async((a, b) => a /. b)
  // => async(1/6)
  ```

### **consume_async** : ([gen_async('a)](#type-gen_asynca), 'a => 'b) => [async(unit)](#type-asynca--jspromiseta)
  > Consumes an async stream, and returns an empty promise.
  ```reason
  from_async(Deno.iter(file, ()))
  ->consume_async(Js.Console.log)
  // (prints the file)
  // => async(unit)
  ```

### **take_async** : ([gen_async('a)](#type-gen_asynca), int) => [gen_async('a)](#type-gen_asynca)
  > Takes the first n elements from the async generator, and returns a new one of the length n.
  ```reason
  infinite_puppies
  ->take_async(5)
  // => async<"puppy1", "puppy2", ... , "puppy5">
  ```

### **take_while_async** : ([gen_async('a)](#type-gen_asynca), ('a => bool)) => [gen_async('a)](#type-gen_asynca)
  > Takes elements of the generator as long as the condition given is met. Returns a new async generator. 

### **keep_async** : ([gen_async('a)](#type-gen_asynca), ('a => bool)) => [gen_async('a)](#type-gen_asynca)
  > Returns an async generator of the items which meet the condition given.

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