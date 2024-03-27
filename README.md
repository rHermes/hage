# Hage

This is a repository that I use to try out new ideas or implement features that I think could be useful later.

It is not stable in **ANY WAY SHAPE OR FORM** and any usage is at your own risk.

## Components

### Low latency logger

The `hage::logger` construct is the result of me trying a technique I heard from a friend in HFT. Here are some of
the features:

- Only serialization is done on the hot thread, stringification is done on the logging thread
- Uses a fast, lock-free single producer, single consumer FIFO for message passing
- Easy to customize for custom user types
- Header only
- Provides async and sync interface
- Type safety for all formatting strings'
- Requires C++20
- Allows for format string passing via template parameters!
- Uses `fmt` for fast formatting.

Here is an example showcasing its use, with two threads. Notice that they are using the synchronous APIs, there are also
non-blocking version of both of these, by appending the `try_` prefix to the functions.

#### Example

```c++
#include <hage/logging.hpp>
#include <thread>

int main()
{
  hage::Logger<hage::RingBuffer<4096>> logger;

  constexpr std::int64_t TIMES = 10;
  std::thread writer([&logger]() {
    for (std::int64_t i = 0; i < TIMES; i++)
      logger.log("Here we are: i={} and my name is: {}", i, "james");
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
in the **hot thread** we serialize a function pointer and the arguments, and push them into a FIFO.  IN the **logger
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
logger.log(hage::format_string<"Hello there {}">(), 23);

// Or using our literals
using namespace hage::literals;
logger.log("Hello there"_fmt, 23);
```


Another semi mistake I've made, is that I've made the Buffer implementation a bit too flexible. Really it will always
be some sort of ring buffer. Now it's very general. The part where this reflects a lot is that the read operations
can fail. This is not really true, as the whole invariant of the queue, is that either it's empty, or we have enough
information on it to log. This was added more for the ability to use a network or files as intermediaries.

In the future I'm going to rip that out and instead use exceptions if we cannot read.

TODO:

- Implement the idea of a maximum size for a message.
- Produce better examples and tests.
- Make error handling in reading better.
- Allow the user to specify sinks.
- Create benchmarks
- Implement logging levels, with filtering
