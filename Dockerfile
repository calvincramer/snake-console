FROM ubuntu:latest

# Resolve Comcast's resolving issue and update
# The successive RUN commands wouldn't keep the 8.8.8.8 DNS resolver
# so I combined them all in one command with &&
RUN echo "nameserver 8.8.8.8" | tee /etc/resolv.conf > /dev/null               \
&& echo "\n\nUPDATING\n\n"                                                     \
&& apt-get update                                                              \
&& apt-get upgrade -y                                                          \
&& apt update                                                                  \
# Feel free to comment out packages you don't want
# Note MAKETOOLS necessary to compile program
&& echo "\n\nINSTALLING MAKETOOLS\n\n" && apt-get install -y build-essential   \
&& echo "\n\nINSTALLING GIT\n\n"        && apt     install -y git              \
&& echo "\n\nINSTALLING GDB\n\n"        && apt-get install -y gdb              \
&& echo "\n\nDONE BUILDING snake_image\n\n"
