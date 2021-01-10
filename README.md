# Combined Checkpoint/Restart Library
A library to produce combined XOR-Partner checkpoints. The API provides following functionalities:
   * ```init``` - initializes the environment and creates partner, XOR, community, and leaders communicators
   * ```protect``` - registers the variable to be protected
   * ```recover``` - checks for recoveries, if found, fills the registered variables
   * ```checkpoint``` - create XOR - partner combined checkpoints
   * ```finalize``` - clean up
   
# Installation and Test
```sh
make            # compiles the shared library providing the combined C/R
make install    # install the library (default /usr/local)
make test       # compiles the test application with the combined library
make slurm_run  # runs a test with the heat-simulator application using SLURM
```
## Configure the install prefix
To configure the install path the make command can be configured with the ```PREFIX``` and ```DESTDIR``` variables:
```sh
make PREFIX=<prefix> install
# if the prefix is not available in LD_LIBRARY_PATH:
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:<prefix>
```
## Run example
A minimum of 6 compute nodes (machines) are needed to perform combined C/R. A sample usage of the API can be found under the ```test``` directory using the heat-simulator application.
The test could be executed also by ```make run``` (after the compilation and installation).

# License
    Apache License Version 2.0
 
