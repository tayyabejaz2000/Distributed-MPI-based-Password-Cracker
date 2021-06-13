# Distributed-MPI-based-Password-Cracker

A Simple Brute Force based distributed password cracker using MPI

# Build

Use CMake to create Makefile
<br>
<code>
$ cmake .
</code>
<br>
and then make to build
<br>
<code>
$ make
</code>

# Run

<code>
$ sudo mpirun --allow-run-as-root -n {no of jobs} {binary}
</code>
<br>
As the code reads <code>/etc/shadow</code> which has special permissions so it needs sudo permissions.
<br>
<code>mpirun</code> doesnt alow to be run as root so <code>--allow-run-as-root</code> is required to run the binary normally.

***
