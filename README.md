# Metro Simulation Using POSIX Threads
A metro simulation program that involves the concepts of scheduling, synchronisation, multi-threading and deadlock prevention in operating systems by using POSIX threads.


* There are 6 threads (ignoring 1 helper thread).

* 4 of the threads are for sections, 1 is for control center and 1 is for logging.

* In section threads, trains are created according to specified probability and added to queues.

* Control center thread handles scheduling and deadlock prevention.

* Logging thread keeps track of the events.


![Sections Figure](https://imagizer.imageshack.com/v2/800x600q90/923/KQF6iZ.png)
