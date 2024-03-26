# Hage

This is a repository that I use to try out new ideas or implement features that I think could be useful later.

It is not stable in **ANY WAY SHAPE OR FORM** and any usage is at your own risk.

## Components

### Low latency logger

The `hage::logger` construct is the result of me trying a technique I heard from a friend in HFT. Here are some of
the features:

- Only serialization is done on the hot thread, stringification is done on the logging thread
- Uses a fast single producer, single consumer FIFO for message passing
- Easy to customize for custom user types
- Header only
- Provides async and sync interface

The idea is that going from a datatype to a string is often expensive, compared to serializing the object. An example
is floats, where a serialization is a no-op, but converting them to a string is quite expensive. The idea is that
in the **hot thread** we serialize a function pointer and the arguments, and push them into a FIFO.  IN the **logger
thread** we deserialize the function pointer, and call it, with the FIFO as an argument.

This works as of now, but the ergonomics are terrible. The main problem, is that any type of ad hoc logging will
need to use some sort of function pointer or trampoline to log. There is also no type safety that guarantees that
the function will unpack the right amount of arguments.

I have considered using a type of "format string" instead of the function pointer, but I've not found a way to keep
the type information when pushing it across the pipe. This remains something I should learn more about, maybe it is
possible. If so, it would make the code much cleaner and make adhoc logging much easier.

Another semi mistake I've made, is that I've made the Buffer implementation a bit too flexible. Really it will always
be some sort of ring buffer. Now it's very general. The part where this reflects a lot is that the read operations
can fail. This is not really true, as the whole invariant of the queue, is that either it's empty, or we have enough
information on it to log. This was added more for the ability to use a network or files as intermediaries.

In the future I'm going to rip that out and instead use exceptions if we cannot read.

TODO:

- Implement the idea of a maximum size for a message.
- Implement type safety all the way through, ensuring that the function is called with its correct parameters.
- Make the ergonomics much better. Make it so that the receiving function doesn't need to take in a buffer at all, it
just takes in its arguments by value.
- Produce better examples and tests.
