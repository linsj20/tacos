[//]: # (This source code is licensed under the MIT license found in the)
[//]: # (LICENSE file in the root directory of this source tree.)
[//]: # 
[//]: # Copyright (c) 2022 Intel Corporation
[//]: # Copyright (c) 2022 Georgia Institute of Technology

# 🌮 TACOS
## [T]opology-[A]ware [Co]llective Algorithm [S]ynthesizer for Distributed Machine Learning

## Latest Release
[Latest Release](https://github.com/astra-sim/tacos/releases)

## Project Status
| branch | macOS | Ubuntu | Format | Coverage |
|:---:|:---:|:---:|:---:|:---:|
| **main** | TBA | TBA | [![format](https://github.com/astra-sim/tacos/actions/workflows/check-clang-format.yml/badge.svg?branch=main)](https://github.com/astra-sim/tacos/actions/workflows/check-clang-format.yml) | TBA |
| **develop** | TBA | TBA | [![format](https://github.com/astra-sim/tacos/actions/workflows/check-clang-format.yml/badge.svg?branch=develop)](https://github.com/astra-sim/tacos/actions/workflows/check-clang-format.yml) | TBA |

## Overview
TACOS receives an arbitrary point-to-point network topology and autonomously synthesizes the topology-aware All-Reduce (Reduce-Scatter and All-Gather) collective communication algorithm. TACOS is powered by the Time-expanded Network (TEN) representation and Utilization Maximizing Link-Chunk Matching algorithm, thereby resulting in greater scalability to large networks.

Below figure summarizes the TACOS framework:
![TACOS Abstraction](https://github.com/astra-sim/tacos/blob/main/docs/images/tacos_overview.png)

Please find more information about TACOS in [this paper](https://arxiv.org/abs/2304.05301).
- William Won, Midhilesh Elavazhagan, Sudarshan Srinivasan, Swati Gupta, and Tushar Krishna, "TACOS: Topology-Aware Collective Algorithm Synthesizer for Distributed Machine Learning," arXiv:2304.05301 [cs.DC]

## Getting Started
We highly recommend using the provided Docker image as the runtime environment, since TACOS requires several dependencies including protobuf and boost. You can either download the Docker image from the Docker Hub, or you may build one locally using the provided script.

1. Download the TACOS project.
```sh
git clone --recurse-submodules https://github.com/astra-sim/tacos.git
```

2. Pull the TACOS Docker Image.
```sh
docker pull astrasim/tacos:latest

# Instead, you may consider building this Docker Image locally.
./utils/build_docker_image.sh
```

3. Start the Docker Container (which becomes your TACOS runtime environment).
```sh
./utils/start_docker_container.sh
```

4. Run TACOS with the provided script.
```sh
[docker] ./tacos.sh
```

The generated algorithm will be stored as the xml file under results/.

If you'd like to analyze the codebase, `src/main.cpp` is the main entry point.

## Install MSCCL

In order to use MSCCL, you may follow these steps to install the MSCCL runtime:

#### 1. Download the source code of msccl and related submodules

```sh
$ git clone https://github.com/Azure/msccl.git --recurse-submodules
```

#### 2. Below is the steps to install MSCCL executor:

```sh
$ git clone https://github.com/Azure/msccl.git --recurse-submodules
$ cd msccl/executor/msccl-executor-nccl
$ make -j src.build
$ cd ../
$ cd ../
```

#### 3. Below is the steps to install msccl-tests-nccl for performance evaluation:

```sh
$ cd tests/msccl-tests-nccl/
$ make MPI=1 MPI_HOME=/path/to/mpi CUDA_HOME=/path/to/cuda NCCL_HOME=$HOME/msccl/executor/msccl-executor-nccl/build/ -j
$ cd ../
$ cd ../
```

For using customized tacos algo in MSCCL runtime, you can put the generated xml file under msccl/executor/msccl-executor-nccl/build/lib/:

#### 4. Below is the command to run test using msccl-executor-nccl
```sh
$ mpirun -np 8 -x LD_LIBRARY_PATH=msccl/executor/msccl-executor-nccl/build/lib/:$LD_LIBRARY_PATH -x NCCL_DEBUG=INFO -x NCCL_DEBUG_SUBSYS=INIT,ENV tests/msccl-tests-nccl/build/all_reduce_perf -b 128 -e 32MB -f 2 -g 1 -c 1 -n 100 -w 100 -G 100 -z 0
```

#### 5. If everything is installed correctly, you should see the following output in log:

```sh
[0] NCCL INFO Connected 1 MSCCL algorithms
```

You may evaluate the performance of `test.xml` by comparing in-place (the new algorithm) vs out-of-place (default ring algorithm) and it should up-to 2-3x faster on 8xA100 NVLink-interconnected GPUs. [MSCCL toolkit](https://github.com/Azure/msccl-tools) has a rich set of algorithms for different Azure SKUs and collective operations with significant speedups over vanilla NCCL.

## Contact Us
For any questions about TACOS, please contact [Will Won](mailto:william.won@gatech.edu)
or [Tushar Krishna](mailto:tushar@ece.gatech.edu). You may also find or open a GitHub Issue in this repository.
