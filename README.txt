Pintos is a simple OS written in C that runs on 32-bit x86 architecture. It's partially implemented and it's up to you to add various features and enhancements. For more information see http://web.stanford.edu/class/cs140/projects/pintos/pintos.html.

This is a complete implementation that passes all tests.

Highlights of my implementation include:

    - A fair scheduler that works across various load types (I/O bound, CPU)
    - Priority scheduling including priority donation to avoid priority inversion
    - Virtual memory subsystem that includes swapping to disk, file memory mapping, and shared read-only pages
    - A multithreaded file system which includes a write back buffer cache with read ahead and sparse files
    
Thanks to Surya for the instructions on how to get pintos to run in qemu:
  https://tssurya.wordpress.com/2014/08/16/installing-pintos-on-your-machine/
