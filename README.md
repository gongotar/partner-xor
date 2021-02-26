# Combined Checkpoint/Restart Library

This library provides a stable, scalable, and fast Checkpoint/Restart mechanism for High-Performance Computing (HPC) applications running on multiple compute nodes (machines). 

### Purpose
HPC applications register their critical variables (memory regions) to the library. To ensure the reliability of the registered data, the library periodically generates checkpoints by performing erasure coding and partner redundancy between the registered memory regions on different compute nodes. The library always keeps the _k_ most recent checkpoints on disk (by default _k_ = 2). In case of soft/hard/correlated failures, the memory regions are restored automatically by the library using erasure coding and/or redundancy data. 

### Methodology
The Combined Checkpoint/Restart Library combines XOR erasure coding and partner redundancy hierarchically in two layers. Fault-tolerance and flexibility in failure recovery are ensured by replicating the checkpointing and XOR parity data from the upper layer to the partner nodes of the lower layer.

## Compile and Install
```sh
git clone https://github.com/gongotar/partner-xor.git
mkdir partner-xor/build && cd partner-xor/build 
cmake -DCMAKE_INSTALL_PREFIX=<installation_path> ..
make
make install
```
Optionally, before performing `make install`, the tests can be performed by `make test`. Notice that this may take a while. The checkpointing path of the tests can be configured using the configuration file under `tests/config.ini` (more information on the configuration files can be found below). Please make sure that the checkpointing path exists and has at least `10 GB` of free space.

### Installed Files
After a successful install, the combined C/R shared library `libcombined.so` is installed under `<installation_path>/lib` and the header file `combined.h` is installed under `<installation_path>/include`.

## API Documentation

The library exposes an API to HPC applications to register the critical variables and to perform checkpoint and recovery. Each API call returns an _integer_ value indicating whether or not the operation was performed successfully. In the case of a successful operation, `COMB_SUCCESS` is returned, otherwise, `COMB_FAILED`. The API provides the following functionalities:
* **```int COMB_Init (MPI_Comm comm, char *config)```** 
   * Initializes the environment and groups the processes into different partner, XOR, community, and leaders groups.
      * ```MPI_Comm comm``` - the MPI communicator of the application.
      * ```char *config``` - a path to the configuration file.
* **```int COMB_Protect (void *data, size_t size)```**
   * Registers the variables to be protected and included in the checkpoints.
      * ```void *data``` - a pointer to the variable/data to be protected.
      * ```size_t size``` - the size of the registered data.
* **```int COMB_Recover (int *restart)```** 
   * Recovers the lost data using XOR erasure coding and/or partner redundancy. This will load the restored data into the registered memory regions (variables) by ```COMB_Protect```. This function should be called after registering the variables to check if there exist any recoveries for the application.
      * ```int *restart``` - the restored data _version_. If nothing is found, `0` is returned.
* **```int COMB_Checkpoint ()```** 
   * Denotes the reasonable points in the code to create the checkpoints from the registered memory regions (e.g., after achieving a milestone or updating a large variable). This function should be called periodically during the application's lifetime. To create the combined XOR-partner checkpoints, first, the XOR checkpoint is computed among the processes in the same XOR group. Then, the data and the computed XOR parity are transferred among the processes in the same partner group.  
* **```int COMB_Finalize (int cleanup)```**
   * Frees up the reserved spaces and communicators. Removes the created groups and optionally deletes the checkpoint files from the disk.
      * ```int cleanup``` - whether or not to remove the checkpoint files from the disk at the end.

## Configuration File
The library reads configuration files of the format [INI](http://www.nongnu.org/chmspec/latest/INI.html). Default configuration files can be found under the `tests` and `example` directories. The following parameters can be configured using a configuration file:
| Parameter | Description |
| :--------- | :----------- |
| **```partner_group_size```** | The number of processes to be grouped within a partner group. Larger values deliver more resiliency at the cost of larger checkpoints and more overhead. The _default_ value `2` (the minimum allowed value) suffices for most of the cases to guarantee a stable scheme.|
| **```xor_group_size```** | The number of processes to be grouped within an XOR group. Larger values deliver lower resiliency, but a bit smaller checkpoints. For many cases, a reasonable choice would be a number between `4` to `20` (the minimum allowed value is `3`). Though, to achieve the optimal performance, this number should be chosen considering the total number of processes in a way that the following statement holds: <br />```total_processes % (partner_group_size * xor_group_size) = 0``` <br />where ```total_processes``` indicates the total number of processes of the application. If the statement above does not hold, the last community will automatically be adjusted according to the remaining processes which may lead to sub-optimal performances. |
| **```cp_path```** | The path to store the checkpoints. To utilize the benefits of the combined XOR-partner C/R, the checkpointing directory should lay on a local disk (rather than a global disk). Though, the library would still produce the checkpoints also on a global disk. _MAKE SURE THE CP_PATH EXISTS_. |
| **```cp_history```** | Indicates the maximum number of checkpoints to keep on the disk. The library keeps the last ```cp_history``` number of recent checkpoints. It is recommended to keep at least `2` checkpoints (_default_) to avoid possible data losses by the failures during checkpointing. |
| **```consider_ranks_per_node```** | Indicates whether or not to arrange the process groups considering the number of processes per compute node. If `1`, the library ensures that the partner, XOR, and community groups contain at-most _**one**_ process from a compute node. This way, the failure of a node will be translated to a single process failure per group, and the chances of a successful recovery increase. If this parameter is set, then, the application _must_ contain at least `partner_group_size * xor_group_size` compute nodes. Otherwise, it is impossible to arrange _one_ process per node in the groups. If `consider_ranks_per_node = 0`, the processes are grouped naturally considering their _rank_ number. In this case, there will be no limitations on the minimum number of compute nodes. Though, a node failure may destruct multiple processes per group, and the reliability decreases. |

## Sample Usage

To demonstrate a sample usage of the library, there is a usage example under the `example` directory. The example application in C simulates the heat distribution within many iterations. The heat data is stored in a large matrix of `double` values (configurable size). During each iteration, several communications are performed between the processes, and the matrix of the next iteration is computed using the previous matrix and the communicated data. The checkpoints contain the two matrices along with the iteration counter (an `integer`).

# License
    Apache License Version 2.0
