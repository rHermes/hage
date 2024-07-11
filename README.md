# Hage

[![CircleCI](https://dl.circleci.com/status-badge/img/circleci/5H4i9qDBtSKj9PnJmod34A/BaVoZYu4QLT3oAPGWpQ67K/tree/main.svg?style=shield)](https://dl.circleci.com/status-badge/redirect/circleci/5H4i9qDBtSKj9PnJmod34A/BaVoZYu4QLT3oAPGWpQ67K/tree/main)

[![CircleCI](https://dl.circleci.com/insights-snapshot/circleci/5H4i9qDBtSKj9PnJmod34A/BaVoZYu4QLT3oAPGWpQ67K/main/build_test/badge.svg?window=30d)](https://app.circleci.com/insights/circleci/5H4i9qDBtSKj9PnJmod34A/BaVoZYu4QLT3oAPGWpQ67K/workflows/build_test/overview?branch=main&reporting-window=last-30-days&insights-snapshot=true)

This is a repository that I use to try out new ideas or implement features that I think could be useful later.

It is not stable in **ANY WAY SHAPE OR FORM** and any usage is at your own risk.

## Components

### Core library

This is a core library that is included by most of the other libraries. It includes:

#### Concepts and traits

Some concepts that is useful across applications.

- `hage::all_same`: all the types are the same.

#### Lifetime tester

A very simple lifetime tester, that can be used for quickly testing out move semantics or other.

#### A more composable `std::from_chars`

Based on the work in the following paper
["A More Composable from_chars"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2584r0.pdf),
we implement some of the suggestions there. This makes `std::from_chars` easier to use.

#### zip_view for c++20

TODO(rHermes): This is a zip view for doing sorting.

#### Assert

This is a simple assert library. It's made so that I could learn about the preprocessor. The main
need here is to be able to turn it off during release.


#### Forward Declared Storage for Fast pimpl

This class helps with implementing the "Fast Pimpl" idiom. It's currently under development

#### TODO

- Implement better documentation. For this libraries to be useful, I will need to have good documentation.
- Think a bit about what kind of dependencies we will require.
- Figure out how to do cmake install.
- Implement policy types for `ForwardDeclaredStorage`
- Implement all the features from https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2584r0.pdf

### Data structures

This library includes datastructures that I used from time to time.

Planned datastructures:
- Slotmap implementation, take inspiration from rust
- Index based linked list, with a skip list
- Interval map, built on top of a map structure.
- A skip list

#### AVLTree

An AVLTree implementation, currently in the process of being completed. Features are:

- Full iterator support
- Node pooling
- Supports most of the same functionality as `std::map`


#### TODO

##### AVLTREE
- Add `generation` tag to all nodes in debug mode, and check this tag when accessing through
an iterator, to detect dangling pointers!
- Add check in debug mode, that the iterator belongs to this tree.
  - Just a simple check to see if `m_tree` is equal to `this`.
  - Actually, according to the standard library, iterators must remain valid.
  - I can do this by introducing a "tree_id" which I get from a thread safe function,
 and compare it to that. This get 90% of the effect for 10% of the effort.
- Add examples and tests for this
- Add allocator support.
- Add support for erasing!
- Add some statistics?
- Add support for custom comperators
- Add support for multimap?
  - Look at how boost does this
- Add support for sets also, that doesn't take up extra space.
  - This would be a good generalization, we can just vary the node type.
- Figure out a way to remove the need for a default constructable value type.


##### General
- Also implement a redblack tree
- Add some benchmarks for all trees
- Add a B-Tree?
- Add WAVL tree?
- Add 2-3 trees?

### Utility library

This is not implemented yet, but here are some components I want:

- `Result` type, that can be used for better error handling.

### Atomics library

The `hage::atomic<T>` template implements the ["Improving C++ concurrency features"][p2643] version of atomics, that
add several "waiting" features to the `.wait()` version of methods. I needed this for my low latency logger.

Features:

- Implements [P2643][p2643] paper.
- Just an alias for `std::atomic` except for the new methods.

This will be removed once the paper gets implemented in the standard and when I regularly use a compiler that implements
them. In the future, this will just be an alias for `std::atomics`. There are no implicit conversions here, so you
choose to use one or the other. *(I have no idea why you would use mine, but here it is)*

Here is a simple example, showing of.

```c++
#include <iostream>
#include <hage/atomic/atomic.hpp>

int main() {
  using namespace std::chrono_literals;

  hage::atomic<int> wow;

  wow.store(100);

  // We only want to wait for max 10 seconds.
  std::cout << "Starting waiting\n";
  wow.wait_for(100, 10s);
  std::cout << "We finished waiting!\n";
  return 0;
}
```

#### Todo

- Implement tests for all types
- Create a better test harness
- Create better examples, like stop sources.
- Actually implement the wait and notify setups.
    - Use futex2 on linux if it's available
    - Actually understand this stuff, very good case study.
- Implement `atomic_ref`
- Maybe implement shared pointers (These are hard, and I'm not sure it's worth it)
    - Implement for `std::shared_ptr` ?????
    - Implement for `std::weak_ptr` ????
- Figure out how to do the comparison in the waiting instructions effectivly.
    - It should be by value representation, but it happens on `==` now.
    - Maybe use `std::memcmp` ???
    - https://github.com/llvm/llvm-project/issues/64830
    - https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html (`__builtin_clear_padding`)

[p2643]: https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p2643r2.html "Improving C++ concurrency features"

### Low latency logger

The `hage::logger` construct is the result of me trying a technique I heard from a friend in HFT. Here are some of
the features:

- Only serialization is done on the hot thread, stringification is done on the logging thread
- Uses a fast, lock-free single producer, single consumer FIFO for message passing
- Easy to customize for custom user types
- Provides async and sync interface
- Type safety for all formatting strings
- Requires C++20
- Allows for format string passing via template parameters!
- Uses `fmt` for fast formatting.
- Allows for user implemented buffer types
- Allows for user implemented logger sinks
- Good test suite (Always improving!)
- Provides ready made composable sink types.

Here is an example showcasing its use, with two threads. Notice that they are using the synchronous APIs, there are also
non-blocking version of both of these, by appending the `try_` prefix to the functions.

#### Example

```c++
#include <hage/logging.hpp>
#include <thread>

int main()
{
  hage::ConsoleSink consoleSink;
  hage::RingBuffer<4096> ringBuffer;
  hage::Logger logger(&ringBuffer, &consoleSink);

  constexpr std::int64_t TIMES = 10;
  std::thread writer([&logger]() {
    for (std::int64_t i = 0; i < TIMES; i++)
      logger.info("Here we are: i={} and my name is: {}", i, "james");
  });

  std::thread reader([&logger]() {
    for (std::int64_t i = 0; i < TIMES; i++)
      logger.read_log();
  });

  writer.join();
  reader.join();

  return 0;
}
```

The idea is that going from a datatype to a string is often expensive, compared to serializing the object. An example
is floats, where a serialization is a no-op, but converting them to a string is quite expensive. The idea is that
in the **hot thread** we serialize a function pointer and the arguments, and push them into a FIFO. IN the **logger
thread** we deserialize the function pointer, and call it, with the FIFO as an argument. This works, because the
function pointer that the user passes in, know the types of its arguments. Since we know these functions at compile
time, we are able to pass the type information and order from the **hot thread** to the **logger thread** without
any communication at runtime.

To improve the ergonomics for the user, and to ensure type safety, we instead expose a format string based API, provided
by `fmt::format`. Since we know the input parameters, we are able to create a custom trampoline for each input statement
which will read the correct number and type of arguments in. We still have to send the format string over the FIFO,
as we cannot capture it from the lambda we use to create the trampoline. This is a source of potential optimization,
as these format strings are created at compile time, and will not go out of scope for the duration of the program.

With the help of some very smart people, I've been able to get the formating message into the lambda without passing it
over the FIFO. The main idea was to introduce a `static_string`, which can be used as a template parameter in another
type called `format_string`. When instantiated like this, the format goes from a datatype, into a class type, and we
can pass it into the lambda. There are two ways to use this version:

```c++
logger.info(hage::format_string<"Hello there {}">(), 23);

// Or using our literals
using namespace hage::literals;
logger.info("Hello there {}"_fmt, 23);
```

I've decided to get the time in the hot thread, but this can easily be changed to put it into the logger thread.
The idea is that we want to know the time when it was logged, rather than when it was

Another semi mistake I've made, is that I've made the Buffer implementation a bit too flexible. Really it will always
be some sort of ring buffer. Now it's very general. The part where this reflects a lot is that the read operations
can fail. This is not really true, as the whole invariant of the queue, is that either it's empty, or we have enough
information on it to log. This was added more for the ability to use a network or files as intermediaries.

In the future I'm going to rip that out and instead use exceptions if we cannot read.

#### TODO

- Update the readme
    - Up-to-date code examples
    - History section moved down, It's not so interesting.
- Produce better examples and tests.
- Make error handling in reading better.
- Create benchmarks
- Add some sort of mode where we can `try_to_write`, and `always_read` which doesn't attempt to read, but merely sees
  if there are any logs available? This needs to be benchmarked.
- Add macros for logging, that gets removed at compile time, if we define away the log level
- Create rotating file sink
  - Add an event for new file creation?
  - Implement more types:
    - Size
    - Date
  - Separate the splitters and the time stamps.
- Create more serializers for standard library.
- Think about how this will integrate towards a stop source.
    - What happens when the writer is done?
- Should I implement some sort of tag system to the loggers that is passed to the sinks? Or should that be on the sinks?
- Add some sort of integration towards co_routines, to allow a single logger thread to async read from multiple loggers.
- Remove more template instantiation by making read operations on strings hit the same template.
    - I already did this with the `SmartSerializer` setup, but we are going to need more than that.
    - Maybe look into this when it becomes more of a problem.
- Implement support for https://en.cppreference.com/w/cpp/utility/source_location
- Implement a global logger
- Rework rotating file sink
  - This is way too complex for what it does, overengineered.
  - Let's just go for an enum or maybe even two base classes.
  - Support infinite backlog?
- Split this repo, into it's own self contained repository.
  - Make it more clear and targed
  - Will inspire actual usage.

## How to build



## How to do CI:

I've setup CircleCI, but in order to build with GCC 13, I had to make a custom docker image as the
`cimg/base` image only supports up to gcc12.


This is mostly for my remembering at this point, but here is how to build and push that docker container:

```bash
docker buildx build . -t rsentinel/gcc-builder --platform linux/amd64,linux/arm64 --push
```


### Todo
- Add clang builder image and use that.
- Move away from github actions?
  - Not sure if I want this, given that it's free and more testing never hurt.
