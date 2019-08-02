### Using alpine image

`docker pull ubuntu`

### Create container

`docker create -it --name=linux ubuntu:__tag__ /bin/bash`

Note doesn't bind folder

### Create container with mounted folder

`docker run --cap-add=SYS_PTRACE --security-opt seccomp=unconfined --name=linux -it -v //c/Users/CalvinLaptop/CalvinLaptop_Files/snake:/snake -v //c/Users/CalvinLaptop/CalvinLaptop_Files/Boost-Learning:/boost ubuntu:v6 /bin/bash`

--cap-add and --security-opt to make `gdb` work

### Start and attach

```
docker start linux
docker attach linux
```

### Commit changes

`docker commit __containername__ __imagename:tag__`

