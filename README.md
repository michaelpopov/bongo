This is an experimental project to clarify thoughts on how to organize processing of requests arriving on network sessions. These requests are passed to working threads, where they are processed in order.

Catalogue:
block_conn.* contain classes providing blocking network I/O. They are used for testing purposes.
nonblock_conn.* contain classes providing non-blocking network I/O.
session_base.* contain implementation of base classes for netwrok session support.
session_demo.* contain implementation of network sessions for different scenarios
   EchoNetSession provides echo functionality on non-blocking network I/O
   BigWriterNetSession is used for testing network session with a large volume responses
   ReqRespSession is used for "full-cycle" tests where processing is distributed on working threads.
utest_*.cpp unit tests for corresponding functionality.


Now this is not the end. It is not even the beginning of the end. But it is, perhaps, the end of the beginning.
   Winston Churchill